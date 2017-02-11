#version 330

void main()
{
    int v = gl_VertexID >> 1;
    int u = gl_VertexID & 1;

    gl_Position = vec4(-1.0 + 4.0*u, -1.0 + 4.0*v, 1.0, 1.0);
}
