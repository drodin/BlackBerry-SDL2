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

#include "SDL_events.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_touch_c.h"
#include "SDL_log.h"

#include "SDL_blackberrytouch.h"

#include "../../core/blackberry/SDL_blackberry.h"

#define ACTION_DOWN 100
#define ACTION_MOVE 101
#define ACTION_UP 102

static SDL_FingerID leftFingerDown = 0;

void BlackBerry_InitTouch(void)
{
    int i;
    int* ids;
    int number = BlackBerry_SYS_GetTouchDeviceIds(&ids);
    if (0 < number) {
        for (i = 0; i < number; ++i) {
            SDL_AddTouch((SDL_TouchID) ids[i], ""); /* no error handling */
        }
        SDL_free(ids);
    }
}

void BlackBerry_OnTouch(int touch_device_id_in, int pointer_finger_id_in, int action, float x, float y, float p)
{
    SDL_TouchID touchDeviceId = 0;
    SDL_FingerID fingerId = 0;

    if (!BlackBerry_Window) {
        return;
    }

    touchDeviceId = (SDL_TouchID)touch_device_id_in;
    if (SDL_AddTouch(touchDeviceId, "") < 0) {
        SDL_Log("error: can't add touch %s, %d", __FILE__, __LINE__);
    }

    fingerId = (SDL_FingerID)pointer_finger_id_in;
    switch (action) {
        case ACTION_DOWN:
            if (!leftFingerDown) {
                /* send moved event */
                SDL_SendMouseMotion(NULL, SDL_TOUCH_MOUSEID, 0, x, y);

                /* send mouse down event */
                SDL_SendMouseButton(NULL, SDL_TOUCH_MOUSEID, SDL_PRESSED, SDL_BUTTON_LEFT);

                leftFingerDown = fingerId;
            }
            SDL_SendTouch(touchDeviceId, fingerId, SDL_TRUE, x, y, p);
            break;
        case ACTION_MOVE:
            if (!leftFingerDown) {
                /* send moved event */
                SDL_SendMouseMotion(NULL, SDL_TOUCH_MOUSEID, 0, x, y);
            }
            SDL_SendTouchMotion(touchDeviceId, fingerId, x, y, p);
            break;
        case ACTION_UP:
            if (fingerId == leftFingerDown) {
                /* send mouse up */
                SDL_SendMouseButton(NULL, SDL_TOUCH_MOUSEID, SDL_RELEASED, SDL_BUTTON_LEFT);
                leftFingerDown = 0;
            }
            SDL_SendTouch(touchDeviceId, fingerId, SDL_FALSE, x, y, p);
            break;
        default:
            break;
    }
}

#endif /* SDL_VIDEO_DRIVER_BLACKBERRY */

/* vi: set ts=4 sw=4 expandtab: */
