#version 430

const float pi = 3.1416926535;

layout(location = 0) uniform float fov;
layout(location = 1) uniform float aspect;

layout(location = 2) uniform sampler2D tex;

in vec4 pos;

out vec4 color;

void main() {
    // focal length
    float focal = 1.0 / tan(fov);

    // ray from camera to near plane
    //vec4 v = vec4((pos.x/aspect) / focal, pos.y / focal, focal, 0.0);
    vec4 v = vec4(
        ((pos.x + dFdx(pos.x) * gl_SamplePosition.x) / aspect) / focal,
        (pos.y + dFdy(pos.y) * gl_SamplePosition.y) / focal,
        focal,
        0.0);
    // sphere position
    vec4 s = vec4(0.0, 0.0, -5.0, 0.0);
    // sphere radius (XXX: use s.w = -r?)
    float r = 2.0;

    // quadratic equation: t^2 v^2 - 2t dot(v, s) + s^2 = r^2
    // coefficients for quadratic formula
    float a = dot(v, v);
    float b = -2.0 * dot(v, s);
    float c = dot(s, s) - r*r;
    // discriminant
    float d = b*b - 4.0*a*c;

    // discard fragments/samples where discriminant is negative
    //if(d < 0.0)
        //discard;
    if(d < 0.0)
        gl_SampleMask[gl_SampleID/32] &= ~(1 << (gl_SampleID%32));
    else
        gl_SampleMask[gl_SampleID/32] |= 1 << (gl_SampleID%32);

    // near solution to quadratic equation
    float t = (-b - sqrt(d)) / (2.0*a);
    // point on sphere
    vec4 p = t*v - s;

    // convert cartesian to spherical coordinates (longitude, latitude)
    float lon = atan(p.y, p.x);
    float lat = asin(p.z / length(p));

    // calculate uv coordinates and grid
    int num_grid = 16;
    vec2 uv = vec2(0.5 + 0.5*(lon / pi), 0.5 + (lat / pi));
    vec2 grid = abs(fract(num_grid*uv - 0.5) - 0.5) / fwidth(num_grid*uv);
    float line = min(min(grid.x, grid.y), 1.0);

    // diffuse lighting
    vec4 normal = normalize(p);
    vec4 light = vec4(-1.0, 0.0, 0.0, 0.0);
    float diffuse = dot(normal, light);

    // texture map
    vec4 tex_color = texture(tex, uv);

    // apply color
    vec4 ambient_color = vec4(0.2, 0.2, 0.2, 1.0);
    vec4 diffuse_color = vec4(0.2, 0.7, 0.4, 1.0);
    vec4 grid_color = vec4(1.0, 1.0, 1.0, 1.0);
    color = tex_color * mix(grid_color, mix(ambient_color, diffuse_color, diffuse), line);
}
