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

    unsigned axes_program;
    unsigned axes_vertex_array;

    unsigned conic_program;

    float yaw, pitch;
    int mouse_x, mouse_y;

    vec4 arcball_quat;

    float p, e;
};

static void init_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;
    gfx->axes_program = shader_load("shaders/axes/axes.glslv", "", "",  "", "shaders/axes/axes.glslf");
    gfx->conic_program = shader_load("shaders/conic/conic.glslv", "", "",  "", "shaders/conic/conic.glslf");

    glGenVertexArrays(1, &gfx->axes_vertex_array);

    glGenQueries(num_queries, gfx->queries);

    gfx->mouse_x = -1;
    gfx->mouse_y = -1;

    gfx->arcball_quat = vec(0.0, 0.0, 0.0, 1.0);

    gfx->p = 1.0;
    gfx->e = 0.0;
}

static void quit_gfx(GLWTWindow *window, struct gfx *gfx)
{
    (void)window;

    glDeleteVertexArrays(1, &gfx->axes_vertex_array);
    glDeleteProgram(gfx->axes_program);
}

static void paint(struct gfx *gfx, int width, int height, int frame)
{
    float t = frame / 60.0;
    (void)t;

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

    glUseProgram(gfx->axes_program);
    glBindVertexArray(gfx->axes_vertex_array);

    int index;
    mat4 projection_matrix = mat_perspective_fovy(M_PI/4.0, (float)width/height, 0.1, 100.0);
    index = glGetUniformLocation(gfx->axes_program, "projection_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&projection_matrix);

    //mat4 camera_rotation = mat_euler(vec(gfx->pitch, gfx->yaw, 0.0, 0.0));
    mat4 camera_rotation = quat_to_mat(gfx->arcball_quat);
    mat4 view_matrix = mmmul(
        mtranslate(vec(0.0, 0.0, -5.0, 1.0)),
        camera_rotation
        );

    index = glGetUniformLocation(gfx->axes_program, "view_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&view_matrix);

    mat4 model_matrix = midentity();
    //mat4 model_matrix = mmmul(mtranslate(vec(0, 0, -3, 0)), mat_euler(vec(0, t/5, 0, 0)));
    //mat4 model_matrix = mat_euler(vec(t/3, 0.0, 0.0, 0.0));
    //mat4 model_matrix = mat_euler(vec(gfx->pitch, gfx->yaw, 0.0, 0.0));
    index = glGetUniformLocation(gfx->axes_program, "model_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&model_matrix);

    glLineWidth(1.0);
    glDrawArrays(GL_LINES, 0, 6);

    // CONIC SECTION
    glUseProgram(gfx->conic_program);

    index = glGetUniformLocation(gfx->conic_program, "projection_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&projection_matrix);
    index = glGetUniformLocation(gfx->conic_program, "view_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&view_matrix);
    index = glGetUniformLocation(gfx->conic_program, "model_matrix");
    glUniformMatrix4fv(index, 1, GL_FALSE, (const float*)&model_matrix);


    float p = gfx->p, e = gfx->e;
    //float i = 0.0, an = 0.0, arg = 0.0;
    float maxf = e <= 1.0 ? M_PI :
        M_PI - acosf(fminf(1.0, 1.0/e));

    float maxr = 5.0; // maximum radius
    float cosf = (p/maxr - 1.0) / e;
    float maxrf = acosf(fmaxf(-1.0, fminf(1.0, cosf)));

    maxf = fminf(maxf, maxrf);

    float f1 = -maxf, f2 = maxf;

    int num_vertices = 256;
    index = glGetUniformLocation(gfx->conic_program, "p");
    glUniform1f(index, p);
    index = glGetUniformLocation(gfx->conic_program, "e");
    glUniform1f(index, e);
    index = glGetUniformLocation(gfx->conic_program, "f1");
    glUniform1f(index, f1);
    index = glGetUniformLocation(gfx->conic_program, "f2");
    glUniform1f(index, f2);
    index = glGetUniformLocation(gfx->conic_program, "num_vertices");
    glUniform1i(index, num_vertices);

    glDrawArrays(GL_LINES, 0, num_vertices);

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
        glwtWindowSetTitle(window, str);

        glwtSwapBuffers(window);

        glwtEventHandle(0);
    }

    quit_gfx(window, gfx);
}

vec4 arcball_vector(float x, float y) {
    return x*x + y*y < 1.0 ?
        vec(x, y, 1.0 - sqrt(x*x + y*y), 0.0) :
        vunit(vec(x, y, 0.0, 0.0));

}

static void event_callback(GLWTWindow *window, const GLWTWindowEvent *event, void *userdata)
{
    struct gfx *gfx = (struct gfx*)userdata;

    int width, height;
    glwtWindowGetSize(window, &width, &height);

    if(event->type == GLWT_WINDOW_MOUSE_MOTION) {

        if(event->motion.buttons & 1 &&
            gfx->mouse_x != -1 && gfx->mouse_y != -1) {
            int old_x = gfx->mouse_x, old_y = gfx->mouse_y;
            int new_x = event->motion.x, new_y = event->motion.y;
            vec4 arcball_old = arcball_vector(2.0 * old_x/width - 1.0, -2.0*old_y/height + 1.0);
            vec4 arcball_new = arcball_vector(2.0 * new_x/width - 1.0, -2.0*new_y/height + 1.0);

            vec4 arcball_half = vunit(arcball_old + arcball_new);
            vec4 axis = vcross(arcball_old, arcball_half);
            axis[3] = vdot(arcball_old, arcball_half)[0];
            vec4 quat = vunit(axis);

            gfx->arcball_quat = qprod(quat, gfx->arcball_quat);

            /*
            int dx = event->motion.x - gfx->mouse_x;
            int dy = event->motion.y - gfx->mouse_y;

            float sensitivity = 2.0;
            gfx->yaw += sensitivity * (float)dx / width;
            gfx->pitch += sensitivity * (float)dy / height;
            */
        }

        gfx->mouse_x = event->motion.x;
        gfx->mouse_y = event->motion.y;
    }

    if(event->type == GLWT_WINDOW_KEY_DOWN) {
        switch(event->key.keysym) {
        case 'P':
            gfx->p = fmaxf(0.0, gfx->p + 0.1*(event->key.mod & GLWT_MOD_SHIFT ? -1.0 : 1.0));
            break;
        case 'E':
            gfx->e = fmaxf(0.0, gfx->e + 0.1*(event->key.mod & GLWT_MOD_SHIFT ? -1.0 : 1.0));
            break;
        default:
            break;
        };
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
        3, 3
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
