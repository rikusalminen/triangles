#version 420

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat4 normal_matrix;

out struct {
    vec4 position;
    vec4 normal;
} frag;

void main()
{
    gl_Position = projection_matrix * view_matrix * model_matrix * position;

    frag.position = view_matrix * model_matrix * position;
    frag.normal = view_matrix * normal_matrix * normal;
}
