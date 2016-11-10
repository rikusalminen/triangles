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
    unsigned queries[num_queries];

    unsigned program;
    unsigned vertex_array;

    int textmode_buffer_size;
    unsigned textmode_buffer;
};

static void init_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;
    gfx->program = shader_load("shaders/nesfont/nesfont.glslv", "", "", "", "shaders/nesfont/nesfont.glslf");

    glGenQueries(num_queries, gfx->queries);

    glGenVertexArrays(1, &gfx->vertex_array);

    gfx->textmode_buffer_size = 4096;
    glGenBuffers(1, &gfx->textmode_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, gfx->textmode_buffer);
    glBufferData(GL_UNIFORM_BUFFER, gfx->textmode_buffer_size, NULL, GL_DYNAMIC_DRAW);
}

static void quit_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;

    glDeleteQueries(num_queries, gfx->queries);

    glDeleteBuffers(1, &gfx->textmode_buffer);
}

#include <string.h>
#include <stdio.h>

static void update_textmode(struct gfx* gfx, int frame) {
    char textmode[gfx->textmode_buffer_size];
    memset(textmode, 0, sizeof(textmode));
    for(int i = 0; i < 25; ++i)
        snprintf(textmode + i*80, 80, "%d hello world  (frame: %d)", i, frame);

    glBindBuffer(GL_UNIFORM_BUFFER, gfx->textmode_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, gfx->textmode_buffer_size, textmode);
}

static void paint(struct gfx *gfx, int width, int height, int frame)
{
    float t = frame / 60.0;
    (void)t;

    update_textmode(gfx, frame);

    for(unsigned i = 0; i < num_queries; ++i)
        glBeginQuery(query_targets[i], gfx->queries[i]);

    const float background[] = { 0.2, 0.4, 0.7, 1.0 };
    glClearBufferfv(GL_COLOR, 0, background);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0, 0);

    glViewport(0, 0, width, height);

    glUseProgram(gfx->program);
    //int index = glGetUniformBlockIndex(gfx->program, "textmode");
    int textmode_binding = 0;
    //glUniformBlockBinding(gfx->program, index, textmode_binding);
    glBindBufferBase(GL_UNIFORM_BUFFER, textmode_binding, gfx->textmode_buffer);

    glBindVertexArray(gfx->vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, 3);

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
