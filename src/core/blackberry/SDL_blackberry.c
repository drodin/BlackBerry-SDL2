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

#ifdef __BLACKBERRY__

#include "SDL_blackberry.h"

#include <errno.h>
#include "sys/keycodes.h"
#include "clipboard/clipboard.h"

#include <bps/screen.h>
#include <bps/navigator.h>
#include <bps/paymentservice.h>
#include <bps/sensor.h>
#include <bps/virtualkeyboard.h>
#ifndef __PLAYBOOK__
#include <bps/battery.h>
#endif

#include "SDL_audio.h"
#include "../../events/SDL_events_c.h"
#include "../../video/blackberry/SDL_blackberrykeyboard.h"
#include "../../video/blackberry/SDL_blackberrytouch.h"
#include "../../video/blackberry/SDL_blackberryvideo.h"
#include "../../video/blackberry/SDL_blackberrywindow.h"

/*******************************************************************************
                               Globals
*******************************************************************************/
static int screenResolution[2];

SDL_Keysym keysym;

screen_context_t screenContext;
screen_window_t screenWindow;
static char windowGroup[64] = {0};

static int lastButtonState = 0;

/* Accelerometer data storage */
static float fLastAccelerometer[3];
static bool bHasNewData;

#ifndef __PLAYBOOK__
typedef struct GameController_t
{
    // Static device info.
    screen_device_t handle;
    int type;
    int analogCount;
    int buttonCount;
    char id[64];

    // Current state.
    bool haveData;
    int buttons;
    int analog0[3];
    int analog1[3];
} GameController;
static GameController _controllers[1];
static int numControllers = 0;
#endif

static int orientation = -1;

const char clipType[] = "text/plain";
char *clipText;

/*******************************************************************************
                 Init
*******************************************************************************/

int SDL_BlackBerry_Init()
{
    LOGE("SDL_BlackBerry_Init\n");

    int rc;

    rc = screen_create_context(&screenContext, 0);
    if (rc) {
        SDL_SetError("Cannot create screen context: %s", strerror(errno));
        return -1;
    }

    rc = bps_initialize();
    if (rc) {
        SDL_SetError("Cannot initializes the BPS library: %s", strerror(errno));
        screen_destroy_context(screenContext);
        return -1;
    }

    rc = navigator_request_events(0);
    if (rc) {
        SDL_SetError("Cannot request navigator events: %s", strerror(errno));
        bps_shutdown();
        screen_destroy_context(screenContext);
        return -1;
    }

    rc = navigator_rotation_lock(true);
    if (rc) {
        SDL_SetError("Cannot set navigator rotation lock: %s", strerror(errno));
        bps_shutdown();
        screen_destroy_context(screenContext);
        return -1;
    }

    rc = paymentservice_request_events(0);
    if (rc) {
        LOGE("Cannot request payment service events: %s", strerror(errno));
    }
#ifdef __BLACKBERRY_DEBUG__
    paymentservice_set_connection_mode(true);
#endif

    rc = screen_create_window(&screenWindow, screenContext);
    if (rc) {
        SDL_SetError("Cannot create window: %s", strerror(errno));
        bps_shutdown();
        screen_destroy_context(screenContext);
        return -1;
    }

    int position[2] = { 0, 0 };
    rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_POSITION, position);
    if (rc) {
        SDL_SetError("Cannot position window: %s", strerror(errno));
        screen_destroy_window(screenWindow);
        bps_shutdown();
        screen_destroy_context(screenContext);
        return -1;
    }

    int idle_mode = SCREEN_IDLE_MODE_KEEP_AWAKE;
    rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_IDLE_MODE, &idle_mode);
    if (rc) {
        SDL_SetError("Cannot disable idle mode: %s", strerror(errno));
        screen_destroy_window(screenWindow);
        bps_shutdown();
        screen_destroy_context(screenContext);
        return -1;
    }

    orientation = atoi(getenv("ORIENTATION"));

#ifndef __PLAYBOOK__
    screenResolution[0] = atoi(getenv("WIDTH"));
    screenResolution[1] = atoi(getenv("HEIGHT"));

    rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_SIZE, screenResolution);
    if (rc) {
        SDL_SetError("Cannot resize window: %s", strerror(errno));
        screen_destroy_window(screenWindow);
        bps_shutdown();
        screen_destroy_context(screenContext);
        return -1;
    }
