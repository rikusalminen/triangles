#version 420

layout (location = 0) in vec2 xy;

void main() {
    gl_Position = vec4(xy, 0.0, 1.0);
}
