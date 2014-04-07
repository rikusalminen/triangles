#version 420

layout (location = 0) uniform sampler2D font_tex;

out vec4 color;

void main()
{
    ivec2 font_size = textureSize(font_tex, 0);
    if(gl_FragCoord.x > font_size.x || gl_FragCoord.y > font_size.y)
        discard;
    color = texelFetch(font_tex, ivec2(gl_FragCoord.xy), 0).r > 0 ? vec4(1,1,1,1) : vec4(0,0,0,0);
    //color = vec4(1,1,1,1);
}
