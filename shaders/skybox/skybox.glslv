#version 420

out vec4 screen_pos;

void main() {
    screen_pos = vec4(
        -1.0 + 4.0 * (gl_VertexID & 1),
        -1.0 + 4.0 * (gl_VertexID >> 1),
        0.0,
        1.0);
    gl_Position = screen_pos;
}
