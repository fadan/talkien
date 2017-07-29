#pragma warning(push)
#pragma warning(disable: 4459)

#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui.cpp"

#include "dock.cpp"
#pragma warning(pop)

static UISettingsFile push_ui_settings(UIState *context, MemoryStack *memstack)
{
    UISettingsFileHeader *header = push_struct(memstack, UISettingsFileHeader);
    header->file_id = UI_SETTINGS_FILE_ID;
    header->file_version = UI_SETTINGS_FILE_VERSION;
    header->dock_size = sizeof(UISettingsFileData);
    header->num_docks = get_num_docks(context);

    UISettingsFileData *data_base = push_array(memstack, get_num_docks(context), UISettingsFileData);
    UISettingsFileData *dest = data_base;

    for (Dock *src = context->first_dock; src; src = src->next, ++dest)
    {
        dest->parent_index = (i8)get_dock_index(context, src->parent);
        dest->prev_index = (i8)get_dock_index(context, src->prev_tab);
        dest->next_index = (i8)get_dock_index(context, src->next_tab);
        dest->children_index[0] = (i8)get_dock_index(context, src->children[0]);
        dest->children_index[1] = (i8)get_dock_index(context, src->children[1]);
        dest->pos[0] = (i16)src->pos.x;
        dest->pos[1] = (i16)src->pos.y;
        dest->size[0] = (i16)src->size.x;
        dest->size[1] = (i16)src->size.y;
        dest->active = (i8)src->active;
        dest->opened = (i8)src->opened;
        dest->status = (u8)src->status;

        copy_string(src->location, dest->location, array_count(dest->location));
        copy_string_and_null_terminate(src->label, dest->label, array_count(dest->label));
    }

    UISettingsFile settings = {0};
    settings.header = header;
    settings.data = data_base;
    return settings;
}

// TODO(dan): dont use stdio
static void save_ui_settings_to_file(UIState *context, char *filename, MemoryStack *memstack)
{
    TempMemoryStack temp_memory = begin_temp_memory(memstack);
    {
        FILE *file = fopen(filename, "wb");
        if (file)
        {
            UISettingsFile settings = push_ui_settings(context, memstack);
            u32 header_size = sizeof(*settings.header);
            u32 data_size = sizeof(*settings.data) * get_num_docks(context);

            fwrite(settings.header, header_size, 1, file);
            fwrite(settings.data, data_size, 1, file);
            fclose(file);
        }
    }
    end_temp_memory(temp_memory);
}

static void load_ui_settings_from_file(UIState *context, char *filename)
{
    LoadedFile file = load_file(filename);
    if (file.contents && file.size > sizeof(UISettingsFileHeader))
    {
        UISettingsFileHeader *header = (UISettingsFileHeader *)file.contents;
        u32 file_size = sizeof(UISettingsFileHeader) + header->num_docks * header->dock_size;

        if (header->file_id == UI_SETTINGS_FILE_ID && header->file_version == UI_SETTINGS_FILE_VERSION && header->dock_size == sizeof(UISettingsFileData) && file_size == file.size)
        {
            // TODO(dan): clear the list

            for (u32 dock_index = 0; dock_index < header->num_docks; ++dock_index)
            {
                Dock *dock = allocate_dock(context);
                init_dock(dock);
            }

            UISettingsFileData *dock_base = (UISettingsFileData *)(header + 1);
            UISettingsFileData *src = dock_base;

            for (Dock *dest = context->first_dock; dest; dest = dest->next, ++src)
            {
                dest->parent = get_dock_by_index(context, src->parent_index);
                dest->prev_tab = get_dock_by_index(context, src->prev_index);
                dest->next_tab = get_dock_by_index(context, src->next_index);
                dest->children[0] = get_dock_by_index(context, src->children_index[0]);
                dest->children[1] = get_dock_by_index(context, src->children_index[1]);
                dest->pos.x = (f32)src->pos[0];
                dest->pos.y = (f32)src->pos[1];
                dest->size.x = (f32)src->size[0];
                dest->size.y = (f32)src->size[1];
                dest->active = src->active;
                dest->opened = src->opened;
                dest->status = (Status)src->status;

                copy_string_and_null_terminate(src->label, dest->label, array_count(dest->location));
                copy_string(src->location, dest->location, array_count(dest->location));

                dest->id = ImHash(dest->label, 0);
            }
        }
    }
    close_file(file);
}   

