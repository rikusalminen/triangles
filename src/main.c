#include <assert.h>
#include <stdio.h>

#include <GLWT/glwt.h>
#include <GLXW/glxw.h>

#include <threedee/threedee.h>

static void cube_gen(float *vertex_data)
{
    for(int face = 0; face < 6; ++face)
    {
        int axis = face % 3;
        int sign = face / 3;
        int uaxis = (axis + 2) % 3;
        int vaxis = (axis + 1) % 3;

        for(int vertex = 0; vertex < 4; ++vertex)
        {
            int u = vertex / 2;
            int v = vertex % 2;

            float *vertex_ptr = vertex_data + (face * 4 + vertex) * 16;
            vertex_ptr[axis] = sign ? -1.0 : 1.0;
            vertex_ptr[uaxis] = -(2*u-1);
            vertex_ptr[vaxis] = (sign ? (2*v-1) : -(2*v-1));
            vertex_ptr[3] = 1.0;

            float *normal_ptr = vertex_ptr + 4;
            normal_ptr[axis] = sign ? -1.0 : 1.0;
            normal_ptr[uaxis] = 0;
            normal_ptr[vaxis] = 0;
            normal_ptr[3] = 0.0;

            float *tangent_ptr = vertex_ptr + 12;
            tangent_ptr[axis] = 0.0;
            tangent_ptr[uaxis] = sign ? 1.0 : -1.0;
            tangent_ptr[vaxis] = 0.0;
            tangent_ptr[3] = 0.0;

            float *uv_ptr = vertex_ptr + 12;
            uv_ptr[0] = u;
            uv_ptr[1] = v;
            uv_ptr[2] = 0.0;
            uv_ptr[3] = 0.0;
        }
    }
}

static const uint16_t cube_indices[] = {
    0, 9, 1, 6, 2, 18, 3, 22, 0xffff,
    4, 1, 5, 10, 6, 22, 7, 14, 0xffff,
    8, 5, 9, 2, 10, 14, 11, 18, 0xffff,
    12, 10, 13, 17, 14, 5, 15, 21, 0xffff,
    16, 2, 17, 21, 18, 9, 19, 13, 0xffff,
    20, 6, 21, 13, 22, 1, 23, 17, 0xffff,
};

extern unsigned shader_load(const char *vert, const char *tess_ctrl, const char *tess_eval, const char *geom, const char *frag);

static const GLenum query_targets[] = {
    GL_TIME_ELAPSED,
    GL_SAMPLES_PASSED,
    GL_PRIMITIVES_GENERATED,
};

static const char* query_names[] = {
    "time elapsed",
    "samples passed",
    "primitives generated",
};

#define num_queries (sizeof(query_targets)/sizeof(*query_targets))

struct gfx
{
    int num_cubes;

    unsigned vertex_buffer;
    unsigned index_buffer;
    unsigned instance_buffer;
    unsigned vertex_array;

    unsigned empty_vertex_array;

    unsigned queries[num_queries];

    unsigned simple_program;

    unsigned ambient_program;
    unsigned shadow_program;
    unsigned lighting_program;
    unsigned stencil_program;

    mat4 projection_matrix;
    mat4 shadow_matrix;
    mat4 view_matrix;

    vec4 light_ambient;
    vec4 light_diffuse;
    vec4 light_specular;
    vec4 light_position;
    float light_shininess;
};

