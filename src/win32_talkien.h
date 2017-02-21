// TODO(dan): replace windows.h
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#include "windows.h"

#define WGL_DRAW_TO_WINDOW_ARB      0x2001
#define WGL_ACCELERATION_ARB        0x2003
#define WGL_SUPPORT_OPENGL_ARB      0x2010
#define WGL_DOUBLE_BUFFER_ARB       0x2011
#define WGL_PIXEL_TYPE_ARB          0x2013
#define WGL_COLOR_BITS_ARB          0x2014
#define WGL_DEPTH_BITS_ARB          0x2022
#define WGL_STENCIL_BITS_ARB        0x2023
#define WGL_TYPE_RGBA_ARB           0x202B
#define WGL_FULL_ACCELERATION_ARB   0x2027

#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB  0x20A9

#define WGL_CONTEXT_MAJOR_VERSION_ARB   0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB   0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB     0x2093
#define WGL_CONTEXT_FLAGS_ARB           0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB    0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB                   0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB      0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB            0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB   0x00000002

#define GL_GET_FUNC(func) wglGetProcAddress(func)

typedef HGLRC (__stdcall * PFNWGLCREATECONTEXTATTRIBSARBPROC)   (HDC, HGLRC, int *);
typedef int   (__stdcall * PFNWGLCHOOSEPIXELFORMATARBPROC)      (HDC, int *, float *, uint , int *, uint *);
typedef int   (__stdcall * PFNWGLSWAPINTERVALEXTPROC)           (int);

#define WGL_EXTENSIONS_LIST \
    WGLARB(CHOOSEPIXELFORMAT,    ChoosePixelFormat) \
    WGLARB(CREATECONTEXTATTRIBS, CreateContextAttribs) \
    WGLEXT(SWAPINTERVAL,         SwapInterval) 

#define WGLARB(a, b) WGLE(a##ARB, b##ARB)
#define WGLEXT(a, b) WGLE(a##EXT, b##EXT)

#define WGLE(a, b) static PFNWGL##a##PROC wgl##b;
WGL_EXTENSIONS_LIST
#undef WGLE

inline void wgl_init_extensions()
{
    #define WGLE(a, b) wgl##b = (PFNWGL##a##PROC)GL_GET_FUNC("wgl" #b);
    WGL_EXTENSIONS_LIST
    #undef WGLE
}

#include "opengl.h"

struct Win32Window
{
    HWND wnd;
    HDC dc;
    HGLRC rc;
    i32 width;
    i32 height;
    b32 initialized;
};

inline f32 win32_get_time()
{
    static i64 start = 0;
    static i64 freq = 0;
    f32 t = 0.0;

    if (!start)
    {
        QueryPerformanceCounter((LARGE_INTEGER *)&start);
        QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
    }
    else
    {
        i64 counter = 0;
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        t = (f32)((counter - start) / (f64)freq);
    }

    return t;
}
