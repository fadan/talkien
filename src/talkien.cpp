#include "platform.h"
#include "opengl.h"
#include "ui.h"
#include "talkien.h"
#include "profiler_process.h"

#include "ui.cpp"
#include "profiler_draw.cpp"
#include "profiler_process.cpp"

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

static void opengl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar *message, GLvoid *user_param)
{
    if (severity == GL_DEBUG_SEVERITY_HIGH)
    {
        char *error = (char *)message;
        assert(error);
    }
}

static AudioRecord *allocate_audio_record(AudioState *state)
{
    AudioRecord *record = 0;
    allocate_freelist(record, state->first_free_record, push_struct(&state->audio_memory, AudioRecord));
    record->next = state->first_record;
    state->first_record = record;
    return record;
}

static void init_recording(AudioState *state, AudioRecord *record, u16 num_channels, u16 bits_per_sample, u32 samples_per_sec)
{
    assert(num_channels == 2); // TODO(dan): stereo for now
    assert(bits_per_sample == 32);

    record->num_buffer_samples = 1 * samples_per_sec; // NOTE(dan): 1 sec buffer
    record->samples = push_array(&state->audio_memory, record->num_buffer_samples, f32, align_clear(16));

    record->total_samples_written = 0;
    record->total_samples_read = 0;

    record->volume[0] = record->volume[1] = 0.5f;
}

static void init_wav(AudioState *state, AudioRecord *record)
{
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
}

static AppState *get_or_create_app_state(AppMemory *memory)
{
    AppState *app_state = memory->app_state;
    if (!app_state)
    {
        platform = memory->platform;
        profiler = memory->profiler;

        app_state = memory->app_state = bootstrap_push_struct(AppState, app_memory);
    }

    if (memory->app_dll_reloaded)
    {
        platform = memory->platform;
        profiler = memory->profiler;
    }
    return app_state;
}

static AudioState *get_or_create_audio_state(AppState *app_state)
{
    AudioState *audio_state = &app_state->audio_state;
    if (!audio_state->initialized)
    {
        audio_state->master_volume[0] = 1.0f;
        audio_state->master_volume[1] = 1.0f;

        init_memory_stack(&audio_state->audio_memory, 1*MB);
        init_memory_stack(&audio_state->mixer_memory, 1*MB);

        audio_state->local_record = allocate_audio_record(audio_state);
        init_recording(audio_state, audio_state->local_record, 2, 32, 44100);

        #if 0
        AudioRecord *test_wav = allocate_audio_record(audio_state);
        init_wav(audio_state, test_wav);
        test_wav->muted = true;
        #endif

        audio_state->initialized = true;
    }
    return audio_state;
}

static void copy_audio_samples(f32 *src, f32 *dest, u32 num_samples)
{
    for (u32 sample_index = 0; sample_index < num_samples; ++sample_index)
    {
        *dest++ = *src++;
    }
}

extern "C" __declspec(dllexport) CAPTURE_SOUND_BUFFER(capture_sound_buffer)
{
    PROFILER_FUNCTION();

    AppState *app_state = get_or_create_app_state(memory);
    AudioState *audio_state = get_or_create_audio_state(app_state);
    AudioRecord *record = audio_state->local_record;

    u32 write_sample_cursor = record->total_samples_written % record->num_buffer_samples;
    u32 num_samples_until_buffer_end = record->num_buffer_samples - write_sample_cursor;

    if (num_samples_to_capture > num_samples_until_buffer_end)
    {
        f32 *src1 = buffer;
        f32 *dest1 = record->samples + write_sample_cursor;
        u32 num_src1_samples = num_samples_until_buffer_end;
        copy_audio_samples(src1, dest1, num_src1_samples);

        f32 *src2 = buffer + num_src1_samples;
        f32 *dest2 = record->samples;
        u32 num_src2_samples = num_samples_to_capture - num_src1_samples;
        copy_audio_samples(src2, dest2, num_src2_samples);
    }
    else
    {
        f32 *src = buffer;
        f32 *dest = record->samples + write_sample_cursor;
        copy_audio_samples(src, dest, num_samples_to_capture);
    }
    record->total_samples_written += num_samples_to_capture;
}

