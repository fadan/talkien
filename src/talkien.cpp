#include "ui.cpp"

struct AppState
{
    MemoryStack total_memory;
};

static void update_and_render(AppMemory *memory, PlatformInput *input, i32 window_width, i32 window_height)
{
    AppState *app_state = memory->app_state;
    if (!app_state)
    {
        app_state = memory->app_state = bootstrap_push_struct(AppState, total_memory);
        init_ui();
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
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // ShowExampleMenuFile();
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {

                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        
        ImGui::SetNextWindowPos(ImVec2(10, 30));

        ImGui::Begin("", 0, ImVec2(0, 0), 0.3f, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
        {
            PlatformMemoryStats memory_stats = platform.get_memory_stats();

            ImGui::Text("Mouse: (%.1f,%.1f) ", global_imgui->MousePos.x, global_imgui->MousePos.y);
            ImGui::Text("Allocs: %d ", global_imgui->MetricsAllocs);
            ImGui::Text("Vertices: %d Indices: %3d  ", global_imgui->MetricsRenderVertices, global_imgui->MetricsRenderIndices);
            ImGui::Text("Windows: %d ", global_imgui->MetricsActiveWindows);
            ImGui::Text("Memory blocks: %d Size: %dK Used: %db ", memory_stats.num_memblocks, memory_stats.total_size/KB, memory_stats.total_used);
            ImGui::Text("Frame time: %.2f ms/f ", 1000.0f / global_imgui->Framerate);
        }
        ImGui::End();
    }
    end_ui();
}
