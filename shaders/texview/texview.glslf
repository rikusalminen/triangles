#version 420

layout(location = 0) uniform sampler2D tex;

out vec4 color;

float linearize(float depth)
{
    const float near = 0.1, far = 10.0;
    return 2.0 * near / (far + near - depth * (far - near));
}

void main()
{
    ivec2 size = textureSize(tex, 0);
    if(gl_FragCoord.x > size.x || gl_FragCoord.y > size.y)
        discard;
    color = vec4(linearize(texelFetch(tex, ivec2(gl_FragCoord.xy), 0).x));
}