#else
    static screen_display_t screenDisplay;
    screen_get_window_property_pv(screenWindow, SCREEN_PROPERTY_DISPLAY, (void **)&screenDisplay);

    screen_display_mode_t screenMode;
    screen_get_display_property_pv(screenDisplay, SCREEN_PROPERTY_MODE, (void**)&screenMode);

    int size[2];
    screen_get_window_property_iv(screenWindow, SCREEN_PROPERTY_BUFFER_SIZE, size);

    screenResolution[0] = size[0];
    screenResolution[1] = size[1];

    if ((orientation == 0) || (orientation == 180)) {
        if (((screenMode.width > screenMode.height) && (size[0] < size[1])) ||
            ((screenMode.width < screenMode.height) && (size[0] > size[1]))) {
                screenResolution[1] = size[0];
                screenResolution[0] = size[1];
        }
    } else { // orientation == 90 || orientation == 270
        if (((screenMode.width > screenMode.height) && (size[0] > size[1])) ||
            ((screenMode.width < screenMode.height && size[0] < size[1]))) {
                screenResolution[1] = size[0];
                screenResolution[0] = size[1];
        }
    }

    screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_ROTATION, &orientation);
#endif

    rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_BUFFER_SIZE, screenResolution);
    if (rc) {
        SDL_SetError("Cannot resize window buffer: %s", strerror(errno));
        screen_destroy_window(screenWindow);
        bps_shutdown();
        screen_destroy_context(screenContext);
        return -1;
    }

    int format = SCREEN_FORMAT_RGBX8888;
    rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_FORMAT, &format);
    if (rc) {
        SDL_SetError("Cannot set window format: %s", strerror(errno));
        screen_destroy_window(screenWindow);
        bps_shutdown();
        screen_destroy_context(screenContext);
        return -1;
    }

    int usage = SCREEN_USAGE_OPENGL_ES1; //FIXME: | SCREEN_USAGE_ROTATION;
    rc = screen_set_window_property_iv(screenWindow, SCREEN_PROPERTY_USAGE, &usage);
    if (rc) {
        SDL_SetError("Cannot set window usage: %s", strerror(errno));
        screen_destroy_window(screenWindow);
        bps_shutdown();
        screen_destroy_context(screenContext);
        return -1;
    }

    int bufferCount = 1;
    rc = screen_create_window_buffers(screenWindow, bufferCount);
    if (rc) {
        SDL_SetError("Cannot create window buffer: %s", strerror(errno));
        screen_destroy_window(screenWindow);
        bps_shutdown();
        screen_destroy_context(screenContext);
        return -1;
    }

#ifndef __PLAYBOOK__
    rc = screen_create_window_group(screenWindow, NULL);
#else
    snprintf(windowGroup, sizeof(double), "%d", getpid());
    rc = screen_create_window_group(screenWindow, windowGroup);
#endif
    if (rc) {
        SDL_SetError("Cannot create window group: %s", strerror(errno));
        screen_destroy_window(screenWindow);
        bps_shutdown();
        screen_destroy_context(screenContext);
        return -1;
    }
#ifndef __PLAYBOOK__
    screen_get_window_property_cv(screenWindow, SCREEN_PROPERTY_GROUP, sizeof(windowGroup), windowGroup);
#endif

    rc = screen_request_events(screenContext);
    if (rc) {
        SDL_SetError("Cannot request screen events: %s", strerror(errno));
        screen_destroy_window_buffers(screenWindow);
        screen_destroy_window(screenWindow);
        bps_shutdown();
        screen_destroy_context(screenContext);
        return -1;
    }

    BlackBerry_SetScreenResolution(screenResolution[0], screenResolution[1], SDL_PIXELFORMAT_RGBX8888);

    LOGE("SDL_BlackBerry_Init finished!\n");

    return 0;
}

int BlackBerry_SYS_InitAccelerometer()
{
    if (sensor_is_supported(SENSOR_TYPE_GRAVITY)) {
        sensor_set_rate(SENSOR_TYPE_GRAVITY, 25000);
        sensor_set_skip_duplicates(SENSOR_TYPE_GRAVITY, true);
        if (sensor_request_events(SENSOR_TYPE_GRAVITY) == BPS_SUCCESS)
            return 1;
    }
    return 0;
}

