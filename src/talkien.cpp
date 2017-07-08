#include "platform.h"
#include "opengl.h"
#include "talkien.h"
#include "ui.cpp"
#include "profiler_process.h"

#include "profiler_draw.cpp"
#include "profiler_process.cpp"

static void opengl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar *message, GLvoid *user_param)
{
    if (severity == GL_DEBUG_SEVERITY_HIGH)
    {
        char *error = (char *)message;
        assert(error);
    }
}

static void init_app_state()
{
    platform.init_opengl(&gl);
    init_ui();

    if (gl.DebugMessageCallback)
    {
        gl.Enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        gl.DebugMessageCallback(opengl_debug_callback, 0);
    }
}

extern "C" __declspec(dllexport) UPDATE_AND_RENDER(update_and_render)
{
    PROFILER_FUNCTION();

    AppState *app_state = memory->app_state;
    if (!app_state)
    {
        app_state = memory->app_state = bootstrap_push_struct(AppState, app_memory);
        platform = memory->platform;
        profiler = memory->profiler;

        init_app_state();
    }

    if (memory->app_dll_reloaded)
    {
        platform = memory->platform;
        profiler = memory->profiler;

        init_app_state();
    }

    ImVec4 clear_color = ImColor(85, 118, 152);
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

        ImGui::Begin("", 0, ImVec2(0, 0), 1.0f, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
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

        ImGui::ShowTestWindow(&show);
    }

    {
        profiler_report(memory);
    }
    end_ui();

}

static LoadedWav load_wav(void *memory, usize size)
{
    LoadedWav wav = {0};
    RiffHeader *header = (RiffHeader *)memory;
    if (header->chunk.id == RiffChunkID_Riff)
    {
        if (header->format == RiffChunkID_Wave)
        {
            RiffIterator iterator;
            u32 total_size = 0;
            u32 sample_size = 0;

            for (iterator = get_riff_iterator(header); riff_iterator_valid(iterator); iterator = next_riff_iterator(iterator))
            {
                RiffChunk *chunk = (RiffChunk *)iterator.at;

                switch (chunk->id)
                {
                    case RiffChunkID_Fmt:
                    {
                        WaveFormatChunk *fmt = (WaveFormatChunk *)(iterator.at + sizeof(RiffChunk));
                        wav.num_channels = fmt->num_channels;
                        wav.samples_per_sec = fmt->samples_per_sec;

                        if (fmt->format_tag == WAVE_FORMAT_PCM)
                        {
                            WaveFormatPCMChunk *pcm = (WaveFormatPCMChunk *)((u8 *)(fmt + 1));
                            wav.bits_per_sample = pcm->bits_per_sample;
                        }
                    } break;

                    case RiffChunkID_Data:
                    {
                        wav.samples = (i16 *)((u8 *)iterator.at + sizeof(RiffChunk));
                        total_size = chunk->size;
                    } break;
                }
            }

            sample_size = wav.num_channels * (wav.bits_per_sample / 8);
            if (sample_size > 0)
            {
                wav.num_samples = total_size / sample_size;
            }
        }
    }
    return wav;
}

// TODO(dan): temp only, remove
#define _CRT_SECURE_NO_WARNINGS

#pragma warning(push)
#pragma warning(disable: 4996)

#include <math.h>
#include <stdio.h>

struct LoadedFile
{
    u32 size;
    void *contents;
};

static LoadedFile load_file(char *filename)
{
    LoadedFile file = {0};
    FILE *handle = fopen(filename, "r");
    if (handle)
    {
        fseek(handle, 0, SEEK_END);
        file.size = ftell(handle);
        file.contents = malloc(file.size);
        fseek(handle, 0, SEEK_SET);
        fread(file.contents, file.size, 1, handle);
        fclose(handle);
    }
    return file;
}

static void close_file(LoadedFile file)
{
    if (file.contents)
    {
        free(file.contents);
    }
}
#pragma warning(pop)

static u32 total_wav_samples;
static u32 played_wav_samples;

static i16 *wav_samples;

