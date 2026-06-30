#version 430 core

layout(std430, binding = 0) buffer ssbo {
  int tilemap[];
};

out vec4 fragColor;

void main(void) {
  int terrain = tilemap[gl_PrimitiveID/2];
  vec3 colour;
  switch (terrain) {
    case 0: /* empty */
      colour = vec3(0.0, 0.0, 0.0);
      break;
    case 1: /* grass */
      colour = vec3(91.0f/255.0f, 148.0f/255.0f, 82.0f/255.0f);
      break;
    case 2: /* desert */
      colour = vec3(254.0f/255.0f, 246.0f/255.0f, 163.0f/255.0f);
      break;
    case 3: /* mountain */
      colour = vec3(198.0f/255.0f, 195.0f/255.0f, 191.0f/255.0f);
      break;
    case 4: /* water */
      colour = vec3(153.0f/255.0f, 192.0f/255.0f, 227.0f/255.0f);
      break;
    default:
      colour = vec3(1.0, 0.0, 1.0);
  }
  fragColor = vec4(colour, 1.0);
}