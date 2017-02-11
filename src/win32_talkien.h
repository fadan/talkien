
// TODO(dan): replace windows.h
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#include "windows.h"

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
