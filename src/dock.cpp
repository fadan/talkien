
static void init_dock(Dock *dock)
{
    dock->id = 0;

    dock->parent = 0;
    dock->next_tab = 0;
    dock->prev_tab = 0;
    dock->children[0] = 0;
    dock->children[1] = 0;

    dock->pos = ImVec2(0, 0);
    dock->size = ImVec2(-1, -1);
    dock->status = Status_Float;
    dock->active = true;
    dock->opened = false;
    dock->first = true;
    dock->last_frame = 0;
    dock->invalid_frames = 0;

    dock->label[0] = 0;
    dock->location[0] = 0;
}

inline Dock *allocate_dock(UIState *context)
{
    Dock *dock = 0;
    allocate_freelist(dock, context->first_free_dock, push_struct(&context->ui_memory, Dock));
    dock->next = context->first_dock;
    context->first_dock = dock;
    return dock;
}

inline void deallocate_dock(UIState *context, Dock *dock)
{
    deallocate_freelist(dock, context->first_free_dock);
}

static void remove_dock(UIState *context, Dock *dock)
{
    for (Dock *prev = 0, *current = context->first_dock; current; prev = current, current = current->next)
    {
        if (current == dock)
        {
            if (!prev)
            {
                context->first_dock = current->next;
            }
            else
            {
                prev->next = current->next;
            }

            deallocate_dock(context, dock);
            break;
        }
    }
}

static b32 is_horizontal(Dock *dock)
{
    b32 horizontal = (dock->children[0]->pos.x < dock->children[1]->pos.x);
    return horizontal;
}

static ImVec2 get_min_size(Dock *dock)
{
    ImVec2 min_size = ImVec2(16, 16 + ImGui::GetTextLineHeightWithSpacing());
    if (dock->children[0])
    {
        ImVec2 size_0 = get_min_size(dock->children[0]);
        ImVec2 size_1 = get_min_size(dock->children[1]);
        if (is_horizontal(dock))
        {
            min_size = ImVec2(size_0.x + size_1.x, ImMax(size_0.y, size_1.y));
        }
        else
        {
            min_size = ImVec2(ImMax(size_0.x, size_1.x), size_0.y + size_1.y);
        }
    }
    return min_size;
}

static void set_parent(Dock *dock, Dock *parent)
{
    dock->parent = parent;

    for (Dock *tab = dock->prev_tab; tab; tab = tab->prev_tab) 
    {
        tab->parent = parent;
    }
    for (Dock *tab = dock->next_tab; tab; tab = tab->next_tab)
    {
        tab->parent = parent;
    }
}

static Dock *get_first_tab(Dock *dock)
{
    Dock *first_tab = dock;
    while (first_tab->prev_tab) 
    {
        first_tab = first_tab->prev_tab;
    }
    return first_tab;
}

static Dock *get_sibling(Dock *dock)
{
    assert(dock->parent);

    Dock *sibling = dock->parent->children[0];
    if (dock->parent->children[0] == get_first_tab(dock))
    {
        sibling = dock->parent->children[1];
    }
    return sibling;
}

static void set_active(Dock *dock)
{
    dock->active = true;

    for (Dock* tab = dock->prev_tab; tab; tab = tab->prev_tab) 
    {
        tab->active = false;
    }
    for (Dock* tab = dock->next_tab; tab; tab = tab->next_tab) 
    {
        tab->active = false;
    }
}

static b32 is_container(Dock *dock)
{
    b32 container = (dock->children[0] != 0);
    return container;
}

static void set_children_pos_size(Dock *dock, ImVec2 pos, ImVec2 size);

static void set_pos_size(Dock *dock, ImVec2 pos, ImVec2 size)
{
    dock->size = size;
    dock->pos = pos;

    for (Dock *tab = dock->prev_tab; tab; tab = tab->prev_tab)
    {
        tab->size = size;
        tab->pos = pos;
    }
    for (Dock *tab = dock->next_tab; tab; tab = tab->next_tab)
    {
        tab->size = size;
        tab->pos = pos;
    }

    if (is_container(dock))
    {
        set_children_pos_size(dock, pos, size);
    }
}

