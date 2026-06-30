#version 430 core

in vec2 coord2d;
uniform vec2 resolution;

void main(void) {
  gl_Position = vec4(coord2d/resolution*2.0-1.0, 0.0, 1.0);
}