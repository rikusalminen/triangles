#version 420

uniform mat4 projection_matrix;
uniform mat4 model_matrix;

void main() {
    vec4 pos = vec4(0.0, 0.0, 0.0, 1.0);
    int axis = gl_VertexID >> 3;
    pos[(axis+0)%3] = (gl_VertexID & 1) >> 0;
    pos[(axis+1)%3] = (gl_VertexID & 2) >> 1;
    pos[(axis+2)%3] = (gl_VertexID & 4) >> 2;

    gl_Position = projection_matrix * model_matrix *
        (vec4(-1.0) + 2.0*pos);
}
