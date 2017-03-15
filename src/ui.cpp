#pragma warning(push)
#pragma warning(disable: 4459)
#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#include "imgui.h"

#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui.cpp"
#pragma warning(pop)

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

enum
{
    attrib_pos,
    attrib_uv,
    attrib_color,

    attrib_count,
};

enum
{
    uniform_tex,
    uniform_proj_mat,

    uniform_count,
};

static GLint attribs[attrib_count];
static GLint uniforms[uniform_count];

static GLuint ui_program;

static GLuint vbo;
static GLuint vao;
static GLuint elements;
static GLuint font_texture;

static ImGuiIO *global_imgui = &GImGui->IO;

inline void check_bindings(GLint *buffer, GLint num_elements)
{
    for (GLint element_index = 0; element_index < num_elements; ++element_index)
    {
        assert(buffer[element_index] != -1);
    }
}

static void init_ui()
{
    u8 *pixels;
    i32 width, height;
    global_imgui->Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    char error[1024];
    ui_program = opengl_create_program(ui_vertex_shader, ui_fragment_shader, error, sizeof(error));
    assert(ui_program);

    attribs[attrib_pos] = gl.GetAttribLocation(ui_program, "pos");
    attribs[attrib_uv] = gl.GetAttribLocation(ui_program, "uv");
    attribs[attrib_color] = gl.GetAttribLocation(ui_program, "color");
    check_bindings(attribs, attrib_count);

    uniforms[uniform_tex] = gl.GetUniformLocation(ui_program, "tex");
    uniforms[uniform_proj_mat] = gl.GetUniformLocation(ui_program, "proj_mat");
    check_bindings(uniforms, uniform_count);

    gl.GenBuffers(1, &vbo);
    gl.GenBuffers(1, &elements);

    gl.GenVertexArrays(1, &vao);
    gl.BindVertexArray(vao);
    gl.BindBuffer(GL_ARRAY_BUFFER, vbo);

    gl.EnableVertexAttribArray(attribs[attrib_pos]);
    gl.EnableVertexAttribArray(attribs[attrib_uv]);
    gl.EnableVertexAttribArray(attribs[attrib_color]);

    gl.VertexAttribPointer(attribs[attrib_pos], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)offset_of(ImDrawVert, pos));
    gl.VertexAttribPointer(attribs[attrib_uv], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)offset_of(ImDrawVert, uv));
    gl.VertexAttribPointer(attribs[attrib_color], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid *)offset_of(ImDrawVert, col));

    gl.GenTextures(1, &font_texture);
    gl.BindTexture(GL_TEXTURE_2D, font_texture);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    global_imgui->Fonts->TexID = (void *)(intptr)font_texture;

    ImGuiStyle *style = &ImGui::GetStyle();

    style->WindowPadding = ImVec2(15, 15);
    style->WindowRounding = 5.0f;
    style->FramePadding = ImVec2(5, 5);
    style->FrameRounding = 4.0f;
    style->ItemSpacing = ImVec2(12, 8);
    style->ItemInnerSpacing = ImVec2(8, 6);
    style->IndentSpacing = 25.0f;
    style->ScrollbarSize = 15.0f;
    style->ScrollbarRounding = 9.0f;
    style->GrabMinSize = 5.0f;
    style->GrabRounding = 3.0f;
 
    style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
    style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
    style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
    style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
    style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_ComboBg] = ImVec4(0.19f, 0.18f, 0.21f, 1.00f);
    style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_CloseButton] = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
    style->Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
    style->Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
    style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
    style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
    style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
    style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
}

static void begin_ui(PlatformInput *input, i32 window_width, i32 window_height)
{
    global_imgui->DisplaySize = ImVec2((f32)window_width, (f32)window_height);
    global_imgui->DeltaTime = input->dt;

    global_imgui->MousePos = ImVec2(input->mouse_x, input->mouse_y);
    global_imgui->MouseDown[0] = input->mouse_buttons[mouse_button_left].is_down != 0;
    global_imgui->MouseDown[1] = input->mouse_buttons[mouse_button_right].is_down != 0;
    global_imgui->MouseDown[2] = input->mouse_buttons[mouse_button_middle].is_down != 0;
    global_imgui->MouseWheel = input->mouse_z;

    global_imgui->KeyCtrl = input->ctrl_down != 0;
    global_imgui->KeyShift = input->shift_down != 0;
    global_imgui->KeyAlt = input->alt_down != 0;

    for (u32 character_index = 0; character_index < input->character_count; ++character_index)
    {
        char *character = input->characters + character_index;
        global_imgui->AddInputCharacter(*character);
    }
    for (u32 button_index = 0; button_index < button_count; ++button_index)
    {
        global_imgui->KeysDown[button_index] = (input->buttons[button_index].is_down != 0);
    }

    ImGui::NewFrame();
}

static void end_ui()
{
    ImVec2 fb_size = global_imgui->DisplaySize;
    ImVec2 fb_scale = global_imgui->DisplayFramebufferScale;
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
    f32 a = 2.0f /  global_imgui->DisplaySize.x;
    f32 b = 2.0f / -global_imgui->DisplaySize.y;
    f32 proj_mat[4][4] =
    {
        { a,     0.0f,  0.0f, 0.0f},
        { 0.0f,  b,     0.0f, 0.0f},
        { 0.0f,  0.0f, -1.0f, 0.0f},
        {-1.0f,  1.0f,  0.0f, 1.0f},
    };
    gl.UseProgram(ui_program);
    gl.Uniform1i(uniforms[uniform_tex], 0);
    gl.UniformMatrix4fv(uniforms[uniform_proj_mat], 1, GL_FALSE, &proj_mat[0][0]);
    gl.BindVertexArray(vao);

    gl.Enable(GL_SCISSOR_TEST);
    data->ScaleClipRects(fb_scale);
    for (i32 list_index = 0; list_index < data->CmdListsCount; ++list_index)
    {
        ImDrawList *list = data->CmdLists[list_index];
        ImDrawIdx *index_buff_offset = 0;

        gl.BindBuffer(GL_ARRAY_BUFFER, vbo);
        gl.BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)list->VtxBuffer.Size * sizeof(ImDrawVert), (GLvoid *)list->VtxBuffer.Data, GL_STREAM_DRAW);

        gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements);
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