#define SAVE_UI_STATE_INTERVAL_SECS 5.0f
static void save_ui_state(UIState *state, MemoryStack *memstack)
{
    static f32 last_save = 0;
    if (last_save > SAVE_UI_STATE_INTERVAL_SECS)
    {
        save_ui_settings_to_file(state, "ui_state.dat", memstack);
        last_save = 0;
    }
    last_save += state->imgui_io->DeltaTime;
}

#define UI_SHADER_VERSION "#version 330\n"

static char *ui_vertex_shader
{
    UI_SHADER_VERSION

    "uniform mat4 proj_mat;\n"
    "in vec2 pos;\n"
    "in vec2 uv;\n"
    "in vec4 color;\n"
    "out vec2 frag_uv;\n"
    "out vec4 frag_color;\n"

    "void main()\n"
    "{\n"
    "   frag_uv = uv;\n"
    "   frag_color = color;\n"
    "   gl_Position = proj_mat * vec4(pos.xy, 0, 1);\n"
    "}\n"
};

static char *ui_fragment_shader
{
    UI_SHADER_VERSION

    "uniform sampler2D tex;\n"
    "in vec2 frag_uv;\n"
    "in vec4 frag_color;\n"
    "out vec4 out_color;\n"

    "void main()\n"
    "{\n"
    "   out_color = frag_color * texture(tex, frag_uv.st);\n"
    "}\n"
};

inline void check_bindings(GLint *buffer, GLint num_elements)
{
    for (GLint element_index = 0; element_index < num_elements; ++element_index)
    {
        assert(buffer[element_index] != -1);
    }
}

