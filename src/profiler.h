enum ProfilerEntryType
{
    ProfilerEntryType_Begin,
    ProfilerEntryType_End,
    ProfilerEntryType_FrameTime,
};

struct ProfilerEntry
{
    char *guid;
    u64 timestamp;
    u16 thread_id;
    u16 type;
    union
    {
        f32 frame_time;
    };
};

struct Profiler
{
    u32 current_entry_array_index;
    u32 volatile entry_array_index__entry_index;

    ProfilerEntry entries[2][4096];
};

extern Profiler *profiler;

#define PROFILER_GUID__(a, b, c)    a "|" #b "|" c
#define PROFILER_GUID_(a, b, c)     PROFILER_GUID__(a, b, c)
#define PROFILER_GUID(name)         PROFILER_GUID_(__FILE__, __LINE__, name)

#define PROFILER_PUSH_ENTRY(entry_guid, entry_type) \
    u32 array_index__entry_index = atomic_add_u32(&profiler->entry_array_index__entry_index, 1); \
    u32 entry_index = array_index__entry_index & 0xFFFFFFF; \
    \
    assert(entry_index < array_count(profiler->entries[0])); \
    \
    ProfilerEntry *entry = profiler->entries[array_index__entry_index >> 31] + entry_index; \
    entry->guid = entry_guid; \
    entry->timestamp = __rdtsc(); \
    entry->thread_id = (u16)get_thread_id(); \
    entry->type = (u16)entry_type;

#define PROFILER_FRAME_TIME(dt) \
    { \
        PROFILER_PUSH_ENTRY(PROFILER_GUID("FRAME_END"), ProfilerEntryType_FrameTime); \
        entry->frame_time = dt; \
    }

#define PROFILER_BLOCK_(guid)   ProfilerBlock profiler_block_##__LINE__(guid)

#define PROFILER_BEGIN_(guid)   if (profiler) { PROFILER_PUSH_ENTRY(guid, ProfilerEntryType_Begin) }
#define PROFILER_END_(guid)     if (profiler) { PROFILER_PUSH_ENTRY(guid, ProfilerEntryType_End) }

#if 1
    #define PROFILER_BEGIN(name)    { PROFILER_BEGIN_(PROFILER_GUID(name)); }
    #define PROFILER_END()          { PROFILER_END_(PROFILER_GUID("END_BLOCK")); }

    #define PROFILER_BLOCK(name)    PROFILER_BLOCK_(PROFILER_GUID(name))
    #define PROFILER_FUNCTION()     PROFILER_BLOCK_(__FUNCTION__)
#else
    #define PROFILER_BEGIN(...)
    #define PROFILER_END(...)

    #define PROFILER_BLOCK(...)
    #define PROFILER_FUNCTION(...)
#endif

struct ProfilerBlock
{
    ProfilerBlock(char *guid)
    {
        PROFILER_BEGIN_(guid);
    }

    ~ProfilerBlock()
    {
        PROFILER_END_(PROFILER_GUID("END_BLOCK"));
    }
};
