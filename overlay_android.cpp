#include "overlay_android.h"
#include <android/log.h>
#include <string.h>

const char* UI_VERT = R"(
    #version 300 es
    layout(location = 0) in vec2 pos;
    void main() {
        gl_Position = vec4(pos, 0.0, 1.0);
    }
)";

// Simple 2D color box in the top left to show the mod is active
const char* UI_FRAG = R"(
    #version 300 es
    precision highp float;
    out vec4 fragColor;
    void main() {
        fragColor = vec4(1.0, 0.0, 1.0, 0.8); // Pink box
    }
)";

GLuint OverlayAndroid::CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    return shader;
}

void OverlayAndroid::Init(int width, int height) {
    GLuint vs = CompileShader(GL_VERTEX_SHADER, UI_VERT);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, UI_FRAG);
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Draw a small box at top left (-0.9 to -0.7 X, 0.7 to 0.9 Y)
    float vertices[] = { 
        -0.9f,  0.9f, 
        -0.7f,  0.9f, 
        -0.9f,  0.7f, 
        -0.7f,  0.7f 
    };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void OverlayAndroid::Render() {
    glUseProgram(program);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}