#if 0
// TODO(dan): the AudioCaptureClient does not give us aligned buffer, also the num_samples_to_capture % 4 != 0
// TODO(dan): possible fix: intermediate audio buffer on the platform side, and then we can also capture/mix on the main thread
static void mix_audio_samples_sse2()
{
    // NOTE(dan): clear buffer
    {
        __m128 zero = _mm_setzero_ps();
        __m128 *dest = (__m128 *)mixer_buffer;

        for (u32 sample_index = 0; sample_index < num_samples; sample_index += 4)
        {
            _mm_store_ps((f32 *)dest++, zero);
        }
    }

    // NOTE(dan): mix
    {
        __m128 master_volume_x4 = _mm_set_ps(master_volume[0], master_volume[1], master_volume[0], master_volume[1]);
        __m128 volume_x4 = _mm_set_ps(volume[0], volume[1], volume[0], volume[1]);
        __m128 total_volume_x4 = _mm_mul_ps(master_volume_x4, volume_x4);

        __m128 *src_x4 = (__m128 *)src;
        __m128 *dest_x4 = (__m128 *)dest;

        for (u32 sample_index = 0; sample_index < num_samples; sample_index += 4)
        {
            __m128 sample = _mm_loadu_ps((f32 *)src_x4++);
            sample = _mm_mul_ps(sample, total_volume_x4);
            _mm_storeu_ps((f32 *)dest_x4++, sample);
        }
    }

    // NOTE(dan): fill buffer
    {
        __m128 one = _mm_set_ps1(1.0f);
        __m128 neg_one = _mm_set_ps1(-1.0f);
        __m128 *source = (__m128 *)mixer_buffer;
        __m128 *dest = (__m128 *)buffer;

        for (u32 sample_index = 0; sample_index < num_samples; sample_index += 4)
        {
            __m128 value = _mm_load_ps((f32 *)source++);
            __m128 clamped_value = _mm_min_ps(_mm_max_ps(value, neg_one), one);
            _mm_store_ps((f32 *)dest++, clamped_value);
        }
    }
}
#endif

static void mix_audio_samples(f32 *src_buffer, f32 *dest_buffer, u32 num_samples_to_mix, f32 master_volume[2], f32 volume[2])
{
    assert((num_samples_to_mix % 2) == 0); // NOTE(dan): 2 channels for now

    f32 volume_0 = clamp(master_volume[0] * volume[0], 0.0f, 1.0f);
    f32 volume_1 = clamp(master_volume[1] * volume[1], 0.0f, 1.0f);

    f32 *src = src_buffer;
    f32 *dest = dest_buffer;
    for (u32 sample_index = 0; sample_index < num_samples_to_mix; sample_index += 2)
    {
        f32 value_0 = *src++;
        f32 value_1 = *src++;

        value_0 *= volume_0;
        value_1 *= volume_1;

        *dest++ += value_0;
        *dest++ += value_1;
    }
}

extern "C" __declspec(dllexport) FILL_SOUND_BUFFER(fill_sound_buffer)
{
    PROFILER_FUNCTION();

    AppState *app_state = get_or_create_app_state(memory);
    AudioState *audio_state = get_or_create_audio_state(app_state);

    TempMemoryStack mixer_memory = begin_temp_memory(&audio_state->mixer_memory);
    {
        f32 *mixer_buffer = push_array(&audio_state->mixer_memory, num_samples_to_mix, f32, align_no_clear(16));

        // NOTE(dan): clear buffer
        {
            f32 *dest = mixer_buffer;
            for (u32 sample_index = 0; sample_index < num_samples_to_mix; ++sample_index)
            {
                *dest++ = 0;
            }
        }

        // NOTE(dan): mix
        for (AudioRecord *record = audio_state->first_record; record; record = record->next)
        {
            assert(record->total_samples_read <= record->total_samples_written);

            u32 num_record_samples_to_mix = (u32)(record->total_samples_written - record->total_samples_read);
            if (num_record_samples_to_mix > num_samples_to_mix)
            {
                num_record_samples_to_mix = num_samples_to_mix;
            }

            if (!record->muted)
            {
                u32 read_sample_cursor = record->total_samples_read % record->num_buffer_samples;
                u32 num_samples_until_buffer_end = record->num_buffer_samples - read_sample_cursor;

                if (num_record_samples_to_mix > num_samples_until_buffer_end)
                {
                    f32 *src1 = record->samples + read_sample_cursor;
                    f32 *dest1 = mixer_buffer;
                    u32 num_src1_samples = num_samples_until_buffer_end;
                    mix_audio_samples(src1, dest1, num_src1_samples, audio_state->master_volume, record->volume);

                    f32 *src2 = record->samples;
                    f32 *dest2 = mixer_buffer + num_src1_samples;
                    u32 num_src2_samples = num_record_samples_to_mix - num_src1_samples;
                    mix_audio_samples(src2, dest2, num_src2_samples, audio_state->master_volume, record->volume);
                }
                else
                {
                    f32 *src = record->samples + read_sample_cursor;
                    f32 *dest = mixer_buffer;
                    mix_audio_samples(src, dest, num_record_samples_to_mix, audio_state->master_volume, record->volume);
                }
            }
            record->total_samples_read += num_record_samples_to_mix;
        }

        // NOTE(dan): fill the buffer
        {
            f32 *source = mixer_buffer;
            f32 *dest = buffer;
            for (u32 sample_index = 0; sample_index < num_samples_to_mix; ++sample_index)
            {
                f32 clamped_value = clamp(*source++, -1.0f, 1.0f);
                *dest++ = clamped_value;
            }
        }
    }
    end_temp_memory(mixer_memory);
}

