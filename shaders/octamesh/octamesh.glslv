#version 330

void main()
{
    const int vertex_data[28] = int[28](
        1, 1, // 0
        0, 0, // 1
        1, 1, // 2
        2, 0, // 3
        1, 1, // 4
        2, 2, // 5
        1, 1, // 6
        1, 2, // 7
        0, 2, // 8
        1, 2, // 9
        1, 3, // 10
        2, 2, // 11
        2, 4, // 12
        2, 2 // 13
    );

    int u = 0, v = 0;
    if(gl_VertexID < 2) {
        u = 2;
        v = 2 - gl_VertexID;
    } else {
        int idx = gl_VertexID - 2;

        int level = idx / 14;
        int unit = 1 << level;

        int vtx = idx % 14;

        u = (2 + vertex_data[vtx*2+0]) * unit;
        v = vertex_data[vtx*2+1] * unit;
    }

    vec3 pos = vec3(-1, -1, 0) + vec3(u / 128.0, v / 128.0, 0.0);
    gl_Position = vec4(pos, 1.0);
}
