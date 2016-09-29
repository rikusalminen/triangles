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
    unsigned triangle_count;
    int dirty_vertex_buffer;
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

    gfx->triangle_count = 32;
    gfx->dirty_vertex_buffer = 1;
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

void emit_quad(struct gfx *gfx, float x, float y, float size) {
    // north
    emit_vertex(gfx, x, y);
    emit_vertex(gfx, x+size, y+size);
    emit_vertex(gfx, x-size, y+size);

    // east
    emit_vertex(gfx, x, y);
    emit_vertex(gfx, x+size, y-size);
    emit_vertex(gfx, x+size, y+size);

    // south
    emit_vertex(gfx, x, y);
    emit_vertex(gfx, x-size, y-size);
    emit_vertex(gfx, x+size, y-size);

    // west
    emit_vertex(gfx, x, y);
    emit_vertex(gfx, x-size, y+size);
    emit_vertex(gfx, x-size, y-size);

}

void emit_quad_corner(struct gfx *gfx, float x, float y, float size, int corner) {
    if(corner == 0) { // XXX: ugly repetition!!
        // west north west
        emit_vertex(gfx, x-size, y);
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x-size, y+size);

        // south south east
        emit_vertex(gfx, x, y-size);
        emit_vertex(gfx, x+size, y-size);
        emit_vertex(gfx, x, y);

        // north
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x+size, y+size);
        emit_vertex(gfx, x-size, y+size);

        // east
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x+size, y-size);
        emit_vertex(gfx, x+size, y+size);
    } else if(corner == 1) {
        // south south west
        emit_vertex(gfx, x, y-size);
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x-size, y-size);

        // east north east
        emit_vertex(gfx, x+size, y);
        emit_vertex(gfx, x+size, y+size);
        emit_vertex(gfx, x, y);

        // west
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x-size, y+size);
        emit_vertex(gfx, x-size, y-size);

        // north
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x+size, y+size);
        emit_vertex(gfx, x-size, y+size);
    } else if(corner == 2) {
        // north north east
        emit_vertex(gfx, x, y+size);
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x+size, y+size);

        // west south west
        emit_vertex(gfx, x-size, y);
        emit_vertex(gfx, x-size, y-size);
        emit_vertex(gfx, x, y);

        // east
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x+size, y-size);
        emit_vertex(gfx, x+size, y+size);

        // south
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x-size, y-size);
        emit_vertex(gfx, x+size, y-size);
    } else if(corner == 3) {
        // east south east
        emit_vertex(gfx, x+size, y);
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x+size, y-size);

        // north north west
        emit_vertex(gfx, x, y+size);
        emit_vertex(gfx, x-size, y+size);
        emit_vertex(gfx, x, y);

        // south
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x-size, y-size);
        emit_vertex(gfx, x+size, y-size);

        // west
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x-size, y+size);
        emit_vertex(gfx, x-size, y-size);
    }
}

void emit_quad_edge(struct gfx *gfx, float x, float y, float size, int edge) {
    if(edge == 0) {  /// XXX: ugly repetition
        // west north west
        emit_vertex(gfx, x-size, y);
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x-size, y+size);

        // east north east
        emit_vertex(gfx, x+size, y);
        emit_vertex(gfx, x+size, y+size);
        emit_vertex(gfx, x, y);

        // north
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x+size, y+size);
        emit_vertex(gfx, x-size, y+size);
    } else if(edge == 1) {
        // north north east
        emit_vertex(gfx, x, y+size);
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x+size, y+size);

        // south south east
        emit_vertex(gfx, x, y-size);
        emit_vertex(gfx, x+size, y-size);
        emit_vertex(gfx, x, y);

        // east
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x+size, y-size);
        emit_vertex(gfx, x+size, y+size);
    } else if(edge == 2) {
        // west south west
        emit_vertex(gfx, x-size, y);
        emit_vertex(gfx, x-size, y-size);
        emit_vertex(gfx, x, y);

        // east south east
        emit_vertex(gfx, x+size, y);
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x+size, y-size);

        // south
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x-size, y-size);
        emit_vertex(gfx, x+size, y-size);
    } else if(edge == 3) {
        // north north west
        emit_vertex(gfx, x, y+size);
        emit_vertex(gfx, x-size, y+size);
        emit_vertex(gfx, x, y);

        // south south west
        emit_vertex(gfx, x, y-size);
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x-size, y-size);

        // west
        emit_vertex(gfx, x, y);
        emit_vertex(gfx, x-size, y+size);
        emit_vertex(gfx, x-size, y-size);
    }
}

static inline int min(int x, int y) { return x < y ? x : y; }
static inline int max(int x, int y) { return x > y ? x : y; }

