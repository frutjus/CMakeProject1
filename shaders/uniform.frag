#version 430 core

out vec4 fragColor;
uniform vec4 colour;

void main(void) {
  fragColor = colour;
}