int BlackBerry_SYS_InitController()
{
#ifndef __PLAYBOOK__
    int deviceCount;
    screen_get_context_property_iv(screenContext, SCREEN_PROPERTY_DEVICE_COUNT, &deviceCount);
    screen_device_t* devices = (screen_device_t*) calloc(deviceCount, sizeof(screen_device_t));
    screen_get_context_property_pv(screenContext, SCREEN_PROPERTY_DEVICES, (void**) devices);

    int ic;
    for (ic = 0; ic < deviceCount; ic++) {
        int type;
        int rc = screen_get_device_property_iv(devices[ic], SCREEN_PROPERTY_TYPE, &type);

        if (!rc && (type == SCREEN_EVENT_GAMEPAD || type == SCREEN_EVENT_JOYSTICK)) {
            GameController* controller = &_controllers[ic];
            controller->handle = devices[ic];

            //loadController
            screen_get_device_property_iv(controller->handle,
                SCREEN_PROPERTY_TYPE, &controller->type);

            screen_get_device_property_cv(controller->handle,
                SCREEN_PROPERTY_ID_STRING, sizeof(controller->id),
                controller->id);

            screen_get_device_property_iv(controller->handle,
                SCREEN_PROPERTY_BUTTON_COUNT, &controller->buttonCount);

            // Check for the existence of analog sticks.
            if (!screen_get_device_property_iv(controller->handle,
                SCREEN_PROPERTY_ANALOG0, controller->analog0)) {
                ++controller->analogCount;
            }

            if (!screen_get_device_property_iv(controller->handle,
                SCREEN_PROPERTY_ANALOG1, controller->analog0)) {
                ++controller->analogCount;
            }

            controller->haveData = false;
            controller->buttons = 0;
            controller->analog0[0] = controller->analog0[1] = controller->analog0[2] = 0;
            controller->analog1[0] = controller->analog1[1] = controller->analog1[2] = 0;
            //loadController end

            numControllers++;
            return 1; //FIXME: Supporting only one Gamepad for now
        }
    }
#endif

    return 0;
}

/*******************************************************************************
                 Handle Events
*******************************************************************************/

void handleNavigatorEvent(bps_event_t *bps_event)
{
    switch (bps_event_get_code(bps_event)) {
    case (NAVIGATOR_EXIT):
        SDL_SendQuit();
        SDL_SendAppEvent(SDL_APP_TERMINATING);
        break;
    case (NAVIGATOR_BACK):
        keysym.mod = KMOD_NONE;
        keysym.scancode = SDL_SCANCODE_ESCAPE;
        keysym.sym = SDLK_ESCAPE;
        BlackBerry_OnKeyDown(keysym);
        break;
    case (NAVIGATOR_SWIPE_DOWN):
        keysym.mod = KMOD_NONE;
        keysym.scancode = SDL_SCANCODE_MENU;
        keysym.sym = SDLK_MENU;
        BlackBerry_OnKeyDown(keysym);
        break;
#ifndef __PLAYBOOK__
    case (NAVIGATOR_KEYBOARD_STATE):
        switch (navigator_event_get_keyboard_state(bps_event)) {
        case NAVIGATOR_KEYBOARD_OPENED:
            SDL_StartTextInput();
            break;
        case NAVIGATOR_KEYBOARD_CLOSED:
            SDL_StopTextInput();
            break;
        }
        break;
#endif
    case (NAVIGATOR_WINDOW_STATE):
        switch (navigator_event_get_window_state(bps_event)) {
        case NAVIGATOR_WINDOW_FULLSCREEN:
            SDL_SendAppEvent(SDL_APP_WILLENTERFOREGROUND);
            SDL_SendAppEvent(SDL_APP_DIDENTERFOREGROUND);
            if (BlackBerry_Window) {
                SDL_SendWindowEvent(BlackBerry_Window, SDL_WINDOWEVENT_FOCUS_GAINED, 0, 0);
                SDL_SendWindowEvent(BlackBerry_Window, SDL_WINDOWEVENT_RESTORED, 0, 0);
            }
            SDL_PauseAudio(0);
            break;
        case NAVIGATOR_WINDOW_THUMBNAIL:
            if (BlackBerry_Window) {
                SDL_SendWindowEvent(BlackBerry_Window, SDL_WINDOWEVENT_FOCUS_LOST, 0, 0);
                SDL_SendWindowEvent(BlackBerry_Window, SDL_WINDOWEVENT_MINIMIZED, 0, 0);
            }
            SDL_PauseAudio(1);
            break;
        case NAVIGATOR_WINDOW_INVISIBLE:
            SDL_SendAppEvent(SDL_APP_WILLENTERBACKGROUND);
            SDL_SendAppEvent(SDL_APP_DIDENTERBACKGROUND);
            SDL_PauseAudio(1);
            break;
        }
        break;
    default:
        break;
    }
}