static void draw_vertical_slider(ImGuiID id, ImRect bb, f32 *value)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    ImGuiContext *imgui = GImGui;

    if (ImGui::ItemAdd(bb, &id))
    {
        int hovered = ImGui::IsHovered(bb, id);
        if (hovered)
        {
            ImGui::SetHoveredID(id);
        }
        if (hovered && imgui->IO.MouseClicked[0])
        {
            ImGui::SetActiveID(id, window);
            ImGui::FocusWindow(window);
        }
        ImGui::SliderBehavior(bb, id, value, 0.0f, 1.0f, 1.0f, 3, ImGuiSliderFlags_Vertical);
    }
}

static void draw_volume_mixer(char *name, f32 *left, f32 *right)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    ImGuiContext *imgui = GImGui;
    ImGuiStyle *style = &imgui->Style;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::BeginGroup();
    {
        ImVec2 name_size = ImGui::CalcTextSize(name, 0, true);
        ImGui::Text(name);

        ImVec2 size = ImVec2(15, 70);
        ImGuiID left_id = window->GetID(left);
        ImGuiID right_id = window->GetID(right);
        ImVec2 min_left_corner = window->DC.CursorPos;
        ImVec2 bottom_left = min_left_corner + ImVec2(size.x + style->FramePadding.x, 0.0f);
        ImVec2 top_right = bottom_left + size;
        ImRect left_bb = ImRect(min_left_corner, min_left_corner + size);
        ImRect right_bb = ImRect(bottom_left, top_right);

        draw_vertical_slider(left_id, left_bb, left);
        ImGui::SameLine();
        draw_vertical_slider(right_id, right_bb, right);
       
        ImVec2 min_p = left_bb.Min - style->FramePadding - ImVec2(0.0f, name_size.y + style->FramePadding.y);
        ImVec2 max_p = right_bb.Max + style->FramePadding;

        f32 total_width = right_bb.Max.x - left_bb.Min.x;
        if (total_width < name_size.x)
        {
            max_p.x += (name_size.x - total_width);
        }
        
        window->DrawList->AddRect(min_p, max_p, ImGui::GetColorU32(ImGuiCol_Border), style->FrameRounding);
    }
    ImGui::EndGroup();
    ImGui::PopStyleVar(); 
}

inline void change_unit_and_size(char **unit, usize *size)
{
    *unit = "B";
    if (*size > 1024*MB)
    {
        *unit = "GB";
        *size /= GB;
    }
    else if (*size > 1024*KB)
    {
        *unit = "MB";
        *size /= MB;
    }
    else if (*size > 1024)
    {
        *unit = "KB";
        *size /= KB;
    }
}