static void init_ui(UIState *state)
{
    u8 *pixels;
    i32 width, height;
    state->imgui_io = &GImGui->IO;
    state->imgui_io->Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    char error[1024];
    state->program = opengl_create_program(ui_vertex_shader, ui_fragment_shader, error, sizeof(error));
    assert(state->program);

    state->attribs[attrib_pos] = gl.GetAttribLocation(state->program, "pos");
    state->attribs[attrib_uv] = gl.GetAttribLocation(state->program, "uv");
    state->attribs[attrib_color] = gl.GetAttribLocation(state->program, "color");
    check_bindings(state->attribs, attrib_count);

    state->uniforms[uniform_tex] = gl.GetUniformLocation(state->program, "tex");
    state->uniforms[uniform_proj_mat] = gl.GetUniformLocation(state->program, "proj_mat");
    check_bindings(state->uniforms, uniform_count);

    gl.GenBuffers(1, &state->vbo);
    gl.GenBuffers(1, &state->elements);

    gl.GenVertexArrays(1, &state->vao);
    gl.BindVertexArray(state->vao);
    gl.BindBuffer(GL_ARRAY_BUFFER, state->vbo);

    gl.EnableVertexAttribArray(state->attribs[attrib_pos]);
    gl.EnableVertexAttribArray(state->attribs[attrib_uv]);
    gl.EnableVertexAttribArray(state->attribs[attrib_color]);

    gl.VertexAttribPointer(state->attribs[attrib_pos], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)offset_of(ImDrawVert, pos));
    gl.VertexAttribPointer(state->attribs[attrib_uv], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)offset_of(ImDrawVert, uv));
    gl.VertexAttribPointer(state->attribs[attrib_color], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid *)offset_of(ImDrawVert, col));

    gl.GenTextures(1, &state->font_texture);
    gl.BindTexture(GL_TEXTURE_2D, state->font_texture);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    state->imgui_io->Fonts->TexID = (void *)(intptr)state->font_texture;

    ImGuiStyle *style = &ImGui::GetStyle();

    style->WindowPadding = ImVec2(15, 15);
    style->WindowRounding = 5.0f;
    style->FramePadding = ImVec2(5, 5);
    style->FrameRounding = 3.0f;
    style->ItemSpacing = ImVec2(12, 8);
    style->ItemInnerSpacing = ImVec2(8, 6);
    style->IndentSpacing = 25.0f;
    style->ScrollbarSize = 15.0f;
    style->ScrollbarRounding = 9.0f;
    style->GrabMinSize = 5.0f;
    style->GrabRounding = 3.0f;

    style->Colors[ImGuiCol_Text] = ImVec4(0.91f, 0.91f, 0.91f, 1.00f);
    style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style->Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
    style->Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    style->Colors[ImGuiCol_FrameBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.75f, 0.42f, 0.02f, 0.40f);
    style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.75f, 0.42f, 0.02f, 0.67f);
    style->Colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 0.80f);
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    style->Colors[ImGuiCol_ComboBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.99f);
    style->Colors[ImGuiCol_CheckMark] = ImVec4(0.75f, 0.42f, 0.02f, 1.00f);
    style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.75f, 0.42f, 0.02f, 0.78f);
    style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.75f, 0.42f, 0.02f, 1.00f);
    style->Colors[ImGuiCol_Button] = ImVec4(0.75f, 0.42f, 0.02f, 0.40f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.75f, 0.42f, 0.02f, 1.00f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.94f, 0.47f, 0.02f, 1.00f);
    style->Colors[ImGuiCol_Header] = ImVec4(0.75f, 0.42f, 0.02f, 0.31f);
    style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.75f, 0.42f, 0.02f, 0.80f);
    style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.75f, 0.42f, 0.02f, 1.00f);
    style->Colors[ImGuiCol_Column] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.75f, 0.42f, 0.02f, 0.78f);
    style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.75f, 0.42f, 0.02f, 1.00f);
    style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.75f, 0.42f, 0.02f, 0.67f);
    style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.75f, 0.42f, 0.02f, 0.95f);
    style->Colors[ImGuiCol_CloseButton] = ImVec4(0.42f, 0.42f, 0.42f, 0.50f);
    style->Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.02f, 0.61f, 0.64f, 1.00f);
    style->Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.02f, 0.61f, 0.64f, 1.00f);
    style->Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 0.57f, 0.65f, 1.00f);
    style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.10f, 0.30f, 1.00f, 1.00f);
    style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 0.40f, 1.00f, 1.00f);
    style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.75f, 0.42f, 0.02f, 0.35f);
    style->Colors[ImGuiCol_PopupBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.94f);
    style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.06f, 0.06f, 0.06f, 0.35f);

    // style->Colors[ImGuiCol_Button]                  = ImColor( 21,  39,  54, 255);
    // style->Colors[ImGuiCol_ChildWindowBg]           = ImColor( 33,  50,  67, 255);
    // style->Colors[ImGuiCol_FrameBg]                 = ImColor( 32,  55,  72, 255);
    // style->Colors[ImGuiCol_MenuBarBg]               = ImColor( 21,  39,  54, 255);
    // style->Colors[ImGuiCol_PopupBg]                 = ImColor( 33,  50,  67, 255);
    // style->Colors[ImGuiCol_Text]                    = ImColor(181, 191, 199, 255);
    // style->Colors[ImGuiCol_TitleBg]                 = ImColor( 21,  39,  54, 255);
    // style->Colors[ImGuiCol_TitleBgActive]           = ImColor( 21,  39,  54, 255);
    // style->Colors[ImGuiCol_TitleBgCollapsed]        = ImColor( 21,  39,  54, 255);
    // style->Colors[ImGuiCol_WindowBg]                = ImColor( 33,  50,  67, 255);
    

    // style->Colors[ImGuiCol_ComboBg]                 = ImVec4(0.19f, 0.18f, 0.21f, 1.00f);
    // style->Colors[ImGuiCol_FrameBgActive]           = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    // style->Colors[ImGuiCol_FrameBgHovered]          = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    // style->Colors[ImGuiCol_ScrollbarBg]             = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    // style->Colors[ImGuiCol_TextSelectedBg]          = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);

    // style->Colors[ImGuiCol_Border]                  = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
    // style->Colors[ImGuiCol_BorderShadow]            = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
    // style->Colors[ImGuiCol_ButtonActive]            = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    // style->Colors[ImGuiCol_ButtonHovered]           = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    // style->Colors[ImGuiCol_CheckMark]               = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    // style->Colors[ImGuiCol_CloseButton]             = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
    // style->Colors[ImGuiCol_CloseButtonActive]       = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
    // style->Colors[ImGuiCol_CloseButtonHovered]      = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
    // style->Colors[ImGuiCol_Column]                  = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    // style->Colors[ImGuiCol_ColumnActive]            = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    // style->Colors[ImGuiCol_ColumnHovered]           = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    // style->Colors[ImGuiCol_Header]                  = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    // style->Colors[ImGuiCol_HeaderActive]            = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    // style->Colors[ImGuiCol_HeaderHovered]           = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    // style->Colors[ImGuiCol_ModalWindowDarkening]    = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
    // style->Colors[ImGuiCol_PlotHistogram]           = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    // style->Colors[ImGuiCol_PlotHistogramHovered]    = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
    // style->Colors[ImGuiCol_PlotLines]               = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    // style->Colors[ImGuiCol_PlotLinesHovered]        = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
    // style->Colors[ImGuiCol_ResizeGrip]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    // style->Colors[ImGuiCol_ResizeGripActive]        = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    // style->Colors[ImGuiCol_ResizeGripHovered]       = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    // style->Colors[ImGuiCol_ScrollbarGrab]           = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    // style->Colors[ImGuiCol_ScrollbarGrabActive]     = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    // style->Colors[ImGuiCol_ScrollbarGrabHovered]    = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    // style->Colors[ImGuiCol_SliderGrab]              = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    // style->Colors[ImGuiCol_SliderGrabActive]        = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    // style->Colors[ImGuiCol_TextDisabled]            = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    

    state->imgui_io->KeyMap[ImGuiKey_Tab] = get_scan_code_offset(button_tab);
    state->imgui_io->KeyMap[ImGuiKey_LeftArrow] = get_scan_code_offset(button_arrow_left);
    state->imgui_io->KeyMap[ImGuiKey_RightArrow] = get_scan_code_offset(button_arrow_right);
    state->imgui_io->KeyMap[ImGuiKey_UpArrow] = get_scan_code_offset(button_arrow_up);
    state->imgui_io->KeyMap[ImGuiKey_DownArrow] = get_scan_code_offset(button_arrow_down);
    state->imgui_io->KeyMap[ImGuiKey_PageUp] = get_scan_code_offset(button_page_up);
    state->imgui_io->KeyMap[ImGuiKey_PageDown] = get_scan_code_offset(button_page_down);
    state->imgui_io->KeyMap[ImGuiKey_Home] = get_scan_code_offset(button_home);
    state->imgui_io->KeyMap[ImGuiKey_End] = get_scan_code_offset(button_end);
    state->imgui_io->KeyMap[ImGuiKey_Delete] = get_scan_code_offset(button_delete);
    state->imgui_io->KeyMap[ImGuiKey_Backspace] = get_scan_code_offset(button_backspace);
    state->imgui_io->KeyMap[ImGuiKey_Enter] = get_scan_code_offset(button_enter);
    state->imgui_io->KeyMap[ImGuiKey_Escape] = get_scan_code_offset(button_escape);
    state->imgui_io->KeyMap[ImGuiKey_A] = get_scan_code_offset(button_a);
    state->imgui_io->KeyMap[ImGuiKey_C] = get_scan_code_offset(button_c);
    state->imgui_io->KeyMap[ImGuiKey_V] = get_scan_code_offset(button_v);
    state->imgui_io->KeyMap[ImGuiKey_X] = get_scan_code_offset(button_x);
    state->imgui_io->KeyMap[ImGuiKey_Y] = get_scan_code_offset(button_y);
    state->imgui_io->KeyMap[ImGuiKey_Z] = get_scan_code_offset(button_z);

    state->imgui_io->IniFilename = 0;

    init_memory_stack(&state->ui_memory, 1*KB );

    init_dock_context(state);
    load_ui_settings_from_file(state, "ui_state.dat");
}