static void set_children_pos_size(Dock *dock,  ImVec2 pos, ImVec2 size)
{
    ImVec2 children_size = dock->children[0]->size;
    if (is_horizontal(dock))
    {
        children_size.y = size.y;
        children_size.x = (f32)((i32)(size.x * dock->children[0]->size.x / (dock->children[0]->size.x + dock->children[1]->size.x)));

        if (children_size.x < get_min_size(dock->children[0]).x)
        {
            children_size.x = get_min_size(dock->children[0]).x;
        }
        else if (size.x - children_size.x < get_min_size(dock->children[1]).x)
        {
            children_size.x = size.x - get_min_size(dock->children[1]).x;
        }
        set_pos_size(dock->children[0], pos, children_size);

        ImVec2 children_pos = pos;
        children_size.x = size.x - dock->children[0]->size.x;
        children_pos.x += dock->children[0]->size.x;
        set_pos_size(dock->children[1], children_pos, children_size);
    }
    else
    {
        children_size.x = size.x;
        children_size.y = (f32)((int)(size.y * dock->children[0]->size.y / (dock->children[0]->size.y + dock->children[1]->size.y)));

        if (children_size.y < get_min_size(dock->children[0]).y)
        {
            children_size.y = get_min_size(dock->children[0]).y;
        }
        else if (size.y - children_size.y < get_min_size(dock->children[1]).y)
        {
            children_size.y = size.y - get_min_size(dock->children[1]).y;
        }
        set_pos_size(dock->children[0], pos, children_size);

        ImVec2 children_pos = pos;
        children_size.y = size.y - dock->children[0]->size.y;
        children_pos.y += dock->children[0]->size.y;
        set_pos_size(dock->children[1], children_pos, children_size);
    }
}

static void init_dock_context(UIState *context)
{
    context->drag_offset = ImVec2(0, 0);
    context->current = 0;
    context->last_frame = 0;
    context->end_action = EndAction_None;
    context->first_free_dock = 0;
    context->first_dock = 0;
}

static Dock *get_dock(UIState *context, char *label, b32 opened)
{
    ImU32 id = ImHash(label, 0);
    Dock *dock = 0;

    for (Dock *test_dock = context->first_dock; test_dock; test_dock = test_dock->next)
    {
        if (test_dock->id == id)
        {
            dock = test_dock;
            break;
        }
    }

    if (!dock)
    {
        dock = allocate_dock(context);
        init_dock(dock);

        dock->id = id;
        dock->status = Status_Float;
        dock->pos = ImVec2(0, 0);
        dock->size = ImGui::GetIO().DisplaySize;
        dock->opened = opened;
        dock->first = true;
        dock->last_frame = 0;
        dock->invalid_frames = 0;
        dock->location[0] = 0;

        copy_string_and_null_terminate(label, dock->label, array_count(dock->label));

        set_active(dock);
    }
    return dock;
}

static void put_in_background(UIState *context)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    ImGuiContext *imgui = GImGui;

    if (imgui->Windows[0] != window)
    {
        for (i32 window_index = 0; window_index < imgui->Windows.Size; ++window_index)
        {
            if (imgui->Windows[window_index] == window)
            {
                for (i32 prev_window_index = window_index - 1; prev_window_index >= 0; --prev_window_index)
                {
                    imgui->Windows[prev_window_index + 1] = imgui->Windows[prev_window_index];
                }

                imgui->Windows[0] = window;
                break;
            }
        }
    }
}

static void splits(UIState *context)
{
    if (ImGui::GetFrameCount() != context->last_frame)
    {
        context->last_frame = ImGui::GetFrameCount();

        put_in_background(context);

        ImU32 color = ImGui::GetColorU32(ImGuiCol_Button);
        ImU32 color_hovered = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImGuiIO *imgui_io = &ImGui::GetIO();

        for (Dock *dock = context->first_dock; dock; dock = dock->next)
        {
            if (is_container(dock))
            {
                ImGui::PushID(dock);
                if (!ImGui::IsMouseDown(0))
                {
                    dock->status = Status_Docked;
                }

                ImVec2 dock_size = ImVec2(0, 0);
                ImGui::SetCursorScreenPos(dock->children[1]->pos);

                ImVec2 min_size_0 = get_min_size(dock->children[0]);
                ImVec2 min_size_1 = get_min_size(dock->children[1]);

                if (is_horizontal(dock))
                {
                    ImGui::InvisibleButton("split", ImVec2(2, dock->size.y));
                    if (dock->status == Status_Dragged) 
                    {
                        dock_size.x = imgui_io->MouseDelta.x;
                    }
                    dock_size.x = -ImMin(-dock_size.x, dock->children[0]->size.x - min_size_0.x);
                    dock_size.x =  ImMin( dock_size.x, dock->children[1]->size.x - min_size_1.x);
                }
                else
                {
                    ImGui::InvisibleButton("split", ImVec2(dock->size.x, 2));
                    if (dock->status == Status_Dragged) 
                    {
                        dock_size.y = imgui_io->MouseDelta.y;
                    }
                    dock_size.y = -ImMin(-dock_size.y, dock->children[0]->size.y - min_size_0.y);
                    dock_size.y =  ImMin( dock_size.y, dock->children[1]->size.y - min_size_1.y);
                }

                ImVec2 new_size_0 = dock->children[0]->size + dock_size;
                ImVec2 new_size_1 = dock->children[1]->size - dock_size;
                ImVec2 new_pos_1 = dock->children[1]->pos + dock_size;
                set_pos_size(dock->children[0], dock->children[0]->pos, new_size_0);
                set_pos_size(dock->children[1], new_pos_1, new_size_1);

                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
                {
                    dock->status = Status_Dragged;
                }

                draw_list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemHovered() ? color_hovered : color);
                ImGui::PopID();
            }
        }
    }
}

