#include "platform.h"
#include "opengl.h"

#include "win32_talkien.h"

static Win32Api global_win32_api;
static Win32Api *win32_api = &global_win32_api;

static Win32State global_win32_state;
static Win32State *win32_state = &global_win32_state;

static void *win32_open_window(char *title, i32 width, i32 height, Win32Api_WNDPROC wndproc, b32 show = true)
{
    void *result = 0;
    Win32Api_WNDCLASSEXA window_class = {0};
    window_class.cbSize        = sizeof(window_class);
    window_class.style         = 0x0020 /* CS_OWNDC */;
    window_class.lpfnWndProc   = wndproc;
    window_class.hInstance     = win32_api->GetModuleHandleA(0);
    window_class.hCursor       = win32_api->LoadCursorA(0, ((char *)((uintptr)((short)(32512)))) /* IDC_ARROW */);
    window_class.lpszClassName = title;

    if (win32_api->RegisterClassExA(&window_class))
    {
        int x = (int)0x80000000 /* CW_USEDEFAULT */;
        int y = (int)0x80000000 /* CW_USEDEFAULT */;
        unsigned int overlapped = ((0x00000000L /* WS_OVERLAPPED */  | 0x00C00000L /* WS_CAPTION */ | 
                                    0x00080000L /* WS_SYSKEYMENU */  | 0x00040000L /* WS_THICKFRAME */ | 
                                    0x00020000L /* WS_MINIMIZEBOX */ | 0x00010000L /* WS_MAXIMIZEBOX */
                                   ) /* WS_OVERLAPPEDWINDOW */ | 0x10000000L /* WS_VISIBLE */);

        result = win32_api->CreateWindowExA(0, window_class.lpszClassName, title, (show) ? overlapped : 0,
                                            x, y, width, height, 0, 0, window_class.hInstance, 0);
    }

    return result;
}

static b32 win32_init_opengl_extensions()
{
    b32 result = false;
    Win32Window dummy = {0};
    dummy.wnd = win32_open_window("TalkienDummyWindow", 0, 0, (Win32Api_WNDPROC)win32_api->DefWindowProcA, false);

    if (dummy.wnd)
    {
        Win32Api_PIXELFORMATDESCRIPTOR descriptor = {0};
        descriptor.nSize        = sizeof(descriptor);
        descriptor.dwFlags      = 0x00000004 /* PFD_DRAW_TO_WINDOW */ | 0x00000020 /* PFD_SUPPORT_OPENGL */ | 0x00000001 /* PFD_DOUBLEBUFFER */;
        descriptor.iPixelType   = 0 /* PFD_TYPE_RGBA */;
        descriptor.cColorBits   = 32;
        descriptor.cDepthBits   = 24;
        descriptor.cStencilBits = 8;
        descriptor.iLayerType   = 0 /* PFD_MAIN_PLANE */;

        dummy.dc = win32_api->GetDC(dummy.wnd);
        i32 pixel_format_index = win32_api->ChoosePixelFormat(dummy.dc, &descriptor);            
        win32_api->SetPixelFormat(dummy.dc, pixel_format_index, &descriptor);

        dummy.rc = win32_api->wglCreateContext(dummy.dc);
        if (win32_api->wglMakeCurrent(dummy.dc, dummy.rc))
        {
            win32_api->wglChoosePixelFormatARB = (Win32Api_wglChoosePixelFormatARB)win32_api->wglGetProcAddress("wglChoosePixelFormatARB");
            win32_api->wglCreateContextAttribsARB = (Win32Api_wglCreateContextAttribsARB)win32_api->wglGetProcAddress("wglCreateContextAttribsARB");
            win32_api->wglSwapIntervalEXT = (Win32Api_wglSwapIntervalEXT)win32_api->wglGetProcAddress("wglSwapIntervalEXT");

            result = win32_api->wglMakeCurrent(0, 0);
        }

        win32_api->wglDeleteContext(dummy.rc);
        win32_api->ReleaseDC(dummy.wnd, dummy.dc);
        win32_api->DestroyWindow(dummy.wnd);
    }

    return result;
}

