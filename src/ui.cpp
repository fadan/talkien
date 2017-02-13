#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#include "imgui.h"

#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui.cpp"

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

    attribs[attrib_pos] = glGetAttribLocationARB(ui_program, "pos");
    attribs[attrib_uv] = glGetAttribLocationARB(ui_program, "uv");
    attribs[attrib_color] = glGetAttribLocationARB(ui_program, "color");
    check_bindings(attribs, attrib_count);

    uniforms[uniform_tex] = glGetUniformLocationARB(ui_program, "tex");
    uniforms[uniform_proj_mat] = glGetUniformLocationARB(ui_program, "proj_mat");
    check_bindings(uniforms, uniform_count);

    glGenBuffersARB(1, &vbo);
    glGenBuffersARB(1, &elements);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);

    glEnableVertexAttribArrayARB(attribs[attrib_pos]);
    glEnableVertexAttribArrayARB(attribs[attrib_uv]);
    glEnableVertexAttribArrayARB(attribs[attrib_color]);

    glVertexAttribPointerARB(attribs[attrib_pos], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)offset_of(ImDrawVert, pos));
    glVertexAttribPointerARB(attribs[attrib_uv], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)offset_of(ImDrawVert, uv));
    glVertexAttribPointerARB(attribs[attrib_color], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid *)offset_of(ImDrawVert, col));

    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    global_imgui->Fonts->TexID = (void *)(intptr)font_texture;
}

static void begin_ui(PlatformInput *input, i32 window_width, i32 window_height)
{
    global_imgui->DisplaySize = ImVec2((f32)window_width, (f32)window_height);
    global_imgui->DeltaTime = input->dt;

    // TODO(dan): scissors seems to be wrong
    global_imgui->MousePos = ImVec2(input->mouse_x, input->mouse_y + 40);
    global_imgui->MouseDown[0] = input->mouse_buttons[mouse_button_left] != 0;
    global_imgui->MouseDown[1] = input->mouse_buttons[mouse_button_right] != 0;
    global_imgui->MouseDown[2] = input->mouse_buttons[mouse_button_middle] != 0;
    global_imgui->MouseWheel = input->mouse_z;

    global_imgui->KeyCtrl = input->ctrl_down != 0;
    global_imgui->KeyShift = input->shift_down != 0;
    global_imgui->KeyAlt = input->alt_down != 0;

    ImGui::NewFrame();
}

static void end_ui()
{
    ImVec4 clear_color = ImColor(114, 144, 154);

    i32 fb_width = (i32)(global_imgui->DisplaySize.x);
    i32 fb_height = (i32)(global_imgui->DisplaySize.y);

    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, fb_width, fb_height);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Render();
    ImDrawData *data = ImGui::GetDrawData();

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTextureARB(GL_TEXTURE0);

    // NOTE(dan): ortho proj matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    
    f32 proj_mat[4][4] =
    {
        { 2.0f / global_imgui->DisplaySize.x, 0.0f,                      0.0f, 0.0f},
        { 0.0f,                     2.0f / -global_imgui->DisplaySize.y, 0.0f, 0.0f},
        { 0.0f,                     0.0f,                               -1.0f, 0.0f},
        {-1.0f,                     1.0f,                                0.0f, 1.0f},
    };

    glUseProgramObjectARB(ui_program);
    glUniform1iARB(uniforms[uniform_tex], 0);
    glUniformMatrix4fvARB(uniforms[uniform_proj_mat], 1, GL_FALSE, &proj_mat[0][0]);
    glBindVertexArray(vao);

    for (i32 list_index = 0; list_index < data->CmdListsCount; ++list_index)
    {
        ImDrawList *list = data->CmdLists[list_index];
        ImDrawIdx *index_buff_offset = 0;

        glBindBufferARB(GL_ARRAY_BUFFER, vbo);
        glBufferDataARB(GL_ARRAY_BUFFER, (GLsizeiptr)list->VtxBuffer.Size * sizeof(ImDrawVert), (GLvoid *)list->VtxBuffer.Data, GL_STREAM_DRAW);

        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, elements);
        glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)list->IdxBuffer.Size * sizeof(ImDrawIdx), (GLvoid *)list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (i32 cmd_index = 0; cmd_index < list->CmdBuffer.Size; ++cmd_index)
        {
            ImDrawCmd *cmd = &list->CmdBuffer[cmd_index];
            if (cmd->UserCallback)
            {
                cmd->UserCallback(list, cmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr)cmd->TextureId);
                glScissor((int)cmd->ClipRect.x, (int)(fb_height - cmd->ClipRect.w), (int)(cmd->ClipRect.z - cmd->ClipRect.x), (int)(cmd->ClipRect.w - cmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)cmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, index_buff_offset);
            }
            index_buff_offset += cmd->ElemCount;
        }
    }
}
