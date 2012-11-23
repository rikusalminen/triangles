#include <stdio.h>
#include <stdlib.h>

#include <GLXW/glxw.h>

static GLuint compile_shader(GLenum shader_type, const char *src)
{
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE)
    {
        GLint size;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);

        char buffer[size+1];
        GLsizei len;
        glGetShaderInfoLog(shader, size, &len, buffer);
        buffer[len] = 0;

        fprintf(stderr, "Shader compiler error:\n%s\n", buffer);

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint link_program(const GLuint *shaders, int num_shaders)
{
    GLuint prog = glCreateProgram();

    for(int i = 0; i < num_shaders; ++i)
        if(shaders[i])
            glAttachShader(prog, shaders[i]);

    glLinkProgram(prog);

    GLint status;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if(status != GL_TRUE)
    {
        GLint size;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &size);

        char buffer[size+1];
        GLsizei len;
        glGetProgramInfoLog(prog, size, &len, buffer);
        buffer[len] = 0;

        fprintf(stderr, "Program linking error:\n%s\n", buffer);

        glDeleteProgram(prog);
        return 0;
    }

    return prog;
}

static char *readfile(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if(!f) return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *ptr = malloc(len+1);
    if(!ptr)
        goto err;

    if(fread(ptr, 1, len, f) != (size_t)len)
        goto err;

    ptr[len] = 0;

    fclose(f);
    return ptr;

err:
    fclose(f);
    free(ptr);
    return NULL;
}

unsigned shader_load(const char *vert, const char *tess_ctrl, const char *tess_eval, const char *geom, const char *frag)
{
    GLenum types[] = {
        GL_VERTEX_SHADER,
        GL_TESS_CONTROL_SHADER,
        GL_TESS_EVALUATION_SHADER,
        GL_GEOMETRY_SHADER,
        GL_FRAGMENT_SHADER
    };
    const int num_shaders = sizeof(types)/sizeof(*types);
    const char *filenames[] = { vert, tess_ctrl, tess_eval, geom, frag };
    GLuint shaders[num_shaders];

    for(int i = 0; i < num_shaders; ++i)
    {
        shaders[i] = 0;

        if(!filenames[i] || !filenames[i][0]) continue;
        const char *src = readfile(filenames[i]);
        if(!src) continue;
        shaders[i] = compile_shader(types[i], src);
        free((void*)src);
    }

    GLuint prog = link_program(shaders, num_shaders);

    for(int i = 0; i < num_shaders; ++i)
    {
        if(shaders[i])
            glDeleteShader(shaders[i]);
    }

    return prog;
}
