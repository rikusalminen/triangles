#version 420

const float pi = 3.141592653589;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;

uniform int num_rings;
uniform int num_slices;

out vec3 normal;
out vec2 texcoord;

void main() {
    int verts_per_slice = 4 + 2*num_rings + 2; // 2 extra vertices for degenerate strip

    int slice = gl_VertexID / verts_per_slice;
    int idx = gl_VertexID % verts_per_slice;

    // degenerate triangles to restart strips
    if(idx == verts_per_slice-2) {
        idx = verts_per_slice-3;
    } else if(idx == verts_per_slice-1) {
        idx = 0;
        slice = (slice+1) % (3+num_slices);
    }

    int rows = 3 + num_rings;
    int row = (idx+1) >> 1;

    float u = (1.0/(3+num_slices)) *
        (slice + ((idx == 0 || idx == (3+2*num_rings)) ? 0.5 : float((idx & 1) ^1 )));
    float v = row / float(rows-1);

    float lon = u * 2.0 * pi;
    float lat = -(v - 0.5) * pi;

    vec3 xyz = vec3(cos(lon) * cos(lat), sin(lon) * cos(lat), sin(lat));

    normal = xyz;
    texcoord = vec2(u, v);
    gl_Position = projection_matrix * view_matrix * vec4(xyz, 1.0);
}
