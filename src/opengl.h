#include <gl/gl.h>

// TODO(dan): replace glext.h
#include "glext.h"

#define GLARB(a, b) GLE(a##ARB, b##ARB)
#define GLEXT(a, b) GLE(a##EXT, b##EXT)

#define GLE(a, b) PFNGL##a##PROC gl##b;
#include GL_EXTLIST
#undef GLE

inline void opengl_init_extensions()
{
    #define GLE(a, b) gl##b = (PFNGL##a##PROC)AGL_GET_FUNC("gl" #b);
    #include GL_EXTLIST
    #undef GLE
}
