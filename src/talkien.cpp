#include "platform.h"
#include "opengl.h"
#include "ui.cpp"

struct AudioState
{
    MemoryStack audio_memory;

    u32 last_mix_length;
    f64 sin_pos;
};

struct AppState
{
    MemoryStack app_memory;
};

Platform platform;
OpenGL gl;

extern "C" __declspec(dllexport) UPDATE_AND_RENDER(update_and_render)
{
    platform = memory->platform;

    AppState *app_state = memory->app_state;
    if (!app_state)
    {
        app_state = memory->app_state = bootstrap_push_struct(AppState, app_memory);
        platform.init_opengl(&gl);
        init_ui();
    }

    if (memory->app_dll_reloaded)
    {
        platform.init_opengl(&gl);
        init_ui();
    }

    ImVec4 clear_color = ImColor(49, 55, 66);
    gl.Enable(GL_SCISSOR_TEST);
    gl.Enable(GL_BLEND);

    gl.Scissor(0, 0, window_width, window_height);
    
    gl.ClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    gl.Clear(GL_COLOR_BUFFER_BIT);

    gl.Viewport(0, 0, window_width, window_height);

    begin_ui(input, window_width, window_height);
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // ShowExampleMenuFile();
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    input->quit_requested = true;
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
            // ImGui::Text("Mix length: %f (%d)", memory->audio_state->last_mix_length / 44100.0f, memory->audio_state->last_mix_length);
            ImGui::Text("Frame time: %.2f ms/f ", 1000.0f / global_imgui->Framerate);
        }
        ImGui::End();

        bool show = true;
        ShowExampleAppConsole(&show);
    }
    end_ui();
}

#include <math.h>

extern "C" __declspec(dllexport) FILL_SOUND_BUFFER(fill_sound_buffer)
{
    platform = memory->platform;

    AudioState *audio_state = memory->audio_state;
    if (!audio_state)
    {
        audio_state = memory->audio_state = bootstrap_push_struct(AudioState, audio_memory);
    }

    for (u32 sample_index = 0; sample_index < num_samples; sample_index += 2)
    {
        f32 s = 0.0f;

        s += (f32)sin(audio_state->sin_pos * 2.0f * 3.141592f * 100.0f / 44100.0f);
        // s += (f32)sinf(pos * 2.0f * 3.141592f * 200.0f / 44100.0f) / 2.0f;
        // s += (f32)sinf(pos * 2.0f * 3.141592f * 300.0f / 44100.0f) / 3.0f;
        // s += (f32)sinf(pos * 2.0f * 3.141592f * 400.0f / 44100.0f) / 4.0f;
        // s += (f32)sinf(pos * 2.0f * 3.141592f * 500.0f / 44100.0f) / 5.0f;
        // s += (f32)sinf(pos * 2.0f * 3.141592f * 600.0f / 44100.0f) / 8.0f;
        // s += (f32)sinf(pos * 2.0f * 3.141592f * 700.0f / 44100.0f) / 10.0f;
        // s += (f32)sinf(pos * 2.0f * 3.141592f * 800.0f / 44100.0f) / 13.0f;

        buffer[sample_index + 1] = buffer[sample_index] = s / 30.0f;

        ++audio_state->sin_pos;
    }

    audio_state->last_mix_length = num_samples / 2;
}
