
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
    extern "C" __int64 _InterlockedExchangeAdd64(__int64 volatile *addend, __int64 value);
    extern "C" long _InterlockedExchange(long volatile *target, long value);
    extern "C" long _InterlockedCompareExchange(long volatile *destination, long exchange, long comparand); 
    extern "C" void _mm_pause();

    extern "C" void *memset(void *target, i32 value, usize target_size);

    inline u64 atomic_add_u64(u64 volatile *value, u64 addend)
    {
        // NOTE(dan): result = current value without addend
        u64 result = _InterlockedExchangeAdd64((__int64 volatile *)value, addend);
        return result;
    }

    inline u32 atomic_exchange_u32(u32 volatile *target, u32 value)
    {
        u32 result = _InterlockedExchange((long volatile *)target, value);
        return result;
    }

    inline u32 atomic_cmpxchg_u32(u32 volatile *value, u32 volatile new_value, u32 expected)
    {
        u32 result = _InterlockedCompareExchange((long volatile *)value, new_value, expected);
        return result;
    }
#endif

enum PlatformButtonScanCode
{
    button_escape =             0x01,      button_1 =                 0x02,        button_2 =               0x03,      button_3 =                0x04,
    button_4 =                  0x05,      button_5 =                 0x06,        button_6 =               0x07,      button_7 =                0x08,
    button_8 =                  0x09,      button_9 =                 0x0A,        button_0 =               0x0B,      button_minus =            0x0C,
    button_equals =             0x0D,      button_backspace =         0x0E,        button_tab =             0x0F,      button_q =                0x10,
    button_w =                  0x11,      button_e =                 0x12,        button_r =               0x13,      button_t =                0x14,
    button_y =                  0x15,      button_u =                 0x16,        button_i =               0x17,      button_o =                0x18,
    button_p =                  0x19,      button_bracket_left =      0x1A,        button_bracket_right =   0x1B,      button_enter =            0x1C,
    button_control_left =       0x1D,      button_a =                 0x1E,        button_s =               0x1F,      button_d =                0x20,
    button_f =                  0x21,      button_g =                 0x22,        button_h =               0x23,      button_j =                0x24,
    button_k =                  0x25,      button_l =                 0x26,        button_semicolon =       0x27,      button_apostrophe =       0x28,
    button_grave =              0x29,      button_shift_left =        0x2A,        button_backslash =       0x2B,      button_z =                0x2C,
    button_x =                  0x2D,      button_c =                 0x2E,        button_v =               0x2F,      button_b =                0x30,
    button_n =                  0x31,      button_m =                 0x32,        button_comma =           0x33,      button_preiod =           0x34,
    button_slash =              0x35,      button_shift_right =       0x36,        button_numpad_multiply = 0x37,      button_alt_left =         0x38,
    button_space =              0x39,      button_caps_lock =         0x3A,        button_f1 =              0x3B,      button_f2 =               0x3C,
    button_f3 =                 0x3D,      button_f4 =                0x3E,        button_f5 =              0x3F,      button_f6 =               0x40,
    button_f7 =                 0x41,      button_f8 =                0x42,        button_f9 =              0x43,      button_f10 =              0x44,
    button_num_lock =           0x45,      button_scroll_lock =       0x46,        button_numpad_7 =        0x47,      button_numpad_8 =         0x48,
    button_numpad_9 =           0x49,      button_numpad_minus =      0x4A,        button_numpad_4 =        0x4B,      button_numpad_5 =         0x4C,
    button_numpad_6 =           0x4D,      button_numpad_plus =       0x4E,        button_numpad_1 =        0x4F,      button_numpad_2 =         0x50,
    button_numpad_3 =           0x51,      button_numpad_0 =          0x52,        button_numpad_period =   0x53,      button_alt_print_screen = 0x54,
    button_bracket_angle =      0x56,      button_f11 =               0x57,        button_f12 =             0x58,      button_oem_1 =            0x5a,
    button_oem_2 =              0x5b,      button_oem_3 =             0x5c,        button_erase_EOF =       0x5d,      button_oem_4 =            0x5e,
    button_oem_5 =              0x5f,      button_zoom =              0x62,        button_help =            0x63,      button_f13 =              0x64,
    button_f14 =                0x65,      button_f15 =               0x66,        button_f16 =             0x67,      button_f17 =              0x68,
    button_f18 =                0x69,      button_f19 =               0x6a,        button_f20 =             0x6b,      button_f21 =              0x6c,
    button_f22 =                0x6d,      button_f23 =               0x6e,        button_oem_6 =           0x6f,      button_katakana =         0x70,
    button_oem_7 =              0x71,      button_f24 =               0x76,        button_sbcschar =        0x77,      button_convert =          0x79,
    button_nonconvert =         0x7B,      
    
