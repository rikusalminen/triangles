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
    unsigned vertex_array;

    unsigned queries[num_queries];

    unsigned program;
};

static void init_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;
    gfx->program = shader_load("shaders/simple/simple.glslv", "", "", "", "shaders/simple/simple.glslf");

    glGenQueries(num_queries, gfx->queries);

    const float vertex_data[] = {
        -1.0, -1.0, 0.0, 1.0,       0.0, 0.0, 1.0, 0.0,
        1.0, -1.0, 0.0, 1.0,        0.0, 0.0, 1.0, 0.0,
        0.0, 1.0, 0.0, 1.0,         0.0, 0.0, 1.0, 0.0,
    };

    glGenBuffers(1, &gfx->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, gfx->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);

    // void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

    // glUnmapBuffer(GL_ARRAY_BUFFER);

    glGenVertexArrays(1, &gfx->vertex_array);
    glBindVertexArray(gfx->vertex_array);

    int index = 0;
    glBindBuffer(GL_ARRAY_BUFFER, gfx->vertex_buffer);
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(index);

    index = 1;
    glBindBuffer(GL_ARRAY_BUFFER, gfx->vertex_buffer);
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0 + 4*sizeof(float));
    glEnableVertexAttribArray(index);

    glBindVertexArray(0);
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

    const float background[] = { 0.2, 0.4, 0.7, 1.0 };
    glClearBufferfv(GL_COLOR, 0, background);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, width, height);
    //glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glUseProgram(gfx->program);

    int index;
    mat4 projection_matrix = mat_perspective_fovy(M_PI/4.0, (float)width/height, 0.1, 100.0);
    index = glGetUniformLocation(gfx->program, "projection_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&projection_matrix);

    mat4 view_matrix = mtranslate(vec(0.0, 0.0, -5.0, 1.0));
    index = glGetUniformLocation(gfx->program, "view_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&view_matrix);

    mat4 model_matrix = midentity();
    //mat4 model_matrix = mat_euler(vec(0.0, t, 0.0, 0.0));
    index = glGetUniformLocation(gfx->program, "model_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&model_matrix);

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
    glUniform4f(index, 2*cos(t), 0.0, 2*sin(t), 1.0);


    glBindVertexArray(gfx->vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    for(unsigned i = 0; i < num_queries; ++i)
        glEndQuery(query_targets[i]);
}

static void main_loop(GLWTWindow *window)
{
    struct gfx gfx;
    init_gfx(window, &gfx);

    printf("Version: %s\n", (const char *)glGetString(GL_VERSION));
    printf("Vendor: %s\n", (const char *)glGetString(GL_VENDOR));
    printf("Renderer: %s\n", (const char *)glGetString(GL_RENDERER));
    printf("GLSL Version: %s\n", (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));

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
