#include "ui.cpp"

static void update_and_render(PlatformInput *input, i32 window_width, i32 window_height)
{
    static b32 initialized = false;
    if (!initialized)
    {
        init_ui();
        initialized = true;
    }
    
    ImVec4 clear_color = ImColor(114, 144, 154);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);

    glScissor(0, 0, window_width, window_height);
    
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, window_width, window_height);

    begin_ui(input, window_width, window_height);
    {
        //static bool show_test_window = true;
        //ImGui::ShowTestWindow(&show_test_window);

        ImGui::Begin("", 0, 0);
        {
            ImGui::Text("window_width: %d", window_width);
            ImGui::Text("window_height: %d", window_height);

            ImGui::Text("mouse_x: %f", input->mouse_x);
            ImGui::Text("mouse_y: %f", input->mouse_y);

            ImGui::Text("ImGui::MousePos.x: %f", global_imgui->MousePos.x);
            ImGui::Text("ImGui::MousePos.y: %f", global_imgui->MousePos.y);

            ImGui::Text("ImGui::DisplaySize.x: %f", global_imgui->DisplaySize.x);
            ImGui::Text("ImGui::DisplaySize.y: %f", global_imgui->DisplaySize.y);

            ImGui::Text("ImGui::Framerate: %f ms/f", 1000.0f / global_imgui->Framerate);
        }
        ImGui::End();
    }
    end_ui();
}
