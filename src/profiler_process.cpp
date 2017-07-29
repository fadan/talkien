
static ProfilerParsedName profiler_parse_name(char *guid)
{
    ProfilerParsedName parsed_name = {};
    u32 num_pipes = 0;
    u32 name_starts_at = 0;

    char *scan = guid;

    for (; *scan; ++scan)
    {
        if (*scan == '|')
        {
            if (num_pipes == 0)
            {
                parsed_name.file_name_length = (u32)(scan - guid);
                parsed_name.line_number = string_to_i32(scan + 1);
            }
            else
            {
                name_starts_at = (u32)(scan - guid + 1);
            }
            ++num_pipes;
        }
        parsed_name.hash_value = 65599 * parsed_name.hash_value + *scan;
    }

    parsed_name.name_length = (u32)(scan - guid) - name_starts_at;
    parsed_name.name = guid + name_starts_at;
    return parsed_name;
}

static ProfilerElement *profiler_get_element_from_guid(ProfilerState *state, u32 index, char *guid)
{
    ProfilerElement *element = 0;
    for (ProfilerElement *chain = state->element_hash[index]; chain; chain = chain->next_in_hash)
    {
        if (strings_are_equal(chain->guid, guid))
        {
            element = chain;
            break;
        }
    }
    return element;
}

static ProfilerLink *profiler_create_link(ProfilerState *state)
{
    ProfilerLink *link = push_struct(&state->profiler_memory, ProfilerLink);
    dllist_init(profiler_get_sentinel(link));
    link->next = link->prev = 0;
    link->element = 0;
    return link;
}

static ProfilerLink *profiler_add_element_to_group(ProfilerState *state, ProfilerLink *parent, ProfilerElement *element)
{
    ProfilerLink *link = profiler_create_link(state);
    if (parent)
    {
        dllist_insert_last(profiler_get_sentinel(parent), link);
    }
    link->element = element;
    return link;
}

static ProfilerElement *profiler_get_element_from_entry(ProfilerState *state, ProfilerEntry *entry, ProfilerLink *parent)
{

    ProfilerParsedName parsed_name = profiler_parse_name(entry->guid);
    u32 index = (parsed_name.hash_value % array_count(state->element_hash));

    ProfilerElement *element = profiler_get_element_from_guid(state, index, entry->guid);
    if (!element)
    {
        element = push_struct(&state->profiler_memory, ProfilerElement);

        // element->original_guid = entry->guid;
        element->guid = push_string(&state->profiler_memory, entry->guid);
        element->file_name_length = parsed_name.file_name_length;
        element->line_number = parsed_name.line_number;
        element->name = push_string(&state->profiler_memory, parsed_name.name);
        element->type = (ProfilerEntryType)entry->type;

        element->next_in_hash = state->element_hash[index];
        state->element_hash[index] = element;

        profiler_add_element_to_group(state, parent, element);
    }
    return element;
}

static void profiler_init(ProfilerState *state)
{
    state->gather_frame_index = 1;

    ProfilerEntry root_event = {};
    root_event.guid = PROFILER_GUID("RootEvent");
    state->root_element = profiler_get_element_from_entry(state, &root_event, 0);

    state->profiler_group = profiler_create_link(state);
}

inline ProfilerFrame *profiler_get_gather_frame(ProfilerState *state)
{
    ProfilerFrame *frame = state->frames + state->gather_frame_index;
    return frame;
}

static void profiler_free_frame(ProfilerState *state, u32 frame_index)
{
    assert(frame_index < PROFILER_NUM_FRAME_HISTORY);
    
    u32 num_freed_entries = 0;
    for (u32 element_hash_index = 0; element_hash_index < array_count(state->element_hash); ++element_hash_index)
    {
        for (ProfilerElement *element = state->element_hash[element_hash_index]; element; element = element->next_in_hash)
        {
            ProfilerElementFrame *element_frame = element->frames + frame_index;
            while (element_frame->oldest_entry)
            {
                ProfilerStoredEntry *free_entry = element_frame->oldest_entry;
                element_frame->oldest_entry = free_entry->next;
                deallocate_freelist(free_entry, state->first_free_stored_entry);
                ++num_freed_entries;
            }
            zero_struct(*element_frame);
        }
    }

    ProfilerFrame *frame = state->frames + frame_index;
    assert(frame->num_stored_entries == num_freed_entries);
    zero_struct(*frame);
}

