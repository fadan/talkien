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
            ImGui::Text("Frame time: %.2f ms ", 1000.0f / global_imgui->Framerate);
        }
        ImGui::End();

        ShowExampleAppConsole(0);
        ImGui::ShowTestWindow(0);

        profiler_report(memory);
    }
    end_ui();

}

// TODO(dan): temp only, remove
#pragma warning(push)
#pragma warning(disable: 4996)

#define _CRT_SECURE_NO_WARNINGS
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

static LoadedWav load_wav(void *memory, usize size)
{
    LoadedWav wav = {0};
    RiffHeader *header = (RiffHeader *)memory;
    if (header->chunk.id == RiffChunkID_Riff)
    {
        if (header->format == RiffChunkID_Wave)
        {
            u32 total_size = 0;
            for (RiffIterator iterator = get_riff_iterator(header); riff_iterator_valid(iterator); iterator = next_riff_iterator(iterator))
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

            u32 sample_size = wav.num_channels * (wav.bits_per_sample / 8);
            if (sample_size > 0)
            {
                wav.num_samples = total_size / sample_size;
            }
        }
    }
    return wav;
}

struct AudioRecord
{
    u64 total_samples_written;
    u64 total_samples_read;
    u32 num_buffer_samples;
    f32 *samples;

    f32 volume[2];
    b32 muted;

    union
    {
        AudioRecord *next;
        AudioRecord *next_free;
    };
};

static AudioRecord *allocate_audio_record(AudioState *state)
{
    AudioRecord *record = 0;
    allocate_freelist(record, state->first_free_record, push_struct(&state->audio_memory, AudioRecord));
    return record;
}

static AudioRecord *init_wav(AudioState *state)
{
    AudioRecord *record = allocate_audio_record(state);
    LoadedFile file = load_file("test2.wav");
    if (file.contents)
    {
        LoadedWav wav = load_wav(file.contents, file.size);

        assert(wav.num_channels == 2);
        assert(wav.bits_per_sample == 16);
        assert(wav.samples_per_sec == 44100);

        record->num_buffer_samples = wav.num_samples * wav.num_channels;
        record->samples = push_array(&state->audio_memory, record->num_buffer_samples, f32, align_no_clear(16));

        i16 *sample = wav.samples;
        for (u32 sample_index = 0; sample_index < record->num_buffer_samples; ++sample_index)
        {
            record->samples[sample_index] = *sample++ / 32678.0f;
        }

        record->total_samples_written = record->num_buffer_samples;
        record->total_samples_read = 0;

        record->volume[0] = record->volume[1] = 0.5f;
    }
    close_file(file);
    return record;
}

static void init_recording(AudioState *state, AudioRecord *record, u16 num_channels, u16 bits_per_sample, u32 samples_per_sec)
{
    assert(num_channels == 2); // TODO(dan): 2 for now
    assert(bits_per_sample == 32);

    record->num_buffer_samples = 5 * samples_per_sec; // NOTE(dan): 1 sec buffer
    record->samples = push_array(&state->audio_memory, record->num_buffer_samples, f32, align_no_clear(16));

    record->total_samples_written = 0;
    record->total_samples_read = 0;

    record->volume[0] = record->volume[1] = 0.5f;
}

static void add_audio_record(AudioState *state, AudioRecord *record)
{
    record->next = state->first_record;
    state->first_record = record;
}

static AudioState *get_or_init_audio_state(AppMemory *memory)
{
    AudioState *audio_state = memory->audio_state;
    if (!audio_state)
    {
        audio_state = memory->audio_state = bootstrap_push_struct(AudioState, audio_memory);

        AudioRecord *local_record = audio_state->local_record = allocate_audio_record(audio_state);
        init_recording(audio_state, local_record, 2, 32, 44100);
        add_audio_record(audio_state, local_record);

        // AudioRecord *test_wav = init_wav(audio_state);
        // test_wav->muted = true;
        // add_audio_record(audio_state, test_wav);
    }
    return audio_state;
}

