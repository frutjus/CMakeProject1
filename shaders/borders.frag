#version 430 core

out vec4 fragColor;
uniform float pixels_per_tile;

void main(void) {
  float alpha = clamp(pixels_per_tile / 100.0, 0.0, 1.0);
  fragColor = vec4(0.125, 0.125, 0.125, alpha);
}