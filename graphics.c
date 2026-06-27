#include "graphics.h"
#include "defs.h"
#include "datatypes.h"
#include "allocator.h"

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
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;

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

INTERNAL
const char vs_str_2d_abs[] =
"#version 430 core\n"
"in vec2 coord2d;"
"uniform vec2 resolution;"
"void main(void) {"
"  gl_Position = vec4(coord2d/resolution*2.0-1.0, 0.0, 1.0);"
"}";

INTERNAL
const char fs_str_uniform[] =
"#version 430 core\n"
"out vec4 fragColor;"
"uniform vec4 colour;"
"void main(void) {"
"  fragColor = colour;"
"}";

INTERNAL
const char vs_str_tiles[] =
"#version 430 core\n"
"in vec3 coord;"
"uniform vec2 camera;"
"uniform vec2 resolution;"
"uniform float pixels_per_tile;"
"void main(void) {"
"  vec3 origin_for_camera = vec3(camera - resolution/2.0f/pixels_per_tile, 0.0);"
"  gl_Position = vec4((coord - origin_for_camera)*pixels_per_tile/vec3(resolution, 1.0)*2.0-1.0, 1.0);"
"}";

INTERNAL
const char fs_str_tiles[] =
"#version 430 core\n"
"layout(std430, binding = 0) buffer ssbo {"
"  int grid[];"
"};"
"out vec4 fragColor;"
"void main(void) {"
"  int alive = grid[gl_PrimitiveID/2];"
"  if (alive == 1) {"
"    fragColor = vec4(220.0f/255.0f, 191.0f/255.0f, 1.0f/255.0f, 1.0);"
"  } else {"
"    fragColor = vec4(88.0f/255.0f, 57.0f/255.0f, 39.0f/255.0f, 1.0);"
"  }"
"}";

INTERNAL
const char fs_str_borders[] =
"#version 430 core\n"
"out vec4 fragColor;"
"uniform float pixels_per_tile;"
"void main(void) {"
"  float alpha = clamp(pixels_per_tile / 100.0, 0.0, 1.0);"
"  fragColor = vec4(0.125, 0.125, 0.125, alpha);"
"}";

