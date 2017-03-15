
#define PLATFORM_UNDEFINED  0
#define PLATFORM_WINDOWS    1
#define PLATFORM_UNIX       2
#define PLATFORM_OSX        3

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM    PLATFORM_WINDOWS
#elif defined(__unix__)
    #define PLATFORM    PLATFORM_UNIX
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM    PLATFORM_OSX
#else
    #define PLATFORM    PLATFORM_UNDEFINED
#endif

#define COMPILER_UNDEFINED  0
#define COMPILER_MSVC       1
#define COMPILER_GCC        2
#define COMPILER_CLANG      3

#if defined(_MSC_VER)
    #define COMPILER    COMPILER_MSVC
#elif defined(__GNUC__)
    #define COMPILER    COMPILER_GCC
#elif defined(__clang__)
    #define COMPILER    COMPILER_CLANG
#else
    #define COMPILER    COMPILER_UNDEFINED
#endif

#define ARCH_32_BIT    0
#define ARCH_64_BIT    1

#if defined(_WIN64) || defined(__x86_64__) || defined(__ia64__) || defined(__LP64__)
    #define ARCH    ARCH_64_BIT
#else
    #define ARCH    ARCH_32_BIT
#endif

typedef unsigned char    u8;
typedef   signed char    i8;
typedef unsigned short  u16;
typedef   signed short  i16;
typedef unsigned int    u32;
typedef   signed int    i32;

#if COMPILER == COMPILER_MSVC
    typedef unsigned __int64    u64;
    typedef          __int64    i64;
#else
    typedef unsigned long long  u64;
    typedef          long long  i64;
#endif

typedef u8             uchar;
typedef u16            ushort;
typedef u32            uint;
typedef unsigned long  ulong;

typedef i32    b32;
typedef float  f32;
typedef double f64;

#if ARCH == ARCH_64_BIT
    typedef u64     usize;
    typedef i64     isize;
    typedef u64     uintptr;
    typedef i64     intptr;
    typedef u64     ulongptr;
    typedef i64     longptr;
#else
    typedef u32     usize;
    typedef i32     isize;
    typedef u32     uintptr;
    typedef i32     intptr;
    typedef ulong   ulongptr;
    typedef long    longptr;
#endif

typedef char test_size_u8[sizeof(u8)   == 1 ? 1 : -1];
typedef char test_size_u16[sizeof(u16) == 2 ? 1 : -1];
typedef char test_size_u32[sizeof(u32) == 4 ? 1 : -1];
typedef char test_size_u64[sizeof(u64) == 8 ? 1 : -1];
typedef char test_size_usize[sizeof(usize) == sizeof(char *) ? 1 : -1];

#if INTERNAL_BUILD
    #if COMPILER == COMPILER_MSVC
        // TODO(dan): message box?
        #define assert(e) if (!(e)) { __debugbreak(); }
    #else
        #define assert(e) if (!(e)) { *(i32 *)0 = 0; }
    #endif
#else
    #define assert(e)
#endif
#define assert_always(...)      assert(!"Invalid codepath! "__VA_ARGS__)
#define invalid_default_case    default: { assert_always(); } break

#define array_count(a)              (sizeof(a) / sizeof((a)[0]))
#define offset_of(type, element)    ((usize)&(((type *)0)->element))
#define swap_values(type, a, b) do { type temp__ = (a); (a) = (b); (b) = temp__; } while (0)

#define max(a, b) ((a) > (b)) ? (a) : (b)

#define KB  (1024LL)
#define MB  (1024LL * KB)
#define GB  (1024LL * MB)
#define TB  (1024LL * GB)

#if COMPILER == COMPILER_MSVC
    #include <intrin.h>
   
    inline u64 atomic_add_u64(u64 volatile *value, u64 addend)
    {
        // NOTE(dan): result = current value without addend
        u64 result = _InterlockedExchangeAdd64((__int64 volatile *)value, addend);
        return result;
    }
#endif

enum
{
    button_tab,
    button_left,
    button_right,
    button_up,
    button_down,
    button_pageup,
    button_pagedown,
    button_home,
    button_end,
    button_delete,
    button_backspace,
    button_enter,
    button_esc,
    button_a,
    button_c,
    button_v,
    button_x,
    button_y,
    button_z,

    button_count,
};

enum
{
    mouse_button_left,
    mouse_button_middle,
    mouse_button_right,
    mouse_button_extended0,
    mouse_button_extended1,

    mouse_button_count,
};

struct PlatformButtonState
{
    u32 transitions;
    b32 is_down;
};

struct PlatformInput
{
    f32 dt;
    b32 quit_requested;

    f32 mouse_x;
    f32 mouse_y;
    f32 mouse_z;

    b32 shift_down;
    b32 ctrl_down;
    b32 alt_down;

    PlatformButtonState buttons[button_count];
    PlatformButtonState mouse_buttons[mouse_button_count];

    u32 character_count;
    char characters[16];
};

inline b32 was_pressed(PlatformButtonState state)
{
    b32 result = ((state.transitions > 1) || ((state.transitions == 1) && (state.is_down)));
    return result;
}

struct PlatformMemoryBlock
{
    usize size;
    usize used;
    u8 *base;
    u64 flags;
    PlatformMemoryBlock *prev;
};

struct PlatformMemoryStats
{
    usize num_memblocks;
    usize total_size;
    usize total_used;
};

#define PLATFORM_ALLOCATE(name)    PlatformMemoryBlock *name(usize size)
#define PLATFORM_DEALLOCATE(name)  void name(PlatformMemoryBlock *memblock)
#define PLATFORM_GET_MEMORY_STATS(name) PlatformMemoryStats name()

