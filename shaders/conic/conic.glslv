#version 330

uniform mat4 projection_matrix;
uniform mat4 view_matrix;

uniform float p;
uniform float e;
uniform float f1;
uniform float f2;
uniform int num_vertices;

uniform vec4 major_axis;
uniform vec4 minor_axis;

void main() {
    float t = float(max(0, gl_VertexID-1) / 2) / ((num_vertices - 1) / 2);

    float f = f1 + (f2-f1) * t;
    float r = p / (1.0 + e * cos(f));
    vec4 pos = vec4(0.0, 0.0, 0.0, 1.0) +
        cos(f) * r * major_axis +
        sin(f) * r * minor_axis;
    gl_Position = projection_matrix * view_matrix * pos;
}
