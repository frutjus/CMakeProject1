#include "opengl.h"
#include "defs.h"
#include "datatypes.h"

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
PFNGLUNIFORM4FPROC glUniform4f;

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
  ASSERT(compile_status == GL_TRUE);

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
  ASSERT(link_status == GL_TRUE);

  return prog;
}

void clear_background(colour c)
{
  glClearColor(c.r, c.g, c.b, c.a);
  glClear(GL_COLOR_BUFFER_BIT);
}


const char vs_str_generic[] =
"#version 430 core\n"
"in vec2 coord2d;"
"uniform vec2 resolution;"
"void main(void) {"
"  gl_Position = vec4(coord2d/resolution*2.0-1.0, 0.0, 1.0);"
"  /*gl_Position = vec4(coord2d, 0.0, 1.0);*/"
"}";

const char fs_str_generic[] =
"#version 430 core\n"
"out vec4 fragColor;"
"uniform vec4 colour;"
"void main(void) {"
"  fragColor = colour;"
"}";

void init_graphics_generic(gl_state* state)
{
  GLuint vs = new_shader(GL_VERTEX_SHADER, vs_str_generic);
  GLuint fs = new_shader(GL_FRAGMENT_SHADER, fs_str_generic);
  state->prog_generic = new_shader_program(vs, fs);

  glGenBuffers(1, &state->vbo_coord2d);
  CATCH_GL_ERROR(errstr);

  glUseProgram(state->prog_generic);
}

void draw_rectangle(gl_state* state, rectf r, colour c)
{
  float vertices[] = {
    r.x, r.y,
    r.x, r.y + r.h,
    r.x + r.w, r.y,
    r.x, r.y + r.h,
    r.x + r.w, r.y + r.h,
    r.x + r.w, r.y,
  };
  /*float vertices[] = {
    -1.0, -1.0,
    -1.0,  1.0,
     1.0, -1.0,
    -1.0,  1.0,
     1.0,  1.0,
     1.0, -1.0,
  };*/

  glBindBuffer(GL_ARRAY_BUFFER,state->vbo_coord2d);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  CATCH_GL_ERROR(errstr);

  glUseProgram(state->prog_generic);

  state->attr_coord2d = glGetAttribLocation(state->prog_generic, "coord2d");
  CATCH_GL_ERROR(errstr);

  glVertexAttribPointer(
    state->attr_coord2d,  // attribute
    2,                  // number of elements per vertex; here (x,y)
    GL_FLOAT,           // type of each element
    GL_FALSE,           // take our values as-is
    0,                  // no extra data between each position
    0                   // offset of first element    
  );
  CATCH_GL_ERROR(errstr);

  glUniform2f(glGetUniformLocation(state->prog_generic, "resolution"), state->resolution.x, state->resolution.y);
  glUniform4f(glGetUniformLocation(state->prog_generic, "colour"), c.r, c.g, c.b, c.a);

  glEnableVertexAttribArray(state->attr_coord2d);
  CATCH_GL_ERROR(errstr);

  glDrawArrays(GL_TRIANGLES, 0, 6);
  CATCH_GL_ERROR(errstr);

  glDisableVertexAttribArray(state->attr_coord2d);
  CATCH_GL_ERROR(errstr);
}