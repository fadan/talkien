#include "platform.h"
#include "win32_talkien.h"

#include "talkien.cpp"

Platform platform;
static Win32State global_state;

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
        assert_always("Init wgl extensions before calling this function");
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
            WGL_CONTEXT_MINOR_VERSION_ARB,  3,
            WGL_CONTEXT_FLAGS_ARB,          CONTEXT_FLAGS,
            WGL_CONTEXT_PROFILE_MASK_ARB,   WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            0,
        };

        win32_init_opengl_extensions();

        RECT rect;
        GetClientRect(result.wnd, &rect);

        i32 client_width = rect.right - rect.left;
        i32 client_height = rect.bottom - rect.top;

        result.dc = GetDC(result.wnd);
        result.rc = win32_opengl_create_context(result.dc, pixel_format_attribs, context_attribs);
        result.width = client_width;
        result.height = client_height;

        result.initialized = wglMakeCurrent(result.dc, result.rc);
    }

    return result;
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

static LRESULT CALLBACK win32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;
    switch (message)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            global_state.running = false;
        } break;

        case WM_SIZE:
        {
            i32 width = (i32)(lparam & 0xffff);
            i32 height = (i32)((lparam >> 16) & 0xffff);

            global_state.window.width = width;
            global_state.window.height = height;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            assert_always("Keyboard input came in as a non-dispatch message");
        } break;

        default:
        {
            result = DefWindowProcA(window, message, wparam, lparam);
        } break;
    }
    return result;
}

static void win32_process_button(PlatformButtonState *state, b32 is_down)
{
    if (state->is_down != is_down)
    {
        state->is_down = is_down;
        ++state->transitions;
    }
}

static void win32_process_messages(PlatformInput *input, PlatformInput *prev_input)
{
    MSG msg;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        UINT message = msg.message;
        switch (message)
        {
            case WM_QUIT:
            {
                global_state.running = false;
            } break;

            case WM_CHAR:
            {
                assert(input->character_count < array_count(input->characters));
                if (input->character_count < array_count(input->characters))
                {
                    input->characters[input->character_count++] = (char)msg.wParam;
                }
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                b32 was_down = ((msg.lParam & (1 << 30)) != 0);
                b32 is_down = ((msg.lParam & (1 << 31)) == 0);

                if (was_down != is_down)
                {
                    switch (msg.wParam)
                    {
                        case VK_TAB:    { win32_process_button(&input->buttons[button_tab],       is_down); } break;
                        case VK_LEFT:   { win32_process_button(&input->buttons[button_left],      is_down); } break;
                        case VK_RIGHT:  { win32_process_button(&input->buttons[button_right],     is_down); } break;
                        case VK_UP:     { win32_process_button(&input->buttons[button_up],        is_down); } break;
                        case VK_DOWN:   { win32_process_button(&input->buttons[button_down],      is_down); } break;
                        case VK_PRIOR:  { win32_process_button(&input->buttons[button_pageup],    is_down); } break;
                        case VK_NEXT:   { win32_process_button(&input->buttons[button_pagedown],  is_down); } break;
                        case VK_HOME:   { win32_process_button(&input->buttons[button_home],      is_down); } break;
                        case VK_END:    { win32_process_button(&input->buttons[button_end],       is_down); } break;
                        case VK_DELETE: { win32_process_button(&input->buttons[button_delete],    is_down); } break;
                        case VK_BACK:   { win32_process_button(&input->buttons[button_backspace], is_down); } break;
                        case VK_RETURN: { win32_process_button(&input->buttons[button_enter],     is_down); } break;
                        case VK_ESCAPE: { win32_process_button(&input->buttons[button_esc],       is_down); } break;
                        case 'A':       { win32_process_button(&input->buttons[button_a],         is_down); } break;
                        case 'C':       { win32_process_button(&input->buttons[button_c],         is_down); } break;
                        case 'V':       { win32_process_button(&input->buttons[button_v],         is_down); } break;
                        case 'X':       { win32_process_button(&input->buttons[button_x],         is_down); } break;
                        case 'Y':       { win32_process_button(&input->buttons[button_y],         is_down); } break;
                        case 'Z':       { win32_process_button(&input->buttons[button_z],         is_down); } break;
                    }
                }

                if (message == WM_KEYDOWN)
                {
                    TranslateMessage(&msg);
                }
            } break;

            case WM_MOUSEWHEEL:
            {
                input->mouse_z = (f32)GET_WHEEL_DELTA_WPARAM(msg.wParam) / (f32)WHEEL_DELTA;
            } break;

            default:
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } break;
        }
    }
}

