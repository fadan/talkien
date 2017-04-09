#include "platform.h"
#include "opengl.h"

#include "win32_talkien.h"

static Win32Api global_win32_api;
static Win32Api *win32_api = &global_win32_api;

static Win32State global_win32_state;
static Win32State *win32_state = &global_win32_state;

static void *win32_create_window(char *title, i32 width, i32 height, Win32Api_WNDPROC wndproc)
{
    Win32Api_WNDCLASSEXA window_class = {0};
    window_class.cbSize        = sizeof(window_class);
    window_class.style         = 0x0020 /* CS_OWNDC */;
    window_class.lpfnWndProc   = wndproc;
    window_class.hInstance     = win32_api->GetModuleHandleA(0);
    window_class.hCursor       = win32_api->LoadCursorA(0, ((char *)((uintptr)((short)(32512)))) /* IDC_ARROW */);
    window_class.lpszClassName = title;

    void *wnd = 0;
    if (win32_api->RegisterClassExA(&window_class))
    {
        int x = (int)0x80000000 /* CW_USEDEFAULT */;
        int y = (int)0x80000000 /* CW_USEDEFAULT */;
        unsigned int overlapped = ((0x00000000L /* WS_OVERLAPPED */  | 0x00C00000L /* WS_CAPTION */ | 
                                    0x00080000L /* WS_SYSKEYMENU */  | 0x00040000L /* WS_THICKFRAME */ | 
                                    0x00020000L /* WS_MINIMIZEBOX */ | 0x00010000L /* WS_MAXIMIZEBOX */
                                   ) /* WS_OVERLAPPEDWINDOW */ | 0x10000000L /* WS_VISIBLE */);
        wnd = win32_api->CreateWindowExA(0, window_class.lpszClassName, title, overlapped, 
                                            x, y, width, height, 0, 0, window_class.hInstance, 0);
    }
    return wnd;
}

static void win32_set_vsync(b32 enable)
{
    Win32Api_wglSwapIntervalEXT wglSwapIntervalEXT = (Win32Api_wglSwapIntervalEXT)win32_api->wglGetProcAddress("wglSwapIntervalEXT");

    if (wglSwapIntervalEXT)
    {
        wglSwapIntervalEXT(enable ? 1 : 0);
    }
}

static void *win32_create_context(void *dc, int *context_attribs)
{
    void *rc = win32_api->wglCreateContext(dc);
    if (win32_api->wglMakeCurrent(dc, rc))
    {
        Win32Api_wglCreateContextAttribsARB wglCreateContextAttribsARB = (Win32Api_wglCreateContextAttribsARB)win32_api->wglGetProcAddress("wglCreateContextAttribsARB");

        if (wglCreateContextAttribsARB)
        {
            win32_api->wglMakeCurrent(dc, 0);
            win32_api->wglDeleteContext(rc);

            rc = wglCreateContextAttribsARB(dc, 0, context_attribs);
        }
    }
    return rc;
}

static void win32_set_pixel_format(void *dc)
{
    Win32Api_PIXELFORMATDESCRIPTOR descriptor = {0};
    descriptor.nSize        = sizeof(descriptor);
    descriptor.dwFlags      = 0x4 /* PFD_DRAW_TO_WINDOW */ | 0x20 /* PFD_SUPPORT_OPENGL */ | 0x1 /* PFD_DOUBLEBUFFER */;
    descriptor.iPixelType   = 0 /* PFD_TYPE_RGBA */;
    descriptor.cColorBits   = 32;
    descriptor.cDepthBits   = 24;
    descriptor.cStencilBits = 8;
    descriptor.iLayerType   = 0 /* PFD_MAIN_PLANE */;


    int pixel_format = win32_api->ChoosePixelFormat(dc, &descriptor);
    if (pixel_format && win32_api->DescribePixelFormat(dc, pixel_format, sizeof(descriptor), &descriptor))
    {
        assert(descriptor.dwFlags & 0x20 /* PFD_SUPPORT_OPENGL */);            
        win32_api->SetPixelFormat(dc, pixel_format, &descriptor);
    }
}

