
static void profiler_draw_frame_history(ProfilerState *state, f32 graph_size_x, f32 graph_size_y, f32 scale_min, f32 scale_max)
{
    // ImGui::SetNextWindowSize(ImVec2(graph_size_x + 30, graph_size_y + 80));
    // ImGui::Begin("Frame History");
    // ImGui::BeginDock("Frame History", 0, 0);
    begin_dock("Frame History", 0, 0);
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
                state->paused = true;
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
                    state->paused = true;
                    state->display_frame_index = frame_index;
                }
            }

            window->DrawList->AddRectFilled(pos_0, pos_1, color);

            t_0 = t_1;
            tp_0 = tp_1;
        }

        if (ImGui::Button(state->paused ? "Continue" : "Pause"))
        {
            state->paused = !state->paused;
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(-320);
        ImGui::LabelText("Pause when frame time exceeds", "");
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 115);
        ImGui::PushItemWidth(100.0f);
        ImGui::DragInt("", &state->pause_condition_ms, 1, 0, 1000, "%.0f ms");
    }
    end_dock();
    // ImGui::EndDock();
    // ImGui::End();
}

static f32 profiler_color_table[][3] =
{
    {0, 1, 0},
    {0, 0, 1},
    {1, 0, 0},
    {1, 1, 0},
    {0, 1, 1},
    {1, 0, 1},
    {1, 0.5f, 0},
    {1, 0, 0.5f},
    {0.5f, 1, 0},
    {0, 1, 0.5f},
    {0.5f, 0, 1},
};

inline void profiler_random_color(f32 *color)
{
    static u32 index = 0;
    color[0] = profiler_color_table[index][0];
    color[1] = profiler_color_table[index][1];
    color[2] = profiler_color_table[index][2];
    ++index;
    if (index == array_count(profiler_color_table))
    {
        index = 0;
    }
}

static void profiler_draw_bars(ProfilerState *state, ImGuiWindow *window, ImRect graph_rect, ProfilerNode *root_node, f32 lane_stride, f32 lane_height, u32 depth_remaining)
{
    f32 frame_span = (f32)(root_node->duration);
    f32 pixel_span = graph_rect.Max.x - graph_rect.Min.x;

    f32 scale = 0.0f;
    if (frame_span > 0)
    {
        scale = pixel_span / frame_span;
    }

    ImU32 color_line = ImGui::GetColorU32(ImGuiCol_PlotLines);

    for (ProfilerStoredEntry *stored_entry = root_node->first_child; stored_entry; stored_entry = stored_entry->profiler_node.next_same_parent)
    {
        ProfilerNode *node = &stored_entry->profiler_node;
        ProfilerElement *element = node->element;

        assert(element);

        f32 this_min_x = graph_rect.Min.x + scale * (f32)(node->parent_relative_clock);
        f32 this_max_x = this_min_x + scale * (f32)(node->duration);

        u32 lane_index = node->thread_index;
        f32 lane_y = graph_rect.Max.y - lane_stride * lane_index;
        ImRect region_rect = ImRect({this_min_x, lane_y - lane_height}, {this_max_x, lane_y});

        window->DrawList->AddRect(region_rect.Min, region_rect.Max, color_line);

        f32 color_f32x4[4];
        profiler_random_color(color_f32x4);

        ImU32 color = (ImU32)ImColor(color_f32x4[0], color_f32x4[1], color_f32x4[2], 1.0f);

        window->DrawList->AddRectFilled(region_rect.Min, region_rect.Max, color_line);

        if (ImGui::IsHovered(region_rect, 0))
        {
            ImGui::SetTooltip("%s: %10ucy", element->guid, node->duration);
        }

        if (depth_remaining > 0)
        {
            profiler_draw_bars(state, window, region_rect, node, 0, lane_height / 2, depth_remaining - 1);
        }
    }
}

static void profiler_draw_timelines(ProfilerState *state, f32 graph_size_x, f32 graph_size_y)
{
    // ImGui::Begin("Profiler");
    // ImGui::BeginDock("Profiler", 0, 0);
    begin_dock("Profiler", 0, 0);
    {
        graph_size_x = ImGui::GetWindowWidth() - 30;
        graph_size_y = ImGui::GetWindowHeight() - 80;

        ImGuiWindow *window = ImGui::GetCurrentWindow();
        ImGuiContext *imgui = GImGui;
        ImGuiStyle *style = &imgui->Style;

        ImGui::Text("Threads on frame #%d", state->display_frame_index);

        ImRect graph_bb = { window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size_x, graph_size_y) };
        ImRect inner_bb = { graph_bb.Min + style->FramePadding, graph_bb.Max - style->FramePadding };

        ImU32 color_frame = ImGui::GetColorU32(ImGuiCol_FrameBg);
        ImU32 color_line = ImGui::GetColorU32(ImGuiCol_PlotLines);

        ImGui::ItemSize(graph_bb, style->FramePadding.y);
        ImGui::ItemAdd(graph_bb, 0);
        ImGui::RenderFrame(graph_bb.Min, graph_bb.Max, color_frame, true, style->FrameRounding);

        u32 num_lanes = state->num_threads;
        f32 lane_height = 0.0f;
        if (num_lanes > 0)
        {
            lane_height = (inner_bb.Max.y - inner_bb.Min.y) / (f32)num_lanes;
        }

        ProfilerElement *root_element = state->root_element;
        ProfilerElementFrame *root_frame = root_element->frames + state->display_frame_index;
        f32 next_x = inner_bb.Min.x;
        u64 relative_clock = 0;

        u64 total_clocks = 0;
        for (ProfilerStoredEntry *entry = root_frame->oldest_entry; entry; entry = entry->next)
        {
            total_clocks += entry->profiler_node.duration;
        }

        for (ProfilerStoredEntry *entry = root_frame->oldest_entry; entry; entry = entry->next)
        {
            ProfilerNode *node = &entry->profiler_node;
            ImRect entry_rect = inner_bb;

            relative_clock += node->duration;
            f32 t = (f32)((f64)relative_clock / (f64)total_clocks);
            entry_rect.Min.x = next_x;
            entry_rect.Max.x = (1.0f - t) * inner_bb.Min.x + t * inner_bb.Max.x;
            next_x = entry_rect.Max.x;

            profiler_draw_bars(state, window, entry_rect, node, lane_height, lane_height, 2);
        }
    }
    end_dock();
    // ImGui::EndDock();
    // ImGui::End();
}

