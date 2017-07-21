#define PROFILER_NUM_FRAME_HISTORY  64

struct ProfilerParsedName
{
    u32 hash_value;
    u32 file_name_length;
    u32 line_number;

    u32 name_length;
    char *name;
};

struct ProfilerLink
{
    ProfilerLink *next;
    ProfilerLink *prev;
    ProfilerLink *first_child;
    ProfilerLink *last_child;

    struct ProfilerElement *element;
};

inline ProfilerLink *profiler_get_sentinel(ProfilerLink *from)
{
    ProfilerLink *link = (ProfilerLink *)&from->first_child;
    return link;
}

struct ProfilerNode
{
    struct ProfilerElement *element;
    struct ProfilerStoredEntry *first_child;
    struct ProfilerStoredEntry *next_same_parent;
    u64 duration;
    u64 duration_of_children;
    u64 parent_relative_clock;
    u32 reserved;
    u16 thread_index;
    u16 core_index;
};

struct ProfilerStoredEntry
{
    union
    {
        ProfilerStoredEntry *next;
        ProfilerStoredEntry *next_free;
    };

    u32 frame_index;

    union
    {
        ProfilerEntry entry;
        ProfilerNode profiler_node;
    };
};

struct ProfilerElementFrame
{
    ProfilerStoredEntry *oldest_entry;
    ProfilerStoredEntry *most_recent_entry;
};

struct ProfilerElement
{
    // char *original_guid;
    char *guid;
    char *name;
    u32 file_name_length;
    u32 line_number;

    ProfilerEntryType type;
    ProfilerElementFrame frames[PROFILER_NUM_FRAME_HISTORY];
    ProfilerElement *next_in_hash;
};

struct ProfilerFrame
{
    u64 begin_timestamp;
    u64 end_timestamp;
    f32 frame_time;

    u32 frame_index;

    u32 num_stored_entries;
    u32 num_profile_blocks;

    ProfilerStoredEntry *root_node;
};

struct ProfilerOpenBlock
{
    union
    {
        ProfilerOpenBlock *parent;
        ProfilerOpenBlock *next_free;
    };

    u32 starting_frame_index;
    ProfilerElement *element;
    u64 begin_timestamp;
    ProfilerStoredEntry *node;
};

struct ProfilerThread
{
    union
    {
        ProfilerThread *next;
        ProfilerThread *next_free;
    };

    u32 id;
    u32 index;

    ProfilerOpenBlock *first_open_block;
};

struct ProfilerState
{
    MemoryStack profiler_memory;

    b32 paused;
    i32 pause_condition_ms;

    u32 total_frame_count;
    u32 display_frame_index;
    u32 most_recent_frame_index;
    u32 gather_frame_index;
    u32 oldest_frame_index;
    u32 num_threads;

    ProfilerFrame frames[PROFILER_NUM_FRAME_HISTORY];
    ProfilerLink *profiler_group;
    ProfilerThread *first_thread;
    ProfilerThread *first_free_thread;
    ProfilerElement *root_element;
    ProfilerElement *element_hash[1024];

    ProfilerOpenBlock *first_free_block;
    ProfilerStoredEntry *first_free_stored_entry;
};
