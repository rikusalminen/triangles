#version 330

uniform vec4 light_ambient;

out vec4 frag_color;

void main() {
    frag_color = light_ambient;
}
