#define GL_TRUE     1
#define GL_FALSE    0

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;

typedef intptr GLsizeiptrARB;

typedef void (__stdcall * PFNGLGENBUFFERSARBPROC) (GLsizei n, GLuint *buffers);
typedef void (__stdcall * PFNGLBINDBUFFERARBPROC) (GLenum target, GLuint buffer);
typedef void (__stdcall * PFNGLBUFFERDATAARBPROC) (GLenum target, GLsizeiptrARB size, const void *data, GLenum usage);
typedef void (__stdcall * PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
typedef void (__stdcall * PFNGLBINDVERTEXARRAYPROC) (GLuint array);
typedef void (__stdcall * PFNGLACTIVETEXTUREARBPROC) (GLenum texture);
typedef void (__stdcall * PFNGLBLENDEQUATIONPROC) (GLenum mode);
typedef void (__stdcall * PFNGLENABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (__stdcall * PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (__stdcall * PFNGLVERTEXATTRIBPOINTERARBPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);

#define GL_EXTENSIONS_LIST \
    GLARB(GENBUFFERS,               GenBuffers) \
    GLARB(BINDBUFFER,               BindBuffer) \
    GLARB(BUFFERDATA,               BufferData) \
      GLE(GENVERTEXARRAYS,          GenVertexArrays) \
      GLE(BINDVERTEXARRAY,          BindVertexArray) \
    GLARB(ACTIVETEXTURE,            ActiveTexture) \
      GLE(BLENDEQUATION,            BlendEquation) \
    GLARB(ENABLEVERTEXATTRIBARRAY,  EnableVertexAttribArray) \
    GLARB(DISABLEVERTEXATTRIBARRAY, DisableVertexAttribArray) \
    GLARB(VERTEXATTRIBPOINTER,      VertexAttribPointer)

#define GLARB(a, b) GLE(a##ARB, b##ARB)
#define GLEXT(a, b) GLE(a##EXT, b##EXT)

#define GLE(a, b) PFNGL##a##PROC gl##b;
GL_EXTENSIONS_LIST
#undef GLE

inline void opengl_init_extensions()
{
    #define GLE(a, b) gl##b = (PFNGL##a##PROC)GL_GET_FUNC("gl" #b);
    GL_EXTENSIONS_LIST
    #undef GLE
}
