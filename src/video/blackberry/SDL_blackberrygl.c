/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "SDL_config.h"

#if SDL_VIDEO_DRIVER_BLACKBERRY

/* BlackBerry SDL video driver implementation */

#include <EGL/egl.h>

#include "../../core/blackberry/SDL_blackberry.h"

#include "SDL_blackberrygl.h"

//#include <dlfcn.h>

EGLDisplay egl_disp;
EGLSurface egl_surf;

static EGLConfig egl_conf;
static EGLContext egl_ctx;

static EGLint swapInterval = 1;

int BlackBerry_GLES_Init(_THIS) {
    LOGE("BlackBerry_GLES_Init\n");

    int usage;
    int rc, num_configs;

    EGLint attrib_list[]= { EGL_RED_SIZE,        8,
                            EGL_GREEN_SIZE,      8,
                            EGL_BLUE_SIZE,       8,
                            EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
                            EGL_RENDERABLE_TYPE, 0,
                            EGL_DEPTH_SIZE,      0,
                            EGL_NONE};

#ifdef SDL_VIDEO_OPENGL_ES
    attrib_list[9] = EGL_OPENGL_ES_BIT;
#elif defined(SDL_VIDEO_OPENGL_ES2)
    attrib_list[9] = EGL_OPENGL_ES2_BIT;
#endif

    if (_this->gl_config.depth_size != 0)
        attrib_list[11] = 24; //EGL_DEPTH_SIZE

    egl_disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_disp == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay");
        BlackBerry_GLES_DeleteContext(_this, egl_ctx);
        return -1;
    }

    rc = eglInitialize(egl_disp, NULL, NULL);
    if (rc != EGL_TRUE) {
        LOGE("eglInitialize");
        BlackBerry_GLES_DeleteContext(_this, egl_ctx);
        return -1;
    }

    rc = eglBindAPI(EGL_OPENGL_ES_API);

    if (rc != EGL_TRUE) {
        LOGE("eglBindApi");
        BlackBerry_GLES_DeleteContext(_this, egl_ctx);
        return -1;
    }

    if(!eglChooseConfig(egl_disp, attrib_list, &egl_conf, 1, &num_configs)) {
        BlackBerry_GLES_DeleteContext(_this, egl_ctx);
        return -1;
    }

    return 0;
}

int BlackBerry_GLES_LoadLibrary(_THIS, const char *path) {
    LOGE("BlackBerry_GLES_LoadLibrary\n");

    if (BlackBerry_GLES_Init(_this)<0)
        return -1;

    if (!_this->gl_config.driver_loaded) {
          _this->gl_config.driver_loaded = 1;
    }

    return 0;
}

void * BlackBerry_GLES_GetProcAddress(_THIS, const char *proc) {
    void * p = eglGetProcAddress(proc);

    if (p == NULL) {
        LOGE("!!! Error: BlackBerry_GLES_GetProcAddress: %s\n", proc);
        //void * dll_handle = dlopen(NULL, RTLD_NOW|RTLD_GLOBAL);
        //p = dlsym(dll_handle, proc);
    }

    return p;
}

void BlackBerry_GLES_UnloadLibrary(_THIS) {
    LOGE("BlackBerry_GLES_UnloadLibrary\n");

    return;
}

SDL_GLContext BlackBerry_GLES_CreateContext(_THIS, SDL_Window * window) {
    LOGE("BlackBerry_GLES_CreateContext\n");

    int rc;

#ifdef SDL_VIDEO_OPENGL_ES2
    EGLint attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    egl_ctx = eglCreateContext(egl_disp, egl_conf, EGL_NO_CONTEXT, attributes);
#elif defined(SDL_VIDEO_OPENGL_ES)
    egl_ctx = eglCreateContext(egl_disp, egl_conf, EGL_NO_CONTEXT, NULL);
#endif

    if (egl_ctx == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext");
        BlackBerry_GLES_DeleteContext(_this, egl_ctx);
        return 0;
    }

    egl_surf = eglCreateWindowSurface(egl_disp, egl_conf, BlackBerry_SYS_GetNativeWindow(), NULL);
    if (egl_surf == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface");
        BlackBerry_GLES_DeleteContext(_this, egl_ctx);
        return 0;
    }

    rc = eglMakeCurrent(egl_disp, egl_surf, egl_surf, egl_ctx);
    if (rc != EGL_TRUE) {
        LOGE("eglMakeCurrent");
        BlackBerry_GLES_DeleteContext(_this, egl_ctx);
        return 0;
    }

    rc = BlackBerry_GLES_SetSwapInterval(_this, swapInterval);
    if (rc) {
        BlackBerry_GLES_DeleteContext(_this, egl_ctx);
        return 0;
    }

    return egl_ctx;
}

int BlackBerry_GLES_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context) {
    LOGE("BlackBerry_GLES_MakeCurrent\n");

    return 0;
}

int BlackBerry_GLES_SetSwapInterval(_THIS, int interval) {
    LOGE("BlackBerry_GLES_SetSwapInterval\n");

    EGLBoolean status;
    status = eglSwapInterval(egl_disp, interval);
    if (status == EGL_TRUE) {
        swapInterval = interval;
        return 0;
    }
    return SDL_SetError("Unable to set the EGL swap interval");
}

int BlackBerry_GLES_GetSwapInterval(_THIS) {
    LOGE("BlackBerry_GLES_GetSwapInterval\n");

    return swapInterval;
}

void BlackBerry_GLES_SwapWindow(_THIS, SDL_Window * window) {
    eglSwapBuffers(egl_disp, egl_surf);
}

void BlackBerry_GLES_DeleteContext(_THIS, SDL_GLContext context) {
    LOGE("BlackBerry_GLES_DeleteContext\n");

    if (egl_disp != EGL_NO_DISPLAY) {
        eglMakeCurrent(egl_disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (egl_surf != EGL_NO_SURFACE) {
            eglDestroySurface(egl_disp, egl_surf);
            egl_surf = EGL_NO_SURFACE;
        }
        if (egl_ctx != EGL_NO_CONTEXT) {
            eglDestroyContext(egl_disp, egl_ctx);
            egl_ctx = EGL_NO_CONTEXT;
        }
        eglTerminate(egl_disp);
        egl_disp = EGL_NO_DISPLAY;
    }
    eglReleaseThread();
}

#endif /* SDL_VIDEO_DRIVER_BLACKBERRY */

/* vi: set ts=4 sw=4 expandtab: */
