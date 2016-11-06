#version 420

const float pi = 3.141592653589;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;

in vec4 screen_pos;

out vec4 color;

void main() {
    vec4 world_pos = inverse(projection_matrix * view_matrix) *
        (screen_pos +
         dFdx(screen_pos) * gl_SamplePosition.xxxx +
         dFdy(screen_pos) * gl_SamplePosition.yyyy);
    world_pos = world_pos / world_pos.wwww;

    float lat = asin(world_pos.z / length(world_pos.xyz));
    float lon = atan(world_pos.y, world_pos.x);

    vec2 uv = vec2(lat, lon) / (pi/8.0);
    vec2 grid = abs(fract(uv - 0.5) - 0.5) / fwidth(uv);
    float line = min(1.0, min(grid.x, grid.y));

    vec4 black = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 white = vec4(1.0, 1.0, 1.0, 1.0);

    color = mix(white, black, line);
}
