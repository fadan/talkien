#define GL_TRUE     1
#define GL_FALSE    0

#define GL_FLOAT            0x1406
#define GL_UNSIGNED_BYTE    0x1401
#define GL_UNSIGNED_SHORT   0x1403
#define GL_UNSIGNED_INT     0x1405

#define GL_OBJECT_COMPILE_STATUS_ARB      0x8B81
#define GL_VERTEX_SHADER_ARB              0x8B31
#define GL_FRAGMENT_SHADER_ARB            0x8B30
#define GL_OBJECT_LINK_STATUS_ARB         0x8B82
#define GL_ARRAY_BUFFER_ARB               0x8892

#define GL_TEXTURE_2D                     0x0DE1
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_LINEAR                         0x2601
#define GL_UNPACK_ROW_LENGTH              0x0CF2
#define GL_RGBA                           0x1908
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_BLEND                          0x0BE2
#define GL_FUNC_ADD                       0x8006
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_CULL_FACE                      0x0B44
#define GL_DEPTH_TEST                     0x0B71
#define GL_TEXTURE0                       0x84C0
#define GL_ARRAY_BUFFER                   0x8892
#define GL_STREAM_DRAW                    0x88E0
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_SCISSOR_TEST                   0x0C11

#define GL_TRIANGLES                      0x0004

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

typedef intptr GLsizeiptr;
typedef intptr GLsizeiptrARB;
typedef uint GLhandleARB;
typedef char GLcharARB;

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

