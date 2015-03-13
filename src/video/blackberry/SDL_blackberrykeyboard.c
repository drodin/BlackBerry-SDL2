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
#ifndef __PLAYBOOK__
#include "SDL_blackberrykeymap.h"
#else
#include "SDL_playbookkeymap.h"
#endif

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
BlackBerry_OnKey(SDL_Keysym keysym, Uint8 state)
{
    int scancode;

    SDL_SetModState(keysym.mod);

    if (keysym.scancode == SDL_SCANCODE_UNKNOWN) {
        for (scancode = SDL_SCANCODE_UNKNOWN; scancode < SDL_NUM_SCANCODES; ++scancode) {
            if (SDL_BlackBerry_keymap[scancode] == keysym.sym) {
                keysym.scancode = scancode;
            }
        }
    }

    return SDL_SendKeyboardKey(state, keysym.scancode);
}

int
BlackBerry_OnKeyDown(SDL_Keysym keysym)
{
    return BlackBerry_OnKey(keysym, SDL_PRESSED);
}

int
BlackBerry_OnKeyUp(SDL_Keysym keysym)
{
    return BlackBerry_OnKey(keysym, SDL_RELEASED);
}

SDL_bool
BlackBerry_HasScreenKeyboardSupport(_THIS)
{
    return SDL_TRUE;
}

SDL_bool
BlackBerry_IsScreenKeyboardShown(_THIS, SDL_Window * window)
{
    return textInputActive;
}

void
BlackBerry_StartTextInput(_THIS)
{
    if (textInputActive)
        return;
    textInputActive = SDL_TRUE;
    SDL_VideoData *videodata = (SDL_VideoData *)_this->driverdata;
    BlackBerry_SYS_ShowTextInput(&videodata->textRect);
}

void
BlackBerry_StopTextInput(_THIS)
{
    if (!textInputActive)
        return;
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

int
BlackBerry_SendTextInput(SDL_Keysym keysym)
{
    char text[5];
    char *end;

    if (keysym.unused && (keysym.sym <= UNICODE_PRIVATE_USE_AREA_FIRST || keysym.sym >= UNICODE_PRIVATE_USE_AREA_LAST)) {
        end = SDL_UCS4ToUTF8((Uint32) keysym.sym, text);
        *end = '\0';
        return SDL_SendKeyboardText(text);
    } else if (keysym.sym == KEYCODE_RETURN) {
        return SDL_SendKeyboardKey((keysym.unused)?SDL_PRESSED:SDL_RELEASED, SDL_SCANCODE_RETURN);
    } else if (keysym.sym == KEYCODE_BACKSPACE) {
        return SDL_SendKeyboardKey((keysym.unused)?SDL_PRESSED:SDL_RELEASED, SDL_SCANCODE_BACKSPACE);
    }

    return 0;
}

#endif /* SDL_VIDEO_DRIVER_BLACKBERRY */

/* vi: set ts=4 sw=4 expandtab: */