static void copy_audio_samples(f32 *src, f32 *dest, u32 num_samples)
{
    #if 1
    memcpy(dest, src, num_samples * sizeof(f32));
    #else
    for (u32 sample_index = 0; sample_index < num_samples; ++sample_index)
    {
        *dest++ = *src++;
    }
    #endif
}

extern "C" __declspec(dllexport) CAPTURE_SOUND_BUFFER(capture_sound_buffer)
{
    PROFILER_FUNCTION();

    AudioState *audio_state = get_or_init_audio_state(memory);
    AudioRecord *local_record = audio_state->local_record;

    #if 0
    {
        for (u32 sample_index = 0; sample_index < num_samples; ++sample_index)
        {
            f32 *sample = local_record->samples + ((local_record->total_samples_written + sample_index) % local_record->num_buffer_samples);

            *sample = buffer[sample_index];
        }
        local_record->total_samples_written += num_samples;
    }
    #else
    {
        u32 remaining_samples = local_record->num_buffer_samples - (local_record->total_samples_written % local_record->num_buffer_samples);
        if (num_samples > remaining_samples)
        {
            f32 *src1 = buffer;
            f32 *dest1 = local_record->samples + (local_record->total_samples_written % local_record->num_buffer_samples);
            u32 src1_sample_count = remaining_samples;
            copy_audio_samples(dest1, src1, src1_sample_count);

            f32 *src2 = buffer + src1_sample_count;
            f32 *dest2 = local_record->samples;
            u32 src2_sample_count = num_samples - src1_sample_count;
            copy_audio_samples(dest2, src2, src2_sample_count);
        }
        else
        {
            f32 *src = buffer;
            f32 *dest = local_record->samples + (local_record->total_samples_written % local_record->num_buffer_samples);
            copy_audio_samples(src, dest, num_samples);
        }
        local_record->total_samples_written += num_samples;
    }
    #endif
}

extern "C" __declspec(dllexport) FILL_SOUND_BUFFER(fill_sound_buffer)
{
    PROFILER_FUNCTION();

    platform = memory->platform;
    profiler = memory->profiler;

    AudioState *audio_state = get_or_init_audio_state(memory);

    for (u32 sample_index = 0; sample_index < num_samples; ++sample_index)
    {
        buffer[sample_index] = 0;
    }

    for (AudioRecord *record = audio_state->first_record; record; record = record->next)
    {
        if (record->muted)
        {
            continue;
        }

        assert(record->total_samples_read <= record->total_samples_written);
        assert((record->total_samples_written - record->total_samples_read) < 0xFFFFFFFF);

        u32 samples_available = (u32)(record->total_samples_written - record->total_samples_read);
        if (samples_available > num_samples)
        {
            samples_available = num_samples;
        }

        #if 0
        {
            for (u32 sample_index = 0; sample_index < samples_available; ++sample_index)
            {
                f32 *sample = record->samples + ((record->total_samples_read + sample_index) % record->num_buffer_samples);

                buffer[sample_index] += *sample;
            }
            record->total_samples_read += samples_available;
        }
        #else
        {
            u32 remaining_samples = record->num_buffer_samples - (record->total_samples_read % record->num_buffer_samples);

            if (samples_available > remaining_samples)
            {
                f32 *src1 = record->samples + (record->total_samples_read % record->num_buffer_samples);
                f32 *dest1 = buffer;
                u32 src1_sample_count = remaining_samples;

                for (u32 sample_index = 0; sample_index < src1_sample_count; ++sample_index)
                {
                    *dest1++ += *src1++;
                }
                record->total_samples_read += src1_sample_count;

                f32 *src2 = record->samples;
                f32 *dest2 = buffer + src1_sample_count;
                u32 src2_sample_count = samples_available - src1_sample_count;

                for (u32 sample_index = 0; sample_index < src2_sample_count; ++sample_index)
                {
                    *dest2++ += *src2++;
                }
                record->total_samples_read += src2_sample_count;
            }
            else
            {
                f32 *src = record->samples + (record->total_samples_read % record->num_buffer_samples);
                f32 *dest = buffer;

                for (u32 sample_index = 0; sample_index < samples_available; ++sample_index)
                {
                    *dest++ += *src++;
                }
                record->total_samples_read += samples_available;
            }
        }
        #endif
    }
}
