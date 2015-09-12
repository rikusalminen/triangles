#version 330

uniform mat4 projection_matrix;
uniform mat4 view_matrix;

uniform float p;
uniform float e;
uniform float f1;
uniform float f2;
uniform int num_vertices;

void main() {
    float t = float(max(0, gl_VertexID-1) / 2) / ((num_vertices - 1) / 2);

    float f = f1 + (f2-f1) * t;
    float r = p / (1.0 + e * cos(f));
    vec4 pos = vec4(cos(f) * r, sin(f) * r, 0.0, 1.0);
    gl_Position = projection_matrix * view_matrix * pos;
}