static void *win32_opengl_create_context(void *dc, int *pixel_format_attribs, int *context_attribs)
{
    void *rc = 0;

    if (win32_api->wglChoosePixelFormatARB)
    {
        Win32Api_PIXELFORMATDESCRIPTOR pixel_format = {0};
        int pixel_format_index = 0;
        unsigned int num_formats = 0;

        win32_api->wglChoosePixelFormatARB(dc, pixel_format_attribs, 0, 1, &pixel_format_index, &num_formats);
        win32_api->DescribePixelFormat(dc, pixel_format_index, sizeof(pixel_format), &pixel_format);
        win32_api->SetPixelFormat(dc, pixel_format_index, &pixel_format);

        if (win32_api->wglCreateContextAttribsARB)
        {
            rc = win32_api->wglCreateContextAttribsARB(dc, 0, context_attribs);
        }
    }
    else
    {
        assert_always("Init wgl extensions before calling this function");
    }

    return rc;
}

static Win32Window win32_open_window_init_with_opengl(char *title, i32 width, i32 height, Win32Api_WNDPROC wndproc)
{
    Win32Window result = {0};
    result.wnd = win32_open_window(title, width, height, wndproc);
    
    if (result.wnd)
    {
        i32 pixel_format_attribs[] =
        {
            0x2001 /* WGL_DRAW_TO_WINDOW_ARB */,  1      /* GL_TRUE */,
            0x2003 /* WGL_ACCELERATION_ARB */,    0x2027 /* WGL_FULL_ACCELERATION_ARB */,
            0x2010 /* WGL_SUPPORT_OPENGL_ARB */,  1      /* GL_TRUE */,
            0x2011 /* WGL_DOUBLE_BUFFER_ARB */,   1      /* GL_TRUE */,
            0x2013 /* WGL_PIXEL_TYPE_ARB */,      0x202B /* WGL_TYPE_RGBA_ARB */,
            0x2014 /* WGL_COLOR_BITS_ARB */,      24,
            0x2022 /* WGL_DEPTH_BITS_ARB */,      24,
            0x2023 /* WGL_STENCIL_BITS_ARB */,    8,
            0,
        };

        #if INTERNAL_BUILD
            #define CONTEXT_FLAGS   0x0001 /* WGL_CONTEXT_DEBUG_BIT_ARB */
        #else
            #define CONTEXT_FLAGS   0x0002 /* WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB */
        #endif

        i32 context_attribs[] =
        {
            0x2091 /* WGL_CONTEXT_MAJOR_VERSION_ARB */,  3,
            0x2092 /* WGL_CONTEXT_MINOR_VERSION_ARB */,  3,
            0x2094 /* WGL_CONTEXT_FLAGS_ARB */,          CONTEXT_FLAGS,
            0x9126 /* WGL_CONTEXT_PROFILE_MASK_ARB */,   0x0002 /* WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB */,
            0,
        };

        win32_init_opengl_extensions();

        Win32Api_RECT rect;
        win32_api->GetClientRect(result.wnd, &rect);

        i32 client_width = rect.right - rect.left;
        i32 client_height = rect.bottom - rect.top;

        result.dc = win32_api->GetDC(result.wnd);
        result.rc = win32_opengl_create_context(result.dc, pixel_format_attribs, context_attribs);
        result.width = client_width;
        result.height = client_height;

        result.initialized = win32_api->wglMakeCurrent(result.dc, result.rc);
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
        win32_api->QueryPerformanceCounter((Win32Api_LARGE_INTEGER *)&start);
        win32_api->QueryPerformanceFrequency((Win32Api_LARGE_INTEGER *)&freq);
    }
    else
    {
        i64 counter = 0;
        win32_api->QueryPerformanceCounter((Win32Api_LARGE_INTEGER *)&counter);
        t = (f32)((counter - start) / (f64)freq);
    }

    return t;
}