    button_media_previous =     0xE010,    button_media_next =        0xE019,      button_numpad_enter =    0xE01C,    button_control_right =    0xE01D,    
    button_volume_mute =        0xE020,    button_launch_app2 =       0xE021,      button_media_play =      0xE022,    button_media_stop =       0xE024,    
    button_volume_down =        0xE02E,    button_volume_up =         0xE030,      button_browser_home =    0xE032,    button_numpad_divide =    0xE035,
    button_print_screen =       0xE037,    button_alt_right =         0xE038,      button_cancel =          0xE046,    button_home =             0xE047,    
    button_arrow_up =           0xE048,    button_page_up =           0xE049,      button_arrow_left =      0xE04B,    button_arrow_right =      0xE04D,    
    button_end =                0xE04F,    button_arrow_down =        0xE050,      button_page_down =       0xE051,    button_insert =           0xE052,    
    button_delete =             0xE053,    button_meta_left =         0xE05B,      button_meta_right =      0xE05C,    button_application =      0xE05D,
    button_power =              0xE05E,    button_sleep =             0xE05F,      button_wake =            0xE063,    button_browser_search =   0xE065,
    button_browser_favorites =  0xE066,    button_browser_refresh =   0xE067,      button_browser_stop =    0xE068,    button_browser_forward =  0xE069,
    button_browser_back =       0xE06A,    button_launch_app1 =      0xE06B,       button_launch_email =    0xE06C,    button_launch_media =     0xE06D,    

    button_pause =              0xE11D45,
};

inline u32 get_scan_code_offset(u32 scan_code)
{
    u32 offset = scan_code;
    if (scan_code >= button_pause)
    {
        offset = button_nonconvert + 1 + (button_launch_media - button_media_previous) + 1 + (scan_code - button_pause);
    }
    else if (scan_code >= button_media_previous)
    {
        offset = button_nonconvert + 1 + (scan_code - button_media_previous);
    }
    assert(offset <= 0xFF);
    return offset;
}

enum
{
    mouse_button_left,
    mouse_button_middle,
    mouse_button_right,
    mouse_button_extended0,
    mouse_button_extended1,

    mouse_button_count,
};

struct PlatformButton
{
    b32 down;
    b32 pressed;
    b32 released;
};

struct PlatformInput
{
    f32 dt;
    b32 quit_requested;

    i32 wheel;
    i32 delta_wheel;
    int mouse_pos[2];
    int delta_mouse_pos[2];

    u32 text_input_length;
    u16 text_input[16];

    PlatformButton buttons[256];
    PlatformButton mouse_buttons[mouse_button_count];
};

inline b32 is_down(PlatformInput *input, u32 scan_code)
{
    u32 offset = get_scan_code_offset(scan_code);
    b32 is_down = input->buttons[offset].down;
    return is_down;
}

inline b32 is_pressed(PlatformInput *input, u32 scan_code)
{
    u32 offset = get_scan_code_offset(scan_code);
    b32 is_pressed = input->buttons[offset].pressed;
    return is_pressed;
}

inline b32 is_released(PlatformInput *input, u32 scan_code)
{
    u32 offset = get_scan_code_offset(scan_code);
    b32 is_released = input->buttons[offset].released;
    return is_released;
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
#define PLATFORM_INIT_OPENGL(name) void name(struct OpenGL *open_gl)

typedef PLATFORM_ALLOCATE(PlatformAllocate);
typedef PLATFORM_DEALLOCATE(PlatformDeallocate);
typedef PLATFORM_GET_MEMORY_STATS(PlatformGetMemoryStats);
typedef PLATFORM_INIT_OPENGL(PlatformInitOpenGL);

struct Platform
{
    PlatformAllocate *allocate;
    PlatformDeallocate *deallocate;
    PlatformGetMemoryStats *get_memory_stats;
    PlatformInitOpenGL *init_opengl;
};

extern Platform platform;

struct AppMemory
{
    struct AppState *app_state;
    struct AudioState *audio_state;
    b32 app_dll_reloaded;

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
static UPDATE_AND_RENDER(update_and_render_stub)
{
}

#define FILL_SOUND_BUFFER(name) void name(AppMemory *memory, f32 *buffer, u32 num_samples)
typedef FILL_SOUND_BUFFER(FillSoundBuffer);
static FILL_SOUND_BUFFER(fill_sound_buffer_stub)
{
    for (u32 sample_index = 0; sample_index < num_samples; ++sample_index)
    {
        buffer[sample_index] = 0;
    }
}