extern "C" __declspec(dllexport) UPDATE_AND_RENDER(update_and_render)
{
    PROFILER_FUNCTION();

    AppState *app_state = get_or_create_app_state(memory);
    UIState *ui_state = &app_state->ui_state;
    if (!app_state->initialized || memory->app_dll_reloaded)
    {
        platform.init_opengl(&gl);

        init_ui(ui_state);

        if (gl.DebugMessageCallback)
        {
            gl.Enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            gl.DebugMessageCallback(opengl_debug_callback, 0);
        }

        app_state->initialized = true;
    }

    AudioState *audio_state = get_or_create_audio_state(app_state);

    ImVec4 clear_color = ImColor(85, 118, 152);
    gl.Enable(GL_SCISSOR_TEST);
    gl.Enable(GL_BLEND);

    gl.Scissor(0, 0, window_width, window_height);
    
    gl.ClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    gl.Clear(GL_COLOR_BUFFER_BIT);

    gl.Viewport(0, 0, window_width, window_height);

    PROFILER_BEGIN("UI");
    begin_ui(ui_state, input, window_width, window_height);
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

            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem(ui_state->show_info_dock ? "Hide Info" : "Show Info"))
                {
                    ui_state->show_info_dock = !ui_state->show_info_dock;
                }
                if (ImGui::MenuItem(ui_state->show_volume_mixer ? "Hide Volume Mixer" : "Show Volume Mixer"))
                {
                    ui_state->show_volume_mixer = !ui_state->show_volume_mixer;
                }
                if (ImGui::MenuItem(ui_state->show_frame_history ? "Hide Frame History" : "Show Frame History"))
                {
                    ui_state->show_frame_history = !ui_state->show_frame_history;
                }
                if (ImGui::MenuItem(ui_state->show_profiler ? "Hide Profiler" : "Show Profiler"))
                {
                    ui_state->show_profiler = !ui_state->show_profiler;
                }
                if (ImGui::MenuItem(ui_state->show_clocks ? "Hide Clocks" : "Show Clocks"))
                {
                    ui_state->show_clocks = !ui_state->show_clocks;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        f32 menubar_height = 20.0f;
        root_dock(ui_state, ImVec2(0, menubar_height), ImVec2((f32)window_width, (f32)window_height - menubar_height));

        if (begin_dock(ui_state, "Info", &ui_state->show_info_dock, 0))
        {
            PlatformMemoryStats memory_stats = platform.get_memory_stats();

            char *total_size_unit = "B";
            usize total_size = memory_stats.total_size;
            change_unit_and_size(&total_size_unit, &total_size);

            char *total_used_unit = "B";
            usize total_used = memory_stats.total_used;
            change_unit_and_size(&total_used_unit, &total_used);

            ImGui::Text("Mouse: (%.1f,%.1f) ", ui_state->imgui_io->MousePos.x, ui_state->imgui_io->MousePos.y);
            ImGui::Text("Allocs: %d ", ui_state->imgui_io->MetricsAllocs);
            ImGui::Text("Vertices: %d Indices: %3d  ", ui_state->imgui_io->MetricsRenderVertices, ui_state->imgui_io->MetricsRenderIndices);
            ImGui::Text("Windows: %d ", ui_state->imgui_io->MetricsActiveWindows);
            ImGui::Text("Memory blocks: %d Total: %d %s Used: %d %s ", memory_stats.num_memblocks, total_size, total_size_unit, total_used, total_used_unit);
            ImGui::Text("Frame time: %.2f ms", 1000.0f / ui_state->imgui_io->Framerate);

        }
        end_dock(ui_state);

        if (begin_dock(ui_state, "Volume Mixer", &ui_state->show_volume_mixer, 0))
        {
            draw_volume_mixer("master", &audio_state->master_volume[0], &audio_state->master_volume[1]);

            ImGui::SameLine();

            for (AudioRecord *record = audio_state->first_record; record; record = record->next)
            {
                draw_volume_mixer("channel", &record->volume[0], &record->volume[1]);
                ImGui::SameLine();
            }
        }
        end_dock(ui_state);

        if (begin_dock(ui_state, "Records", 0, 0))
        {
            u32 record_index = 0;
            for (AudioRecord *record = audio_state->first_record; record; record = record->next, ++record_index)
            {
                ImVec2 graph_size = ImVec2(0, 100);
                char label[40];
                ImFormatString(label, array_count(label), "Record #%u", record_index);

                ImGui::PlotLines(label, record->samples, record->num_buffer_samples, sizeof(f32), "L", -1.0f, 1.0f, graph_size, 4);

                ImFormatString(label, array_count(label), "Record #%u", record_index);

                ImGui::PlotLines(label, record->samples, record->num_buffer_samples, sizeof(f32)*2, "R", -1.0f, 1.0f, graph_size, 4);
            }
        }
        end_dock(ui_state);

        // ShowExampleAppConsole(0);
        // ImGui::ShowTestWindow(0);

        profiler_report(ui_state, memory);
    }
    end_ui(ui_state);
    PROFILER_END();

    save_ui_state(ui_state, &app_state->app_memory);
}
