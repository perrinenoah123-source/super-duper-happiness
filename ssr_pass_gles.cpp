#include "ssr_pass_gles.h"
#include <android/log.h>
#include <string.h>

#define LOG_TAG "SSR_MOD"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

const char* VERT = R"(
    #version 300 es
    layout(location = 0) in vec2 pos;
    out vec2 uv;
    void main() {
        uv = pos * 0.5 + 0.5;
        gl_Position = vec4(pos, 0.0, 1.0);
    }
)";

const char* FRAG = R"(
    #version 300 es
    precision highp float;
    in vec2 uv;
    out vec4 fragColor;

    uniform sampler2D uColor;
    uniform sampler2D uDepth;
    uniform mat4 uProj;

    vec3 getViewPos(vec2 coord, float depth) {
        vec4 clip = vec4(coord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
        vec4 view = inverse(uProj) * clip;
        return view.xyz / view.w;
    }

    void main() {
        vec3 color = texture(uColor, uv).rgb;
        float depth = texture(uDepth, uv).r;

        if (depth >= 0.999) {
            fragColor = vec4(color, 1.0);
            return;
        }

        vec3 viewPos = getViewPos(uv, depth);
        vec3 viewNormal = normalize(-viewPos); 
        vec3 viewRay = reflect(normalize(viewPos), viewNormal);

        vec3 reflection = vec3(0.0);
        float stepSize = 0.1;
        vec3 rayPos = viewPos;
        bool hit = false;

        for (int i = 0; i < 32; i++) {
            rayPos += viewRay * stepSize;
            vec4 clipPos = uProj * vec4(rayPos, 1.0);
            vec3 ndc = clipPos.xyz / clipPos.w;
            vec2 rayUv = ndc.xy * 0.5 + 0.5;

            if (rayUv.x < 0.0 || rayUv.x > 1.0 || rayUv.y < 0.0 || rayUv.y > 1.0) break;

            float rayDepth = texture(uDepth, rayUv).r;
            vec3 rayScenePos = getViewPos(rayUv, rayDepth);

            if (abs(rayScenePos.z - rayPos.z) < 0.5) {
                reflection = texture(uColor, rayUv).rgb;
                hit = true;
                break;
            }
            stepSize *= 1.2;
        }

        if (hit) {
            fragColor = vec4(mix(color, reflection, 0.5), 1.0);
        } else {
            fragColor = vec4(color, 1.0);
        }
    }
)";

GLuint SSRRenderer::CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        LOGI("Shader Error: %s", info);
    }
    return shader;
}

void SSRRenderer::Init(int w, int h) {
    width = w;
    height = h;
    memset(proj, 0, sizeof(proj));

    GLuint vs = CompileShader(GL_VERTEX_SHADER, VERT);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, FRAG);
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    InitBuffers();

    glGenTextures(1, &color_tex);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &depth_tex);
    glBindTexture(GL_TEXTURE_2D, depth_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &ssr_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, ssr_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSRRenderer::InitBuffers() {
    float vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void SSRRenderer::SetProjection(const float* matrix) { memcpy(proj, matrix, sizeof(float) * 16); }
void SSRRenderer::SetView(const float* matrix) { /* could be used for better normal reconstruction */ }

void SSRRenderer::Render(GLuint game_fbo, int w, int h) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, game_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ssr_fbo);
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);

    glUseProgram(program);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glUniform1i(glGetUniformLocation(program, "uColor"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depth_tex);
    glUniform1i(glGetUniformLocation(program, "uDepth"), 1);

    glUniformMatrix4fv(glGetUniformLocation(program, "uProj"), 1, GL_FALSE, proj);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}