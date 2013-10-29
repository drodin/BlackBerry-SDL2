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

#include "SDL_syswm.h"
#include "../SDL_sysvideo.h"
#include "../../events/SDL_keyboard_c.h"
#include "../../events/SDL_mouse_c.h"

#include "SDL_blackberryvideo.h"
#include "SDL_blackberrywindow.h"

int
BlackBerry_CreateWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data;

    if (BlackBerry_Window) {
        return SDL_SetError("BlackBerry only supports one window");
    }

    /* Adjust the window data to match the screen */
    window->x = 0;
    window->y = 0;
    window->w = BlackBerry_ScreenWidth;
    window->h = BlackBerry_ScreenHeight;

    window->flags &= ~SDL_WINDOW_RESIZABLE; /* window is NEVER resizeable */
    window->flags |= SDL_WINDOW_FULLSCREEN; /* window is always fullscreen */
    window->flags &= ~SDL_WINDOW_HIDDEN;
    window->flags |= SDL_WINDOW_SHOWN; /* only one window on BlackBerry */
    window->flags |= SDL_WINDOW_INPUT_FOCUS; /* always has input focus */

    /* One window, it always has focus */
    SDL_SetMouseFocus(window);
    SDL_SetKeyboardFocus(window);

    data = (SDL_WindowData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        return SDL_OutOfMemory();
    }

    data->native_window = BlackBerry_SYS_GetNativeWindow();
    data->window_group = BlackBerry_SYS_GetNativeWindowGroup();

    if (!data->native_window) {
        SDL_free(data);
        return SDL_SetError("Could not fetch native window");
    }

    data->egl_surface = SDL_EGL_CreateSurface(_this, (NativeWindowType) data->native_window);

    if (data->egl_surface == EGL_NO_SURFACE ) {
        SDL_free(data);
        return SDL_SetError("Could not create GLES window surface");
    }

    window->driverdata = data;
    BlackBerry_Window = window;

    return 0;
}

void
BlackBerry_SetWindowTitle(_THIS, SDL_Window * window)
{
    BlackBerry_SYS_SetActivityTitle(window->title);
}

void
BlackBerry_DestroyWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data;

    if (window == BlackBerry_Window) {
        BlackBerry_Window = NULL;

        if (window->driverdata) {
            data = (SDL_WindowData *) window->driverdata;
            if (data->native_window) {
                BlackBerry_SYS_ReleaseNativeWindow(data->native_window);
            }
            SDL_free(window->driverdata);
            window->driverdata = NULL;
        }
    }
}

SDL_bool
BlackBerry_GetWindowWMInfo(_THIS, SDL_Window * window,
    struct SDL_SysWMinfo * info)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;

    info->subsystem = SDL_SYSWM_BLACKBERRY;
    info->info.blackberry.window = data->native_window;
    info->info.blackberry.window_group = data->window_group;
    return SDL_TRUE;
}

#endif /* SDL_VIDEO_DRIVER_BLACKBERRY */

/* vi: set ts=4 sw=4 expandtab: */