static Win32Window win32_open_window_init_with_opengl(char *title, int width, int height, Win32Api_WNDPROC wndproc)
{
    Win32Window window = {0};
    window.wnd = win32_create_window(title, width, height, wndproc);
    if (window.wnd)
    {
        window.dc = win32_api->GetDC(window.wnd);
        
        win32_set_pixel_format(window.dc);

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

        window.rc = win32_create_context(window.dc, context_attribs);

        Win32Api_RECT rect;
        win32_api->GetClientRect(window.wnd, &rect);

        window.width = rect.right - rect.left;
        window.height = rect.bottom - rect.top;

        win32_api->wglMakeCurrent(window.dc, window.rc);
    }
    return window;
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

        default:
        {
            result = win32_api->DefWindowProcA(wnd, message, wparam, lparam);
        } break;
    }
    return result;
}

static void win32_update_button(PlatformButton *button, b32 down)
{
    b32 was_down = button->down;
    button->down = down;
    button->pressed = !was_down && down;
    button->released = was_down && !down;
}

static void win32_get_raw_input(PlatformInput *input, intptr lparam)
{
    Win32Api_RAWINPUT raw_input;
    unsigned int raw_input_size = sizeof(raw_input);
    win32_api->GetRawInputData((void *)lparam, 0x10000003 /* RID_INPUT */, &raw_input, &raw_input_size, sizeof(Win32Api_RAWINPUTHEADER));
    
    if (raw_input.header.dwType == 0 /* RIM_TYPEMOUSE */)
    {
        input->delta_mouse_pos[0] += raw_input.data.mouse.lLastX;
        input->delta_mouse_pos[1] += raw_input.data.mouse.lLastY;

        unsigned short button_flags = raw_input.data.mouse.usButtonFlags;

        int left_button_down = input->mouse_buttons[mouse_button_left].down;
        if (button_flags & 0x0001 /* RI_MOUSE_LEFT_BUTTON_DOWN */) left_button_down = true;
        if (button_flags & 0x0002 /* RI_MOUSE_LEFT_BUTTON_UP */)   left_button_down = false;
        win32_update_button(&input->mouse_buttons[mouse_button_left], left_button_down);

        int middle_button_down = input->mouse_buttons[mouse_button_middle].down;
        if (button_flags & 0x0010 /* RI_MOUSE_MIDDLE_BUTTON_DOWN */) middle_button_down = true;
        if (button_flags & 0x0020 /* RI_MOUSE_MIDDLE_BUTTON_UP */)   middle_button_down = false;
        win32_update_button(&input->mouse_buttons[mouse_button_middle], middle_button_down);

        int right_button_down = input->mouse_buttons[mouse_button_right].down;
        if (button_flags & 0x0004 /* RI_MOUSE_RIGHT_BUTTON_DOWN */) right_button_down = true;
        if (button_flags & 0x0008 /* RI_MOUSE_RIGHT_BUTTON_UP */)   right_button_down = false;
        win32_update_button(&input->mouse_buttons[mouse_button_right], right_button_down);

        int extended0_button_down = input->mouse_buttons[mouse_button_extended0].down;
        if (button_flags & 0x0040 /* RI_MOUSE_BUTTON_4_DOWN */) extended0_button_down = true;
        if (button_flags & 0x0080 /* RI_MOUSE_BUTTON_4_UP */)   extended0_button_down = false;
        win32_update_button(&input->mouse_buttons[mouse_button_extended0], extended0_button_down);

        int extended1_button_down = input->mouse_buttons[mouse_button_extended1].down;
        if (button_flags & 0x0100 /* RI_MOUSE_BUTTON_5_DOWN */) extended1_button_down = true;
        if (button_flags & 0x0200 /* RI_MOUSE_BUTTON_5_UP */)   extended1_button_down = false;
        win32_update_button(&input->mouse_buttons[mouse_button_extended1], extended1_button_down);

        if (button_flags & 0x0400 /* RI_MOUSE_WHEEL */)
        {
            input->delta_wheel += ((short)raw_input.data.mouse.usButtonData) / 120 /* WHEEL_DELTA */;
        }
    }
    else if (raw_input.header.dwType == 1 /* RIM_TYPEKEYBOARD */)
    {
        unsigned int scan_code = raw_input.data.keyboard.MakeCode;
        unsigned short flags = raw_input.data.keyboard.Flags;

        assert(scan_code <= 0xFF);

        if (flags & 2 /* RI_KEY_E0 */)
        {
            scan_code |= 0xE000;
        }
        else if (flags & 4 /* RI_KEY_E1 */)
        {
            scan_code |= 0xE100;
        }

        // NOTE(dan): pause scan_code is in 2 parts: WM_INPUT \w 0xE11D and WM_INPUT \w 0x45
        if (win32_state->pause_scan_code_read)
        {
            if (scan_code == 0x45)
            {
                scan_code = 0xE11D45;
            }
            win32_state->pause_scan_code_read = false;
        }
        else if (scan_code == 0xE11D)
        {
            win32_state->pause_scan_code_read = true;
        }
        else if (scan_code == 0x54)
        {
            // NOTE(dan): alt + print screen returns 0x54, but we want 0xE037 for GetKeyNameText
            scan_code = 0xE037;
        }

        if (!(scan_code == 0xE11D || scan_code == 0xE02A || scan_code == 0xE0AA || scan_code == 0xE0B6 || scan_code == 0xE036))
        {
            u32 offset = get_scan_code_offset(scan_code);
            b32 is_down = !(flags & 1 /* RI_KEY_BREAK */);

            win32_update_button(&input->buttons[offset], is_down);
        }
    }
}

