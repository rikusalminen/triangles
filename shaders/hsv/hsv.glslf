#version 330

out vec4 frag_color;

vec4 hsv2rgb(float h, float s, float v) {
    float c = v * s;
    float hh = h * 6.0;
    float x = c * (1.0 - abs(mod(hh, 2) - 1.0));
    float m = v - c;

    int i = int(hh) % 6;
    vec4 rgb = vec4(0.0, 0.0, 0.0, 1.0);
    rgb[(1 + 2*i)%3] = x + m;
    rgb[((i+1)/2)%3] = c + m;
    rgb[(2+(i/2))%3] = 0.0 + m;

    return rgb;
}

vec4 color_pattern(int seed) {
    float h = mod(0.01 + seed * 0.21, 1.0);
    float s = 0.75 + 0.25 * mod(0.11 + seed * 0.41, 1.0);
    float v = 0.5 + 0.5 * mod(0.51 + seed * 0.31, 1.0);
    return hsv2rgb(h, s, v);
}

void main() {
    int blocksize = 16;
    if(gl_FragCoord.y > blocksize)
        discard;

    int block = int(gl_FragCoord.x) / blocksize;
    vec4 color = color_pattern(block);

    frag_color = color;
}