static void handlePointerEvent(screen_event_t event, screen_window_t window)
{
    int buttonState = 0;
    screen_get_event_property_iv(event, SCREEN_PROPERTY_BUTTONS, &buttonState);

    int coords[2];
    screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, coords);

    if (coords[0] < 0 || coords[1] < 0) {
        //FIXME: Detected pointer swipe event
        return;
    }

    SDL_SendMouseMotion(NULL, 0, 0, coords[0], coords[1]);

    if (buttonState != lastButtonState) {
        if (buttonState & SCREEN_LEFT_MOUSE_BUTTON)
            SDL_SendMouseButton(NULL, 0, SDL_PRESSED, SDL_BUTTON_LEFT);
        if (lastButtonState & SCREEN_LEFT_MOUSE_BUTTON)
            SDL_SendMouseButton(NULL, 0, SDL_RELEASED, SDL_BUTTON_LEFT);

        if (buttonState & SCREEN_RIGHT_MOUSE_BUTTON)
            SDL_SendMouseButton(NULL, 0, SDL_PRESSED, SDL_BUTTON_RIGHT);
        if (lastButtonState & SCREEN_RIGHT_MOUSE_BUTTON)
            SDL_SendMouseButton(NULL, 0, SDL_RELEASED, SDL_BUTTON_RIGHT);
    }

    lastButtonState = buttonState;
}

static void handleKeyboardEvent(screen_event_t event)
{
    int flags = 0;
    screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_FLAGS, &flags);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_SYM, &(keysym.sym));

    keysym.mod = KMOD_NONE;
    keysym.scancode = SDL_SCANCODE_UNKNOWN;

    if (flags & KEY_SCAN_VALID) { // Physical Keyboard
        int modifiers = 0;
        screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_MODIFIERS, &modifiers);
        if (modifiers & KEYMOD_SHIFT)
            keysym.mod |= KMOD_LSHIFT;
        if (modifiers & KEYMOD_CTRL)
            keysym.mod |= KMOD_LCTRL;
        if (modifiers & KEYMOD_ALT)
            keysym.mod |= KMOD_LALT;
        if (modifiers & KEYMOD_CAPS_LOCK)
            keysym.mod |= KMOD_CAPS;
        if (modifiers & KEYMOD_NUM_LOCK)
            keysym.mod |= KMOD_NUM;

        int scan = 0;
        screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_SCAN, &scan);
        keysym.scancode = scan;

        int posted = (flags & 0x1) ? BlackBerry_OnKeyDown(keysym) : BlackBerry_OnKeyUp(keysym);
    } else { // Virtual Keyboard
        if (flags & 0x1)
            BlackBerry_SendTextInput(keysym);
    }
}

static void handleMtouchEvent(screen_event_t event, screen_window_t window, int action)
{
    int finger;
    int position[2];
    int pressure;

    screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_ID, (int*) &finger);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, position);
    screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_PRESSURE, (int*) &pressure);

    //action: SCREEN_EVENT_MTOUCH_TOUCH = 100, SCREEN_EVENT_MTOUCH_MOVE == 101, SCREEN_EVENT_MTOUCH_RELEASE == 102
    BlackBerry_OnTouch(0/*touch_device_id*/, finger, action, position[0], position[1], pressure);
}

#ifndef __PLAYBOOK__
void handleControllerEvent(screen_event_t screen_event)
{
    screen_device_t device;
    screen_get_event_property_pv(screen_event, SCREEN_PROPERTY_DEVICE, (void**) &device);

    GameController* controller = &_controllers[0]; //FIXME: Supporting only one Gamepad for now
    if (controller->handle != NULL && device == controller->handle) {
        screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_BUTTONS, &controller->buttons);

        if (controller->analogCount > 0) {
            screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_ANALOG0, controller->analog0);
        }

        if (controller->analogCount == 2) {
            screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_ANALOG1, controller->analog1);
        }

        controller->haveData = true;
    }
}
#endif