static void win32_update_input(PlatformInput *input, PlatformInput *prev_input, f32 dt)
{
    *input = {0};

    for (u32 button_index = 0; button_index < button_count; ++button_index)
    {
        input->buttons[button_index].is_down = prev_input->buttons[button_index].is_down;
    }

    win32_process_messages(input, prev_input);
    
    input->dt = dt;
    input->shift_down = (GetKeyState(VK_SHIFT) & (1 << 15));
    input->alt_down = (GetKeyState(VK_MENU) & (1 << 15));
    input->ctrl_down = (GetKeyState(VK_CONTROL) & (1 << 15));

    POINT mouse_p;
    GetCursorPos(&mouse_p);
    ScreenToClient(global_state.window.wnd, &mouse_p);

    input->mouse_x = (f32)mouse_p.x;
    input->mouse_y = (f32)mouse_p.y;

    DWORD mouse_button_id[mouse_button_count] =
    {
        VK_LBUTTON,
        VK_MBUTTON,
        VK_RBUTTON,
        VK_XBUTTON1,
        VK_XBUTTON2,
    };

    for (u32 mouse_button_index = 0; mouse_button_index < mouse_button_count; ++mouse_button_index)
    {
        input->mouse_buttons[mouse_button_index] = prev_input->mouse_buttons[mouse_button_index];
        input->mouse_buttons[mouse_button_index].transitions = 0;

        b32 is_down = GetKeyState(mouse_button_id[mouse_button_index]) & (1 << 15);
        win32_process_button(&input->mouse_buttons[mouse_button_index], is_down);
    }
}

static PLATFORM_ALLOCATE_PROC(win32_allocate)
{
    assert(sizeof(Win32MemoryBlock) == 64);

    usize page_size = 4096;
    usize total_size = size + sizeof(Win32MemoryBlock);
    usize base_offset = sizeof(Win32MemoryBlock);

    Win32MemoryBlock *block = (Win32MemoryBlock *)VirtualAlloc(0, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(block);

    block->memblock.base = (u8 *)block + base_offset;
    assert(block->memblock.used == 0);
    assert(block->memblock.prev == 0);

    Win32MemoryBlock *sentinel = &global_state.memory_sentinel;
    block->next = sentinel;
    block->memblock.size = size;

    begin_mutex(&global_state.memory_mutex);
    block->prev = sentinel->prev;
    block->prev->next = block;
    block->prev->prev = block;
    end_mutex(&global_state.memory_mutex);

    PlatformMemoryBlock *memblock = &block->memblock;
    return memblock;
}

static PLATFORM_DEALLOCATE_PROC(win32_deallocate)
{
    if (memblock)
    {
        Win32MemoryBlock *block = (Win32MemoryBlock *)memblock;

        begin_mutex(&global_state.memory_mutex);
        block->prev->next = block->next;
        block->next->prev = block->prev;
        end_mutex(&global_state.memory_mutex);

        b32 result = VirtualFree(block, 0, MEM_RELEASE);
        assert(result);
    }
}

static PLATFORM_GET_MEMORY_STATS(win32_get_memory_stats)
{
    PlatformMemoryStats result = {0};

    begin_mutex(&global_state.memory_mutex);
    Win32MemoryBlock *sentinel = &global_state.memory_sentinel;
    for (Win32MemoryBlock *memblock = sentinel->next; memblock != sentinel; memblock = memblock->next)
    {
        ++result.num_memblocks;

        result.total_size += memblock->memblock.size;
        result.total_used += memblock->memblock.used;
    }
    end_mutex(&global_state.memory_mutex);

    return result;
}

i32 WinMain(HINSTANCE instance, HINSTANCE prev_instance, char *cmd_line, i32 cmd_show)
{
    Win32State *state = &global_state;
    state->memory_sentinel.prev = &state->memory_sentinel;
    state->memory_sentinel.next = &state->memory_sentinel;
    state->window = win32_open_window_init_with_opengl("Talkien", 1280, 720, win32_window_proc);

    if (state->window.initialized)
    {
        PlatformInput inputs[2] = {0};
        PlatformInput *input = &inputs[0];
        PlatformInput *prev_input = &inputs[1];

        AppMemory app_memory = {0}; 
        app_memory.platform.allocate = win32_allocate;
        app_memory.platform.deallocate = win32_deallocate;
        app_memory.platform.get_memory_stats = win32_get_memory_stats;

        platform = app_memory.platform;

        if (wglSwapIntervalEXT)
        {
            wglSwapIntervalEXT(1);
        }
        
        f32 t0 = win32_get_time();
        f32 dt = 0;

        state->running = true;
        while (state->running)
        {
            win32_update_input(input, prev_input, dt);
            update_and_render(&app_memory, input, state->window.width, state->window.height);
            swap_values(PlatformInput *, input, prev_input);

            SwapBuffers(state->window.dc);
            f32 t1 = win32_get_time();
            dt = t1 - t0;
            t0 = t1;
        }
    }
    
    return 0;
}