inline void profiler_increment_frame_index(u32 *index)
{
    *index = (*index + 1) % PROFILER_NUM_FRAME_HISTORY;
}

static void profiler_free_oldest_frame(ProfilerState *state)
{
    profiler_free_frame(state, state->oldest_frame_index);
    if (state->oldest_frame_index == state->most_recent_frame_index)
    {
        profiler_increment_frame_index(&state->most_recent_frame_index);
    }
    profiler_increment_frame_index(&state->oldest_frame_index);
}

inline void profiler_init_frame(ProfilerState *state, ProfilerFrame *frame, u64 begin_timestamp)
{
    frame->frame_index = state->total_frame_count++;
    frame->begin_timestamp = begin_timestamp;
}

static ProfilerThread *profiler_get_thread(ProfilerState *state, u32 thread_id)
{
    ProfilerThread *thread = 0;
    for (ProfilerThread *test_thread = state->first_thread; test_thread; test_thread = test_thread->next)
    {
        if (test_thread->id == thread_id)
        {
            thread = test_thread;
            break;
        }
    }

    if (!thread)
    {
        allocate_freelist(thread, state->first_free_thread, push_struct(&state->profiler_memory, ProfilerThread));

        thread->id = thread_id;
        thread->index = state->num_threads++;
        thread->first_open_block = 0;
        thread->next = state->first_thread;
        state->first_thread = thread;
    }
    return thread;
}

static ProfilerStoredEntry *profiler_store_entry(ProfilerState *state, ProfilerElement *element, ProfilerEntry *entry)
{
    ProfilerStoredEntry *stored_entry = 0;
    while (!stored_entry)
    {
        stored_entry = state->first_free_stored_entry;
        if (stored_entry)
        {
            state->first_free_stored_entry = stored_entry->next_free;
        }
        else
        {
            stored_entry = push_struct(&state->profiler_memory, ProfilerStoredEntry);
        }
    }

    ProfilerFrame *gather_frame = profiler_get_gather_frame(state);

    stored_entry->next = 0;
    stored_entry->frame_index = gather_frame->frame_index;
    stored_entry->entry = *entry;

    ++gather_frame->num_stored_entries;

    ProfilerElementFrame *frame = element->frames + state->gather_frame_index;
    if (frame->most_recent_entry)
    {
        frame->most_recent_entry = frame->most_recent_entry->next = stored_entry;
    }
    else
    {
        frame->oldest_entry = frame->most_recent_entry = stored_entry;
    }

    return stored_entry;
}

static ProfilerOpenBlock *profiler_allocate_open_block(ProfilerState *state, ProfilerElement *element, u32 frame_index, ProfilerEntry *entry, ProfilerOpenBlock **first_open_block)
{
    ProfilerOpenBlock *open_block = 0;
    allocate_freelist(open_block, state->first_free_block, push_struct(&state->profiler_memory, ProfilerOpenBlock));

    open_block->starting_frame_index = frame_index;
    open_block->begin_timestamp = entry->timestamp;
    open_block->element = element;
    open_block->next_free = 0;

    open_block->parent = *first_open_block;
    *first_open_block = open_block;

    return open_block;
}

static void profiler_deallocate_open_block(ProfilerState *state, ProfilerOpenBlock **first_open_block)
{
    ProfilerOpenBlock *free_block = *first_open_block;
    *first_open_block = free_block->parent;

    free_block->next_free = state->first_free_block;
    state->first_free_block = free_block;
}

