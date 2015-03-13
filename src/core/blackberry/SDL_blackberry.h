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

#ifndef SDL_blackberry_h
#define SDL_blackberry_h

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

#include <screen/screen.h>

#include "SDL_power.h"
#include "SDL_rect.h"

#ifdef __BLACKBERRY_DEBUG__
#define LOGE(...)  fprintf(stderr,__VA_ARGS__)
#else
#define LOGE(...) do {} while (0)
#endif

extern int SDL_BlackBerry_Init();

extern int BlackBerry_SYS_InitAccelerometer();
extern int BlackBerry_SYS_InitController();

/* Interface from the SDL library into the BlackBerry System */
extern void BlackBerry_SYS_SetActivityTitle(const char *title);
extern SDL_bool BlackBerry_SYS_GetAccelerometerValues(float values[3]);
extern SDL_bool BlackBerry_SYS_GetControllerInfo(int device_id, int* buttons, int* axes);
extern SDL_bool BlackBerry_SYS_GetControllerValues(int device_id, int* buttons, float* axes);
extern void BlackBerry_SYS_ShowTextInput(SDL_Rect *inputRect);
extern void BlackBerry_SYS_HideTextInput();
extern screen_window_t BlackBerry_SYS_GetNativeWindow();
extern char* BlackBerry_SYS_GetNativeWindowGroup();
extern void BlackBerry_SYS_ReleaseNativeWindow(screen_window_t native_window);
extern void BlackBerry_SYS_ProcessEvents();

/* Clipboard support */
int BlackBerry_SYS_SetClipboardText(const char* text);
char* BlackBerry_SYS_GetClipboardText();
SDL_bool BlackBerry_SYS_HasClipboardText();

/* Power support */
int BlackBerry_SYS_GetPowerInfo(SDL_PowerState* state, int* seconds, int* percent);

/* Touch support */
int BlackBerry_SYS_GetTouchDeviceIds(int **ids);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif //SDL_blackberry_h

/* vi: set ts=4 sw=4 expandtab: */
