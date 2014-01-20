#version 420

const float pi = 3.141592653589;

uniform mat4 projection_matrix;
uniform mat4 model_matrix;

uniform int lod_level;

void main()
{
    int tri = gl_VertexID / 3;
    int idx = gl_VertexID % 3;
    int diamond = tri / 2;
    int top = tri % 2;
    int col = diamond / (2 << lod_level);
    int row = diamond % (2 << lod_level);
    int neg = 2 * top - 1;
    int slices = 1 << lod_level;
    int slice = col % slices;

    int ui = -slice + (2 * col + row + 1) - (2 * (idx % 2) - 1 + (idx / 2)) * neg;
    int vi = (slices - 1 - slice) + (row + 1) + (idx / 2) * neg;

    float u = ui / (10.0 * (1 << lod_level));
    float v = vi / (3.0 * (1 << lod_level));

    float lon = u * 2.0 * pi;
    float lat = (-v + 0.5) * pi;

    vec3 xyz = vec3(cos(lon) * cos(lat), sin(lon) * cos(lat), sin(lat));
    //vec3 xyz = vec3(lon - 3.0, lat, -5.0);

    gl_Position = projection_matrix * model_matrix * vec4(xyz, 1.0);
}
