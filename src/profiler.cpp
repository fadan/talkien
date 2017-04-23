
struct ProfilerFrame
{
    u64 begin_clock;
    u64 end_clock;
    f32 dt;
};

struct ProfilerState
{
    MemoryStack profiler_memory;

    u32 gather_frame_index;
    u32 frame_count;
    ProfilerFrame frames[128];
};

static void profiler_histogram(float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window->SkipItems)
    {
        ImGuiContext& g = *GImGui;
        ImGuiStyle& style = g.Style;

        ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size.x, graph_size.y));
        ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
        
        ImGui::ItemSize(frame_bb, style.FramePadding.y);
        
        if (ImGui::ItemAdd(frame_bb, NULL))
        {

            ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

            int res_w = ImMin((int)graph_size.x, values_count);
            int item_count = values_count;

            // Tooltip on hover
            int v_hovered = -1;
            if (ImGui::IsHovered(inner_bb, 0))
            {
                float t = ImClamp((g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
                int v_idx = (int)(t * item_count);
                assert(v_idx >= 0 && v_idx < values_count);

                float v0 = values_getter(data, (v_idx + values_offset) % values_count);
                float v1 = values_getter(data, (v_idx + 1 + values_offset) % values_count);
                ImGui::SetTooltip("%d: %8.4g", v_idx, v0);
                v_hovered = v_idx;
            }

            float t_step = 1.0f / (float)res_w;

            float v0 = values_getter(data, (0 + values_offset) % values_count);
            float t0 = 0.0f;
            ImVec2 tp0 = ImVec2( t0, 1.0f - ImSaturate((v0 - scale_min) / (scale_max - scale_min)) );    // Point in the normalized space of our target rectangle

            ImU32 col_base = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
            ImU32 col_hovered = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered);

            for (int n = 0; n < res_w; n++)
            {
                float t1 = t0 + t_step;
                int v1_idx = (int)(t0 * item_count + 0.5f);
                assert(v1_idx >= 0 && v1_idx < values_count);
                float v1 = values_getter(data, (v1_idx + values_offset + 1) % values_count);
                ImVec2 tp1 = ImVec2( t1, 1.0f - ImSaturate((v1 - scale_min) / (scale_max - scale_min)) );

                // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
                ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
                ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(tp1.x, 1.0f));
                if (pos1.x >= pos0.x + 2.0f)
                {
                    pos1.x -= 1.0f;
                }

                window->DrawList->AddRectFilled(pos0, pos1, v_hovered == v1_idx ? col_hovered : col_base);
                t0 = t1;
                tp0 = tp1;
            }

            // Text overlay
            if (overlay_text)
            {
                ImGui::RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, overlay_text, NULL, NULL, ImVec2(0.5f,0.0f));
            }
        }
    }
}

static void profiler_draw(ProfilerState *state)
{
    f32 graph_size_x = 600;
    f32 graph_size_y = 100;

    f32 scale_min = 0.01f;
    f32 scale_max = 0.02f;

    ImGui::SetNextWindowSize(ImVec2(graph_size_x + 30, graph_size_y + 50));
    ImGui::Begin("Frame Times");
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

        u32 frame_count = array_count(state->frames);
        f32 t_0 = 0.0f;
        f32 t_step = 1.0f / frame_count;
        f32 value_0 = state->frames[0].dt;

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

            f32 value_0 = state->frames[value_index % frame_count].dt;
            f32 value_1 = state->frames[(value_index + 1) % frame_count].dt;

            ImGui::SetTooltip("%d: %8.4g", value_index, value_0);
            hovered_frame_index = value_index;
        }

        for (u32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            f32 frame_time = state->frames[frame_index].dt;
            f32 t_1 = t_0 + t_step;
            u32 value_index = (u32)(t_0 * frame_count + 0.5f);
            assert(value_index < frame_count);

            f32 value = state->frames[value_index % frame_count].dt;
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

            window->DrawList->AddRectFilled(pos_0, pos_1, color);

            t_0 = t_1;
            tp_0 = tp_1;
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
    }

    ProfilerFrame *gather_frame = state->frames + state->gather_frame_index;
    gather_frame->begin_clock = profiler->events[0].clock;

    u32 event_count = profiler->event_count;
    for (u32 event_index = 0; event_index < event_count; ++event_index)
    {
        ProfilerEvent *event = profiler->events + event_index;

        switch (event->type)
        {
            case ProfilerEventType_BeginBlock:
            {
            } break;
            
            case ProfilerEventType_EndBlock:
            {
            } break;

            case ProfilerEventType_FrameEnd:
            {
                atomic_exchange_u32(&profiler->event_count, 0);

                gather_frame->end_clock = event->clock;
                gather_frame->dt = event->value_f32;

                ++state->frame_count;
                state->gather_frame_index = (state->gather_frame_index + 1) % array_count(state->frames);
            } break;
        }
    }

    profiler_draw(state);
}