void init_graphics(gl_state* state, allocator scratch, int grid_size)
{
  GLuint vs = new_shader(GL_VERTEX_SHADER, vs_str_tiles);
  GLuint fs = new_shader(GL_FRAGMENT_SHADER, fs_str_tiles);
  state->prog_cells = new_shader_program(vs, fs);

  fs = new_shader(GL_FRAGMENT_SHADER, fs_str_borders);
  state->prog_borders = new_shader_program(vs, fs);

  vs = new_shader(GL_VERTEX_SHADER, vs_str_2d_abs);
  fs = new_shader(GL_FRAGMENT_SHADER, fs_str_uniform);
  state->prog_rect = new_shader_program(vs, fs);

  long long int vertices_size = sizeof(vec3f) * (grid_size + 1) * (grid_size + 1);
  vec3f *vertices = alloc(&scratch, vertices_size);

  long long int cell_elements_size = 6 * sizeof(GLuint) * (grid_size) * (grid_size);
  struct { GLuint bl, tl1, br1, br2, tl2, tr; } *cell_elements = alloc(&scratch, cell_elements_size);

  long long int border_elements_size = 8 * sizeof(GLuint) * (grid_size) * (grid_size);
  struct { GLuint l1, l2, t1, t2, r1, r2, b1, b2; } *border_elements = alloc(&scratch, border_elements_size);

  for (int i = 0; i <= grid_size; i++)
  {
    for (int j = 0; j <= grid_size; j++)
    {
      vertices[i * (grid_size + 1) + j] = (vec3f){ (float)j, (float)i, 0.0f };

      if (i < grid_size && j < grid_size)
      {
        GLuint bl = (i)     * (grid_size + 1) + j;
        GLuint tl = (i + 1) * (grid_size + 1) + j;
        GLuint br = (i)     * (grid_size + 1) + j + 1;
        GLuint tr = (i + 1) * (grid_size + 1) + j + 1;

        cell_elements[i * grid_size + j].bl  = bl;
        cell_elements[i * grid_size + j].tl1 = tl;
        cell_elements[i * grid_size + j].tl2 = tl;
        cell_elements[i * grid_size + j].tr  = tr;
        cell_elements[i * grid_size + j].br1 = br;
        cell_elements[i * grid_size + j].br2 = br;

        border_elements[i * grid_size + j].l1 = bl;
        border_elements[i * grid_size + j].l2 = tl;
        border_elements[i * grid_size + j].t1 = tl;
        border_elements[i * grid_size + j].t2 = tr;
        border_elements[i * grid_size + j].r1 = tr;
        border_elements[i * grid_size + j].r2 = br;
        border_elements[i * grid_size + j].b1 = br;
        border_elements[i * grid_size + j].b2 = bl;
      }
    }
  }

  glGenBuffers(5, &state->buffers);
  CATCH_GL_ERROR(errstr);

  glGenVertexArrays(3, &state->vertex_arrays);
  
  glBindVertexArray(state->vao_cells);
  {
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo_grid);
    glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices, GL_STATIC_DRAW);
    CATCH_GL_ERROR(errstr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ebo_cells);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cell_elements_size, cell_elements, GL_STATIC_DRAW);
    CATCH_GL_ERROR(errstr);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, state->ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, state->ssbo);

    glUseProgram(state->prog_cells);
    GLuint attr = glGetAttribLocation(state->prog_cells, "coord");
    CATCH_GL_ERROR(errstr);

    glVertexAttribPointer(attr, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    CATCH_GL_ERROR(errstr);

    glEnableVertexAttribArray(attr);
    CATCH_GL_ERROR(errstr);
  }

  glBindVertexArray(state->vao_borders);
  {
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo_grid);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ebo_borders);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, border_elements_size, border_elements, GL_STATIC_DRAW);
    CATCH_GL_ERROR(errstr);

    glUseProgram(state->prog_borders);
    GLuint attr = glGetAttribLocation(state->prog_borders, "coord");
    CATCH_GL_ERROR(errstr);

    glVertexAttribPointer(attr, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    CATCH_GL_ERROR(errstr);

    glEnableVertexAttribArray(attr);
    CATCH_GL_ERROR(errstr);
  }

  glBindVertexArray(state->vao_rect);
  {
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo_rect);

    glUseProgram(state->prog_rect);
    GLuint attr = glGetAttribLocation(state->prog_rect, "coord2d");
    CATCH_GL_ERROR(errstr);

    glVertexAttribPointer(attr, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    CATCH_GL_ERROR(errstr);

    glEnableVertexAttribArray(attr);
    CATCH_GL_ERROR(errstr);
  }

  glBindVertexArray(0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

void render_grid(gl_state* state, int* grid, int grid_size)
{
  glUseProgram(state->prog_cells);

  glUniform2f(glGetUniformLocation(state->prog_cells, "camera"), state->camera.x, state->camera.y);
  glUniform2f(glGetUniformLocation(state->prog_cells, "resolution"), state->resolution.x, state->resolution.y);
  glUniform1f(glGetUniformLocation(state->prog_cells, "pixels_per_tile"), state->pixels_per_tile);

  glBindVertexArray(state->vao_cells);

  glBufferData(GL_SHADER_STORAGE_BUFFER, grid_size * grid_size * sizeof(*grid), grid, GL_DYNAMIC_COPY);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  glDrawElements(GL_TRIANGLES, 6 * grid_size * grid_size, GL_UNSIGNED_INT, NULL);
  CATCH_GL_ERROR(errstr);

  glBindVertexArray(0);
}

void render_grid_borders(gl_state* state, int grid_size)
{
  glUseProgram(state->prog_borders);

  glUniform2f(glGetUniformLocation(state->prog_borders, "camera"), state->camera.x, state->camera.y);
  glUniform2f(glGetUniformLocation(state->prog_borders, "resolution"), state->resolution.x, state->resolution.y);
  glUniform1f(glGetUniformLocation(state->prog_borders, "pixels_per_tile"), state->pixels_per_tile);

  glBindVertexArray(state->vao_borders);

  glDrawElements(GL_LINES, 8 * grid_size * grid_size, GL_UNSIGNED_INT, NULL);
  CATCH_GL_ERROR(errstr);

  glBindVertexArray(0);
}

void render_pause_icon(gl_state* gl_st)
{
  float pause_icon_width = 20.0f;
  float pause_icon_height = 20.0f;
  float pause_icon_pos_x = gl_st->resolution.x - pause_icon_width - 10.0f;
  float pause_icon_pos_y = gl_st->resolution.y - pause_icon_height - 10.0f;
  rectf pause_l = {pause_icon_pos_x,      pause_icon_pos_y, 6.0f, 20.0f};
  rectf pause_r = {pause_icon_pos_x + 14, pause_icon_pos_y, 6.0f, 20.0f};
  draw_rectangle(gl_st, pause_l, (colour){ 1.0f, 1.0f, 1.0f, 1.0f });
  draw_rectangle(gl_st, pause_r, (colour){ 1.0f, 1.0f, 1.0f, 1.0f });
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
  glBindVertexArray(state->vao_rect);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  CATCH_GL_ERROR(errstr);

  glUseProgram(state->prog_rect);

  glUniform2f(glGetUniformLocation(state->prog_rect, "resolution"), state->resolution.x, state->resolution.y);
  glUniform4f(glGetUniformLocation(state->prog_rect, "colour"), c.r, c.g, c.b, c.a);

  glDrawArrays(GL_TRIANGLES, 0, 6);
  CATCH_GL_ERROR(errstr);

  glBindVertexArray(0);
}