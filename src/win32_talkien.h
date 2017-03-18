typedef intptr (__stdcall *Win32Api_WNDPROC)(void *wnd, unsigned int msg, uintptr wparam, intptr lparam);
typedef intptr (__stdcall *Win32Api_PROC)();

typedef Win32Api_PROC (__stdcall Win32GetProcAddress)(void *module, char *proc);
typedef void * (__stdcall Win32LoadLibrary)(char *lib);
typedef void * (__stdcall Win32Api_FreeLibrary)(void *module);

#if 1
    extern "C" __declspec(dllimport) Win32Api_PROC __stdcall GetProcAddress(void *module, char *proc);
    extern "C" __declspec(dllimport) void * __stdcall LoadLibraryA(char *lib);
    extern "C" __declspec(dllimport) int __stdcall FreeLibrary(void *module); 

    #define win32_get_proc_address GetProcAddress
    #define win32_load_library LoadLibraryA
    #define win32_free_library FreeLibrary

    typedef void *HINSTANCE;
#else
    #include <windows.h>

    static Win32GetProcAddress *win32_get_proc_address = (Win32GetProcAddress *)GetProcAddress;
    static Win32LoadLibrary *win32_load_library = (Win32LoadLibrary *)LoadLibraryA;
    static Win32Api_FreeLibrary *win32_free_library = (Win32Api_FreeLibrary *)FreeLibrary;
#endif

typedef void * (__stdcall *Win32Api_wglCreateContextAttribsARB)(void *dc, void *rc, int *attribs);
typedef int (__stdcall * Win32Api_wglChoosePixelFormatARB)(void *, int *, float *, unsigned int, int *, unsigned int *);
typedef int (__stdcall * Win32Api_wglSwapIntervalEXT)(int);

struct Win32Api_WNDCLASSEXA
{
    unsigned int cbSize;
    unsigned int style;
    Win32Api_WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    void *hInstance;
    void *hIcon;
    void *hCursor;
    void *hBrush;
    char *lpszMenuName;
    char *lpszClassName;
    void *hIconSm;
};

struct Win32Api_PIXELFORMATDESCRIPTOR
{
    unsigned short nSize;
    unsigned short nVersion;
    unsigned int dwFlags;
    unsigned char iPixelType;
    unsigned char cColorBits;
    unsigned char cRedBits;
    unsigned char cRedShift;
    unsigned char cGreenBits;
    unsigned char cGreenShift;
    unsigned char cBlueBits;
    unsigned char cBlueShift;
    unsigned char cAlphaBits;
    unsigned char cAlphaShift;
    unsigned char cAccumBits;
    unsigned char cAccumRedBits;
    unsigned char cAccumGreenBits;
    unsigned char cAccumBlueBits;
    unsigned char cAccumAlphaBits;
    unsigned char cDepthBits;
    unsigned char cStencilBits;
    unsigned char cAuxBuffers;
    unsigned char iLayerType;
    unsigned char bReserved;
    unsigned int dwLayerMask;
    unsigned int dwVisibleMask;
    unsigned int dwDamageMask;
};

struct Win32Api_RECT
{
    int left;
    int top;
    int right;
    int bottom;    
};

struct Win32Api_POINT
{
    int x;
    int y;
};

struct Win32Api_MSG
{
    void *wnd;
    unsigned int message;
    uintptr wParam;
    intptr lParam;
    unsigned int time;
    Win32Api_POINT pt;
};

struct Win32Api_FILETIME
{
    unsigned int dwLowDateTime;
    unsigned int dwHighDateTime;
};

union Win32Api_LARGE_INTEGER
{
    struct
    {
        unsigned int LowPart;
        unsigned int HighPart;
    };
    struct
    {
        unsigned int LowPart;
        int HighPart;
    } u;
    i64 QuadPart;
};

enum Win32Api_GET_FILEEX_INFO_LEVELS
{
    Win32Api_GetFileExInfoStandard,
    Win32Api_GetFileExMaxInfoLevel,
};

struct Win32Api_WIN32_FILE_ATTRIBUTE_DATA
{
    unsigned int dwFileAttributes;
    Win32Api_FILETIME ftCreationTime;
    Win32Api_FILETIME ftLastAccessTime;
    Win32Api_FILETIME ftLastWriteTime;
    unsigned int nFileSizeHigh;
    unsigned int nFileSizeLow;
};

