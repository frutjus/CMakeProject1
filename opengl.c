#include "opengl.h"
#include "defs.h"

PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLISSHADERPROC glIsShader;
PFNGLISPROGRAMPROC glIsProgram;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM1FVPROC glUniform1fv;
PFNGLNAMEDBUFFERDATAPROC glNamedBufferData;
PFNGLENABLEVERTEXARRAYATTRIBPROC glEnableVertexArrayAttrib;
PFNGLDISABLEVERTEXARRAYATTRIBPROC glDisableVertexArrayAttrib;
PFNGLCREATEBUFFERSPROC glCreateBuffers;
PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
PFNGLUNIFORM1IVPROC glUniform1iv;
PFNGLBINDBUFFERBASEPROC glBindBufferBase;
PFNGLMEMORYBARRIERPROC glMemoryBarrier;

char info_log[1024];

char* get_gl_log(GLuint object)
{
  GLint log_length = 0;
  if (glIsShader(object))
  {
    glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
    ASSERT(log_length < ARRAY_COUNT(info_log));
    glGetShaderInfoLog(object, ARRAY_COUNT(info_log), NULL, info_log);
  }
  else if (glIsProgram(object))
  {
    glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
    ASSERT(log_length < ARRAY_COUNT(info_log));
    glGetProgramInfoLog(object, ARRAY_COUNT(info_log), NULL, info_log);
  }
  else
  {
    UNIMPLEMENTED("gl object not implemented")
  }

  return info_log;
}

GLuint new_shader(GLenum type, const char* source)
{
  GLuint sh = glCreateShader(type);
  glShaderSource(sh, 1, &source, NULL);
  glCompileShader(sh);

  GLint compile_status;
  glGetShaderiv(sh, GL_COMPILE_STATUS, &compile_status);
  char *log = get_gl_log(sh);
  if (compile_status == GL_FALSE)
  {
    ASSERT(0);
  }

  return sh;
}

GLuint new_shader_program(GLuint vertex, GLuint fragment)
{
  GLuint prog = glCreateProgram();
  glAttachShader(prog, vertex);
  glAttachShader(prog, fragment);
  glLinkProgram(prog);

  GLint link_status;
  glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
  char *log = get_gl_log(prog);
  if (link_status == GL_FALSE)
  {
    ASSERT(0);
  }

  glUseProgram(prog);

  return prog;

  //prog->uniform_resolution = glGetUniformLocation(prog->ID, "resolution");
}

//void set_uniform_value(const char *name, float val);

void clear_background(colour c)
{
  glClearColor(c.r, c.g, c.b, c.a);
  glClear(GL_COLOR_BUFFER_BIT);
}