static void do_undock(UIState *context, Dock *dock)
{
    if (dock->prev_tab)
    {
        set_active(dock->prev_tab);
    }
    else if (dock->next_tab)
    {
        set_active(dock->next_tab);
    }
    else
    {
        dock->active = false;
    }

    Dock *container = dock->parent;
    if (container)
    {
        Dock *sibling = get_sibling(dock);
        if (container->children[0] == dock)
        {
            container->children[0] = dock->next_tab;
        }
        else if (container->children[1] == dock)
        {
            container->children[1] = dock->next_tab;
        }

        b32 remove_container = (!container->children[0] || !container->children[1]);
        if (remove_container)
        {
            if (container->parent)
            {
                Dock **child = (container->parent->children[0] == container) ? &container->parent->children[0] : &container->parent->children[1];
                *child = sibling;

                set_pos_size(*child, container->pos, container->size);
                set_parent(*child, container->parent);
            }
            else
            {
                if (container->children[0])
                {
                    set_parent(container->children[0], 0);
                    set_pos_size(container->children[0], container->pos, container->size);
                }
                if (container->children[1])
                {
                    set_parent(container->children[1], 0);
                    set_pos_size(container->children[1], container->pos, container->size);
                }
            }

            for (Dock *test_dock = context->first_dock; test_dock; test_dock = test_dock->next)
            {
                if (test_dock == container)
                {
                    remove_dock(context, test_dock);
                    break;
                }
            }

            remove_dock(context, container);
        }
    }

    if (dock->prev_tab) 
    {
        dock->prev_tab->next_tab = dock->next_tab;
    }
    if (dock->next_tab) 
    {
        dock->next_tab->prev_tab = dock->prev_tab;
    }

    dock->parent = 0;
    dock->prev_tab = dock->next_tab = 0;
}

static void check_nonextistent(UIState *context)
{
    int frame_limit = ImMax(0, ImGui::GetFrameCount() - 2);
    for (Dock *dock = context->first_dock; dock; dock = dock->next)
    {
        if (!is_container(dock) && (dock->status != Status_Float))
        {
            if (dock->last_frame < frame_limit)
            {
                ++dock->invalid_frames;
                if (dock->invalid_frames > 2)
                {
                    do_undock(context, dock);
                    dock->status = Status_Float;
                }

                break;
            }

            dock->invalid_frames = 0;
        }
    }
}

static Dock *get_root_dock(UIState *context)
{
    Dock *root_dock = 0;
    for (Dock *dock = context->first_dock; dock; dock = dock->next)
    {
        if (!dock->parent && (dock->status == Status_Docked || dock->children[0]))
        {
            root_dock = dock;
            break;
        }
    }
    return root_dock;
}

static void begin_panel(UIState *context)
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | 
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImVec2 display_size = ImGui::GetIO().DisplaySize;

    Dock *root = get_root_dock(context);
    if (root)
    {
        ImVec2 percentage = ImVec2(display_size.x / root->size.x, display_size.y / root->size.y );
        ImVec2 rescaled_pos = root->pos * percentage;
        ImVec2 rescaled_size = root->size * percentage;

        ImGui::SetNextWindowPos(rescaled_pos);
        ImGui::SetNextWindowSize(rescaled_size);
    }
    else
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(display_size);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("###DockPanel", 0, flags);

    splits(context);
    check_nonextistent(context);
}

