#version 330

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;

layout(location = 2) in mat4 model_matrix;
layout(location = 6) in mat4 normal_matrix;

out struct frag_t {
    vec4 position;
    vec4 normal;
} frag;

void main()
{
    gl_Position = projection_matrix * view_matrix * model_matrix * position;

    frag.position = view_matrix * model_matrix * position;
    frag.normal = view_matrix * normal_matrix * normal;
}