static void profiler_gather(ProfilerState *state, ProfilerEntry *entries, u32 num_entries)
{
    PROFILER_FUNCTION();

    for (u32 entry_index = 0; entry_index < num_entries; ++entry_index)
    {
        ProfilerEntry *entry = entries + entry_index;
        
        if (entry->type == ProfilerEntryType_FrameTime)
        {
            ProfilerFrame *gather_frame = profiler_get_gather_frame(state);

            gather_frame->end_timestamp = entry->timestamp;
            if (gather_frame->root_node)
            {
                gather_frame->root_node->profiler_node.duration = (gather_frame->end_timestamp - gather_frame->begin_timestamp);
            }

            gather_frame->frame_time = entry->frame_time;

            ++state->total_frame_count;

            if (state->paused)
            {
                profiler_free_frame(state, state->gather_frame_index);
            }
            else
            {
                state->most_recent_frame_index = state->gather_frame_index;
                profiler_increment_frame_index(&state->gather_frame_index);
                if (state->gather_frame_index == state->oldest_frame_index)
                {
                    profiler_free_oldest_frame(state);
                }
                gather_frame = profiler_get_gather_frame(state);
            }
            profiler_init_frame(state, gather_frame, entry->timestamp);
        }
        else
        {
            ProfilerFrame *gather_frame = profiler_get_gather_frame(state);

            u32 frame_index = state->total_frame_count - 1;
            ProfilerThread *thread = profiler_get_thread(state, entry->thread_id);

            switch (entry->type)
            {
                case ProfilerEntryType_Begin:
                {
                    ++gather_frame->num_profile_blocks;
                    
                    ProfilerElement *element = profiler_get_element_from_entry(state, entry, state->profiler_group);
                    ProfilerStoredEntry *parent_entry = gather_frame->root_node;
                    u64 clock_basis = gather_frame->begin_timestamp;
                    
                    if (thread->first_open_block)
                    {
                        parent_entry = thread->first_open_block->node;
                        clock_basis = thread->first_open_block->begin_timestamp;
                    }
                    else if (!parent_entry)
                    {
                        ProfilerEntry null_entry = {};
                        parent_entry = profiler_store_entry(state, state->root_element, &null_entry);
                        ProfilerNode *node = &parent_entry->profiler_node;
                        node->element = 0;
                        node->first_child = 0;
                        node->next_same_parent = 0;
                        node->parent_relative_clock = 0;
                        node->duration = 0;
                        node->duration_of_children = 0;
                        node->thread_index = 0;
                        node->core_index = 0;

                        clock_basis = gather_frame->begin_timestamp;
                        gather_frame->root_node = parent_entry;
                    }

                    ProfilerStoredEntry *stored_entry = profiler_store_entry(state, element, entry);
                    ProfilerNode *node = &stored_entry->profiler_node;
                    node->element = element;
                    node->first_child = 0;
                    node->parent_relative_clock = entry->timestamp - clock_basis;
                    node->duration = 0;
                    node->duration_of_children = 0;
                    node->thread_index = (u16)thread->index;
                    node->core_index = 0;

                    node->next_same_parent = parent_entry->profiler_node.first_child;
                    parent_entry->profiler_node.first_child = stored_entry;

                    ProfilerOpenBlock *block = profiler_allocate_open_block(state, element, frame_index, entry, &thread->first_open_block);
                    block->node = stored_entry;
                } break;

                case ProfilerEntryType_End:
                {
                    if (thread->first_open_block)
                    {
                        ProfilerOpenBlock *matching_block = thread->first_open_block;
                        assert(thread->id == entry->thread_id);

                        ProfilerNode *node = &matching_block->node->profiler_node;
                        node->duration = entry->timestamp - matching_block->begin_timestamp;

                        profiler_deallocate_open_block(state, &thread->first_open_block);

                        if (thread->first_open_block)
                        {
                            ProfilerNode *parent_node = &thread->first_open_block->node->profiler_node;
                            parent_node->duration_of_children += node->duration;
                        }
                    }
                } break;
            }
        }
    }
}

static void profiler_report(AppMemory *memory)
{
    PROFILER_FUNCTION();

    profiler->current_entry_array_index = !profiler->current_entry_array_index;
    u32 array_index__entry_index = atomic_exchange_u32(&profiler->entry_array_index__entry_index, profiler->current_entry_array_index << 31);
    u32 entry_array_index = array_index__entry_index >> 31;
    u32 num_entries = array_index__entry_index & 0xFFFFFFF;
    
    ProfilerState *state = memory->profiler_state;
    if (!state)
    {
        state = memory->profiler_state = bootstrap_push_struct(ProfilerState, profiler_memory);
        profiler_init(state);
    }

    if (memory->app_dll_reloaded)
    {
        num_entries = 0;
    }

    if (!state->paused)
    {
        state->display_frame_index = state->most_recent_frame_index;
    }

    profiler_gather(state, profiler->entries[entry_array_index], num_entries);

    profiler_draw_frame_history(state, 600, 100, 0.01f, 0.02f);
    profiler_draw_timelines(state, 400, 200);
    profiler_draw_clocks(state, 400, 200);
}