static void end_panel(UIState *context)
{
    ImGui::End();
    ImGui::PopStyleVar();
}

static Dock *get_dock_at(UIState *context)
{
    Dock *dock = 0;

    for (Dock *test_dock = context->first_dock; test_dock; test_dock = test_dock->next)
    {
        if (!is_container(test_dock) && (test_dock->status == Status_Docked))
        {
            if (ImGui::IsMouseHoveringRect(test_dock->pos, test_dock->pos + test_dock->size, false))
            {
                dock = test_dock;
                break;
            }
        }
    }
    return dock;
}

static ImRect get_docked_rect(UIState *context, ImRect rect, Slot dock_slot)
{
    ImVec2 half_size = rect.GetSize() * 0.5f;

    ImRect docked_rect = rect;
    switch (dock_slot)
    {
        case Slot_Top: 
        {
            docked_rect = ImRect(rect.Min, rect.Min + ImVec2(rect.Max.x, half_size.y));
        } break;
        case Slot_Right: 
        {
            docked_rect = ImRect(rect.Min + ImVec2(half_size.x, 0), rect.Max);
        } break;
        case Slot_Bottom: 
        {
            docked_rect = ImRect(rect.Min + ImVec2(0, half_size.y), rect.Max);
        } break;
        case Slot_Left: 
        {
            docked_rect = ImRect(rect.Min, rect.Min + ImVec2(half_size.x, rect.Max.y));
        } break;
    }
    return docked_rect;
}

static ImRect get_slot_rect(UIState *context, ImRect parent_rect, Slot dock_slot)
{
    ImVec2 size = parent_rect.Max - parent_rect.Min;
    ImVec2 center = parent_rect.Min + size * 0.5f;

    ImRect slot_rect = ImRect(center - ImVec2(20, 20), center + ImVec2(20, 20));
    switch (dock_slot)
    {
        case Slot_Top: 
        {
            slot_rect = ImRect(center + ImVec2(-20, -50), center + ImVec2(20, -30));
        } break;
        case Slot_Right: 
        {
            slot_rect = ImRect(center + ImVec2(30, -20), center + ImVec2(50, 20));
        } break;
        case Slot_Bottom: 
        {
            slot_rect = ImRect(center + ImVec2(-20, +30), center + ImVec2(20, 50));
        } break;
        case Slot_Left: 
        {
            slot_rect = ImRect(center + ImVec2(-50, -20), center + ImVec2(-30, 20));
        } break;
    }
    return slot_rect;
}

static ImRect get_slot_rect_on_border(UIState *context, ImRect parent_rect, Slot dock_slot)
{
    ImVec2 size = parent_rect.Max - parent_rect.Min;
    ImVec2 center = parent_rect.Min + size * 0.5f;

    ImRect slot_rect = ImRect();
    switch (dock_slot)
    {
        case Slot_Top:
        {
            slot_rect = ImRect(ImVec2(center.x - 20, parent_rect.Min.y + 10), ImVec2(center.x + 20, parent_rect.Min.y + 30));
        } break;
        case Slot_Left:
        {
            slot_rect = ImRect(ImVec2(parent_rect.Min.x + 10, center.y - 20), ImVec2(parent_rect.Min.x + 30, center.y + 20));
        } break;
        case Slot_Bottom:
        {
            slot_rect = ImRect(ImVec2(center.x - 20, parent_rect.Max.y - 30), ImVec2(center.x + 20, parent_rect.Max.y - 10));
        } break;
        case Slot_Right:
        {
            slot_rect = ImRect(ImVec2(parent_rect.Max.x - 30, center.y - 20), ImVec2(parent_rect.Max.x - 10, center.y + 20));
        } break;
        invalid_default_case;
    }
    return slot_rect;
}