void handleScreenEvent(bps_event_t *bps_event)
{
    int type;
    screen_event_t screen_event = screen_event_get_event(bps_event);
    screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TYPE, &type);

    screen_window_t window;
    screen_get_event_property_pv(screen_event, SCREEN_PROPERTY_WINDOW, (void **) &window);

    switch (type) {
    case SCREEN_EVENT_POINTER:
        handlePointerEvent(screen_event, window);
        break;
    case SCREEN_EVENT_KEYBOARD:
        handleKeyboardEvent(screen_event);
        break;
    case SCREEN_EVENT_MTOUCH_TOUCH:
        case SCREEN_EVENT_MTOUCH_MOVE:
        case SCREEN_EVENT_MTOUCH_RELEASE:
        handleMtouchEvent(screen_event, window, type);
        break;
#ifndef __PLAYBOOK__
    case SCREEN_EVENT_GAMEPAD:
        case SCREEN_EVENT_JOYSTICK:
        handleControllerEvent(screen_event);
        break;
#endif
    default:
        break;
    }
}

void handleSensorEvent(bps_event_t* bps_event)
{
    if (SENSOR_GRAVITY_READING == bps_event_get_code(bps_event)) {
        float x, y, z;

        sensor_event_get_xyz(bps_event, &x, &y, &z);

        switch (orientation) {
        case 270:
            fLastAccelerometer[0] = -y;
            fLastAccelerometer[1] = -x;
            break;
        case 180:
            fLastAccelerometer[0] = x;
            fLastAccelerometer[1] = -y;
            break;
        case 90:
            fLastAccelerometer[0] = y;
            fLastAccelerometer[1] = x;
            break;
        default:
            fLastAccelerometer[0] = -x;
            fLastAccelerometer[1] = y;
            break;
        }
        fLastAccelerometer[0] /= SENSOR_GRAVITY_EARTH;
        fLastAccelerometer[1] /= SENSOR_GRAVITY_EARTH;
        fLastAccelerometer[2] = z / SENSOR_GRAVITY_EARTH;
        bHasNewData = true;
    }
}

void handlePaymentEvent(bps_event_t *bps_event)
{
    if (paymentservice_event_get_response_code(bps_event) == SUCCESS_RESPONSE) {
        if (bps_event_get_code(bps_event) == PURCHASE_RESPONSE) {
            unsigned request_id;
            paymentservice_get_existing_purchases_request(false, windowGroup, &request_id);
        } else {
            int purchases = paymentservice_event_get_number_purchases(bps_event);

            int i = 0;
            for (i = 0; i < purchases; i++) {
                const char* good_id = paymentservice_event_get_digital_good_id(bps_event, i);
                const char* good_sku = paymentservice_event_get_digital_good_sku(bps_event, i);
                if (good_id != NULL && good_sku != NULL) {
                    SDL_Event event;
                    event.type = SDL_USEREVENT;
                    event.user.code = 1;
                    event.user.data1 = (void *) good_id;
                    event.user.data2 = (void *) good_sku;
                    SDL_PushEvent(&event);
                }
            }
        }
    } else {
        unsigned request_id = paymentservice_event_get_request_id(bps_event);
        int error_id = paymentservice_event_get_error_id(bps_event);
        const char* error_text = paymentservice_event_get_error_text(bps_event);

        LOGE("Payment System error. Request ID: %d  Error ID: %d  Text: %s\n",
            request_id, error_id, error_text ? error_text : "N/A");
    }
}

void BlackBerry_SYS_ProcessEvents()
{
    bps_event_t *global_bps_event = NULL;
    bps_get_event(&global_bps_event, 0);

    while (global_bps_event) {
        int domain = bps_event_get_domain(global_bps_event);

        if (domain == navigator_get_domain()) {
            handleNavigatorEvent(global_bps_event);
        } else if (domain == screen_get_domain()) {
            handleScreenEvent(global_bps_event);
        } else if (domain == sensor_get_domain()) {
            handleSensorEvent(global_bps_event);
        } else if (domain == paymentservice_get_domain()) {
            handlePaymentEvent(global_bps_event);
        } else {
            LOGE("BlackBerry - unhandled event domain: %d\n", domain);
        }

        bps_get_event(&global_bps_event, 0);
    }
}

/*******************************************************************************
             Functions called by SDL
*******************************************************************************/

screen_window_t BlackBerry_SYS_GetNativeWindow(void)
{
    return screenWindow;
}

char* BlackBerry_SYS_GetNativeWindowGroup(void)
{
    return windowGroup;
}

void BlackBerry_SYS_ReleaseNativeWindow(screen_window_t native_window)
{
    screen_destroy_window_buffers(screenWindow);
    screen_destroy_window(screenWindow);
    screen_stop_events(screenContext);
    bps_shutdown();
    screen_destroy_context(screenContext);
}