struct AudioRecord
{
    u32 num_samples_written;
    u32 num_samples_read;
    u32 max_sample_count;
    f32 *samples;

    f32 volume[2];

    union
    {
        AudioRecord *next;
        AudioRecord *next_free;
    };
};

static AudioRecord *allocate_audio_record(AudioState *state)
{
    AudioRecord *record;
    allocate_freelist(record, state->first_free_record, push_struct(&state->audio_memory, AudioRecord));
    return record;
}

static void init_recording(AudioState *state, AudioRecord *record, u16 num_channels, u16 bits_per_sample, u32 samples_per_sec)
{
    assert(num_channels == 2); // TODO(dan): 2 for now
    assert(bits_per_sample == 32);

    record->max_sample_count = 1 * samples_per_sec; // NOTE(dan): 1 sec buffer
    record->samples = push_array(&state->audio_memory, record->max_sample_count, f32, align_no_clear(16));

    record->num_samples_written = 0;
    record->num_samples_read = 0;

    record->volume[0] = record->volume[1] = 0.5f;
}

static AudioState *get_or_init_audio_state(AppMemory *memory)
{
    AudioState *audio_state = memory->audio_state;
    if (!audio_state)
    {
        audio_state = memory->audio_state = bootstrap_push_struct(AudioState, audio_memory);
        audio_state->local_record = allocate_audio_record(audio_state);
        init_recording(audio_state, audio_state->local_record, 2, 32, 44100);
    }
    return audio_state;
}

extern "C" __declspec(dllexport) CAPTURE_SOUND_BUFFER(capture_sound_buffer)
{
    PROFILER_FUNCTION();

    AudioState *audio_state = get_or_init_audio_state(memory);
    AudioRecord *local_record = audio_state->local_record;

    u32 remaining_samples = local_record->max_sample_count - local_record->num_samples_written;
    if (num_samples > remaining_samples)
    {
        f32 *src1 = buffer;
        f32 *dest1 = local_record->samples + local_record->num_samples_written;
        u32 src1_sample_count = remaining_samples;

        f32 *src2 = buffer + src1_sample_count;
        f32 *dest2 = local_record->samples;
        u32 src2_sample_count = num_samples - src1_sample_count;

        for (u32 sample_index = 0; sample_index < src1_sample_count; ++sample_index)
        {
            *dest1++ = *src1++;
        }

        for (u32 sample_index = 0; sample_index < src2_sample_count; ++sample_index)
        {
            *dest2++ = *src1++;
        }

        local_record->num_samples_written = src2_sample_count;
    }
    else
    {
        f32 *src = buffer;
        f32 *dest = local_record->samples + local_record->num_samples_written;

        for (u32 sample_index = 0; sample_index < num_samples; ++sample_index)
        {
            *dest++ = *src++;
        }

        local_record->num_samples_written += num_samples;
    }

    assert(local_record->num_samples_written < local_record->max_sample_count);
}

extern "C" __declspec(dllexport) FILL_SOUND_BUFFER(fill_sound_buffer)
{
    PROFILER_FUNCTION();

    platform = memory->platform;
    profiler = memory->profiler;

    AudioState *audio_state = get_or_init_audio_state(memory);

    if (!wav_samples)
    {
        LoadedFile file = load_file("test2.wav");
        if (file.contents)
        {
            LoadedWav wav = load_wav(file.contents, file.size);

            total_wav_samples = wav.num_samples;
            played_wav_samples = 0;

            wav_samples = wav.samples;
        }
        // close_file(file);
    }

#if 0
    // PROFILER_BEGIN("Clear Sound Buffer");
    for (u32 sample_index = 0; sample_index < num_samples; ++sample_index)
    {
        buffer[sample_index] = 0;
    }
    // PROFILER_END();
#else
    i16 *sample = wav_samples + played_wav_samples;

    for (u32 sample_index = 0; sample_index < num_samples; ++sample_index)
    {
        buffer[sample_index] = *sample++ / 32678.0f;
    }

    played_wav_samples += num_samples;
#endif
}
