#pragma once

//#define WINGDIAPI __declspec(dllimport)
//#define APIENTRY __stdcall

#include "lib/gl.h"
#include "lib/glext.h"
#include "lib/glu.h"
#include "defs.h"
#include "datatypes.h"
#include "allocator.h"

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
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;

#ifdef DEBUG
#define CATCH_GL_ERROR(errstr)                             \
for (GLenum err = glGetError(); err != GL_NO_ERROR; false) \
{                                                          \
  const char *errstr = gluErrorString(err);                \
  ASSERT(0);                                               \
}
#else
#define CATCH_GL_ERROR(errstr)
#endif

typedef struct {
  float r,g,b,a;
} colour;

//typedef struct {
//  GLuint vao;
//  GLuint prog;
//} renderer;

typedef struct {
  // shader programs
  GLuint prog_cells, prog_borders, prog_rect;
  // buffer objects
  union {
    GLuint buffers;
    struct {
      GLuint
        vbo_grid, vbo_rect,
        ebo_cells, ebo_borders,
        ssbo;
    };
  };
  // state objects
  union {
    GLuint vertex_arrays;
    struct {
      GLuint
        vao_cells, vao_borders, vao_rect;
    };
  };
  // parameters
  vec2f camera;
  vec2f resolution;
  float pixels_per_tile;
} gl_state;

GLuint new_shader(GLenum type, const char* source);

GLuint new_shader_program(GLuint vertex, GLuint fragment);

void clear_background(colour c);

void init_graphics(gl_state* state, allocator scratch, int grid_size);

void render_grid(gl_state* state, int* grid, int grid_size);

void render_grid_borders(gl_state* state, int grid_size);

void render_pause_icon(gl_state* gl_st);

void draw_rectangle(gl_state* state, rectf r, colour c);