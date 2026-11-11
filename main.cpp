#include <jni.h>
#include <android/log.h>
#include "egl_hook.h"

#define LOG_TAG "SSR_MOD"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

__attribute__((constructor))
void init() {
    LOGI("Roblox SSR Mod loading...");
    init_egl_hook();
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnLoad called");
    return JNI_VERSION_1_6;
}