static void begin_ui(UIState *state, PlatformInput *input, i32 window_width, i32 window_height)
{
    state->imgui_io->DisplaySize = ImVec2((f32)window_width, (f32)window_height);
    state->imgui_io->DeltaTime = input->dt;

    state->imgui_io->MousePos = ImVec2((f32)input->mouse_pos[0], (f32)input->mouse_pos[1]);
    state->imgui_io->MouseDown[0] = input->mouse_buttons[mouse_button_left].down != 0;
    state->imgui_io->MouseDown[1] = input->mouse_buttons[mouse_button_right].down != 0;
    state->imgui_io->MouseDown[2] = input->mouse_buttons[mouse_button_middle].down != 0;
    state->imgui_io->MouseWheel = (f32)input->wheel;

    state->imgui_io->KeyCtrl = (is_down(input, button_control_left) || is_down(input, button_control_right));
    state->imgui_io->KeyShift = (is_down(input, button_shift_left) || is_down(input, button_shift_right));
    state->imgui_io->KeyAlt = (is_down(input, button_alt_left) || is_down(input, button_alt_right));

    for (u32 character_index = 0; character_index < input->text_input_length; ++character_index)
    {
        u16 *c = input->text_input + character_index;
        state->imgui_io->AddInputCharacter(*c);
    }

    for (u32 button_index = 0; button_index < array_count(input->buttons); ++button_index)
    {
        state->imgui_io->KeysDown[button_index] = (input->buttons[button_index].down != 0);
    }

    ImGui::NewFrame();
}

