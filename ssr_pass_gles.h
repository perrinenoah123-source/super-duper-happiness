#pragma once
#include <GLES3/gl3.h>

class SSRRenderer {
public:
    void Init(int width, int height);
    void Render(GLuint game_fbo, int width, int height);
    void SetProjection(const float* matrix);
    void SetView(const float* matrix);

private:
    GLuint program = 0;
    GLuint vao = 0, vbo = 0;
    GLuint ssr_fbo = 0;
    GLuint color_tex = 0, depth_tex = 0;
    int width = 0, height = 0;

    float proj[16];
    float view[16];

    GLuint CompileShader(GLenum type, const char* source);
    void InitBuffers();
};