static void init_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;
    gfx->simple_program = shader_load("shaders/simple/simple.glslv", "", "", "", "shaders/simple/simple.glslf");

    gfx->ambient_program = shader_load("shaders/ambient/ambient.glslv", "", "", "", "shaders/ambient/ambient.glslf");
    gfx->shadow_program = shader_load("shaders/shadow_volume/shadow_volume.glslv", "", "", "shaders/shadow_volume/shadow_volume.glslg", "shaders/shadow_volume/shadow_volume.glslf");
    gfx->lighting_program = shader_load("shaders/lighting/lighting.glslv", "", "", "", "shaders/lighting/lighting.glslf");
    gfx->stencil_program = shader_load("shaders/stencil/stencil.glslv", "", "", "", "shaders/stencil/stencil.glslf");

    glGenQueries(num_queries, gfx->queries);

    gfx->num_cubes = 256;

    glGenBuffers(1, &gfx->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, gfx->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, 24 * 16 * sizeof(float), NULL, GL_STATIC_DRAW);

    void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    cube_gen((float*)ptr);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    glGenBuffers(1, &gfx->index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx->index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

    glGenBuffers(1, &gfx->instance_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, gfx->instance_buffer);
    glBufferData(GL_ARRAY_BUFFER, gfx->num_cubes * 32 * sizeof(float), NULL, GL_STATIC_DRAW);

    glGenVertexArrays(1, &gfx->empty_vertex_array);

    glGenVertexArrays(1, &gfx->vertex_array);
    glBindVertexArray(gfx->vertex_array);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx->index_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, gfx->vertex_buffer);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 16*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 16*sizeof(float), (void*)0 + 4*sizeof(float));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 16*sizeof(float), (void*)0 + 8*sizeof(float));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 16*sizeof(float), (void*)0 + 12*sizeof(float));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->instance_buffer);
    for(int i = 0; i < 4; ++i) {
        int index = 2;
        glVertexAttribPointer(index + i, 4, GL_FLOAT, GL_FALSE, 32*sizeof(float),
            (void*)0 + i*4*sizeof(float));
        glEnableVertexAttribArray(index + i);
        glVertexAttribDivisor(index+i, 1);
    }

    for(int i = 0; i < 4; ++i) {
        int index = 6;
        glVertexAttribPointer(index + i, 4, GL_FLOAT, GL_FALSE, 32*sizeof(float),
            (void*)0 + (16 + 4*i)*sizeof(float));
        glEnableVertexAttribArray(index + i);
        glVertexAttribDivisor(index+i, 1);
    }

    glBindVertexArray(0);
}

static void quit_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;

    glDeleteBuffers(1, &gfx->vertex_buffer);

    glDeleteVertexArrays(1, &gfx->vertex_array);
}

static void update_cubes(struct gfx *gfx, float t) {
    (void)t;

    glBindBuffer(GL_ARRAY_BUFFER, gfx->instance_buffer);
    void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

    int rows = 16;
    for(int i = 0; i < gfx->num_cubes; ++i) {
        int row = i % rows;
        int col = i / rows;

        mat4 model_matrix = mmmul(
            mtranslate(vec(3.0*(row - rows/2), -10.0, 3.0*(col-rows/2), 1.0)),
            midentity());
            //mat_euler(vec(t/3, t, 0.0, 0.0)));
        mat4 normal_matrix = minverse_transpose(mat3_to_mat4(model_matrix));

        mstream((mat4*)ptr + i*2, model_matrix);
        mstream((mat4*)ptr + i*2+1, normal_matrix);

    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
}

static void gfx_animate(struct gfx *gfx, int width, int height, float t) {
    gfx->projection_matrix = mat_perspective_fovy(M_PI/4.0, (float)width/height, 0.1, 100.0);
    gfx->shadow_matrix = mat_perspective_fovy_inf_z(M_PI/4.0, (float)width/height, 0.1);
    gfx->view_matrix = mtranslate(vec(0.0, 0.0, -40.0, 1.0));

    gfx->light_ambient = (vec4){ 0.2, 0.2, 0.2, 1.0 };
    gfx->light_diffuse = (vec4){ 0.6, 0.6, 0.6, 0.0 };
    gfx->light_specular = (vec4){ 1.0, 1.0, 1.0, 1.0 };
    //gfx->light_position = (vec4){ 0.0, 10.0, 0.0, 1.0 };
    gfx->light_position = (vec4){ 15.0*sin(t), 5.0, 15.0*cos(t), 1.0 };
    gfx->light_shininess = 32.0;
}

static void draw_cubes(struct gfx *gfx) {
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xffff);

    glBindVertexArray(gfx->vertex_array);
    glDrawElementsInstanced(GL_TRIANGLE_STRIP_ADJACENCY,
        sizeof(cube_indices)/sizeof(*cube_indices), GL_UNSIGNED_SHORT,
        (void*)0,
        gfx->num_cubes);
}