static void win32_get_text_input(PlatformInput *input, uintptr wparam)
{
    if (input->text_input_length < (array_count(input->text_input) - 1))
    {
        input->text_input[input->text_input_length++] = (unsigned short)wparam;
        input->text_input[input->text_input_length] = 0;
    }
}

static void win32_process_messages(PlatformInput *input)
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

            case 0x00FF /* WM_INPUT */:
            {
                win32_get_raw_input(input, msg.lParam);
            } break;

            case 0x0102 /* WM_CHAR */:
            {
                win32_get_text_input(input, msg.wParam);
            } break;

            default:
            {
                win32_api->TranslateMessage(&msg);
                win32_api->DispatchMessageA(&msg);
            } break;
        }
    }
}

static void win32_update_input(PlatformInput *input, f32 dt)
{
    input->dt = dt;
    input->text_input_length = 0;
    input->delta_mouse_pos[0] = 0;
    input->delta_mouse_pos[1] = 0;
    input->delta_wheel = 0;
    input->wheel = 0;

    win32_process_messages(input);

    input->wheel += input->delta_wheel;

    Win32Api_POINT mouse_p;
    win32_api->GetCursorPos(&mouse_p);
    win32_api->ScreenToClient(win32_state->window.wnd, &mouse_p);

    input->mouse_pos[0] = mouse_p.x;
    input->mouse_pos[1] = mouse_p.y;
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
            state->fill_sound_buffer = (FillSoundBuffer *)win32_get_proc_address(state->app_dll, "fill_sound_buffer");
        }
    }

    if (!state->update_and_render)
    {
        state->update_and_render = update_and_render_stub;
    }
    if (!state->fill_sound_buffer)
    {
        state->fill_sound_buffer = fill_sound_buffer_stub;
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
    state->fill_sound_buffer = fill_sound_buffer_stub;
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

static void win32_init_rawinput(Win32State *state)
{
    Win32Api_RAWINPUTDEVICE device[2] = {0};
    device[0].usUsagePage = 0x01 /* HID_USAGE_PAGE_GENERIC */;
    device[0].usUsage = 0x02 /* HID_USAGE_GENERIC_MOUSE */;
    device[0].hwndTarget = state->window.wnd;

    device[1].usUsagePage = 0x01 /* HID_USAGE_PAGE_GENERIC */;
    device[1].usUsage = 0x06 /* HID_USAGE_GENERIC_KEYBOARD */;
    device[1].hwndTarget = state->window.wnd;

    b32 registered = win32_api->RegisterRawInputDevices(device, 2, sizeof(device[0]));
    assert(registered);
}

//

// #define MAX_AUDIO_BUFFER_CHANNEL (500 * 44100 / 1000)           // NOTE(dan): 500 ms buffer
// #define MAX_AUDIO_BUFFER         (MAX_AUDIO_BUFFER_CHANNEL * 2) // NOTE(dan): 2 channels

// static float audio_buffer[MAX_AUDIO_BUFFER];
// static unsigned int volatile audio_read_pos;

// static void win32_fill_sound_buffer(float *buffer, unsigned int num_samples)
// {
//     unsigned int read_pos = audio_read_pos;
//     for (unsigned int float_index = 0; float_index < num_samples; float_index += 2)
//     {
//         buffer[float_index]     = audio_buffer[read_pos];
//         buffer[float_index + 1] = audio_buffer[read_pos + 1];

//         audio_buffer[read_pos] = 0;
//         audio_buffer[read_pos + 1] = 0;

//         read_pos += 2;

//         if (read_pos >= MAX_AUDIO_BUFFER)
//         {
//             read_pos = 0;
//         }
//     }
//     audio_read_pos = read_pos;
// }

// //

// static float audio_temp_buffer[MAX_AUDIO_BUFFER];
// static int audio_write_start_pos;
// static int audio_write_end_pos;
// static int prev_audio_read_pos;

// static void win32_update_audio()
// {
//     // pre
//     unsigned int num_samples = ((audio_write_end_pos - audio_write_start_pos) + MAX_AUDIO_BUFFER) % MAX_AUDIO_BUFFER;

//     // update
//     win32_state->fill_sound_buffer(&win32_state->app_memory, audio_temp_buffer, num_samples);


//     // post
//     int cur_read_pos = audio_read_pos;
//     int advanced = (cur_read_pos - prev_audio_read_pos + MAX_AUDIO_BUFFER) % MAX_AUDIO_BUFFER;

//     audio_write_start_pos = (audio_write_start_pos + advanced + MAX_AUDIO_BUFFER) % MAX_AUDIO_BUFFER;
//     audio_write_end_pos   = (audio_write_end_pos   + advanced + MAX_AUDIO_BUFFER) % MAX_AUDIO_BUFFER;

//     assert(audio_write_start_pos == (cur_read_pos + (50 * 44100 / 1000) * 2 + MAX_AUDIO_BUFFER) % MAX_AUDIO_BUFFER);
//     prev_audio_read_pos = cur_read_pos;

//     unsigned int write_pos = audio_write_start_pos;
//     for (unsigned int float_index = 0; float_index < num_samples; float_index += 2)
//     {
//         audio_buffer[write_pos]     = audio_temp_buffer[float_index];
//         audio_buffer[write_pos + 1] = audio_temp_buffer[float_index + 1];

//         write_pos += 2;

//         if (write_pos >= MAX_AUDIO_BUFFER)
//         {
//             write_pos = 0;
//         }
//     }
// }

static int win32_sound_thread_proc(void *param)
{
    void *buffer_ready_event = win32_api->CreateEventA(0, 0, 0, 0);
    if (win32_state->audio_client->vtbl->SetEventHandle(win32_state->audio_client, buffer_ready_event) >= 0)
    {
        unsigned int buffer_frame_count;
        if (win32_state->audio_client->vtbl->GetBufferSize(win32_state->audio_client, &buffer_frame_count) >= 0)
        {
            if (win32_state->audio_client->vtbl->Start(win32_state->audio_client) >= 0)
            {
                for (;;)
                {
                    if (win32_api->WaitForSingleObject(buffer_ready_event, 0xFFFFFFFF /* INFINITE */) != 0 /* WAIT_OBJECT_0 */)
                    {
                        break;
                    }

                    unsigned int padding_frame_count;
                    if (win32_state->audio_client->vtbl->GetCurrentPadding(win32_state->audio_client, &padding_frame_count) < 0)
                    {
                        break;
                    }

                    f32 *buffer;
                    unsigned int fill_frame_count = buffer_frame_count - padding_frame_count;
                    if (win32_state->audio_render->vtbl->GetBuffer(win32_state->audio_render, fill_frame_count, (unsigned char **)&buffer) < 0)
                    {
                        break;
                    }

                    u32 num_samples = fill_frame_count * 2;
                    win32_state->fill_sound_buffer(&win32_state->app_memory, buffer, num_samples);

                    if (win32_state->audio_render->vtbl->ReleaseBuffer(win32_state->audio_render, fill_frame_count, 0) < 0)
                    {
                        break;
                    }
                }
            }
        }
    }
    win32_state->audio_client->vtbl->Stop(win32_state->audio_client);
    return 0;
}

static void win32_init_audio(Win32State *state)
{
    Win32Api_WAVEFORMATEX audio_format = { 3 /* WAVE_FORMAT_IEEE_FLOAT */, 2, 44100, 44100 * 8, 8, 32, 0 };
    u64 buffer_duration = 20 * 1000 * 10;

    Win32Api_IMMDeviceEnumerator *device_enumerator = 0;
    Win32Api_IMMDevice *audio_device = 0;

    win32_api->CoInitializeEx(0, 0 /* COINITBASE_MULTITHREADED */);

    if (win32_api->CoCreateInstance(&Win32Api_uuid_MMDeviceEnumerator, 0, 0x1 /* CLSCTX_INPROC_SERVER */, &Win32Api_uuid_IMMDeviceEnumerator, (void **)&device_enumerator) >= 0)
    {
        if (device_enumerator->vtbl->GetDefaultAudioEndpoint(device_enumerator, 0 /* eRender */, 0 /* eConsole */, (void **)&audio_device) >= 0)
        {
            if (audio_device->vtbl->Activate(audio_device, &WIn32Api_uuid_IAudioClient, 0x1 /* CLSCTX_INPROC_SERVER */, 0, (void **)&state->audio_client) >= 0)
            {
                int flags = 0x00040000 /* AUDCLNT_STREAMFLAGS_EVENTCALLBACK */ 
                          | 0x00100000 /* AUDCLNT_STREAMFLAGS_RATEADJUST */ 
                          | 0x80000000 /* AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM */;
                if (state->audio_client->vtbl->Initialize(state->audio_client, 0 /* AUDCLNT_SHAREMODE_SHARED */, flags, buffer_duration, 0, &audio_format, 0) >= 0)
                {
                    if (state->audio_client->vtbl->GetService(state->audio_client, &Win32Api_uuid_IAudioRenderClient, (void **)&state->audio_render) >= 0)
                    {
                        state->sound_sample_rate = audio_format.nSamplesPerSec;
           
                        void *thread = win32_api->CreateThread(0, 0, win32_sound_thread_proc, 0, 0, 0);
                        if (thread)
                        {
                            win32_api->SetThreadPriority(thread, 2 /* THREAD_PRIORITY_HIGHEST */);
                        }
                    }
                }
            }
        }
    }

    if (device_enumerator)
    {
        device_enumerator->vtbl->Release(device_enumerator);
    }
    if (audio_device)
    {
        audio_device->vtbl->Release(audio_device);
    }
}

int __stdcall WinMain(HINSTANCE instance, HINSTANCE prev_instance, char *cmd_line, int cmd_show)
{
    win32_state->app_memory.platform.allocate = win32_allocate;
    win32_state->app_memory.platform.deallocate = win32_deallocate;
    win32_state->app_memory.platform.get_memory_stats = win32_get_memory_stats;
    win32_state->app_memory.platform.init_opengl = win32_init_opengl;

    win32_state->memory_sentinel.prev = &win32_state->memory_sentinel;
    win32_state->memory_sentinel.next = &win32_state->memory_sentinel;

    win32_init_win32_api(win32_api);
    win32_init_exe_path(win32_state);
    win32_init_rawinput(win32_state);
    win32_load_app_dll(win32_state);
    win32_init_audio(win32_state);

    win32_state->window = win32_open_window_init_with_opengl("Talkien", 1280, 720, win32_window_proc);
    if (win32_state->window.rc)
    {
        win32_set_vsync(true);

        f32 t0 = win32_get_time();
        f32 dt = 0;

        win32_state->running = true;
        while (win32_state->running)
        {
            win32_update_input(&win32_state->input, dt);
            
            win32_state->update_and_render(&win32_state->app_memory, &win32_state->input, win32_state->window.width, win32_state->window.height);

            win32_reload_app_dll_if_needed(win32_state);

            if (win32_state->input.quit_requested)
            {
                win32_state->running = false;
            }

            win32_api->SwapBuffers(win32_state->window.dc);
            f32 t1 = win32_get_time();
            dt = t1 - t0;
            t0 = t1;
        }
    }
    
    return 0;
}
