#define TIMESTEP_HZ     60
#define TIMESTEP_SEC    (1.0f / TIMESTEP_HZ)

#include "platform.h"
#include "win32_talkien.h"

static HWND global_main_window;

#define win32_open_window(title, width, height, wndproc) win32_open_window_(title##"WndClass", title, width, height, wndproc)
static HWND win32_open_window_(char *wndclass_name, char *title, i32 width, i32 height, WNDPROC wndproc)
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
        result = CreateWindowExA(0, window_class.lpszClassName, title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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
            global_main_window = 0;
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
                global_main_window = 0;
            } break;

            default:
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } break;
        }
    }
}

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

i32 WinMain(HINSTANCE instance, HINSTANCE prev_instance, char *cmd_line, i32 cmd_show)
{
    global_main_window = win32_open_window("Talkien", 1280, 720, win32_window_proc);

    f32 dt_carried = 0;
    f32 t0 = 0;

    while (global_main_window)
    {
        f32 t = win32_get_time();
        f32 dt = t - t0;

        t0 = t;
        dt_carried += dt;

        while (dt_carried > TIMESTEP_SEC)
        {
            dt_carried -= TIMESTEP_SEC;

            win32_process_messages();
        }
    }

    return 0;
}