#define WIN32_API_FUNCTION_LIST \
    WIN32_API(kernel32, CompareFileTime, int __stdcall, (Win32Api_FILETIME *filetime1, Win32Api_FILETIME *filetime2)) \
    WIN32_API(kernel32, CopyFileA, int __stdcall, (char *filename, char *new_filename, int fail_if_exists)) \
    WIN32_API(kernel32, GetFileAttributesExA, int __stdcall, (char *filename, Win32Api_GET_FILEEX_INFO_LEVELS info_level_id, void *file_info)) \
    WIN32_API(kernel32, GetModuleFileNameA, unsigned int __stdcall, (void *module, char *filename, unsigned int size)) \
    WIN32_API(kernel32, GetModuleHandleA, void * __stdcall, (char *module)) \
    WIN32_API(kernel32, QueryPerformanceCounter, int __stdcall, (Win32Api_LARGE_INTEGER *perf_count)) \
    WIN32_API(kernel32, QueryPerformanceFrequency, int __stdcall, (Win32Api_LARGE_INTEGER *freq)) \
    WIN32_API(kernel32, VirtualAlloc, void * __stdcall, (void *addr, usize size, unsigned int alloc_type, unsigned int protect)) \
    WIN32_API(kernel32, VirtualFree, int __stdcall, (void *addr, usize size, usize free_type)) \
    \
    WIN32_API(gdi32, ChoosePixelFormat, int __stdcall, (void *dc, Win32Api_PIXELFORMATDESCRIPTOR *pfd)) \
    WIN32_API(gdi32, DescribePixelFormat, int __stdcall, (void *dc, int pixel_format, unsigned int bytes, Win32Api_PIXELFORMATDESCRIPTOR *pfd)) \
    WIN32_API(gdi32, SetPixelFormat, int __stdcall, (void *dc, int format, Win32Api_PIXELFORMATDESCRIPTOR *pfd)) \
    WIN32_API(gdi32, SwapBuffers, int __stdcall, (void *dc)) \
    \
    WIN32_API(opengl32, wglCreateContext, void * __stdcall, (void *dc)) \
    WIN32_API(opengl32, wglDeleteContext, int __stdcall, (void *rc)) \
    WIN32_API(opengl32, wglGetProcAddress, Win32Api_PROC, (char *proc)) \
    WIN32_API(opengl32, wglMakeCurrent, int __stdcall, (void *dc, void *rc)) \
    \
    WIN32_API(user32, CreateWindowExA, void * __stdcall, (unsigned int ex_style, char *class_name, char *window_name, unsigned int style, int x, int y, int width, int height, void *parent, void *menu, void *instance, void *param)) \
    WIN32_API(user32, DefWindowProcA, intptr __stdcall, (void *wnd, unsigned int msg, uintptr wparam, intptr lparam)) \
    WIN32_API(user32, DestroyWindow, int __stdcall, (void *wnd)) \
    WIN32_API(user32, DispatchMessageA, intptr __stdcall, (Win32Api_MSG *msg)) \
    WIN32_API(user32, GetClientRect, int __stdcall, (void *wnd, Win32Api_RECT *rect)) \
    WIN32_API(user32, GetCursorPos, int __stdcall, (Win32Api_POINT *point)) \
    WIN32_API(user32, GetDC, void * __stdcall, (void *wnd)) \
    WIN32_API(user32, GetKeyState, short __stdcall, (int virtual_key)) \
    WIN32_API(user32, LoadCursorA, void * __stdcall, (void *instance, char *cursor_name)) \
    WIN32_API(user32, PeekMessageA, int __stdcall, (Win32Api_MSG *msg, void *wnd, unsigned int msg_filter_min, unsigned int msg_filter_max, unsigned int remove_msg)) \
    WIN32_API(user32, RegisterClassExA, unsigned short __stdcall, (Win32Api_WNDCLASSEXA *)) \
    WIN32_API(user32, ReleaseDC, int __stdcall, (void *wnd, void *dc)) \
    WIN32_API(user32, ScreenToClient, int __stdcall, (void *wnd, Win32Api_POINT *point)) \
    WIN32_API(user32, TranslateMessage, int __stdcall, (Win32Api_MSG *msg))

#define WIN32_API(dll, name, return_type, params) typedef return_type Win32Api_##name params;
WIN32_API_FUNCTION_LIST
#undef WIN32_API

struct Win32Api
{
    #define WIN32_API(dll, name, return_type, params) Win32Api_##name *name;
    WIN32_API_FUNCTION_LIST
    #undef WIN32_API

    Win32Api_wglChoosePixelFormatARB wglChoosePixelFormatARB;
    Win32Api_wglCreateContextAttribsARB wglCreateContextAttribsARB;
    Win32Api_wglSwapIntervalEXT wglSwapIntervalEXT;
};

static void win32_init_win32_api(Win32Api *win32_api)
{
    void *user32 = win32_load_library("user32.dll");
    void *gdi32 = win32_load_library("gdi32.dll");
    void *opengl32 = win32_load_library("opengl32.dll");
    void *kernel32 = win32_load_library("kernel32.dll");
    
    #define WIN32_API(dll, name, return_type, params) win32_api->name = (Win32Api_##name *)win32_get_proc_address(dll, #name);
    WIN32_API_FUNCTION_LIST
    #undef WIN32_API
}

struct Win32Window
{
    void *wnd;
    void *dc;
    void *rc;
    i32 width;
    i32 height;
    b32 initialized;
};

struct Win32MemoryBlock
{
    PlatformMemoryBlock memblock;
    Win32MemoryBlock *prev;
    Win32MemoryBlock *next;
    u64 flags;
};

#define MAX_FILENAME_SIZE 260

struct Win32State
{
    b32 running;
    Win32Window window;

    AppMemory app_memory;


    Mutex memory_mutex;
    Win32MemoryBlock memory_sentinel;

    u32 exe_filename_length;
    u32 exe_path_length;
    char exe_filename[MAX_FILENAME_SIZE];
    char app_dll_filename[MAX_FILENAME_SIZE];
    char app_dll_lock_filename[MAX_FILENAME_SIZE];
    char app_temp_dll_filename[MAX_FILENAME_SIZE];

    void *app_dll;
    b32 app_dll_reloaded;
    Win32Api_FILETIME app_dll_last_write;
    UpdateAndRender *update_and_render;
};