static void set_dock_pos_size(UIState *context, Dock *dest, Dock *dock, Slot dock_slot, Dock *container)
{
    assert(!dock->prev_tab && !dock->next_tab && !dock->children[0] && !dock->children[1]);

    dest->pos = container->pos;
    dest->size = container->size;
    dock->pos = container->pos;
    dock->size = container->size;

    switch (dock_slot)
    {
        case Slot_Bottom:
        {
            dest->size.y *= 0.5f;
            dock->size.y *= 0.5f;
            dock->pos.y += dest->size.y;
        } break;
        case Slot_Right:
        {
            dest->size.x *= 0.5f;
            dock->size.x *= 0.5f;
            dock->pos.x += dest->size.x;
        } break;
        case Slot_Left:
        {
            dest->size.x *= 0.5f;
            dock->size.x *= 0.5f;
            dest->pos.x += dock->size.x;
        } break;
        case Slot_Top:
        {
            dest->size.y *= 0.5f;
            dock->size.y *= 0.5f;
            dest->pos.y += dock->size.y;
        } break;
        invalid_default_case;
    }
    set_pos_size(dest, dest->pos, dest->size);

    if (container->children[1]->pos.x < container->children[0]->pos.x || container->children[1]->pos.y < container->children[0]->pos.y)
    {
        Dock *child_0 = container->children[0];
        container->children[0] = container->children[1];
        container->children[1] = child_0;
    }
}

static void do_dock(UIState *context, Dock *dock, Dock *dest, Slot dock_slot)
{
    assert(!dock->parent);

    if (!dest)
    {
        dock->status = Status_Docked;
        set_pos_size(dock, ImVec2(0, 0), ImGui::GetIO().DisplaySize);
    }
    else if (dock_slot == Slot_Tab)
    {
        Dock *last_tab = dest;
        while (last_tab->next_tab)
        {
            last_tab = last_tab->next_tab;
        }

        last_tab->next_tab = dock;
        dock->prev_tab = last_tab;
        dock->size = last_tab->size;
        dock->pos = last_tab->pos;
        dock->parent = dest->parent;
        dock->status = Status_Docked;
    }
    else if (dock_slot == Slot_None)
    {
        dock->status = Status_Float;
    }
    else
    {
        Dock *container = allocate_dock(context);
        init_dock(container);

        container->children[0] = get_first_tab(dest);
        container->children[1] = dock;
        container->next_tab = 0;
        container->prev_tab = 0;
        container->parent = dest->parent;
        container->size = dest->size;
        container->pos = dest->pos;
        container->status = Status_Docked;
        container->label[0] = 0;

        if (!dest->parent)
        {
        }
        else if (get_first_tab(dest) == dest->parent->children[0])
        {
            dest->parent->children[0] = container;
        }
        else
        {
            dest->parent->children[1] = container;
        }

        set_parent(dest, container);
        dock->parent = container;
        dock->status = Status_Docked;

        set_dock_pos_size(context, dest, dock, dock_slot, container);
    }
    set_active(dock);
}

static b32 dock_slots(UIState *context, Dock *dock, Dock *dest_dock, ImRect rect, b32 on_border)
{
    ImU32 color = ImGui::GetColorU32(ImGuiCol_Button);
    ImU32 color_hovered = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;

    b32 result = false;
    for (i32 slot_index = 0; slot_index < (on_border ? 4 : 5); ++slot_index)
    {
        ImRect slot_rect = on_border ? get_slot_rect_on_border(context, rect, (Slot)slot_index) : get_slot_rect(context, rect, (Slot)slot_index);
        b32 hovered = slot_rect.Contains(mouse_pos);
        draw_list->AddRectFilled(slot_rect.Min, slot_rect.Max, hovered ? color_hovered : color);

        if (hovered)
        {
            if (!ImGui::IsMouseDown(0))
            {
                do_dock(context, dock, dest_dock ? dest_dock : get_root_dock(context), (Slot)slot_index);

                result = true;
                break;
            }

            ImRect docked_rect = get_docked_rect(context, rect, (Slot)slot_index);
            draw_list->AddRectFilled(docked_rect.Min, docked_rect.Max, ImGui::GetColorU32(ImGuiCol_Button));
        }
    }
    return result;
}

static void handle_drag(UIState *context, Dock *dock)
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("##Overlay", 0, ImVec2(0, 0), 0.0f, flags);
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        draw_list->PushClipRectFullScreen();

        ImU32 docked_color = ImGui::GetColorU32(ImGuiCol_FrameBg);
        docked_color = (docked_color & 0x00ffFFFF) | 0x80000000;
        dock->pos = ImGui::GetIO().MousePos - context->drag_offset;

        Dock *dest_dock = get_dock_at(context);
        if (dest_dock)
        {
            if (dock_slots(context, dock, dest_dock, ImRect(dest_dock->pos, dest_dock->pos + dest_dock->size), false))
            {
                draw_list->PopClipRect();
            }
        }
        else
        {
            if (dock_slots(context, dock, 0, ImRect(ImVec2(0, 0), ImGui::GetIO().DisplaySize), true))
            {
                draw_list->PopClipRect();
            }
            else
            {
                draw_list->AddRectFilled(dock->pos, dock->pos + dock->size, docked_color);
                draw_list->PopClipRect();

                if (!ImGui::IsMouseDown(0))
                {
                    dock->status = Status_Float;
                    dock->location[0] = 0;
                    set_active(dock);
                }
            }
        }        
    }
    ImGui::End();
}

