
enum ProfilerEventType
{
    ProfilerEventType_BeginBlock,
    ProfilerEventType_EndBlock,
    ProfilerEventType_FrameEnd,
};

struct ProfilerEvent
{
    char *name;
    u64 clock;
    u16 thread_id;
    u16 type;

    union
    {
        f32 value_f32;
    };
};

struct Profiler
{
    u32 volatile event_count;
    ProfilerEvent events[4096];
};

extern Profiler *profiler;

#define PROFILER_PUSH_EVENT(event_name, event_type) \
    u32 event_index = atomic_add_u32(&profiler->event_count, 1); \
    assert(event_index < array_count(profiler->events)); \
    ProfilerEvent *event = profiler->events + event_index; \
    event->name = event_name; \
    event->clock = __rdtsc(); \
    event->thread_id = (u16)get_thread_id(); \
    event->type = (u16)event_type;

#define PROFILER_BEGIN_(name)       PROFILER_PUSH_EVENT(name, ProfilerEventType_BeginBlock)
#define PROFILER_END_()             PROFILER_PUSH_EVENT("PROFILER_END", ProfilerEventType_EndBlock)
#define PROFILER_BEGIN(name)        { PROFILER_BEGIN_(#name); }
#define PROFILER_END()              { PROFILER_END_(); }

#define PROFILER_BLOCK(name)    ProfilerBlock profiler_block_##name(#name)
#define PROFILER_FUNCTION()     ProfilerBlock profiler_block_##__FUNCTION__(#__FUNCTION__)

#define PROFILER_FRAME_END(dt)    { PROFILER_PUSH_EVENT("FRAME_MARKER", ProfilerEventType_FrameEnd); event->value_f32 = dt; }

struct ProfilerBlock
{
    ProfilerBlock(char *name)
    {
        PROFILER_BEGIN_(name);
    }

    ~ProfilerBlock()
    {
        PROFILER_END_();
    }
};
