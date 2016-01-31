#version 420

in vec3 normal;
in vec2 texcoord;

out vec4 color;

const int num_grid = 8;

void main() {
    vec2 grid = vec2(2, 1) * num_grid *
        (texcoord +
         dFdx(texcoord) * gl_SamplePosition.xx +
         dFdy(texcoord) * gl_SamplePosition.yy);

    if(mod(grid.x, 1.0) > 0.9 || mod(grid.y, 1.0) > 0.9 ||
        mod(grid.x, 1.0) < 0.1 || mod(grid.y, 1.0) < 0.1)
        gl_SampleMask[0] = 1 << gl_SampleID;
        //discard;

    color = vec4(1.0, 1.0, 1.0, 1.0);
}