static char get_location_code(UIState *context, Dock *dock)
{
    char location = '0';
    if (dock)
    {
        if (is_horizontal(dock->parent))
        {
            if ((dock->pos.x < dock->parent->children[0]->pos.x) || (dock->pos.x < dock->parent->children[1]->pos.x))
            {
                location = '1';
            }
        }
        else
        {
            if ((dock->pos.y < dock->parent->children[0]->pos.y) || (dock->pos.y < dock->parent->children[1]->pos.y))
            {
                location = '2';
            }
            else
            {
                location = '3';
            }
        }
    }
    return location;
}

static void fill_location(UIState *context, Dock *dock)
{
    if (dock->status != Status_Float)
    {
        char *c = dock->location;

        Dock *tmp = dock;
        while (tmp->parent)
        {
            *c = get_location_code(context, tmp);
            tmp = tmp->parent;
            ++c;
        }

        *c = 0;
    }
}

static void draw_tabbar_list_button(UIState *context, Dock *dock)
{
    if (dock->next_tab)
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        if (ImGui::InvisibleButton("list", ImVec2(16, 16)))
        {
            ImGui::OpenPopup("tab_list_popup");
        }
        if (ImGui::BeginPopup("tab_list_popup"))
        {
            Dock *tab = dock;
            while (tab)
            {
                bool dummy = false;
                if (ImGui::Selectable(tab->label, &dummy))
                {
                    set_active(tab);
                }
                tab = tab->next_tab;
            }
            ImGui::EndPopup();
        }

        b32 hovered = ImGui::IsItemHovered();
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        ImVec2 center = (min + max) * 0.5f;
        ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);
        ImU32 color_active = ImGui::GetColorU32(ImGuiCol_FrameBgActive);

        draw_list->AddRectFilled(    ImVec2(center.x - 4, min.y + 3), ImVec2(center.x + 4, min.y + 5),                               hovered ? color_active : text_color);
        draw_list->AddTriangleFilled(ImVec2(center.x - 4, min.y + 7), ImVec2(center.x + 4, min.y + 7), ImVec2(center.x, min.y + 12), hovered ? color_active : text_color);
    }
}

