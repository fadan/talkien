struct ProfilerThread
{
    u32 id;

    u32 num_entries;
    ProfilerEntry entries[1024];
};

struct ProfilerFrame
{
    u64 begin_timestamp;
    u64 end_timestamp;

    f32 frame_time;

    u32 num_threads;
    ProfilerThread threads[32];
};

struct ProfilerState
{
    MemoryStack profiler_memory;

    u32 display_frame_index;

    i32 pause_condition_ms;
    b32 gather_paused;

    u32 gather_frame_index;
    ProfilerFrame frames[64];
};

static void profiler_init(ProfilerState *state)
{
    state->gather_frame_index = 0;
    state->gather_paused = false;
    state->pause_condition_ms = 500;
}

static void profiler_init_frame(ProfilerState *state, u32 frame_index, u64 begin_timestamp)
{
    ProfilerFrame *frame = state->frames + frame_index;
    frame->begin_timestamp = begin_timestamp;
    frame->end_timestamp = 0;
    frame->frame_time = 0.0f;
    frame->num_threads = 0;
}

static ProfilerThread *profiler_get_thread_for_frame(ProfilerFrame *frame, u32 thread_id)
{
    ProfilerThread *thread = 0;
    for (u32 thread_index = 0; thread_index < frame->num_threads; ++thread_index)
    {
        ProfilerThread *test_thread = &frame->threads[thread_index];
        if (test_thread->id == thread_id)
        {
            thread = test_thread;
            break;
        }
    }
    
    if (!thread)
    {
        assert(frame->num_threads < array_count(frame->threads));
        thread = &frame->threads[frame->num_threads++];
        thread->id = thread_id;
        thread->num_entries = 0;
    }

    return thread;
}

inline void profiler_store_thread_entry(ProfilerThread *thread, ProfilerEntry *entry)
{
    assert(thread->num_entries < array_count(thread->entries));
    thread->entries[thread->num_entries++] = *entry;
}

static void profiler_gather(ProfilerState *state, ProfilerEntry *entries, u32 num_entries)
{
    ProfilerFrame *gather_frame = state->frames + state->gather_frame_index;

    for (u32 entry_index = 0; entry_index < num_entries; ++entry_index)
    {
        ProfilerEntry *entry = entries + entry_index;
        ProfilerThread *thread = profiler_get_thread_for_frame(gather_frame, entry->thread_id);

        switch (entry->type)
        {
            case ProfilerEntryType_Begin:
            case ProfilerEntryType_End:
            {
                profiler_store_thread_entry(thread, entry);
            } break;

            case ProfilerEntryType_FrameTime:
            {
                gather_frame->end_timestamp = entry->timestamp;
                gather_frame->frame_time = entry->frame_time;

                if (!state->gather_paused)
                {
                    ++state->gather_frame_index;
                    if (state->gather_frame_index >= array_count(state->frames))
                    {
                        state->gather_frame_index = 0;
                    }
                }

                profiler_init_frame(state, state->gather_frame_index, entry->timestamp);
            } break;
        }
    }
}

static void profiler_draw_frame_history(ProfilerState *state)
{
}