static intptr __stdcall win32_window_proc(void *wnd, unsigned int message, uintptr wparam, intptr lparam)
{
    intptr result = 0;
    switch (message)
    {
        case 0x0010 /* WM_CLOSE */:
        case 0x0002 /* WM_DESTROY */:
        {
            win32_state->running = false;
        } break;

        case 0x0005 /* WM_SIZE */:
        {
            i32 width = (i32)(lparam & 0xffff);
            i32 height = (i32)((lparam >> 16) & 0xffff);

            win32_state->window.width = width;
            win32_state->window.height = height;
        } break;

        case 0x0104 /* WM_SYSKEYDOWN */:
        case 0x0105 /* WM_SYSKEYUP */:
        case 0x0100 /* WM_KEYDOWN */:
        case 0x0101 /* WM_KEYUP */:
        {
            assert_always("Keyboard input came in as a non-dispatch message");
        } break;

        default:
        {
            result = win32_api->DefWindowProcA(wnd, message, wparam, lparam);
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
    Win32Api_MSG msg;
    while (win32_api->PeekMessageA(&msg, 0, 0, 0, 0x0001 /* PM_REMOVE */))
    {
        unsigned int message = msg.message;
        switch (message)
        {
            case 0x0012 /* WM_QUIT */:
            {
                win32_state->running = false;
            } break;

            case 0x0102 /* WM_CHAR */:
            {
                assert(input->character_count < array_count(input->characters));
                if (input->character_count < array_count(input->characters))
                {
                    input->characters[input->character_count++] = (char)msg.wParam;
                }
            } break;

            case 0x0104 /* WM_SYSKEYDOWN */:
            case 0x0105 /* WM_SYSKEYUP */:
            case 0x0100 /* WM_KEYDOWN */:
            case 0x0101 /* WM_KEYUP */:
            {
                b32 was_down = ((msg.lParam & (1 << 30)) != 0);
                b32 is_down = ((msg.lParam & (1 << 31)) == 0);

                if (was_down != is_down)
                {
                    switch (msg.wParam)
                    {
                        case 0x09 /* VK_TAB */:    { win32_process_button(&input->buttons[button_tab],       is_down); } break;
                        case 0x25 /* VK_LEFT */:   { win32_process_button(&input->buttons[button_left],      is_down); } break;
                        case 0x27 /* VK_RIGHT */:  { win32_process_button(&input->buttons[button_right],     is_down); } break;
                        case 0x26 /* VK_UP */:     { win32_process_button(&input->buttons[button_up],        is_down); } break;
                        case 0x28 /* VK_DOWN */:   { win32_process_button(&input->buttons[button_down],      is_down); } break;
                        case 0x21 /* VK_PRIOR */:  { win32_process_button(&input->buttons[button_pageup],    is_down); } break;
                        case 0x22 /* VK_NEXT */:   { win32_process_button(&input->buttons[button_pagedown],  is_down); } break;
                        case 0x24 /* VK_HOME */:   { win32_process_button(&input->buttons[button_home],      is_down); } break;
                        case 0x23 /* VK_END */:    { win32_process_button(&input->buttons[button_end],       is_down); } break;
                        case 0x2E /* VK_DELETE */: { win32_process_button(&input->buttons[button_delete],    is_down); } break;
                        case 0x08 /* VK_BACK */:   { win32_process_button(&input->buttons[button_backspace], is_down); } break;
                        case 0x0D /* VK_RETURN */: { win32_process_button(&input->buttons[button_enter],     is_down); } break;
                        case 0x1B /* VK_ESCAPE */: { win32_process_button(&input->buttons[button_esc],       is_down); } break;
                        case 'A':                  { win32_process_button(&input->buttons[button_a],         is_down); } break;
                        case 'C':                  { win32_process_button(&input->buttons[button_c],         is_down); } break;
                        case 'V':                  { win32_process_button(&input->buttons[button_v],         is_down); } break;
                        case 'X':                  { win32_process_button(&input->buttons[button_x],         is_down); } break;
                        case 'Y':                  { win32_process_button(&input->buttons[button_y],         is_down); } break;
                        case 'Z':                  { win32_process_button(&input->buttons[button_z],         is_down); } break;
                    }
                }

                if (message == 0x0100 /* WM_KEYDOWN */)
                {
                    win32_api->TranslateMessage(&msg);
                }
            } break;

            case 0x020A /* WM_MOUSEWHEEL */:
            {
                input->mouse_z = (f32)((short)((short)((((ulongptr)(msg.wParam)) >> 16) & 0xffff))) / 120.0f;
            } break;

            default:
            {
                win32_api->TranslateMessage(&msg);
                win32_api->DispatchMessageA(&msg);
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
    input->shift_down = (win32_api->GetKeyState(0x10 /* VK_SHIFT */) & (1 << 15));
    input->alt_down = (win32_api->GetKeyState(0x12 /* VK_MENU */) & (1 << 15));
    input->ctrl_down = (win32_api->GetKeyState(0x11 /* VK_CONTROL */) & (1 << 15));

    Win32Api_POINT mouse_p;
    win32_api->GetCursorPos(&mouse_p);
    win32_api->ScreenToClient(win32_state->window.wnd, &mouse_p);

    input->mouse_x = (f32)mouse_p.x;
    input->mouse_y = (f32)mouse_p.y;

    unsigned int mouse_button_id[mouse_button_count] =
    {
        0x01 /* VK_LBUTTON */,
        0x04 /* VK_MBUTTON */,
        0x02 /* VK_RBUTTON */,
        0x05 /* VK_XBUTTON1 */,
        0x06 /* VK_XBUTTON2 */,
    };

    for (u32 mouse_button_index = 0; mouse_button_index < mouse_button_count; ++mouse_button_index)
    {
        input->mouse_buttons[mouse_button_index] = prev_input->mouse_buttons[mouse_button_index];
        input->mouse_buttons[mouse_button_index].transitions = 0;

        b32 is_down = win32_api->GetKeyState(mouse_button_id[mouse_button_index]) & (1 << 15);
        win32_process_button(&input->mouse_buttons[mouse_button_index], is_down);
    }
}

static PLATFORM_ALLOCATE(win32_allocate)
{
    assert(sizeof(Win32MemoryBlock) == 64);

    usize page_size = 4096;
    usize total_size = size + sizeof(Win32MemoryBlock);
    usize base_offset = sizeof(Win32MemoryBlock);

    Win32MemoryBlock *block = (Win32MemoryBlock *)win32_api->VirtualAlloc(0, total_size, 0x2000 /* MEM_RESERVE */ | 0x1000 /* MEM_COMMIT */, 0x04 /* PAGE_READWRITE */);
    assert(block);

    block->memblock.base = (u8 *)block + base_offset;
    assert(block->memblock.used == 0);
    assert(block->memblock.prev == 0);

    Win32MemoryBlock *sentinel = &win32_state->memory_sentinel;
    block->next = sentinel;
    block->memblock.size = size;

    begin_mutex(&win32_state->memory_mutex);
    block->prev = sentinel->prev;
    block->prev->next = block;
    block->prev->prev = block;
    end_mutex(&win32_state->memory_mutex);

    PlatformMemoryBlock *memblock = &block->memblock;
    return memblock;
}

static PLATFORM_DEALLOCATE(win32_deallocate)
{
    if (memblock)
    {
        Win32MemoryBlock *block = (Win32MemoryBlock *)memblock;

        begin_mutex(&win32_state->memory_mutex);
        block->prev->next = block->next;
        block->next->prev = block->prev;
        end_mutex(&win32_state->memory_mutex);

        b32 result = win32_api->VirtualFree(block, 0, 0x8000 /* MEM_RELEASE */);
        assert(result);
    }
}

static PLATFORM_GET_MEMORY_STATS(win32_get_memory_stats)
{
    PlatformMemoryStats result = {0};

    begin_mutex(&win32_state->memory_mutex);
    Win32MemoryBlock *sentinel = &win32_state->memory_sentinel;
    for (Win32MemoryBlock *memblock = sentinel->next; memblock != sentinel; memblock = memblock->next)
    {
        ++result.num_memblocks;

        result.total_size += memblock->memblock.size;
        result.total_used += memblock->memblock.used;
    }
    end_mutex(&win32_state->memory_mutex);

    return result;
}

static PLATFORM_INIT_OPENGL(win32_init_opengl)
{
    void *module = win32_load_library("opengl32.dll");
    #define GLCORE(a, b) open_gl->##b = (PFNGL##a##PROC)win32_get_proc_address(module, "gl" #b); 
    GL_FUNCTION_LIST_1_1
    #undef GLCORE

    #define GLCORE(a, b) open_gl->##b = (PFNGL##a##PROC)win32_api->wglGetProcAddress("gl" #b); 
    GL_FUNCITON_LIST
    #undef GLCORE
}

static void win32_build_filename(char *pathname, u32 pathname_size,
                                 char *filename, u32 filename_size, 
                                 char *out, u32 max_out_size)
{
    if ((pathname_size + filename_size + 1) < max_out_size)
    {
        u32 out_index = 0;
        for (u32 char_index = 0; char_index < pathname_size; ++char_index)
        {
            out[out_index++] = pathname[char_index];
        }
        for (u32 char_index = 0; char_index < filename_size; ++char_index)
        {
            out[out_index++] = filename[char_index];
        }
        out[out_index] = '\0';
    }
}

static void win32_init_exe_path(Win32State *state)
{
    state->exe_filename_length = win32_api->GetModuleFileNameA(0, state->exe_filename, sizeof(state->exe_filename));
    state->exe_path_length = state->exe_filename_length;

    for (u32 char_index = state->exe_path_length - 1; char_index > 0; --char_index)
    {
        if (*(state->exe_filename + char_index) == '\\')
        {
            state->exe_path_length = char_index + 1;
            break;
        }
    }

    char app_dll_filename[] = "talkien.dll";
    win32_build_filename(state->exe_filename, state->exe_path_length,
                         app_dll_filename, array_count(app_dll_filename),
                         state->app_dll_filename, MAX_FILENAME_SIZE);

    char app_dll_lock_filename[] = "talkien.lock.dll";
    win32_build_filename(state->exe_filename, state->exe_path_length,
                         app_dll_lock_filename, array_count(app_dll_lock_filename),
                         state->app_dll_lock_filename, MAX_FILENAME_SIZE);

    char app_temp_dll_filename[] = "talkien.temp.dll";
    win32_build_filename(state->exe_filename, state->exe_path_length,
                         app_temp_dll_filename, array_count(app_temp_dll_filename),
                         state->app_temp_dll_filename, MAX_FILENAME_SIZE);
}

static void win32_load_app_dll(Win32State *state)
{
    Win32Api_WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!win32_api->GetFileAttributesExA(state->app_dll_lock_filename, Win32Api_GetFileExInfoStandard, &attr))
    {
        state->app_dll_last_write = {};
        if (win32_api->GetFileAttributesExA(state->app_dll_filename, Win32Api_GetFileExInfoStandard, &attr))
        {
            state->app_dll_last_write = attr.ftLastWriteTime;
        }

        win32_api->CopyFileA(state->app_dll_filename, state->app_temp_dll_filename, 0);

        state->app_dll = win32_load_library(state->app_temp_dll_filename);
        if (state->app_dll)
        {
            state->update_and_render = (UpdateAndRender *)win32_get_proc_address(state->app_dll, "update_and_render");
        }
    }

    if (!state->update_and_render)
    {
        state->update_and_render = update_and_render_stub;
    }
}

static void win32_unload_app_dll(Win32State *state)
{
    if (state->app_dll)
    {
        win32_free_library(state->app_dll);
        state->app_dll = 0;
    }

    state->update_and_render = update_and_render_stub;
}

static void win32_reload_app_dll_if_needed(Win32State *state)
{
    state->app_dll_reloaded = false;

    Win32Api_WIN32_FILE_ATTRIBUTE_DATA attr;
    if (win32_api->GetFileAttributesExA(state->app_dll_filename, Win32Api_GetFileExInfoStandard, &attr))
    {
        if (win32_api->CompareFileTime(&attr.ftLastWriteTime, &state->app_dll_last_write) != 0)
        {
            win32_unload_app_dll(state);
            win32_load_app_dll(state);

            state->app_memory.app_dll_reloaded = true;
        }
    }
}

int __stdcall WinMain(HINSTANCE instance, HINSTANCE prev_instance, char *cmd_line, int cmd_show)
{
    Win32State *state = win32_state;

    win32_init_win32_api(win32_api);

    state->memory_sentinel.prev = &state->memory_sentinel;
    state->memory_sentinel.next = &state->memory_sentinel;
    state->window = win32_open_window_init_with_opengl("Talkien", 1280, 720, win32_window_proc);

    win32_init_exe_path(state);

    if (state->window.initialized)
    {
        PlatformInput inputs[2] = {0};
        PlatformInput *input = &inputs[0];
        PlatformInput *prev_input = &inputs[1];

        AppMemory *app_memory = &win32_state->app_memory;
        app_memory->platform.allocate = win32_allocate;
        app_memory->platform.deallocate = win32_deallocate;
        app_memory->platform.get_memory_stats = win32_get_memory_stats;
        app_memory->platform.init_opengl = win32_init_opengl;

        if (win32_api->wglSwapIntervalEXT)
        {
            win32_api->wglSwapIntervalEXT(1);
        }
        
        win32_load_app_dll(state);

        f32 t0 = win32_get_time();
        f32 dt = 0;

        state->running = true;
        while (state->running)
        {
            win32_update_input(input, prev_input, dt);
            
            state->update_and_render(app_memory, input, state->window.width, state->window.height);

            win32_reload_app_dll_if_needed(win32_state);

            if (input->quit_requested)
            {
                state->running = false;
            }

            swap_values(PlatformInput *, input, prev_input);
            win32_api->SwapBuffers(state->window.dc);
            f32 t1 = win32_get_time();
            dt = t1 - t0;
            t0 = t1;
        }
    }
    
    return 0;
}
