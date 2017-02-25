#version 430

const float pi = 3.1416926535;

layout(location = 0) uniform float fov;
layout(location = 1) uniform float aspect;

layout(location = 2) uniform sampler2D tex;

layout(location = 3) uniform mat4 model_matrix;

in vec4 pos;

out vec4 color;

vec2 ray_sphere_intersect(vec3 origin, vec3 ray, vec4 sphere) {
    vec3 s = sphere.xyz - origin;
    float a = dot(ray, ray);
    float b = -2.0 * dot(ray, s);
    float c = dot(s, s) - sphere.w*sphere.w;
    float d = b*b - 4.0*a*c;
    return d < 0.0 ?
        vec2(1.0, -1.0) :
        (vec2((-b - sqrt(d))/(2.0*a), (-b + sqrt(d)/(2.0*a))));
}

void main() {
    // focal length
    float focal = 1.0 / tan(fov);

    // camera position
    vec3 origin = vec3(0.0, 0.0, 0.0);

    // ray from camera to near plane
    //vec3 ray = vec3(pos.x / focal, pos.y / (aspect*focal), -focal, 0.0);
    vec3 ray = vec3(
        (pos.x + dFdx(pos.x) * gl_SamplePosition.x) / focal,
        (pos.y + dFdy(pos.y) * gl_SamplePosition.y) / (aspect * focal),
        -focal);

    // sphere position
    vec4 sphere = vec4(0.0, 0.0, -5.0, 2.0);

    vec2 ts = ray_sphere_intersect(origin, ray, sphere);

    // discard fragments/samples that don't hit sphere
    //if(ts.x > ts.y) discard;
    if(ts.x > ts.y)
        gl_SampleMask[gl_SampleID/32] &= ~(1 << (gl_SampleID%32));
    else
        gl_SampleMask[gl_SampleID/32] |= 1 << (gl_SampleID%32);

    // point on sphere
    float t = ts.x;
    vec4 p = vec4(origin + t*ray - sphere.xyz, 1.0);

    gl_FragDepth = p.z / sphere.w;

    // transform by inverse model matrix
    p = inverse(model_matrix) * p;

    // convert cartesian to spherical coordinates (longitude, latitude)
    float lon = atan(p.y, p.x);
    float lat = asin(p.z / length(p));

    // calculate uv coordinates and grid
    vec2 num_grid = vec2(16, 8);;
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
    vec4 diffuse_color = vec4(0.8, 0.8, 0.8, 1.0);
    vec4 grid_color = vec4(1.0, 1.0, 1.0, 1.0);
    color = mix(grid_color, tex_color * mix(ambient_color, diffuse_color, diffuse), line);
    //color = tex_color * mix(grid_color, mix(ambient_color, diffuse_color, diffuse), line);
}
