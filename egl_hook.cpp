#include "egl_hook.h"
#include "ssr_pass_gles.h"
#include "overlay_android.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/log.h>
#include "dobby.h"
#include <string.h>

#define LOG_TAG "SSR_MOD"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surface);
static eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

typedef void (*glBindFramebuffer_t)(GLenum target, GLuint framebuffer);
static glBindFramebuffer_t orig_glBindFramebuffer = nullptr;

typedef void (*glUniformMatrix4fv_t)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
static glUniformMatrix4fv_t orig_glUniformMatrix4fv = nullptr;

typedef __eglMustCastToProperFunctionPointerType (*eglGetProcAddress_t)(const char* procname);
static eglGetProcAddress_t orig_eglGetProcAddress = nullptr;

static SSRRenderer* ssr_renderer = nullptr;
static OverlayAndroid* overlay = nullptr;
static GLuint game_fbo = 0;
static int screen_width = 0;
static int screen_height = 0;

void hooked_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    if (ssr_renderer && count == 1 && value) {
        if (value[15] == 0.0f) {
            ssr_renderer->SetProjection(value);
        } else if (value[15] == 1.0f && value[3] == 0.0f && value[7] == 0.0f && value[11] == 0.0f) {
            ssr_renderer->SetView(value);
        }
    }
    orig_glUniformMatrix4fv(location, count, transpose, value);
}

void hooked_glBindFramebuffer(GLenum target, GLuint framebuffer) {
    if (target == GL_FRAMEBUFFER && framebuffer != 0) {
        game_fbo = framebuffer; 
    }
    orig_glBindFramebuffer(target, framebuffer);
}

__eglMustCastToProperFunctionPointerType hooked_eglGetProcAddress(const char* procname) {
    __eglMustCastToProperFunctionPointerType result = orig_eglGetProcAddress(procname);
    if (result) {
        if (strcmp(procname, "glBindFramebuffer") == 0) return (__eglMustCastToProperFunctionPointerType)hooked_glBindFramebuffer;
        if (strcmp(procname, "glUniformMatrix4fv") == 0) return (__eglMustCastToProperFunctionPointerType)hooked_glUniformMatrix4fv;
    }
    return result;
}

EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!ssr_renderer) {
        EGLint w, h;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
        screen_width = w;
        screen_height = h;
        
        ssr_renderer = new SSRRenderer();
        ssr_renderer->Init(w, h);
        
        overlay = new OverlayAndroid();
        overlay->Init(w, h);
    }

    if (game_fbo != 0) {
        ssr_renderer->Render(game_fbo, screen_width, screen_height);
    }
    
    // Draw UI overlay on top (or bottom depending on your preference)
    if (overlay) {
        overlay->Render();
    }

    return orig_eglSwapBuffers(dpy, surface);
}

void init_egl_hook() {
    void* handle = dlopen("libEGL.so", RTLD_NOW);
    if (handle) {
        void* target = dlsym(handle, "eglSwapBuffers");
        if (target) DobbyHook(target, (void*)hooked_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
        
        target = dlsym(handle, "eglGetProcAddress");
        if (target) DobbyHook(target, (void*)hooked_eglGetProcAddress, (void**)&orig_eglGetProcAddress);
    }

    handle = dlopen("libGLESv2.so", RTLD_NOW);
    if (handle) {
        void* target = dlsym(handle, "glBindFramebuffer");
        if (target) DobbyHook(target, (void*)hooked_glBindFramebuffer, (void**)&orig_glBindFramebuffer);
        
        target = dlsym(handle, "glUniformMatrix4fv");
        if (target) DobbyHook(target, (void*)hooked_glUniformMatrix4fv, (void**)&orig_glUniformMatrix4fv);
    }
}