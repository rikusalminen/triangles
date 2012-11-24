#version 420

uniform vec4 light_ambient;
uniform vec4 light_diffuse;
uniform vec4 light_specular;
uniform float light_shininess;
uniform vec4 light_position;

uniform mat4 view_matrix;

out vec4 color;

in struct {
    vec4 position;
    vec4 normal;
} frag;

void main()
{
    vec4 light_vec = normalize(view_matrix * light_position - frag.position);
    vec4 eye_vec = -normalize(frag.position);

    color =
        light_ambient +
        light_diffuse * clamp(dot(frag.normal, light_vec), 0, 1) +
        light_specular * pow(clamp(dot(eye_vec, reflect(-light_vec, frag.normal)), 0, 1), light_shininess);
}
