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

    unsigned ico_program;
    unsigned ico_vertex_array;

    int lod_level;
    int outer_level, inner_level;
};

static void init_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;
    gfx->ico_program = shader_load("shaders/cubesphere/cubesphere.glslv", "shaders/cubesphere/cubesphere.glsltc", "shaders/cubesphere/cubesphere.glslte",  "shaders/cubesphere/cubesphere.glslg", "shaders/cubesphere/cubesphere.glslf");


    glGenVertexArrays(1, &gfx->ico_vertex_array);

    glGenQueries(num_queries, gfx->queries);
}

static void quit_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;

    glDeleteVertexArrays(1, &gfx->ico_vertex_array);
    glDeleteProgram(gfx->ico_program);
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
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glUseProgram(gfx->ico_program);
    glBindVertexArray(gfx->ico_vertex_array);

    int index;
    mat4 projection_matrix = mat_perspective_fovy(M_PI/4.0, (float)width/height, 0.1, 100.0);
    index = glGetUniformLocation(gfx->ico_program, "projection_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&projection_matrix);

    /*
    mat4 view_matrix = mtranslate(vec(0.0, 0.0, -5.0, 1.0));
    index = glGetUniformLocation(gfx->program, "view_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&view_matrix);
    */


    //mat4 model_matrix = midentity();
    mat4 model_matrix = mmmul(mtranslate(vec(0, 0, -3, 0)), mat_euler(vec(0, t, 0, 0)));
    //mat4 model_matrix = mat_euler(vec(t/3, t, 0.0, 0.0));
    index = glGetUniformLocation(gfx->ico_program, "model_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&model_matrix);

    index = glGetUniformLocation(gfx->ico_program, "lod_level");
    glUniform1i(index, gfx->lod_level);

    index = glGetUniformLocation(gfx->ico_program, "outer_level");
    glUniform1i(index, gfx->outer_level);

    index = glGetUniformLocation(gfx->ico_program, "inner_level");
    glUniform1i(index, gfx->inner_level);

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xffff);

    glPointSize(5.0);
    glLineWidth(1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glPatchParameteri(GL_PATCH_VERTICES, 3);
    glDrawArrays(GL_PATCHES, 0, 18);

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

#define GL_TEXTURE_SPARSE_ARB             0x91A6
#define GL_VIRTUAL_PAGE_SIZE_INDEX_ARB    0x91A7
#define GL_NUM_SPARSE_LEVELS_ARB          0x91AA
#define GL_NUM_VIRTUAL_PAGE_SIZES_ARB     0x91A8
#define GL_VIRTUAL_PAGE_SIZE_X_ARB        0x9195
#define GL_VIRTUAL_PAGE_SIZE_Y_ARB        0x9196
#define GL_VIRTUAL_PAGE_SIZE_Z_ARB        0x9197
#define GL_MAX_SPARSE_TEXTURE_SIZE_ARB    0x9198
#define GL_MAX_SPARSE_3D_TEXTURE_SIZE_ARB 0x9199
#define GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS_ARB 0x919A
#define GL_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_ARB 0x91A9

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

    int max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    printf("Max texture size: %d\n", max_texture_size);

    int max_3d_texture_size;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_3d_texture_size);
    printf("Max 3d texture size: %d\n", max_3d_texture_size);

    int max_array_texture_layers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_array_texture_layers);
    printf("Max array texture size: %d\n", max_array_texture_layers);

    uint64_t ms = max_texture_size, ms3 = max_3d_texture_size;
    uint64_t total_pixels = ms*ms*ms3;
    printf("total_pixels: %lu  (%lu G)\n", total_pixels, total_pixels / (1024*1024*1024));

    int num_compressed_texture_formats;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &num_compressed_texture_formats);
    printf("num_compressed_texture_formats: %d\n", num_compressed_texture_formats);

    int max_size[4] = { 0, 0, 0, 0 };
    glGetIntegerv(GL_MAX_SPARSE_TEXTURE_SIZE_ARB, max_size);
    printf("sparse texture_max_size: %dx%dx%dx%d\n",
        max_size[0], max_size[1], max_size[2], max_size[3]);

    int compressed_formats[num_compressed_texture_formats];
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, compressed_formats);
    for(int i = 0; i < num_compressed_texture_formats; ++i) {
        int fmt = compressed_formats[i];
        printf("%X\n", fmt);

        int num_page_sizes = -1;
        glGetInternalformativ(GL_TEXTURE_2D, fmt, GL_NUM_VIRTUAL_PAGE_SIZES_ARB, 1, &num_page_sizes);

        int page_size_x[num_page_sizes], page_size_y[num_page_sizes], page_size_z[num_page_sizes];
        glGetInternalformativ(GL_TEXTURE_2D, fmt, GL_VIRTUAL_PAGE_SIZE_X_ARB, num_page_sizes, page_size_x);
        glGetInternalformativ(GL_TEXTURE_2D, fmt, GL_VIRTUAL_PAGE_SIZE_Y_ARB, num_page_sizes, page_size_y);
        glGetInternalformativ(GL_TEXTURE_2D, fmt, GL_VIRTUAL_PAGE_SIZE_Z_ARB, num_page_sizes, page_size_z);

        for(int j = 0; j < num_page_sizes; ++j) {
            printf("%dx%dx%d\t", page_size_x[j], page_size_y[j], page_size_z[j]);
        }
        printf("\n");
    }

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
                if(gfx->lod_level > 0) gfx->lod_level--;
                break;
            case GLWT_KEY_O:
                gfx->outer_level += (event->key.mod & GLWT_MOD_SHIFT) ? -1 : 1;
                break;
            case GLWT_KEY_I:
                gfx->inner_level += (event->key.mod & GLWT_MOD_SHIFT) ? -1 : 1;
                break;
            default:
                break;
        }
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
        4, 4
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