struct ProfilerStats
{
    f64 min;
    f64 max;
    f64 sum;
    f64 avg;
    u32 count;
};

struct ProfilerClockEntry
{
    ProfilerElement *element;
    ProfilerStats stats;
};

inline void profiler_begin_stats(ProfilerStats *stats)
{
    stats->min = F32_MAX;
    stats->max = -F32_MAX;
    stats->sum = 0.0f;
    stats->count = 0;
}

inline void profiler_accum_stats(ProfilerStats *stats, f64 value)
{
    ++stats->count;
    if (stats->min > value)
    {
        stats->min = value;
    }
    if (stats->max < value)
    {
        stats->max = value;
    }
    stats->sum += value;
}

inline void profiler_end_stats(ProfilerStats *stats)
{
    if (stats->count)
    {
        stats->avg = stats->sum / (f64)stats->count;
    }
    else
    {
        stats->min = 0.0f;
        stats->max = 0.0f;
        stats->avg = 0.0f;
    }
}

static void profiler_draw_clocks(ProfilerState *state, f32 graph_size_x, f32 graph_size_y)
{
    // ImGui::Begin("Clocks");
    // ImGui::BeginDock("Clocks", 0, 0);
    begin_dock("Clocks", 0, 0);
    {
        graph_size_x = ImGui::GetWindowWidth() - 30;
        graph_size_y = ImGui::GetWindowHeight() - 80;

        ImGuiWindow *window = ImGui::GetCurrentWindow();
        ImGuiContext *imgui = GImGui;
        ImGuiStyle *style = &imgui->Style;

        ImRect graph_bb = { window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size_x, graph_size_y) };
        ImRect inner_bb = { graph_bb.Min + style->FramePadding, graph_bb.Max - style->FramePadding };

        ImU32 color_frame = ImGui::GetColorU32(ImGuiCol_FrameBg);
        ImU32 color_line = ImGui::GetColorU32(ImGuiCol_PlotLines);

        TempMemoryStack temp_memory = begin_temp_memory(&state->profiler_memory);
        {
            ProfilerElement *root_element = state->root_element;

            u32 num_links = 0;
            for (ProfilerLink *link = profiler_get_sentinel(state->profiler_group)->next; link != profiler_get_sentinel(state->profiler_group); link = link->next)
            {
                ++num_links;
            }

            ProfilerClockEntry *clock_entries = push_array(temp_memory.memstack, num_links, ProfilerClockEntry, no_clear());
            SortItem *sort_a = push_array(temp_memory.memstack, num_links, SortItem, no_clear());
            SortItem *sort_b = push_array(temp_memory.memstack, num_links, SortItem, no_clear());

            f64 total_time = 0.0f;
            u32 index = 0;
            for (ProfilerLink *link = profiler_get_sentinel(state->profiler_group)->next; link != profiler_get_sentinel(state->profiler_group); link = link->next, ++index)
            {
                assert(link->first_child == profiler_get_sentinel(link));

                ProfilerClockEntry *clock_entry = clock_entries + index;
                SortItem *sort_item = sort_a + index;

                clock_entry->element = link->element;
                ProfilerElement *element = clock_entry->element;

                profiler_begin_stats(&clock_entry->stats);
                for (ProfilerStoredEntry *stored_entry = element->frames[state->display_frame_index].oldest_entry; stored_entry; stored_entry = stored_entry->next)
                {
                    u64 clocks_with_children = stored_entry->profiler_node.duration;
                    u64 clocks_without_children = clocks_with_children - stored_entry->profiler_node.duration_of_children;
                    profiler_accum_stats(&clock_entry->stats, (f64)clocks_without_children);
                }
                profiler_end_stats(&clock_entry->stats);

                total_time += clock_entry->stats.sum;

                sort_item->key = -(f32)clock_entry->stats.sum;
                sort_item->index = index;
            }

            radix_sort(num_links, sort_a, sort_b);

            f64 running_sum = 0.0f;
            f64 pc = 0.0f;
            if (total_time > 0.0f)
            {
                pc = 100.0f / total_time;
            }

            for (index = 0; index < num_links; ++index)
            {
                ProfilerClockEntry *clock_entry = clock_entries + sort_a[index].index;
                ProfilerStats *stats = &clock_entry->stats;
                ProfilerElement *element = clock_entry->element;

                running_sum += stats->sum;

                ImGui::Text("%10ucy %05.02f%% %4d %s", (u32)stats->sum, (pc * stats->sum), stats->count, element->name);
            }
        }
        end_temp_memory(temp_memory);
    }
    end_dock();
    // ImGui::EndDock();
    // ImGui::End();
}
