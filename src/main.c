#include <assert.h>
#include <stdio.h>

#include <GLWT/glwt.h>
#include <GLXW/glxw.h>

#include <threedee/threedee.h>

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
    unsigned vertex_buffer;
    unsigned index_buffer;
    unsigned vertex_array;

    unsigned queries[num_queries];

    unsigned program;

    unsigned shadowmap_shadow_program;
    unsigned shadowmap_light_program;

    int shadowmap_width, shadowmap_height;
    unsigned shadowmap_texture;
    unsigned shadowmap_framebuf;

    unsigned texview_program;
    unsigned texview_sampler;
};

static unsigned load_buffer(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    assert(f != NULL);
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);

    void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

    size_t bytes_read = fread(ptr, 1, size, f);

    glUnmapBuffer(GL_ARRAY_BUFFER);
    fclose(f);

    assert(bytes_read == size);

    return buffer;
}

static void init_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;

    GLint num_formats;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &num_formats);
    GLint formats[num_formats];
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, &formats[0]);

    printf("%d compressed texture formats:\n", num_formats);
    for(int i = 0; i < num_formats; ++i)
        printf("%X\n", formats[i]);


    gfx->program = shader_load("shaders/simple/simple.glslv", "", "", "", "shaders/simple/simple.glslf");
    gfx->texview_program = shader_load("shaders/texview/texview.glslv", "", "", "", "shaders/texview/texview.glslf");

    gfx->shadowmap_shadow_program = shader_load("shaders/shadowmap/shadowmap_shadow.glslv", "", "", "", "");
    gfx->shadowmap_light_program = shader_load("shaders/shadowmap/shadowmap_light.glslv", "", "", "", "shaders/shadowmap/shadowmap_light.glslf");

    glGenQueries(num_queries, gfx->queries);

    gfx->vertex_buffer = load_buffer("../trimuncher/Suzanne.vbo");
    gfx->index_buffer = load_buffer("../trimuncher/Suzanne.ibo");

    glGenVertexArrays(1, &gfx->vertex_array);
    glBindVertexArray(gfx->vertex_array);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx->index_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, gfx->vertex_buffer);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0 + 4*sizeof(float));
    glEnableVertexAttribArray(1);

    /*
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 16*sizeof(float), (void*)0 + 8*sizeof(float));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 16*sizeof(float), (void*)0 + 12*sizeof(float));
    glEnableVertexAttribArray(3);
    */

    glBindVertexArray(0);

    gfx->shadowmap_width = 512;
    gfx->shadowmap_height = 512;

    glGenTextures(1, &gfx->shadowmap_texture);
    glBindTexture(GL_TEXTURE_2D, gfx->shadowmap_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, gfx->shadowmap_width, gfx->shadowmap_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

    glGenSamplers(1, &gfx->texview_sampler);
    glSamplerParameteri(gfx->texview_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(gfx->texview_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &gfx->shadowmap_framebuf);
    glBindFramebuffer(GL_FRAMEBUFFER, gfx->shadowmap_framebuf);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gfx->shadowmap_texture, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void quit_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;

    glDeleteBuffers(1, &gfx->vertex_buffer);

    glDeleteVertexArrays(1, &gfx->vertex_array);
}

static void paint(struct gfx *gfx, int width, int height, int frame)
{
    float t = frame / 60.0;

    for(unsigned i = 0; i < num_queries; ++i)
        glBeginQuery(query_targets[i], gfx->queries[i]);

    glUseProgram(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);

    const float background[] = { 0.2, 0.4, 0.7, 1.0 };
    glClearBufferfv(GL_COLOR, 0, background);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0, 0);

#if 1
    glBindFramebuffer(GL_FRAMEBUFFER, gfx->shadowmap_framebuf);
    glViewport(0, 0, gfx->shadowmap_width, gfx->shadowmap_height);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);

    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0, 0);
#endif

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glUseProgram(gfx->shadowmap_shadow_program);

    int index;
    //mat4 projection_matrix = mat_perspective_fovy(M_PI/4.0, (float)width/height, 0.1, 100.0);
    mat4 projection_matrix = mat_perspective_fovy(M_PI/4.0, (float)gfx->shadowmap_width/gfx->shadowmap_height, 0.1, 10.0);
    index = glGetUniformLocation(gfx->shadowmap_shadow_program, "projection_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&projection_matrix);

    mat4 view_matrix = mtranslate(vec(0.0, 0.0, -5.0, 1.0));
    index = glGetUniformLocation(gfx->shadowmap_shadow_program, "view_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&view_matrix);

    //mat4 model_matrix = midentity();
    mat4 model_matrix = mat_euler(vec(t/3, t, 0.0, 0.0));
    index = glGetUniformLocation(gfx->shadowmap_shadow_program, "model_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&model_matrix);

#if 0
    mat4 normal_matrix = minverse_transpose(mat3_to_mat4(model_matrix));
    index = glGetUniformLocation(gfx->program, "normal_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&normal_matrix);

    index = glGetUniformLocation(gfx->program, "light_ambient");
    glUniform4f(index, 0.2, 0.2, 0.2, 1.0);

    index = glGetUniformLocation(gfx->program, "light_diffuse");
    glUniform4f(index, 0.6, 0.6, 0.6, 0.0);

    index = glGetUniformLocation(gfx->program, "light_specular");
    glUniform4f(index, 1.0, 1.0, 1.0, 1.0);

    index = glGetUniformLocation(gfx->program, "light_shininess");
    glUniform1f(index, 32.0);

    index = glGetUniformLocation(gfx->program, "light_position");
    glUniform4f(index, 2.0, 0.0, 0.0, 1.0);
    //glUniform4f(index, 2*cos(t), 0.0, 2*sin(t), 1.0);
#endif

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xffff);


    glBindVertexArray(gfx->vertex_array);
    //glDrawElements(GL_TRIANGLE_STRIP_ADJACENCY, sizeof(cube_indices)/sizeof(*cube_indices), GL_UNSIGNED_SHORT, (void*)0);
    //glDrawElements(GL_TRIANGLES_ADJACENCY, 6084/2, GL_UNSIGNED_SHORT, (void*)0);
    glDrawElements(GL_TRIANGLES, 6084/2, GL_UNSIGNED_SHORT, (void*)0);

#if 0
    int viewport[4];
    uint32_t pixel;
    glGetIntegerv(GL_VIEWPORT, viewport);
    glReadPixels(
            viewport[0] + viewport[2] / 2,
            viewport[1] + viewport[3] / 2,
            1, 1,
            GL_DEPTH_COMPONENT,
            GL_UNSIGNED_INT,
            &pixel
            );
    printf("%08X\n", pixel);
    //glTexSubImage2D();
#endif

    // texview
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_FALSE);

    glUseProgram(gfx->texview_program);

    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, gfx->shadowmap_texture);
    glBindSampler(0, gfx->texview_sampler);

    glUniform1i(0, 0);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

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
        4, 2
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
