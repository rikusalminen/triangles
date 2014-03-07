#version 420

void main()
{
    int idx = gl_VertexID % 4;
    int face = gl_VertexID / 4;

    int dir = face % 3;
    int pos = face / 3;

    int nz = dir >> 1;
    int ny = dir & 1;
    int nx = 1 ^ (ny | nz);

    vec3 d = vec3(nx, ny, nz);
    float flip = 1 - 2 * pos;

    vec3 n = flip * d;
    vec3 u = -d.yzx;
    vec3 v = flip * d.zxy;

    int gray = (idx >> 1) ^ idx;
    vec3 xyz = n + (1-2*(gray&1))*u + (1-2*(gray>>1))*v;

    gl_Position = vec4(xyz, 1.0);
}
