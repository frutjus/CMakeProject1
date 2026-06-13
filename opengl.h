#pragma once

#include "lib/gl.h"
#include "lib/glext.h"
#include "lib/glu.h"
#include "defs.h"
#include "datatypes.h"

extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLISSHADERPROC glIsShader;
extern PFNGLISPROGRAMPROC glIsProgram;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM2FPROC glUniform2f;
extern PFNGLUNIFORM1FVPROC glUniform1fv;
extern PFNGLNAMEDBUFFERDATAPROC glNamedBufferData;
extern PFNGLENABLEVERTEXARRAYATTRIBPROC glEnableVertexArrayAttrib;
extern PFNGLDISABLEVERTEXARRAYATTRIBPROC glDisableVertexArrayAttrib;
extern PFNGLCREATEBUFFERSPROC glCreateBuffers;
extern PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
extern PFNGLUNIFORM1IVPROC glUniform1iv;
extern PFNGLBINDBUFFERBASEPROC glBindBufferBase;
extern PFNGLMEMORYBARRIERPROC glMemoryBarrier;
extern PFNGLUNIFORM4FPROC glUniform4f;

#define CATCH_GL_ERROR(errstr)                             \
for (GLenum err = glGetError(); err != GL_NO_ERROR; false) \
{                                                          \
  const char *errstr = gluErrorString(err);                \
  ASSERT(0);                                               \
}

typedef struct {
  float r,g,b,a;
} colour;

typedef struct {
  GLuint prog_generic, vbo_coord2d, attr_coord2d;
  vecf2 resolution;
} gl_state;

GLuint new_shader(GLenum type, const char* source);

GLuint new_shader_program(GLuint vertex, GLuint fragment);

void clear_background(colour c);

void init_graphics_generic(gl_state* state);

void draw_rectangle(gl_state* state, rectf r, colour c);