static void profiler_draw(ProfilerState *state)
{
    f32 graph_size_x = 600;
    f32 graph_size_y = 100;

    f32 scale_min = 0.01f;
    f32 scale_max = 0.02f;

    ImGui::SetNextWindowSize(ImVec2(graph_size_x + 30, graph_size_y + 80));
    ImGui::Begin("Frame History");
    {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        ImGuiContext *imgui = GImGui;
        ImGuiStyle *style = &imgui->Style;
        ImRect graph_bb = { window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size_x, graph_size_y) };
        ImRect inner_bb = { graph_bb.Min + style->FramePadding, graph_bb.Max - style->FramePadding };

        ImU32 color_base = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
        ImU32 color_hovered = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered);
        ImU32 color_frame = ImGui::GetColorU32(ImGuiCol_FrameBg);
        ImU32 color_current = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
        ImU32 color_line = ImGui::GetColorU32(ImGuiCol_PlotLines);

        u32 frame_count = array_count(state->frames);
        f32 t_0 = 0.0f;
        f32 t_step = 1.0f / frame_count;
        f32 value_0 = state->frames[0].frame_time;

        ImVec2 tp_0 = { t_0, 1.0f - ImSaturate((value_0 - scale_min) / (scale_max - scale_min)) };
        
        ImGui::ItemSize(graph_bb, style->FramePadding.y);
        ImGui::ItemAdd(graph_bb, 0);
        ImGui::RenderFrame(graph_bb.Min, graph_bb.Max, color_frame, true, style->FrameRounding);

        i32 hovered_frame_index = -1;
        if (ImGui::IsHovered(inner_bb, 0))
        {
            f32 t = ImClamp((imgui->IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
            u32 value_index = (u32)(t * frame_count);
            assert(value_index < frame_count);

            f32 value_0 = state->frames[value_index % frame_count].frame_time;
            f32 value_1 = state->frames[(value_index + 1) % frame_count].frame_time;

            ImGui::SetTooltip("%d: %8.4g ms", value_index, value_0);
            hovered_frame_index = value_index;
        }

        for (u32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            f32 frame_time = state->frames[frame_index].frame_time;
            f32 t_1 = t_0 + t_step;
            u32 value_index = (u32)(t_0 * frame_count + 0.5f);
            assert(value_index < frame_count);

            f32 value = state->frames[value_index % frame_count].frame_time;
            if ((state->pause_condition_ms > 0.0f) && (value > (state->pause_condition_ms / 1000.0f)))
            {
                state->gather_paused = true;
            }

            ImVec2 tp_1 = { t_1, 1.0f - ImSaturate((value - scale_min) / (scale_max - scale_min)) };

            ImVec2 pos_0 = ImLerp(inner_bb.Min, inner_bb.Max, tp_0);
            ImVec2 pos_1 = ImLerp(inner_bb.Min, inner_bb.Max, { tp_1.x, 1.0f });

            if (pos_1.x >= (pos_0.x + 2.0f))
            {
                pos_1.x -= 1.0f;
            }

            ImU32 color = color_base;
            if (state->gather_frame_index == frame_index)
            {
                color = color_current;
            }
            if (hovered_frame_index == (i32)value_index)
            {
                color = color_hovered;
            }

            if (ImGui::IsHovered({pos_0, pos_1}, 0) && ImGui::IsMouseClicked(0, false))
            {
                if (state->gather_frame_index != frame_index)
                {
                    state->gather_paused = true;
                    state->display_frame_index = frame_index;
                }
            }

            window->DrawList->AddRectFilled(pos_0, pos_1, color);

            t_0 = t_1;
            tp_0 = tp_1;
        }

        if (ImGui::Button(state->gather_paused ? "Continue" : "Pause"))
        {
            state->gather_paused = !state->gather_paused;
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(-320);
        ImGui::LabelText("Pause when frame time exceeds", "");
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 115);
        ImGui::PushItemWidth(100.0f);
        ImGui::DragInt("", &state->pause_condition_ms, 1, 1, 1000, "%.0f ms");
    }
    ImGui::End();

    graph_size_x = 400;
    graph_size_y = 200;

    // ImGui::SetNextWindowSize(ImVec2(graph_size_x + 30, graph_size_y + 80));
    ImGui::Begin("Profiler");
    {
        graph_size_x = ImGui::GetWindowWidth() - 30;
        graph_size_y = ImGui::GetWindowHeight() - 80;

        if (!state->gather_paused)
        {
            state->display_frame_index = (array_count(state->frames) + state->gather_frame_index - 1) % array_count(state->frames);
        }

        u32 display_frame_index = state->display_frame_index;
        if (display_frame_index != state->gather_frame_index)
        {
            ImGuiWindow *window = ImGui::GetCurrentWindow();
            ImGuiContext *imgui = GImGui;
            ImGuiStyle *style = &imgui->Style;

            ImGui::Text("Threads on frame #%d", display_frame_index);

            ImRect graph_bb = { window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size_x, graph_size_y) };
            ImRect inner_bb = { graph_bb.Min + style->FramePadding, graph_bb.Max - style->FramePadding };

            ImU32 color_frame = ImGui::GetColorU32(ImGuiCol_FrameBg);
            ImU32 color_line = ImGui::GetColorU32(ImGuiCol_PlotLines);

            ImGui::ItemSize(graph_bb, style->FramePadding.y);
            ImGui::ItemAdd(graph_bb, 0);
            ImGui::RenderFrame(graph_bb.Min, graph_bb.Max, color_frame, true, style->FrameRounding);

            ProfilerFrame *frame = state->frames + display_frame_index;

            f32 graph_height = inner_bb.Max.y - inner_bb.Min.y;
            f32 lane_height = graph_height / frame->num_threads;

            ImVec2 min = inner_bb.Min;
            ImVec2 max = inner_bb.Max;
            
            u64 clocks_elapsed = frame->end_timestamp - frame->begin_timestamp;
            f64 lane_width = max.x - min.x;
            
            f64 scale = 0.0f;
            if(clocks_elapsed > 0)
            {
                scale = lane_width / (f64)clocks_elapsed;
            }

            for (u32 thread_index = 0; thread_index < frame->num_threads; ++thread_index)
            {
                ProfilerThread *thread = frame->threads + thread_index;
                ImVec2 bottom_left = {min.x, min.y + thread_index * lane_height};
                ImVec2 top_right = {max.x, min.y + (thread_index + 1) * lane_height};

                window->DrawList->AddRect(bottom_left, top_right, color_line);

                ProfilerEntry *open_entry = 0;

                for (u32 entry_index = 0; entry_index < thread->num_entries; ++entry_index)
                {
                    ProfilerEntry *entry = thread->entries + entry_index;

                    if (entry->type == ProfilerEntryType_Begin)
                    {
                        open_entry = entry;
                    }

                    if (open_entry && (entry->type == ProfilerEntryType_End))
                    {
                        ProfilerEntry *close_entry = entry;

                        f32 this_min_x = bottom_left.x + (f32)(scale * (open_entry->timestamp - frame->begin_timestamp));
                        f32 this_max_x = bottom_left.x + (f32)(scale * (close_entry->timestamp - frame->begin_timestamp));

                        ImVec2 this_bottom_left = {this_min_x, min.y + thread_index * lane_height};
                        ImVec2 this_top_right = {this_max_x, min.y + (thread_index + 1) * lane_height};

                        if (ImGui::IsHovered({this_bottom_left, this_top_right}, 0))
                        {
                            ImGui::SetTooltip(open_entry->guid);
                        }

                        window->DrawList->AddRect(this_bottom_left, this_top_right, color_line);

                        // v3 Color = Colors[PointerToU32(CloseEvent->GUID)%ArrayCount(Colors)];
                        // r32 ThisMinX = ProfileRect.Min.x + Scale*(r32)(OpenEvent->Clock - Frame->BeginClock);
                        // r32 ThisMaxX = ProfileRect.Min.x + Scale*(r32)(CloseEvent->Clock - Frame->BeginClock);

                        // u32 LaneIndex = 0;
                        // rectangle2 RegionRect = RectMinMax(
                        //     V2(ThisMinX, ProfileRect.Max.y - LaneHeight*(LaneIndex + 1)),
                        //     V2(ThisMaxX, ProfileRect.Max.y - LaneHeight*(LaneIndex + 0)));

                        // PushRect(&DebugState->RenderGroup, NoTransform, RegionRect, 0.0f, V4(Color, 1));
                    }
                }
            }
        }
    }
    ImGui::End();
}

static void profiler_report(AppMemory *memory)
{
    ProfilerState *state = memory->profiler_state;
    if (!state)
    {
        state = memory->profiler_state = bootstrap_push_struct(ProfilerState, profiler_memory);
        profiler_init(state);
    }
    
    profiler->current_entry_array_index = !profiler->current_entry_array_index;
    u32 array_index__entry_index = atomic_exchange_u32(&profiler->entry_array_index__entry_index, profiler->current_entry_array_index << 31);

    u32 entry_array_index = array_index__entry_index >> 31;
    u32 num_entries = array_index__entry_index & 0xFFFFFFF;

    profiler_gather(state, profiler->entries[entry_array_index], num_entries);
    profiler_draw(state);
}
