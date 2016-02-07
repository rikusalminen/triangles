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
    const int magic = 0x6a6645; // 24 bits of vertex data

    int u = (magic >> (2*((tri%4)*3 + vtx))) & 3;
    u ^= (tri>>1 & ~u<<1) & 2;

    int v = (magic >> (2*(((tri+6)%4)*3 + vtx))) & 3;
    v ^= (~(tri>>1 ^ tri) & ~v<<1) & 2;

    return ivec2(u, v);
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
    const int magic = 0xa61; // 12 bits of vertex data

    int u = (magic >> (2*((tri%2)*3 + vtx))) & 3;
    u ^= (tri & ~u<<1) & 2;

    int v = (magic >> (2*(((tri+3)%2)*3 + vtx))) & 3;
    v ^= (~(tri<<1 ^ tri) & ~v<<1) & 2;

    return ivec2(u, v);
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
