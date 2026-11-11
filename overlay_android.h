#pragma once
#include <GLES3/gl3.h>

class OverlayAndroid {
public:
    void Init(int width, int height);
    void Render();
private:
    GLuint program = 0;
    GLuint vao = 0, vbo = 0;
    GLuint CompileShader(GLenum type, const char* source);
};