static b32 tabbar(UIState *context, Dock *dock, b32 close_button)
{
    f32 tabbar_height = 2 * ImGui::GetTextLineHeightWithSpacing();
    ImVec2 size = ImVec2(dock->size.x, tabbar_height);

    ImGui::SetCursorScreenPos(dock->pos);
    char tmp[20];
    ImFormatString(tmp, IM_ARRAYSIZE(tmp), "tabs%d", (int)dock->id);

    b32 tab_closed = false;
    if (ImGui::BeginChild(tmp, size, true))
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImU32 color = ImGui::GetColorU32(ImGuiCol_FrameBg);
        ImU32 color_active = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
        ImU32 color_hovered = ImGui::GetColorU32(ImGuiCol_FrameBgHovered);
        ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);
        ImU32 text_color_disabled = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        f32 line_height = ImGui::GetTextLineHeightWithSpacing();
        f32 tab_base = 0;

        draw_tabbar_list_button(context, dock);

        Dock *dock_tab = dock;
        while (dock_tab)
        {
            ImGui::SameLine(0, 15);

            char *text_end = (char *)ImGui::FindRenderedTextEnd(dock_tab->label);
            ImVec2 text_size = ImVec2(ImGui::CalcTextSize(dock_tab->label, text_end).x, line_height);

            if (ImGui::InvisibleButton(dock_tab->label, text_size))
            {
                set_active(dock_tab);
            }

            if (ImGui::IsItemActive() && ImGui::IsMouseDragging())
            {
                context->drag_offset = ImGui::GetMousePos() - dock_tab->pos;
                do_undock(context, dock_tab);
                dock_tab->status = Status_Dragged;
            }

            b32 hovered = ImGui::IsItemHovered();
            ImVec2 pos = ImGui::GetItemRectMin();
            text_size.x += 20 + ImGui::GetStyle().ItemSpacing.x;
            tab_base = pos.y;

            if (dock_tab->active && close_button)
            {
                ImGui::SameLine();
                tab_closed = ImGui::InvisibleButton("close", ImVec2(16, 16));

                ImVec2 center = ((ImGui::GetItemRectMin() + ImGui::GetItemRectMax()) * 0.5f);
                center.y += 2;

                ImU32 close_color = ImGui::IsItemHovered() ? text_color : text_color_disabled;
                draw_list->AddLine(center + ImVec2(-3.5f, -3.5f), center + ImVec2( 3.5f, 3.5f), close_color);
                draw_list->AddLine(center + ImVec2( 3.5f, -3.5f), center + ImVec2(-3.5f, 3.5f), close_color);
            }
            else 
            {
                if (!dock_tab->active && close_button) 
                {
                    ImGui::SameLine();
                    ImGui::InvisibleButton("close", ImVec2(16, 16));

                    ImVec2 center = ((ImGui::GetItemRectMin() + ImGui::GetItemRectMax()) * 0.5f);
                    center.y += 2;
                    draw_list->AddLine(center + ImVec2(-3.5f, -3.5f), center + ImVec2( 3.5f, 3.5f), text_color_disabled);
                    draw_list->AddLine(center + ImVec2( 3.5f, -3.5f), center + ImVec2(-3.5f, 3.5f), text_color_disabled);
                }
            }

            draw_list->PathClear();
            #if 1
            draw_list->PathLineTo(pos + ImVec2(-15, text_size.y));
            draw_list->PathBezierCurveTo(pos + ImVec2(-10, text_size.y), pos + ImVec2(-5, 0), pos + ImVec2(0, 0), 10);
            draw_list->PathLineTo(pos + ImVec2(text_size.x, 0));
            draw_list->PathBezierCurveTo(pos + ImVec2(text_size.x + 5, 0), pos + ImVec2(text_size.x + 10, text_size.y), pos + ImVec2(text_size.x + 15, text_size.y), 10);
            #else
            draw_list->PathLineTo(pos + ImVec2(-15, text_size.y)); // bottomleft
            draw_list->PathLineTo(pos + ImVec2(-5, 0)); // topleft
            draw_list->PathLineTo(pos + ImVec2(text_size.x, 0)); // topright
            draw_list->PathLineTo(pos + ImVec2(text_size.x + 10, text_size.y));
            #endif
            draw_list->PathFill(hovered ? color_hovered : (dock_tab->active ? color_active : color));

            pos.y += 4;
            draw_list->AddText(pos, text_color, dock_tab->label, 0);

            dock_tab = dock_tab->next_tab;
        }

        ImVec2 cp = ImVec2(dock->pos.x, tab_base + line_height);
        draw_list->AddLine(cp, cp + ImVec2(dock->size.x, 0), color);
    }

    ImGui::EndChild();
    return tab_closed;
}

static void root_dock(UIState *context, ImVec2 pos, ImVec2 size)
{
    Dock *root = get_root_dock(context);
    if (root)
    {
        ImVec2 min_size = get_min_size(root);
        set_pos_size(root, pos, ImMax(min_size, size));
    }
}

static void set_dock_active(UIState *context)
{
    assert(context->current);
    if (context->current)
    {
        set_active(context->current);
    }
}

static Slot get_slot_from_location_code(UIState *context, char code)
{
    Slot slot = Slot_Right;
    switch (code)
    {
        case '1':
        {
            slot = Slot_Left;
        } break;
        case '2':
        {
            slot = Slot_Top;
        } break;
        case '3':
        {
            slot = Slot_Bottom;
        } break;
    }
    return slot;
}

static void try_dock_to_stored_location(UIState *context, Dock *dock)
{
    if ((dock->status != Status_Docked) && (dock->location[0] != 0))
    {
        Dock *tmp = get_root_dock(context);
        if (tmp)
        {
            Dock *prev = 0;
            char *c = dock->location + string_length(dock->location) - 1;

            while (c >= dock->location && tmp)
            {
                prev = tmp;
                tmp = (*c == get_location_code(context, tmp->children[0])) ? tmp->children[0] : tmp->children[1];
                if(tmp) 
                {
                    --c;
                }
            }

            if (tmp && tmp->children[0])
            {
                tmp = tmp->parent;
            }

            do_dock(context, dock, tmp ? tmp : prev, tmp && !tmp->children[0] ? Slot_Tab : get_slot_from_location_code(context, *c));
        }
    }
}

