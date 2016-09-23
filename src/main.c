#include <assert.h>
#include <stdio.h>
#include <string.h>

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

    unsigned vertex_buffer_size;
    unsigned vertex_buffer;
    unsigned identity_vertex_array;

    unsigned identity_program;


    int lod_level;
    int outer_level, inner_level;

    float *vertex_ptr;
    unsigned vertex_count;

    float mouse_x, mouse_y;
};

static void init_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;

    glGenQueries(num_queries, gfx->queries);

    gfx->identity_program = shader_load("shaders/identity/identity.glslv", "", "",  "", "shaders/identity/identity.glslf");
    glGenVertexArrays(1, &gfx->identity_vertex_array);

    gfx->vertex_buffer_size = 1024*1024*16;
    glGenBuffers(1, &gfx->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, gfx->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, gfx->vertex_buffer_size, NULL, GL_STATIC_DRAW);

    glBindVertexArray(gfx->identity_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, gfx->vertex_buffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (const void*)0);
    glEnableVertexAttribArray(0);
}

static void quit_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;

    glDeleteVertexArrays(1, &gfx->identity_vertex_array);
    glDeleteProgram(gfx->identity_program);
}

void emit_vertex(struct gfx *gfx, float x, float y) {
    *gfx->vertex_ptr++ = x;
    *gfx->vertex_ptr++ = y;
    gfx->vertex_count += 1;
}

void octamesh_recursive(struct gfx *gfx, int depth, float x, float y) {
    float size = 1.0 / (1 << depth);

    if(depth == gfx->lod_level) {
        if(gfx->mouse_x > x-size && gfx->mouse_y > y-size &&
            gfx->mouse_x < x+size && gfx->mouse_y < y+size)
            return;

        emit_vertex(gfx, x-size, y-size);
        emit_vertex(gfx, x+size, y-size);
        emit_vertex(gfx, x-size, y+size);

        return;
    }

    for(int i = 0; i < 4; ++i) {
        float dx = (i&1) ? -1 : 1, dy = (i>>1) ? -1 : 1;
        octamesh_recursive(gfx, depth + 1, x+dx*size/2.0, y+dy*size/2.0);
    }
}

void fill_vertex_buffer(struct gfx *gfx) {
    glBindBuffer(GL_ARRAY_BUFFER, gfx->vertex_buffer);
    void *mapped_ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

    gfx->vertex_ptr = (float*)mapped_ptr;
    gfx->vertex_count = 0;

    octamesh_recursive(gfx, 0, 0.0, 0.0);

    glUnmapBuffer(GL_ARRAY_BUFFER);
}

static void paint(struct gfx *gfx, int width, int height, int frame)
{
    float t = frame / 60.0;
    (void)t;

    fill_vertex_buffer(gfx);

    for(unsigned i = 0; i < num_queries; ++i)
        glBeginQuery(query_targets[i], gfx->queries[i]);

    const float background[] = { 0.2, 0.4, 0.7, 1.0 };
    glClearBufferfv(GL_COLOR, 0, background);

    glClearBufferfi(GL_DEPTH_STENCIL, 0, 1.0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, width, height);
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    //glFrontFace(GL_CCW);

    //glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LEQUAL);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0);

    glUseProgram(gfx->identity_program);
    glBindVertexArray(gfx->identity_vertex_array);

    glDrawArrays(GL_TRIANGLES, 0, gfx->vertex_count);

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

static void main_loop(GLWTWindow *window, struct gfx *gfx)
{
    init_gfx(window, gfx);

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
        paint(gfx, width, height, frame++);

        uint64_t query_results[num_queries];
        for(unsigned i = 0; i < num_queries; ++i)
            glGetQueryObjectui64v(gfx->queries[i], GL_QUERY_RESULT, &query_results[i]);

        assert(glGetError() == GL_NO_ERROR);

        const int str_size = 1024;
        char str[str_size];
        char *ptr = str;
        for(unsigned i = 0; i < num_queries; ++i)
            ptr += snprintf(ptr, str_size - (ptr - str), "%s: %lu  ", query_names[i], query_results[i]);
        ptr += snprintf(ptr, str_size - (ptr - str), "LOD: %d  ", gfx->lod_level);
        ptr += snprintf(ptr, str_size - (ptr - str), "inner: %d  ", gfx->inner_level);
        ptr += snprintf(ptr, str_size - (ptr - str), "outer: %d  ", gfx->outer_level);
        glwtWindowSetTitle(window, str);

        glwtSwapBuffers(window);

        glwtEventHandle(0);
    }

    quit_gfx(window, gfx);
}

static void event_callback(GLWTWindow *window, const GLWTWindowEvent *event, void *userdata)
{
    (void)window;

    struct gfx *gfx = (struct gfx*)userdata;

    if(event->type == GLWT_WINDOW_KEY_DOWN)
    {
        switch(event->key.keysym)
        {
            case GLWT_KEY_PLUS:
            case GLWT_KEY_KEYPAD_PLUS:
                gfx->lod_level++;
                break;
            case GLWT_KEY_MINUS:
            case GLWT_KEY_KEYPAD_MINUS:
                if(gfx->lod_level > 0 ) gfx->lod_level--;
                break;
            default:
                break;
        }
    }

    if(event->type == GLWT_WINDOW_MOUSE_MOTION) {
        int sx = event->motion.x;
        int sy = event->motion.y;

        int width, height;
        glwtWindowGetSize(window, &width, &height);

        float x = -1.0 + 2.0 * (sx / (float)width);
        float y = 1.0 - 2.0 * (sy / (float)height);

        gfx->mouse_x = x;
        gfx->mouse_y = y;
    }
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

    struct gfx gfx;
    memset(&gfx, 0, sizeof(struct gfx));

    GLWTWindow *window = 0;
    if(glwtInit(&config, NULL, NULL) != 0 ||
        !(window = glwtWindowCreate("", 1024, 768, NULL, event_callback, &gfx)))
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

    main_loop(window, &gfx);

    glwtWindowDestroy(window);
    glwtQuit();
    return 0;
}