static void end_ui(UIState *state)
{
    ImVec2 fb_size = state->imgui_io->DisplaySize;
    ImVec2 fb_scale = state->imgui_io->DisplayFramebufferScale;
    i32 fb_width = (i32)(fb_size.x * fb_scale.x);
    i32 fb_height = (i32)(fb_size.y * fb_scale.y);

    assert(fb_width);
    assert(fb_height);

    ImGui::Render();
    ImDrawData *data = ImGui::GetDrawData();    

    gl.Enable(GL_BLEND);
    gl.BlendEquation(GL_FUNC_ADD);
    gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl.Disable(GL_CULL_FACE);
    gl.Disable(GL_DEPTH_TEST);
    gl.ActiveTexture(GL_TEXTURE0);
  
    gl.Viewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    f32 a = 2.0f /  state->imgui_io->DisplaySize.x;
    f32 b = 2.0f / -state->imgui_io->DisplaySize.y;
    f32 proj_mat[4][4] =
    {
        { a,     0.0f,  0.0f, 0.0f},
        { 0.0f,  b,     0.0f, 0.0f},
        { 0.0f,  0.0f, -1.0f, 0.0f},
        {-1.0f,  1.0f,  0.0f, 1.0f},
    };
    gl.UseProgram(state->program);
    gl.Uniform1i(state->uniforms[uniform_tex], 0);
    gl.UniformMatrix4fv(state->uniforms[uniform_proj_mat], 1, GL_FALSE, &proj_mat[0][0]);
    gl.BindVertexArray(state->vao);

    gl.Enable(GL_SCISSOR_TEST);
    data->ScaleClipRects(fb_scale);
    for (i32 list_index = 0; list_index < data->CmdListsCount; ++list_index)
    {
        ImDrawList *list = data->CmdLists[list_index];
        ImDrawIdx *index_buff_offset = 0;

        gl.BindBuffer(GL_ARRAY_BUFFER, state->vbo);
        gl.BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)list->VtxBuffer.Size * sizeof(ImDrawVert), (GLvoid *)list->VtxBuffer.Data, GL_STREAM_DRAW);

        gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->elements);
        gl.BufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)list->IdxBuffer.Size * sizeof(ImDrawIdx), (GLvoid *)list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (i32 cmd_index = 0; cmd_index < list->CmdBuffer.Size; ++cmd_index)
        {
            ImDrawCmd *cmd = &list->CmdBuffer[cmd_index];
            if (cmd->UserCallback)
            {
                cmd->UserCallback(list, cmd);
            }
            else
            {
                gl.BindTexture(GL_TEXTURE_2D, (GLuint)(intptr)cmd->TextureId);
                gl.Scissor((int)cmd->ClipRect.x, (int)(fb_height - cmd->ClipRect.w), (int)(cmd->ClipRect.z - cmd->ClipRect.x), (int)(cmd->ClipRect.w - cmd->ClipRect.y));
                gl.DrawElements(GL_TRIANGLES, (GLsizei)cmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, index_buff_offset);
            }
            index_buff_offset += cmd->ElemCount;
        }
    }
    gl.Disable(GL_SCISSOR_TEST);
}
