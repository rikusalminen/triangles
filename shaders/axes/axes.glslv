#version 330

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;

out vec4 color;

void main() {
    vec4 pos = vec4(0.0, 0.0, 0.0, 1.0);
    pos[(gl_VertexID / 2) % 3] = float(gl_VertexID & 1);

    vec4 col = vec4(0.0, 0.0, 0.0, 1.0);
    col[(gl_VertexID / 2) % 3] = 1.0;

    color = col;
    gl_Position = projection_matrix * view_matrix * model_matrix * pos;
}