void BlackBerry_SYS_SetActivityTitle(const char *title)
{
}

SDL_bool BlackBerry_SYS_GetAccelerometerValues(float values[3])
{
    int i;
    SDL_bool retval = SDL_FALSE;

    if (bHasNewData) {
        for (i = 0; i < 3; ++i) {
            values[i] = fLastAccelerometer[i];
        }
        bHasNewData = false;
        retval = SDL_TRUE;
    }

    return retval;
}

SDL_bool BlackBerry_SYS_GetControllerInfo(int device_id, int* buttons, int* axes)
{
    SDL_bool retval = SDL_FALSE;

#ifndef __PLAYBOOK__
    if (device_id <= numControllers) {
        GameController* controller = &_controllers[device_id - 1];
        *buttons = controller->buttonCount;
        *axes = controller->analogCount * 3;
        retval = SDL_TRUE;
    }
#endif

    return retval;
}

SDL_bool BlackBerry_SYS_GetControllerValues(int device_id, int* buttons, float* axes)
{
    SDL_bool retval = SDL_FALSE;

#ifndef __PLAYBOOK__
    if (device_id <= numControllers) {
        GameController* controller = &_controllers[device_id - 1];
        if (controller->haveData) {
            *buttons = controller->buttons;
            if (controller->analogCount > 0) {
                axes[0] = controller->analog0[0] / 128.0f;
                axes[1] = controller->analog0[1] / 128.0f;
                axes[2] = controller->analog0[2] / 256.0f;
            }
            if (controller->analogCount == 2) {
                axes[3] = controller->analog1[0] / 128.0f;
                axes[4] = controller->analog1[1] / 128.0f;
                axes[5] = controller->analog1[2] / 256.0f;
            }

            controller->haveData = false;
            retval = SDL_TRUE;
        }
    }
#endif

    return retval;
}

int BlackBerry_SYS_SetClipboardText(const char* text)
{
    int size;
    if (empty_clipboard() != -1) {
        size = set_clipboard_data(clipType, strlen(text), text);
        if (size != -1)
            return size;
    }

    return 0;
}

char* BlackBerry_SYS_GetClipboardText()
{
    if (clipText)
        SDL_free(clipText);
    int size = get_clipboard_data(clipType, &clipText);
    if (size != -1 && clipText)
        return clipText;

    return SDL_strdup("");
}

SDL_bool BlackBerry_SYS_HasClipboardText()
{
    if (is_clipboard_format_present(clipType) != -1)
        return SDL_TRUE;

    return SDL_FALSE;
}

int BlackBerry_SYS_GetPowerInfo(SDL_PowerState* state, int* seconds, int* percent)
{
#ifndef __PLAYBOOK__
    int rc;
    battery_info_t* info;

    rc = battery_get_info(&info);
    if (rc) {
        LOGE("Cannot get battery info: %s", strerror(errno));
        return -1;
    }

    int charger = battery_info_get_charger_info(info);

    if (charger == BATTERY_CHARGER_CHARGING) {
        *state = SDL_POWERSTATE_CHARGING;
    } else if (charger == BATTERY_CHARGER_PLUGGED) {
        *state = SDL_POWERSTATE_CHARGED;
    } else {
        *state = SDL_POWERSTATE_ON_BATTERY;
    }

    *seconds = battery_info_get_time_to_empty(info) * 60;
    *percent = battery_info_get_state_of_charge(info);

    battery_free_info(&info);
    return 0;
#else
    return -1;
#endif
}

/* returns number of found touch devices as return value and ids in parameter ids */
int BlackBerry_SYS_GetTouchDeviceIds(int **ids)
{
    int number = 1;
    *ids = NULL;
    *ids = SDL_malloc(number * sizeof(*ids[0]));
    *ids[0] = 0;
    return number;
}

void BlackBerry_SYS_ShowTextInput(SDL_Rect *inputRect)
{
    virtualkeyboard_change_options(VIRTUALKEYBOARD_LAYOUT_DEFAULT, VIRTUALKEYBOARD_ENTER_DEFAULT);
    virtualkeyboard_show();
}

void BlackBerry_SYS_HideTextInput()
{
    virtualkeyboard_hide();
}

#endif /* __BLACKBERRY__ */

/* vi: set ts=4 sw=4 expandtab: */
