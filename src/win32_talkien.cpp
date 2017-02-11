#define TIMESTEP_HZ     60
#define TIMESTEP_SEC    (1.0f / TIMESTEP_HZ)

#include "platform.h"
#include "win32_talkien.h"

static Win32Window global_window;
static b32 global_running;

#define win32_open_window(title, width, height, wndproc, ...) win32_open_window_(title##"WndClass", title, width, height, wndproc, __VA_ARGS__)
static HWND win32_open_window_(char *wndclass_name, char *title, i32 width, i32 height, WNDPROC wndproc, b32 show = true)
{
    HWND result = 0;
    WNDCLASSEXA window_class = {0};
    window_class.cbSize        = sizeof(window_class);
    window_class.style         = CS_OWNDC;
    window_class.lpfnWndProc   = wndproc;
    window_class.hInstance     = GetModuleHandleA(0);
    window_class.hCursor       = LoadCursorA(0, IDC_ARROW);
    window_class.lpszClassName = wndclass_name;

    if (RegisterClassExA(&window_class))
    {
        ulong style = (show) ? (WS_OVERLAPPEDWINDOW | WS_VISIBLE) : 0;
        result = CreateWindowExA(0, window_class.lpszClassName, title, style,
                                 CW_USEDEFAULT, CW_USEDEFAULT, width, height, 
                                 0, 0, window_class.hInstance, 0);
    }

    return result;
}

static LRESULT CALLBACK win32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;
    switch (message)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            global_running = false;
        } break;

        default:
        {
            result = DefWindowProcA(window, message, wparam, lparam);
        } break;
    }
    return result;
}

static void win32_process_messages()
{
    MSG msg;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        UINT message = msg.message;
        switch (message)
        {
            case WM_QUIT:
            {
                global_running = false;
            } break;

            default:
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } break;
        }
    }
}


static b32 win32_init_opengl_extensions()
{
    b32 result = false;
    Win32Window dummy = {0};
    dummy.wnd = win32_open_window("TalkienDummyWindow", CW_USEDEFAULT, CW_USEDEFAULT, DefWindowProcA, false);

    if (dummy.wnd)
    {
        PIXELFORMATDESCRIPTOR descriptor = {0};
        descriptor.nSize        = sizeof(descriptor);
        descriptor.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        descriptor.iPixelType   = PFD_TYPE_RGBA;
        descriptor.cColorBits   = 32;
        descriptor.cDepthBits   = 24;
        descriptor.cStencilBits = 8;
        descriptor.iLayerType   = PFD_MAIN_PLANE;

        dummy.dc = GetDC(dummy.wnd);
        i32 pixel_format_index = ChoosePixelFormat(dummy.dc, &descriptor);            
        SetPixelFormat(dummy.dc, pixel_format_index, &descriptor);

        dummy.rc = wglCreateContext(dummy.dc);
        if (wglMakeCurrent(dummy.dc, dummy.rc))
        {
            wgl_init_extensions();
            opengl_init_extensions();

            result = wglMakeCurrent(0, 0);
        }

        wglDeleteContext(dummy.rc);
        ReleaseDC(dummy.wnd, dummy.dc);
        DestroyWindow(dummy.wnd);
    }

    return result;
}

static HGLRC win32_opengl_create_context(HDC dc, i32 *pixel_format_attribs, i32 *context_attribs)
{
    HGLRC rc = 0;

    if (wglChoosePixelFormatARB)
    {
        PIXELFORMATDESCRIPTOR pixel_format = {0};
        i32 pixel_format_index = 0;
        GLuint extended = 0;

        wglChoosePixelFormatARB(dc, pixel_format_attribs, 0, 1, &pixel_format_index, &extended);
        DescribePixelFormat(dc, pixel_format_index, sizeof(pixel_format), &pixel_format);
        SetPixelFormat(dc, pixel_format_index, &pixel_format);

        if (wglCreateContextAttribsARB)
        {
            rc = wglCreateContextAttribsARB(dc, 0, context_attribs);
        }
    }
    else
    {
        assert(!"Init wgl extensions before calling this function!");
    }

    return rc;
}

#define win32_open_window_init_with_opengl(title, width, height, wndproc) win32_open_window_init_with_opengl_(title##"WndClass", title, width, height, wndproc)
static Win32Window win32_open_window_init_with_opengl_(char *wndclass_name, char *title, i32 width, i32 height, WNDPROC wndproc)
{
    Win32Window result = {0};
    result.wnd = win32_open_window_(wndclass_name, title, width, height, wndproc);
    
    if (result.wnd)
    {
        i32 pixel_format_attribs[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,     24,
            WGL_DEPTH_BITS_ARB,     24,
            WGL_STENCIL_BITS_ARB,   8,
            0,
        };

        #if INTERNAL_BUILD
            #define CONTEXT_FLAGS   WGL_CONTEXT_DEBUG_BIT_ARB 
        #else
            #define CONTEXT_FLAGS   WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
        #endif

        i32 context_attribs[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB,  3,
            WGL_CONTEXT_MINOR_VERSION_ARB,  1,
            WGL_CONTEXT_FLAGS_ARB,          CONTEXT_FLAGS,
            WGL_CONTEXT_PROFILE_MASK_ARB,   WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            0,
        };

        win32_init_opengl_extensions();

        result.dc = GetDC(result.wnd);
        result.rc = win32_opengl_create_context(result.dc, pixel_format_attribs, context_attribs);
        result.initialized = wglMakeCurrent(result.dc, result.rc);
    }

    return result;
}

i32 WinMain(HINSTANCE instance, HINSTANCE prev_instance, char *cmd_line, i32 cmd_show)
{
    global_window = win32_open_window_init_with_opengl("Talkien", 1280, 720, win32_window_proc);

    f32 dt_carried = 0;
    f32 t0 = 0;

    global_running = true;
    while (global_running)
    {
        f32 t = win32_get_time();
        f32 dt = t - t0;

        t0 = t;
        dt_carried += dt;

        while (dt_carried > TIMESTEP_SEC)
        {
            dt_carried -= TIMESTEP_SEC;

            win32_process_messages();

            SwapBuffers(global_window.dc);
        }
    }

    return 0;
}
