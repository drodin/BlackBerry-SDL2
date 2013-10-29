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

#include "../../events/SDL_events_c.h"

#include "SDL_blackberrykeyboard.h"

#include "../../core/blackberry/SDL_blackberry.h"

static SDL_bool textInputActive = SDL_FALSE;

void BlackBerry_InitKeyboard(void)
{
    SDL_Keycode keymap[SDL_NUM_SCANCODES];

    /* Add default scancode to key mapping */
    SDL_GetDefaultKeymap(keymap);
    SDL_SetKeymap(0, keymap, SDL_NUM_SCANCODES);
}

int
BlackBerry_OnKeyDown(SDL_Keysym keysym)
{
    if (textInputActive) {
        static char text[8];
        char *end;
        end = SDL_UCS4ToUTF8((Uint32) keysym.sym, text);
        *end = '\0';
        SDL_SendKeyboardText(text);
    }
    SDL_SetModState(keysym.mod);
    return SDL_SendKeyboardKey(SDL_PRESSED, keysym.scancode);
}

int
BlackBerry_OnKeyUp(SDL_Keysym keysym)
{
    SDL_SetModState(keysym.mod);
    return SDL_SendKeyboardKey(SDL_RELEASED, keysym.scancode);
}

SDL_bool
BlackBerry_HasScreenKeyboardSupport(_THIS)
{
    return SDL_TRUE;
}

SDL_bool
BlackBerry_IsScreenKeyboardShown(_THIS, SDL_Window * window)
{
    return SDL_IsTextInputActive();
}

void
BlackBerry_StartTextInput(_THIS)
{
    textInputActive = SDL_TRUE;
    SDL_VideoData *videodata = (SDL_VideoData *)_this->driverdata;
    BlackBerry_SYS_ShowTextInput(&videodata->textRect);
}

void
BlackBerry_StopTextInput(_THIS)
{
    textInputActive = SDL_FALSE;
    BlackBerry_SYS_HideTextInput();
}

void
BlackBerry_SetTextInputRect(_THIS, SDL_Rect *rect)
{
    SDL_VideoData *videodata = (SDL_VideoData *)_this->driverdata;

    if (!rect) {
        SDL_InvalidParamError("rect");
        return;
    }

    videodata->textRect = *rect;
}

#endif /* SDL_VIDEO_DRIVER_BLACKBERRY */

/* vi: set ts=4 sw=4 expandtab: */
