#version 330

layout(location = 0) in vec4 position;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;

layout(location = 2) in mat4 model_matrix;

void main()
{
    gl_Position = projection_matrix * view_matrix * model_matrix * position;
}
