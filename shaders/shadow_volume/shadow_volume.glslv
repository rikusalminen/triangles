#version 330

layout(location = 0) in vec4 position;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;

out struct camera_space_ {
    vec4 position;
} camera_space;

void main() {
    camera_space.position = view_matrix * model_matrix * position;
    gl_Position = projection_matrix * view_matrix * model_matrix * position;
}