typedef PLATFORM_ALLOCATE(PlatformAllocate);
typedef PLATFORM_DEALLOCATE(PlatformDeallocate);
typedef PLATFORM_GET_MEMORY_STATS(PlatformGetMemoryStats);

struct Platform
{
    PlatformAllocate *allocate;
    PlatformDeallocate *deallocate;
    PlatformGetMemoryStats *get_memory_stats;
};

extern Platform platform;

struct AppMemory
{
    struct AppState *app_state;

    Platform platform;
};

struct Mutex
{
    u64 volatile ticket;
    u64 volatile serving;
};

inline void begin_mutex(Mutex *mutex)
{
    u64 ticket = atomic_add_u64(&mutex->ticket, 1);
    while (ticket != mutex->serving)
    {
        _mm_pause();
    }
}

inline void end_mutex(Mutex *mutex)
{
    atomic_add_u64(&mutex->serving, 1);
}

inline void zero_size(void *dest, usize size)
{
    u8 *byte = (u8 *)dest;
    while (size--)
    {
        *byte++ = 0;
    }
}

struct MemoryStack
{
    PlatformMemoryBlock *memblock;
    usize min_stack_size;
    u64 flags;
    u64 temp_stacks;
};

struct TempMemoryStack
{
    PlatformMemoryBlock *memblock;
    MemoryStack *memstack;
    usize used;
};

struct MemoryStackParams
{
    b32 clear_to_zero;
    u32 alignment;
};

#define DEFAULT_MEMORY_STACK_ALINGMENT  4
#define DEFAULT_MEMORY_STACK_SIZE       1*MB

inline MemoryStackParams default_params()               { return {true,  DEFAULT_MEMORY_STACK_ALINGMENT}; }
inline MemoryStackParams no_clear()                     { return {false, DEFAULT_MEMORY_STACK_ALINGMENT}; }
inline MemoryStackParams align_no_clear(u32 alignment)  { return {false, alignment}; }
inline MemoryStackParams align_clear(u32 alignment)     { return {true,  alignment}; }

inline usize get_next_memory_stack_offset(MemoryStack *memstack, usize alignment)
{
    usize result = 0;
    usize mask = alignment - 1;
    usize next_base = (usize)memstack->memblock->base + memstack->memblock->used;
    if (next_base & mask)
    {
        result = alignment - (next_base & mask);
    }
    return result;
}

#define push_struct(memstack, type, ...)        (type *)push_size(memstack, sizeof(type), ## __VA_ARGS__)
#define push_array(memstack, count, type, ...)  (type *)push_size(memstack, (count) * sizeof(type), ## __VA_ARGS__)

inline void *push_size(MemoryStack *memstack, usize size, MemoryStackParams params = default_params())
{
    usize effective_size = 0;
    if (memstack->memblock)
    {
        usize offset = get_next_memory_stack_offset(memstack, params.alignment);
        effective_size = size + offset;
    }

    if (!memstack->memblock || ((memstack->memblock->used + effective_size) > memstack->memblock->size))
    {
        if (!memstack->min_stack_size)
        {
            memstack->min_stack_size = DEFAULT_MEMORY_STACK_SIZE;
        }

        effective_size = size;
        usize stack_size = max(effective_size, memstack->min_stack_size);

        PlatformMemoryBlock *memblock = platform.allocate(stack_size);
        memblock->prev = memstack->memblock;
        memstack->memblock = memblock;
    }

    assert((memstack->memblock->used + effective_size) <= memstack->memblock->size);

    usize offset = get_next_memory_stack_offset(memstack, params.alignment);
    void *result = memstack->memblock->base + memstack->memblock->used + offset;
    memstack->memblock->used += effective_size;

    if (params.clear_to_zero)
    {
        zero_size(result, size);
    }

    return result;
}

#define bootstrap_push_struct(type, member, ...) (type *)bootstrap_push_size(sizeof(type), offset_of(type, member), ## __VA_ARGS__)

inline void *bootstrap_push_size(usize size, usize offset, MemoryStackParams params = default_params())
{
    MemoryStack bootstrap = {};
    void *result = push_size(&bootstrap, size, params);

    *(MemoryStack *)((u8 *)result + offset) = bootstrap;
    
    return result;
}

inline TempMemoryStack begin_temp_memory(MemoryStack *memstack)
{
    TempMemoryStack result = {0};
    result.memstack = memstack;
    result.memblock = memstack->memblock;
    result.used = memstack->memblock ? memstack->memblock->used : 0;

    ++memstack->temp_stacks;
    return result;
}

inline void free_last_memory_block(MemoryStack *memstack)
{
    PlatformMemoryBlock *memblock = memstack->memblock;
    memstack->memblock = memblock->prev;
    platform.deallocate(memblock);
}

inline void end_temp_memory(TempMemoryStack tempmem)
{
    MemoryStack *memstack = tempmem.memstack;
    while (memstack->memblock != tempmem.memblock)
    {
        free_last_memory_block(memstack);
    }

    if (memstack->memblock)
    {
        assert(memstack->memblock->used >= tempmem.used);
        
        memstack->memblock->used = tempmem.used;
    }

    assert(memstack->temp_stacks > 0);
    --memstack->temp_stacks;
}

inline void check_memory_stack(MemoryStack *memstack)
{
    assert(memstack->temp_stacks == 0);
}

#define UPDATE_AND_RENDER(name) void name(AppMemory *memory, PlatformInput *input, i32 window_width, i32 window_height)
typedef UPDATE_AND_RENDER(UpdateAndRender);
