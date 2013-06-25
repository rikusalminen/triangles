#version 420

const float pi = 3.141592653589;

uniform mat4 projection_matrix;
uniform mat4 model_matrix;

void main()
{
    int tri = gl_VertexID / 3;
    int idx = gl_VertexID % 3;
    int diamond = tri / 2;
    int top = tri % 2;
    int col = diamond / 2;
    int row = diamond % 2;
    int neg = 2 * top - 1;

    int ui = (2 * col + row + 1) - (2 * (idx % 2) - 1 + (idx / 2)) * neg;
    int vi = (row + 1) + (idx / 2) * neg;

    float u = ui / 10.0;
    float v = vi / 3.0;

    float lon = u * 2.0 * pi;
    float lat = (-v + 0.5) * pi;

    vec3 xyz = vec3(cos(lon) * cos(lat), sin(lon) * cos(lat), sin(lat));

    //gl_Position = vec4(u, v, 0, 1);
    //gl_Position = vec4(2 * u - 1, -(2.0 * v - 1), 0, 1);
    gl_Position = projection_matrix * model_matrix * vec4(xyz, 1.0);
}
