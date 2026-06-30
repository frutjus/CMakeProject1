#version 430 core

in vec3 coord;
uniform vec2 camera;
uniform vec2 resolution;
uniform float pixels_per_tile;

void main(void) {
  vec3 origin_for_camera = vec3(camera - resolution/2.0f/pixels_per_tile, 0.0);
  gl_Position = vec4((coord - origin_for_camera)*pixels_per_tile/vec3(resolution, 1.0)*2.0-1.0, 1.0);
}