static void ambient_draw(struct gfx *gfx) {
    glUseProgram(gfx->ambient_program);

    int index;
    index = glGetUniformLocation(gfx->ambient_program, "projection_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&gfx->projection_matrix);

    index = glGetUniformLocation(gfx->ambient_program, "view_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&gfx->view_matrix);

    index = glGetUniformLocation(gfx->ambient_program, "light_ambient");
    glUniform4fv(index, 1, (const float*)&gfx->light_ambient);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_STENCIL_TEST);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);

    glDepthMask(~0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDepthMask(~0);

    draw_cubes(gfx);
}

static void shadow_draw(struct gfx *gfx) {
    glUseProgram(gfx->shadow_program);

    int index;
    index = glGetUniformLocation(gfx->shadow_program, "projection_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&gfx->projection_matrix);

    index = glGetUniformLocation(gfx->shadow_program, "view_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&gfx->view_matrix);

    index = glGetUniformLocation(gfx->shadow_program, "shadow_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&gfx->shadow_matrix);

    index = glGetUniformLocation(gfx->shadow_program, "light_position");
    glUniform4fv(index, 1, (const float*)&gfx->light_position);


    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    glDepthMask(0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_CULL_FACE);

    glEnable(GL_STENCIL_TEST);
    glStencilFuncSeparate(GL_FRONT, GL_ALWAYS, 0, ~0);
    glStencilFuncSeparate(GL_BACK, GL_ALWAYS, 0, ~0);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);

    //glStencilOpSeparate(GL_BACK, GL_INCR, GL_INCR, GL_INCR);
    //glStencilOpSeparate(GL_FRONT, GL_INCR, GL_INCR, GL_INCR);

    draw_cubes(gfx);
}

static void stencil_draw(struct gfx *gfx) {

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(0);

    glUseProgram(gfx->stencil_program);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilFuncSeparate(GL_BACK, GL_EQUAL, 0, 0xff);
    glStencilFuncSeparate(GL_FRONT, GL_EQUAL, 0, 0xff);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);

    glBindVertexArray(gfx->empty_vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

static void lighting_draw(struct gfx *gfx) {
    glUseProgram(gfx->lighting_program);

    int index;
    index = glGetUniformLocation(gfx->lighting_program, "projection_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&gfx->projection_matrix);

    index = glGetUniformLocation(gfx->lighting_program, "view_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&gfx->view_matrix);

    index = glGetUniformLocation(gfx->lighting_program, "light_position");
    glUniform4fv(index, 1, (const float*)&gfx->light_position);

    index = glGetUniformLocation(gfx->lighting_program, "light_diffuse");
    glUniform4fv(index, 1, (const float*)&gfx->light_diffuse);

    index = glGetUniformLocation(gfx->lighting_program, "light_specular");
    glUniform4fv(index, 1, (const float*)&gfx->light_specular);

    index = glGetUniformLocation(gfx->lighting_program, "light_shininess");
    glUniform1f(index, gfx->light_shininess);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glDepthMask(0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glEnable(GL_STENCIL_TEST);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);

    glStencilFuncSeparate(GL_FRONT, GL_EQUAL, 0, 0xff);
    glStencilFuncSeparate(GL_BACK, GL_EQUAL, 0, 0xff);

    draw_cubes(gfx);

    glDisable(GL_BLEND);
}

void simple_draw(struct gfx *gfx) {
    glUseProgram(gfx->simple_program);

    int index;
    index = glGetUniformLocation(gfx->simple_program, "projection_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&gfx->projection_matrix);

    index = glGetUniformLocation(gfx->simple_program, "view_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&gfx->view_matrix);

    index = glGetUniformLocation(gfx->simple_program, "light_position");
    glUniform4fv(index, 1, (const float*)&gfx->light_position);

    index = glGetUniformLocation(gfx->simple_program, "light_ambient");
    glUniform4fv(index, 1, (const float*)&gfx->light_ambient);

    index = glGetUniformLocation(gfx->simple_program, "light_diffuse");
    glUniform4fv(index, 1, (const float*)&gfx->light_diffuse);

    index = glGetUniformLocation(gfx->simple_program, "light_specular");
    glUniform4fv(index, 1, (const float*)&gfx->light_specular);

    index = glGetUniformLocation(gfx->simple_program, "light_shininess");
    glUniform1f(index, gfx->light_shininess);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    draw_cubes(gfx);
}

static void paint(struct gfx *gfx, int width, int height, int frame)
{
    float t = frame / 60.0;

    for(unsigned i = 0; i < num_queries; ++i)
        glBeginQuery(query_targets[i], gfx->queries[i]);

    const float background[] = { 0.2, 0.4, 0.7, 1.0 };
    glClearBufferfv(GL_COLOR, 0, background);

    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    gfx_animate(gfx, width, height, t);
    update_cubes(gfx, t);

    (void)ambient_draw;
    (void)shadow_draw;
    (void)stencil_draw;
    (void)lighting_draw;
#if 0
    ambient_draw(gfx);
    shadow_draw(gfx);
    stencil_draw(gfx);
    lighting_draw(gfx);
#else
    simple_draw(gfx);
#endif

    for(unsigned i = 0; i < num_queries; ++i)
        glEndQuery(query_targets[i]);
}

static void parse_gl_version(const char *str, int *major, int *minor, int *gles)
{
    // GL version string: "major.minor" or "OpenGL ES major.minor"
    int es = str[0] == 'O';
    *major = str[es ? 10 : 0] - '0'; // TODO: Update this when OpenGL 10.0 comes around :D
    *minor = str[es ? 12 : 2] - '0'; // ...or x.10
    *gles = es;
}

static void main_loop(GLWTWindow *window)
{
    struct gfx gfx;
    init_gfx(window, &gfx);

    printf("Version: %s\n", (const char *)glGetString(GL_VERSION));
    printf("Vendor: %s\n", (const char *)glGetString(GL_VENDOR));
    printf("Renderer: %s\n", (const char *)glGetString(GL_RENDERER));
    printf("GLSL Version: %s\n", (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));

    int major, minor, gles;
    parse_gl_version((const char*)glGetString(GL_VERSION), &major, &minor, &gles);

    printf("OpenGL%s %d.%d\n", gles ? " ES" : "", major, minor);

    int frame = 0;

    while(!glwtWindowClosed(window))
    {
        int width, height;
        glwtWindowGetSize(window, &width, &height);
        paint(&gfx, width, height, frame++);

        uint64_t query_results[num_queries];
        for(unsigned i = 0; i < num_queries; ++i)
            glGetQueryObjectui64v(gfx.queries[i], GL_QUERY_RESULT, &query_results[i]);

        assert(glGetError() == GL_NO_ERROR);

        const int str_size = 1024;
        char str[str_size];
        char *ptr = str;
        for(unsigned i = 0; i < num_queries; ++i)
            ptr += snprintf(ptr, str_size - (ptr - str), "%s: %lu  ", query_names[i], query_results[i]);
        glwtWindowSetTitle(window, str);

        glwtSwapBuffers(window);

        glwtEventHandle(0);
    }

    quit_gfx(window, &gfx);
}

extern void APIENTRY gl_debug_callback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    GLvoid* userParam);

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    GLWTConfig config = {
        8, 8, 8, 8,
        24, 8,
        4, 1,
        GLWT_API_OPENGL | GLWT_PROFILE_CORE | GLWT_PROFILE_DEBUG,
        3, 3
    };

    GLWTWindow *window = 0;
    if(glwtInit(&config, NULL, NULL) != 0 ||
        !(window = glwtWindowCreate("", 1024, 768, NULL, NULL, NULL)))
    {
        glwtQuit();
        return -1;
    }

    glwtWindowShow(window, 1);
    glwtMakeCurrent(window);
    glwtSwapInterval(window, 1);

    glxwInit();

    if(glDebugMessageCallbackARB)
    {
        void *debug_data = NULL;
        glDebugMessageCallbackARB(&gl_debug_callback, debug_data);
    }

    main_loop(window);

    glwtWindowDestroy(window);
    glwtQuit();
    return 0;
}