typedef void (__stdcall * PFNGLATTACHOBJECTARBPROC) (GLhandleARB containerObj, GLhandleARB obj);
typedef void (__stdcall * PFNGLBINDATTRIBLOCATIONARBPROC) (GLhandleARB programObj, GLuint index, const GLcharARB *name);
typedef void (__stdcall * PFNGLCOMPILESHADERARBPROC) (GLhandleARB shaderObj);
typedef GLhandleARB (__stdcall * PFNGLCREATEPROGRAMOBJECTARBPROC) (void);
typedef GLhandleARB (__stdcall * PFNGLCREATESHADEROBJECTARBPROC) (GLenum shaderType);
typedef void (__stdcall * PFNGLDELETEOBJECTARBPROC) (GLhandleARB obj);
typedef void (__stdcall * PFNGLDETACHOBJECTARBPROC) (GLhandleARB containerObj, GLhandleARB attachedObj);
typedef void (__stdcall * PFNGLGETINFOLOGARBPROC) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
typedef void (__stdcall * PFNGLGETOBJECTPARAMETERIVARBPROC) (GLhandleARB obj, GLenum pname, GLint *params);
typedef void (__stdcall * PFNGLLINKPROGRAMARBPROC) (GLhandleARB programObj);
typedef void (__stdcall * PFNGLUSEPROGRAMOBJECTARBPROC) (GLhandleARB programObj);
typedef void (__stdcall * PFNGLSHADERSOURCEARBPROC) (GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length);
typedef GLint (__stdcall * PFNGLGETATTRIBLOCATIONARBPROC) (GLhandleARB programObj, const GLcharARB *name);
typedef GLint (__stdcall * PFNGLGETUNIFORMLOCATIONARBPROC) (GLhandleARB programObj, const GLcharARB *name);
typedef void (__stdcall * PFNGLUNIFORM1IARBPROC) (GLint location, GLint v0);
typedef void (__stdcall * PFNGLUNIFORM1FARBPROC) (GLint location, GLfloat v0);
typedef void (__stdcall * PFNGLUNIFORMMATRIX4FVARBPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (__stdcall * PFNGLUNIFORM4FVARBPROC) (GLint location, GLsizei count, const GLfloat *value);

#define gdi_import extern "C" __declspec(dllimport)

gdi_import void __stdcall glGenTextures(GLsizei n, GLuint *textures);
gdi_import void __stdcall glBindTexture (GLenum target, GLuint texture);
gdi_import void __stdcall glTexParameteri (GLenum target, GLenum pname, GLint param);
gdi_import void __stdcall glPixelStorei (GLenum pname, GLint param);
gdi_import void __stdcall glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
gdi_import void __stdcall glViewport (GLint x, GLint y, GLsizei width, GLsizei height);
gdi_import void __stdcall glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
gdi_import void __stdcall glClear (GLbitfield mask);
gdi_import void __stdcall glEnable (GLenum cap);
gdi_import void __stdcall glBlendFunc (GLenum sfactor, GLenum dfactor);
gdi_import void __stdcall glDisable (GLenum cap);
gdi_import void __stdcall glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
gdi_import void __stdcall glScissor (GLint x, GLint y, GLsizei width, GLsizei height);

#define GL_EXTENSIONS_LIST \
    GLARB(GENBUFFERS,               GenBuffers) \
    GLARB(BINDBUFFER,               BindBuffer) \
    GLARB(BUFFERDATA,               BufferData) \
    GLE  (GENVERTEXARRAYS,          GenVertexArrays) \
    GLE  (BINDVERTEXARRAY,          BindVertexArray) \
    GLARB(ACTIVETEXTURE,            ActiveTexture) \
    GLE  (BLENDEQUATION,            BlendEquation) \
    GLARB(ENABLEVERTEXATTRIBARRAY,  EnableVertexAttribArray) \
    GLARB(DISABLEVERTEXATTRIBARRAY, DisableVertexAttribArray) \
    GLARB(VERTEXATTRIBPOINTER,      VertexAttribPointer) \
    \
    GLARB(ATTACHOBJECT,             AttachObject) \
    GLARB(BINDATTRIBLOCATION,       BindAttribLocation) \
    GLARB(COMPILESHADER,            CompileShader) \
    GLARB(CREATEPROGRAMOBJECT,      CreateProgramObject) \
    GLARB(CREATESHADEROBJECT,       CreateShaderObject) \
    GLARB(DELETEOBJECT,             DeleteObject) \
    GLARB(DETACHOBJECT,             DetachObject) \
    GLARB(GETINFOLOG,               GetInfoLog) \
    GLARB(GETOBJECTPARAMETERIV,     GetObjectParameteriv) \
    GLARB(LINKPROGRAM,              LinkProgram) \
    GLARB(USEPROGRAMOBJECT,         UseProgramObject) \
    GLARB(SHADERSOURCE,             ShaderSource) \
    GLARB(GETATTRIBLOCATION,        GetAttribLocation) \
    GLARB(GETUNIFORMLOCATION,       GetUniformLocation) \
    GLARB(UNIFORM1I,                Uniform1i) \
    GLARB(UNIFORM1F,                Uniform1f) \
    GLARB(UNIFORMMATRIX4FV,         UniformMatrix4fv) \
    GLARB(UNIFORM4FV,               Uniform4fv)

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

static GLuint opengl_compile_shader(char const *source, GLenum type, char *error, u32 error_length)
{
    GLuint handle = glCreateShaderObjectARB(type);
    glShaderSourceARB(handle, 1, &source, 0);
    glCompileShaderARB(handle);

    GLint status;
    glGetObjectParameterivARB(handle, GL_OBJECT_COMPILE_STATUS_ARB, &status);

    if (!status)
    {
        glGetInfoLogARB(handle, error_length, 0, error);
        invalid_codepath;
    }

    return handle;
}

static GLuint opengl_create_program(char *vertex_source, char *fragment_source, char *error, u32 error_length)
{
    GLuint handle = glCreateProgramObjectARB();

    if (vertex_source)
    {
        GLuint vertex_shader = opengl_compile_shader(vertex_source, GL_VERTEX_SHADER_ARB, error, error_length);
        glAttachObjectARB(handle, vertex_shader);
        glDeleteObjectARB(vertex_shader);
    }
    if (fragment_source)
    {
        GLuint fragment_shader = opengl_compile_shader(fragment_source, GL_FRAGMENT_SHADER_ARB, error, error_length);
        glAttachObjectARB(handle, fragment_shader);
        glDeleteObjectARB(fragment_shader);
    }

    glLinkProgramARB(handle);

    GLint status;
    glGetObjectParameterivARB(handle, GL_OBJECT_LINK_STATUS_ARB, &status);

    if (!status)
    {
        glGetInfoLogARB(handle, error_length, 0, error);
        invalid_codepath;
    }

    return handle;
}