int octamesh_recursive(struct gfx *gfx, int depth, int x, int y) {
    int org_x = (gfx->lod_level == 0) ?
        (gfx->mouse_x < 0.0 ? 0 : 1) :
        (int)rintf((gfx->mouse_x+1.0)/2.0 * (1 << gfx->lod_level));
    int org_y = (gfx->lod_level == 0) ?
        (gfx->mouse_y < 0.0 ? 0 : 1) :
        (int)rintf((gfx->mouse_y+1.0)/2.0 * (1 << gfx->lod_level));


#if 0
    // clamp org_x, org_y to inside this quad
    int fac = 1 << (gfx->lod_level - depth);
    org_x = max(fac*x, min(org_x, fac*(x+1)));
    org_y = max(fac*y, min(org_y, fac*(y+1)));
#endif

#if 0
    if(depth == 0) {
        float sx = -1.0 + 2.0*org_x/(1 << gfx->lod_level);
        float sy = -1.0 + 2.0*org_y/(1 << gfx->lod_level);
        emit_quad(gfx, sx, sy, 0.01);
    }
#endif

    if(depth > gfx->lod_level) {
        // XXX: terminator!
        return 0;
    }

    int size = 1 << depth;
    float screenpos[2] = { -1.0 + 2.0 * (x+0.5)/size, -1.0 + 2.0 * (y+0.5)/size };
    float mouse[2] = { gfx->mouse_x, gfx->mouse_y };
    int major_axis = fabsf(screenpos[0] - mouse[0]) < fabsf(screenpos[1] - mouse[1]) ? 0 : 1;
    int major = (mouse[major_axis] < screenpos[major_axis]) ? 0 : 1;
    int minor = (mouse[!major_axis] < screenpos[!major_axis]) ? 0 : 1;

    int child_mask = 0;
    int num_children = 0;
    for(int i = 0; i < 4; ++i) {
        int child[2] = { x << 1, y << 1 };
        child[major_axis] |= (i & 1) ^ major;
        child[!major_axis] |= (i >> 1) ^ minor;

        if(depth > gfx->lod_level) continue; // XXX: debugging

        // clamp origin to edge
        int shift = max(0, gfx->lod_level - depth - 1);
        int cx = max(child[0] << shift, min(org_x, (child[0]+1) << shift));
        int cy = max(child[1] << shift, min(org_y, (child[1]+1) << shift));

        printf("%*c  child %d: (%d, %d)\n", 4*depth, ' ', i, child[0], child[1]);

        int dx = cx - org_x;
        int dy = cy - org_y;

        int oddmask = (1 << max(0, gfx->lod_level - depth - 1));
        if(org_x & oddmask) {
            printf("%*c  odd x  org: %d  mask: %x  d: (%d, %d)\n", 4*depth, ' ', org_x, oddmask, dx, dy);
            dx = max(0, fabs(dx)-1);
        }

        if(org_y & oddmask) {
            printf("%*c  odd y  org: %d  mask: %x  d: (%d, %d)\n", 4*depth, ' ', org_y, oddmask, dx, dy);
            dy = max(0, fabs(dy)-1);
        }

        int dist = abs(dx) + abs(dy); // manhattan distance!
        int threshold = max(0, (gfx->lod_level - depth) - 1); // (1 << (gfx->lod_level - depth));

        printf("%*c  depth: %d  quad: %d  dist: %d  threshold: %d\n", 4*depth, ' ', depth, i, dist, threshold);
        printf("%*c  c: (%d, %d)   org: (%d, %d)\n\n", 4*depth, ' ', cx, cy, org_x, org_y);

        if(dist > threshold) {
            continue;
        }

        if(octamesh_recursive(gfx, depth + 1, child[0], child[1])) {
            int xy = ((major ^ (i & 1)) << (major_axis)) |
                ((minor ^ (i >> 1)) << (!major_axis));
            int mask = 1 << xy;
            child_mask |= mask;
            num_children += 1;
        }
    }

    printf("%*c  patch  num_children: %d  mask: %d\n", 4*depth, ' ', num_children, child_mask);

    if(num_children == 0) {
        emit_quad(gfx, screenpos[0], screenpos[1], 1.0/size);
    }

    if(num_children == 1) {
        int corner = 31 - __builtin_clz(child_mask);
        printf("%*c  patch  corner %d\n", 4*depth, ' ', corner);

        emit_quad_corner(gfx, screenpos[0], screenpos[1], 1.0/size, corner);

    }

    if(num_children == 2) {
        int edge = -1;
        if(child_mask == 3) edge = 0; // XXX: this is ugly!
        if(child_mask == 5) edge = 1;
        if(child_mask == 12) edge = 2;
        if(child_mask == 10) edge = 3;

        printf("%*c  patch  edge %d\n", 4*depth, ' ', edge);
        emit_quad_edge(gfx, screenpos[0], screenpos[1], 1.0/size, edge);
    }

    //emit_patched_quad(gfx, x, y, size, num_children, child_mask);

    return 1;
}

void fill_vertex_buffer(struct gfx *gfx) {
    glBindBuffer(GL_ARRAY_BUFFER, gfx->vertex_buffer);
    void *mapped_ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

    gfx->vertex_ptr = (float*)mapped_ptr;
    gfx->vertex_count = 0;

    octamesh_recursive(gfx, 0, 0.0, 0.0);
    printf("\n\n");

    glUnmapBuffer(GL_ARRAY_BUFFER);
}

static void paint(struct gfx *gfx, int width, int height, int frame)
{
    float t = frame / 60.0;
    (void)t;

    if(gfx->dirty_vertex_buffer) {
        fill_vertex_buffer(gfx);
        gfx->dirty_vertex_buffer = 0;
    }

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

    //glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LEQUAL);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0);

    glUseProgram(gfx->identity_program);
    glBindVertexArray(gfx->identity_vertex_array);

    unsigned vertex_count = (gfx->vertex_count < 3*gfx->triangle_count) ?
        gfx->vertex_count : 3*gfx->triangle_count;
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);

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
                gfx->dirty_vertex_buffer = 1;
                break;
            case GLWT_KEY_MINUS:
            case GLWT_KEY_KEYPAD_MINUS:
                if(gfx->lod_level > 0 ) gfx->lod_level--;
                gfx->dirty_vertex_buffer = 1;
                break;
            case GLWT_KEY_T:
                gfx->triangle_count += (event->key.mod & GLWT_MOD_SHIFT) ?
                    (gfx->triangle_count > 0 ? -1 : 0) : 1;
                gfx->dirty_vertex_buffer = 1;
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

        if(gfx->mouse_x != x || gfx->mouse_y != y)
            gfx->dirty_vertex_buffer = 1;

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
