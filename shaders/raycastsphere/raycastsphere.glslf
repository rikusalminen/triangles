#version 430

const float pi = 3.1416926535;

layout(location = 0) uniform float fov;
layout(location = 1) uniform float aspect;

layout(location = 2) uniform float timer;

in vec4 pos;

out vec4 color;

vec2 ray_sphere_intersect(vec3 origin, vec3 ray, vec3 sphere, float radius) {
    vec3 s = sphere - origin;
    float a = dot(ray, ray);
    float b = -2.0 * dot(ray, s);
    float c = dot(s, s) - radius*radius;
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
    vec3 ray = normalize(vec3(pos.x / focal, pos.y / (aspect*focal), -focal));
    //vec3 ray = normalize(vec3(
        //(pos.x + dFdx(pos.x) * gl_SamplePosition.x) / focal,
        //(pos.y + dFdy(pos.y) * gl_SamplePosition.y) / (aspect * focal),
        //-focal));

    // light vector
    vec3 light = vec3(cos(timer), 0.0, sin(timer));

    // sphere position
    vec3 sphere = vec3(0.0, 0.0, -10000.0e3);
    float radius = 6471.0e3;
    float inner_radius = 6371.0e3;

    vec2 ts = ray_sphere_intersect(origin, ray, sphere, radius);

    // discard fragments/samples that don't hit sphere
    if(ts.x > ts.y) discard;
    //if(ts.x > ts.y)
        //gl_SampleMask[gl_SampleID/32] &= ~(1 << (gl_SampleID%32));
    //else
        //gl_SampleMask[gl_SampleID/32] |= 1 << (gl_SampleID%32);

    // XXX: raycast against planet surface
    vec2 inner_ts = ray_sphere_intersect(origin, ray, sphere, inner_radius);
    if(inner_ts.x < inner_ts.y)
        ts.y = min(ts.y, inner_ts.y);

    float scale_height = 8.0e3; // XXX: magic constants

    int primary_steps = 16;
    int secondary_steps = 0;

    vec3 rayleigh_coefficient = vec3(5.5e-6, 13.0e-6, 22.4e-6);
    vec3 total_rayleigh = vec3(0.0, 0.0, 0.0); // accumulator
    float primary = 0; // optical depth accumulator
    for(int i = 0; i < primary_steps; ++i) {
        float t = ts.x + (ts.y - ts.x) * ((i+0.5) / float(primary_steps));
        float dt = (ts.y - ts.x) / float(primary_steps);
        vec3 primary_pos = origin + t * ray - sphere;

        float primary_height = length(primary_pos) - inner_radius;
        float primary_optical_depth = dt * exp(-primary_height / scale_height);
        primary += primary_optical_depth;

        float t2 = ray_sphere_intersect(primary_pos, light, sphere, radius).y;


        //vec2 ground = ray_sphere_intersect(primary_pos, -light, sphere, inner_radius);
        //if(ground.y > ground.x)
            //continue;

        float secondary = 0; // optical depth accumulator
        for(int j = 0; j < secondary_steps; ++j) {
            float tt = ((j+0.5) / float(secondary_steps)) * t2;
            float dtt = t2 / float(secondary_steps);
            vec3 secondary_pos = primary_pos + tt * light;

            float secondary_height = length(secondary_pos) - inner_radius;
            float secondary_optical_depth =
                dtt * exp(-secondary_height / scale_height);

            secondary += secondary_optical_depth;
        }

        vec3 attenuation = exp(-rayleigh_coefficient * (primary + secondary));

        total_rayleigh += attenuation * primary_optical_depth;
    }

    float mu = dot(ray, light);
    float rayleigh_phase = 3.0 / (16.0 * pi) * (1.0 + mu*mu);
    vec3 rayleigh = rayleigh_phase * rayleigh_coefficient * total_rayleigh;

    float intensity = 22.0;

    color = 1.0 - exp(-1.0 * vec4(intensity * rayleigh, 1.0)); // XXX: alpha
}
