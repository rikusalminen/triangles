#version 420

in vec3 normal;
in vec2 texcoord;

out vec4 color;

const int num_grid = 8;
const vec4 diffuse_color = vec4(0.2, 0.4, 0.7, 1.0);
const vec4 grid_color = vec4(1.0, 1.0, 1.0, 1.0);

void main() {
    vec2 coord = num_grid * vec2(2, 1) *
        (texcoord +
         dFdx(texcoord) * gl_SamplePosition.xx +
         dFdy(texcoord) * gl_SamplePosition.yy);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
    float line = min(grid.x, grid.y);

    // Just visualize the grid lines directly
    color = mix(grid_color, diffuse_color, min(line, 1.0));
}
