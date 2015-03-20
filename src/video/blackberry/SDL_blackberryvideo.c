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

/* BlackBerry SDL video driver implementation
*/

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"
#include "../../events/SDL_windowevents_c.h"

#include "SDL_blackberryvideo.h"
#include "SDL_blackberryclipboard.h"
#include "SDL_blackberryevents.h"
#include "SDL_blackberrykeyboard.h"
#include "SDL_blackberrytouch.h"
#include "SDL_blackberrywindow.h"

#define BLACKBERRY_VID_DRIVER_NAME "BlackBerry"

/* Initialization/Query functions */
static int BlackBerry_VideoInit(_THIS);
static void BlackBerry_VideoQuit(_THIS);

#include "SDL_blackberrygl.h"

static int
BlackBerry_Available(void)
{
    return 1;
}

static void
BlackBerry_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device);
}

static SDL_VideoDevice *
BlackBerry_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;
    SDL_VideoData *data;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (SDL_VideoData*) SDL_calloc(1, sizeof(SDL_VideoData));
    if (!data) {
        SDL_OutOfMemory();
        SDL_free(device);
        return NULL;
    }

    device->driverdata = data;

    /* Set the function pointers */
    device->VideoInit = BlackBerry_VideoInit;
    device->VideoQuit = BlackBerry_VideoQuit;
    device->PumpEvents = BlackBerry_PumpEvents;

    device->CreateWindow = BlackBerry_CreateWindow;
    device->SetWindowTitle = BlackBerry_SetWindowTitle;
    device->DestroyWindow = BlackBerry_DestroyWindow;
    device->GetWindowWMInfo = BlackBerry_GetWindowWMInfo;

    device->free = BlackBerry_DeleteDevice;

    /* GL pointers */
    device->GL_LoadLibrary = BlackBerry_GLES_LoadLibrary;
    device->GL_GetProcAddress = BlackBerry_GLES_GetProcAddress;
    device->GL_UnloadLibrary = BlackBerry_GLES_UnloadLibrary;
    device->GL_CreateContext = BlackBerry_GLES_CreateContext;
    device->GL_MakeCurrent = BlackBerry_GLES_MakeCurrent;
    device->GL_SetSwapInterval = BlackBerry_GLES_SetSwapInterval;
    device->GL_GetSwapInterval = BlackBerry_GLES_GetSwapInterval;
    device->GL_SwapWindow = BlackBerry_GLES_SwapWindow;
    device->GL_DeleteContext = BlackBerry_GLES_DeleteContext;

    /* Text input */
    device->StartTextInput = BlackBerry_StartTextInput;
    device->StopTextInput = BlackBerry_StopTextInput;
    device->SetTextInputRect = BlackBerry_SetTextInputRect;

    /* Screen keyboard */
    device->HasScreenKeyboardSupport = BlackBerry_HasScreenKeyboardSupport;
    device->IsScreenKeyboardShown = BlackBerry_IsScreenKeyboardShown;

    /* Clipboard */
    device->SetClipboardText = BlackBerry_SetClipboardText;
    device->GetClipboardText = BlackBerry_GetClipboardText;
    device->HasClipboardText = BlackBerry_HasClipboardText;

    return device;
}

VideoBootStrap BlackBerry_bootstrap = {
    BLACKBERRY_VID_DRIVER_NAME, "SDL BlackBerry video driver",
    BlackBerry_Available, BlackBerry_CreateDevice
};


int
BlackBerry_VideoInit(_THIS)
{
    if (SDL_BlackBerry_Init() < 0)
        return -1;

    SDL_DisplayMode mode;

    mode.format = SDL_PIXELFORMAT_RGBX8888;
    mode.w = BlackBerry_ScreenWidth;
    mode.h = BlackBerry_ScreenHeight;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;
    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }

    SDL_AddDisplayMode(&_this->displays[0], &mode);

    BlackBerry_InitKeyboard();

    BlackBerry_InitTouch();

    /* We're done! */
    return 0;
}

void
BlackBerry_VideoQuit(_THIS)
{
}

#endif /* SDL_VIDEO_DRIVER_BLACKBERRY */

/* vi: set ts=4 sw=4 expandtab: */