static b32 begin_dock(UIState *context, char *label, b32 *opened, ImGuiWindowFlags extra_flags)
{
    Dock *dock = get_dock(context, label, !opened || *opened);
    if (!dock->opened && (!opened || *opened)) 
    {
        try_dock_to_stored_location(context, dock);
    }

    dock->last_frame = ImGui::GetFrameCount();
    if (!strings_are_equal(dock->label, label))
    {
        copy_string_and_null_terminate(label, dock->label, array_count(dock->label));
    }

    context->end_action = EndAction_None;
    if (dock->first && opened) 
    {
        *opened = dock->opened;
    }
    dock->first = false;

    b32 result = false;
    if (opened && !*opened)
    {
        if (dock->status != Status_Float)
        {
            fill_location(context, dock);
            do_undock(context, dock);

            dock->status = Status_Float;
        }
        dock->opened = false;
    }
    else
    {
        dock->opened = true;
        context->end_action = EndAction_Panel;

        begin_panel(context);

        context->current = dock;
        if (dock->status == Status_Dragged) 
        {
            handle_drag(context, dock);
        }

        if (dock->status == Status_Float)
        {
            ImGui::SetNextWindowPos(dock->pos);
            ImGui::SetNextWindowSize(dock->size);

            result = ImGui::Begin(label, (bool *)opened, dock->size, -1.0f, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_ShowBorders | extra_flags);

            context->end_action = EndAction_End;
            dock->pos = ImGui::GetWindowPos();
            dock->size = ImGui::GetWindowSize();

            ImGuiContext *imgui = GImGui;
            if (imgui->ActiveId == ImGui::GetCurrentWindow()->MoveId && imgui->IO.MouseDown[0])
            {
                context->drag_offset = ImGui::GetMousePos() - dock->pos;
                do_undock(context, dock);
                dock->status = Status_Dragged;
            }
        }
        else
        {
            if (!dock->active && dock->status != Status_Dragged)
            {
            }
            else
            {
                context->end_action = EndAction_EndChild;

                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));

                float tabbar_height = ImGui::GetTextLineHeightWithSpacing();
                if (tabbar(context, get_first_tab(dock), opened != 0))
                {
                    fill_location(context, dock);
                    *opened = false;
                }

                ImVec2 pos = dock->pos;
                ImVec2 size = dock->size;
                pos.y  += tabbar_height + ImGui::GetStyle().WindowPadding.y;
                size.y -= tabbar_height + ImGui::GetStyle().WindowPadding.y;

                ImGui::SetCursorScreenPos(pos);

                ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | 
                                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | extra_flags;

                char child_name[40];
                u32 label_length = string_length(label);
                copy_string(label, child_name, array_count(child_name));
                copy_string_and_null_terminate("_docked", child_name + label_length, array_count(child_name) - label_length);

                result = ImGui::BeginChild(child_name, size, true, flags);
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
            }
        }
    }
    return result;
}

static void end_dock(UIState *context)
{
    if (context->end_action == EndAction_End)
    {
        ImGui::End();
    }
    else if (context->end_action == EndAction_EndChild)
    {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }

    context->current = 0;

    if (context->end_action > EndAction_None) 
    {
        end_panel(context);
    }
}

inline i32 get_dock_index(UIState *context, Dock *dock)
{
    i32 index = -1;
    if (dock)
    {
        i32 test_index = 0;
        for (Dock *test_dock = context->first_dock; test_dock; test_dock = test_dock->next, ++test_index)
        {
            if (dock == test_dock)
            {
                index = test_index;
                break;
            }
        }
    }
    return index;
}

inline Dock *get_dock_by_index(UIState *context, i32 index)
{
    Dock *dock = 0;
    {
        i32 test_index = 0;
        for (Dock *test_dock = context->first_dock; test_dock; test_dock = test_dock->next, ++test_index)
        {
            if (test_index == index)
            {
                dock = test_dock;
                break;
            }
        }
    }
    return dock;
}

inline u32 get_num_docks(UIState *context)
{
    u32 num_docks = 0;
    for (Dock *dock = context->first_dock; dock; dock = dock->next)
    {
        ++num_docks;
    }
    return num_docks;
}
