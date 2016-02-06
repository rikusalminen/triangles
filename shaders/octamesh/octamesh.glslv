#version 330

/*
(0,2)           (2,2)
    +-----+-----+
    |\    |    /|
    | \ 5 | 4 / |
    |  \  |  /  |
    | 6 \ | / 3 |
    |    \|/    |
    +-----+-----+
    |    /|\    |
    | 7 / | \ 2 |
    |  /  |  \  |
    | / 0 | 1 \ |
    |/    |    \|
    +-----+-----+
(0,0)           (2,0)

    first vertex of every triangle is right angle (at outside edge)
*/
ivec2 quad_octant(int tri, int vtx) {
    ivec2 uv = ivec2(0, 0);
    uv[(tri & 2) >> 1 ^ 0] =
        (1 ^ (vtx >> 1) ^ (tri & (vtx >> 1 | vtx) & 1)) |
        (((tri>>2 ^ tri) & (~tri ^ vtx) & (vtx >> 1 | vtx) & 1) << 1);
    uv[(tri & 2) >> 1 ^ 1] =
        ((vtx >> 1 | vtx) & (tri ^ vtx) & 1) |
        (((tri >> 1 ^ tri)&2) & ((~vtx & 1) ^ (tri & (vtx >> 1 | vtx) & 1))<<1);
    return uv;
}


/*
(0,2)           (2,2)
    +-----------+
    |\         /|
    | \       / |
    |  \  2  /  |
    |   \   /   |
    |    \ /    |
    |  3  +  1  |
    |    / \    |
    |   /   \   |
    |  /  0  \  |
    | /       \ |
    |/         \|
    +-----------+
(0,0)           (2,0)
    first vertex of every triangle is right angle (in the middle)
*/
ivec2 quad_quadrant(int tri, int vtx) {
    return ivec2(
        ((1^(vtx>>1 | vtx))&1) |
        ((tri<<1 ^ tri) & vtx<<1 & 2) | (vtx & ~tri & 2),
        ((1^(vtx>>1 | vtx))&1) |
        (tri & vtx<<1 & 2) | ((tri<<1 ^ tri) & vtx & 2)
    );
}

void main()
{
    int tri = gl_VertexID / 3, vtx = gl_VertexID % 3;
    int strip = tri / 10, strip_tri = tri % 10;
    int quad = strip_tri / 7, quad_tri = strip_tri % 7;
    int unit = 1 << strip;

    ivec2 uv = ivec2(2, quad*2) + (quad_tri == 2 ?
        quad_quadrant(1, vtx) :
        quad_octant(quad_tri < 2 ? quad_tri : quad_tri + 1, vtx));

    vec3 pos = vec3(-1, -1, 0) + vec3(unit * uv.x / 128.0, unit * uv.y / 128.0, 0.0);
    gl_Position = vec4(pos, 1.0);
}
