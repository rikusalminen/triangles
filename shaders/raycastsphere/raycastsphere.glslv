#version 430

out vec4 pos;

void main() {
    pos = vec4(
        -1.0 + 4.0 * (gl_VertexID >> 1),
        -1.0 + 4.0 * (gl_VertexID & 1),
        0.0,
        1.0);
    gl_Position = pos;
}
