// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021-2022, Fewnity - Grégory Machefer
//
// This file is part of Counter Strike Nintendo DS Multiplayer Edition (CS:DS)

#include <math.h>
#include "input.h"
#include "ui.h"
#include "movements.h"
#include "keyboard.h"
#include "gun.h"
#include "tutorial.h"

// Last position of the style / finger
Vector2 LastTouch;
// Held keys
u32 keys = 0;
// Pressed keys
u32 keysdown = 0;
// Released keys
u32 keysup = 0;
// Position of the style / finger
touchPosition touch;

// Game pad areas
int gamepadTouchAreaX1 = GAMEPAD_TOUCH_AREA_X1_RIGHT_HANDLED;
int gamepadTouchAreaX2 = GAMEPAD_TOUCH_AREA_X2_RIGHT_HANDLED;
int gamepadTouchAreaY1 = 30;
int gamepadTouchAreaY2 = 194;

// Game pad sensitivity
float sensitivity = DEFAULT_SENSITIVITY;
float buttonsSensitivity = DEFAULT_SENSITIVITY;

// Current used slider index
int currentSlider = -1;

// Angles added to the player/camera when the stylus is moved
int xAngleAdded = 0;
int yAngleAdded = 0;

// All inputs data
Input inputs[INPUT_COUNT];
// Is scnanning for inputs?
bool scanForInput = false;
// Current input index scanned
int currentInputScanned = 0;
// All inputs names
const char *inputsNames[INPUT_NAMES_COUNT];

void initInputs()
{
    // Set default inputs
    inputs[FIRE_BUTTON].value = KEY_L;
    inputs[FIRE_BUTTON].nameIndex = 9;
    inputs[JUMP_BUTTON].value = KEY_R;
    inputs[JUMP_BUTTON].nameIndex = 8;

    inputs[LEFT_BUTTON].value = KEY_LEFT;
    inputs[LEFT_BUTTON].nameIndex = 5;
    inputs[RIGHT_BUTTON].value = KEY_RIGHT;
    inputs[RIGHT_BUTTON].nameIndex = 4;
    inputs[UP_BUTTON].value = KEY_UP;
    inputs[UP_BUTTON].nameIndex = 6;
    inputs[DOWN_BUTTON].value = KEY_DOWN;
    inputs[DOWN_BUTTON].nameIndex = 7;
    inputs[LOOK_LEFT_BUTTON].value = KEY_Y;
    inputs[LOOK_LEFT_BUTTON].nameIndex = 11;
    inputs[LOOK_RIGHT_BUTTON].value = KEY_A;
    inputs[LOOK_RIGHT_BUTTON].nameIndex = 0;
    inputs[LOOK_UP_BUTTON].value = KEY_X;
    inputs[LOOK_UP_BUTTON].nameIndex = 10;
    inputs[LOOK_DOWN_BUTTON].value = KEY_B;
    inputs[LOOK_DOWN_BUTTON].nameIndex = 1;

    inputs[DEFUSE_BUTTON].value = KEY_SELECT;
    inputs[DEFUSE_BUTTON].nameIndex = 2;
    inputs[SCOPE_BUTTON].value = KEY_START;
    inputs[SCOPE_BUTTON].nameIndex = 3;

    inputs[LEFT_GUN].value = -1; // Unasigned
    inputs[LEFT_GUN].nameIndex = 12;
    inputs[RIGHT_GUN].value = -1; // Unasigned
    inputs[RIGHT_GUN].nameIndex = 12;

    // Set inputs names list
    inputsNames[0] = "A";
    inputsNames[1] = "B";
    inputsNames[2] = "Select";
    inputsNames[3] = "Start";
    inputsNames[4] = "Right";
    inputsNames[5] = "Left";
    inputsNames[6] = "Up";
    inputsNames[7] = "Down";
    inputsNames[8] = "R";
    inputsNames[9] = "L";
    inputsNames[10] = "X";
    inputsNames[11] = "Y";
    inputsNames[12] = "Unassigned";
    inputsNames[13] = "Scanning...";
    inputsNames[14] = "Already used";
}

/**
 * @brief Read keys, touch screen and keyboard
 *
 */
void readKeys()
{
    scanKeys();
    keys = keysHeld();
    keysdown = keysDown();
    keysup = keysUp();
    touchRead(&touch);

    readKeyboard();
}

/**
 * @brief Read the touch screen
 *
 * @param touch
 * @param NeedChangeScreen
 * @param AlwaysUpdateBottomScreen
 * @param ButtonToShow
 * @param UpdateBottomScreenFrameCount
 * @param SendTeam
 * @param forceCheck
 */
// Smooth camera rotation with the stylus.
//
// The touch screen is read by the ARM7. With wifi enabled (online games), it
// doesn't send a new touch position every frame: the position stays the same
// for a frame or two, then jumps. The camera turned in steps (0, 0, 4, 0, 4...)
// and looked jerky. Now the movement of a new touch position is spread over
// the frames since the previous one: the camera turns at the same speed every
// frame. The total rotation is the same.
//
// Like in Metroid Prime Hunters:
// - Fast movements turn more than slow ones (acceleration), so a quick flick
//   can turn around while slow movements stay precise for aiming.
// - When the stylus leaves the screen during a movement, the camera keeps
//   turning and slows down (inertia). Touching the screen again stops it.
typedef struct
{
    float pending;       // Rotation not applied yet
    float speed;         // Rotation per frame (absolute value)
    int framesSinceMove; // Frames since the touch position changed
    float velocity;      // Average rotation per frame while dragging
    float inertia;       // Rotation per frame after the stylus is lifted
} TouchAxis;

static TouchAxis touchAxisX = {0, 0, 0, 0, 0};
static TouchAxis touchAxisY = {0, 0, 0, 0, 0};

// Settings (see the comment above)
// Touch camera acceleration (set in Settings > Controls > Touch camera)
float touchAcceleration = DEFAULT_TOUCH_ACCELERATION; // Extra rotation per unit of movement in a frame
float touchMaxGain = DEFAULT_TOUCH_MAX_GAIN;          // Limit of the acceleration
#define TOUCH_INERTIA_MIN_SPEED 1.0f // Slower movements stop when the stylus is lifted
#define TOUCH_INERTIA_FRICTION_X 0.3f // Speed kept per frame (horizontal)
#define TOUCH_INERTIA_FRICTION_Y 0.15f // Vertical inertia stops faster

static bool touchCameraDragging = false;

static void touchAxisAdd(TouchAxis *axis, float movement)
{
    if (movement == 0)
        return;

    // Acceleration: fast movements turn more
    float gain = 1 + touchAcceleration * fabsf(movement);
    if (gain > touchMaxGain)
        gain = touchMaxGain;
    movement *= gain;

    int frames = axis->framesSinceMove;
    // After a pause (or a new touch), apply the movement now
    if (frames < 1 || frames > 3)
        frames = 1;

    axis->pending += movement;
    axis->speed = fabsf(movement) / frames;
    axis->framesSinceMove = 0;
}

static float touchAxisStep(TouchAxis *axis, float friction)
{
    axis->framesSinceMove++;

    float step = 0;
    if (axis->pending != 0)
    {
        step = axis->pending;
        if (fabsf(step) > axis->speed)
            step = (step > 0) ? axis->speed : -axis->speed;

        axis->pending -= step;
        if (fabsf(axis->pending) < 0.001f)
            axis->pending = 0;
    }

    if (touchCameraDragging)
    {
        // Average speed of the last frames (used for the inertia)
        axis->velocity = axis->velocity * 0.6f + step * 0.4f;
    }
    else if (axis->inertia != 0)
    {
        step += axis->inertia;
        axis->inertia *= friction;
        if (fabsf(axis->inertia) < 0.05f)
            axis->inertia = 0;
    }

    return step;
}

// Called when the stylus touches the gamepad area
static void touchCameraStart(void)
{
    // Touching the screen stops the inertia (like in Metroid Prime Hunters)
    touchAxisX.inertia = touchAxisY.inertia = 0;
    touchAxisX.velocity = touchAxisY.velocity = 0;
    touchCameraDragging = true;
}

// Called when the stylus leaves the gamepad area
static void touchCameraEnd(void)
{
    if (!touchCameraDragging)
        return;
    touchCameraDragging = false;

    // Keep turning only after a fast movement
    touchAxisX.inertia = fabsf(touchAxisX.velocity) >= TOUCH_INERTIA_MIN_SPEED ? touchAxisX.velocity : 0;
    touchAxisY.inertia = fabsf(touchAxisY.velocity) >= TOUCH_INERTIA_MIN_SPEED ? touchAxisY.velocity : 0;
    touchAxisX.velocity = touchAxisY.velocity = 0;
}

static void addTouchCameraMovement(float x, float y)
{
    touchAxisAdd(&touchAxisX, x);
    touchAxisAdd(&touchAxisY, y);
}

static void smoothTouchCamera(void)
{
    if (localPlayer->IsDead)
    {
        touchAxisX.inertia = touchAxisY.inertia = 0;
        touchAxisX.pending = touchAxisY.pending = 0;
        return;
    }

    float x = touchAxisStep(&touchAxisX, TOUCH_INERTIA_FRICTION_X);
    float y = touchAxisStep(&touchAxisY, TOUCH_INERTIA_FRICTION_Y);
    if (x != 0 || y != 0)
        AddAnglesToPlayer(x, y);
}

void ReadTouchScreen(touchPosition touch, bool *NeedChangeScreen, bool *AlwaysUpdateBottomScreen, int *ButtonToShow, int *UpdateBottomScreenFrameCount, bool *SendTeam, bool forceCheck)
{
    if ((*UpdateBottomScreenFrameCount == 0 && uiTimer == 0) || forceCheck)
    {
        for (int i = 0; i < checkBoxToShow; i++)
        {
            // Check checkboxs
            if ((keysdown & KEY_TOUCH && touch.px >= AllCheckBoxs[i].xPos && touch.px <= AllCheckBoxs[i].xPos + AllCheckBoxs[i].xSize && touch.py >= AllCheckBoxs[i].yPos && touch.py <= AllCheckBoxs[i].yPos + AllCheckBoxs[i].ySize))
            {
                *AllCheckBoxs[i].value = !*AllCheckBoxs[i].value;
                *UpdateBottomScreenFrameCount += 8;
                if (currentMenu == CONTROLSSETTINGS) // Update screen button (map menu)
                {
                    if (i == 0)
                    {
                        UpdateGamepadArea();
                    }
                }
                return;
            }
        }

        if (keys & KEY_TOUCH)
        {
            if (keysdown & KEY_TOUCH && currentSlider == -1)
            {
                // Check if the player is just touching a slider
                for (int i = 0; i < sliderToShow; i++)
                {
                    if ((touch.px >= AllSliders[i].xPos && touch.px <= AllSliders[i].xPos + AllSliders[i].xSize && touch.py >= AllSliders[i].yPos - 10 && touch.py <= AllSliders[i].yPos + 10))
                    {
                        currentSlider = i;
                        break;
                    }
                }
            }
            if (currentSlider != -1)
            {
                // Check used slider value
                if ((touch.px >= AllSliders[currentSlider].xPos && touch.px <= AllSliders[currentSlider].xPos + AllSliders[currentSlider].xSize && touch.py >= AllSliders[currentSlider].yPos - 10 && touch.py <= AllSliders[currentSlider].yPos + 10))
                {
                    *AllSliders[currentSlider].value = map(touch.px, AllSliders[currentSlider].xPos, AllSliders[currentSlider].xPos + AllSliders[currentSlider].xSize, AllSliders[currentSlider].min, AllSliders[currentSlider].max);
                    // Snap the value to the slider step
                    Slider *slider = &AllSliders[currentSlider];
                    if (slider->step > 0)
                    {
                        float snapped = slider->min + roundf((*slider->value - slider->min) / slider->step) * slider->step;
                        if (snapped < slider->min)
                            snapped = slider->min;
                        if (snapped > slider->max)
                            snapped = slider->max;
                        *slider->value = snapped;
                    }
                }
                return;
            }
        }
        else
        {
            currentSlider = -1;
        }

        // Check for all buttons click
        for (int i = 0; i < *ButtonToShow; i++)
        {
            if (!AllButtons[i].isHidden && (keysdown & KEY_TOUCH && touch.px >= AllButtons[i].xPos && touch.px <= AllButtons[i].xPos + AllButtons[i].xSize && touch.py >= AllButtons[i].yPos && touch.py <= AllButtons[i].yPos + AllButtons[i].ySize))
            {
                AllButtons[i].OnClick(AllButtons[i].parameter);
                return;
            }
        }

        // Check back button click
        if (useQuitButton && keysdown & KEY_TOUCH && touch.px >= 236 && touch.px <= 236 + 20 && touch.py >= 1 && touch.py <= 1 + 20)
        {
            if (currentMenu == SETTINGS)
            {
                uiTimer = 8;
                actionOfUiTimer = SAVE;
            }
            lastOpenedMenu();
            return;
        }

        if (!localPlayer->IsDead && currentMenu == CONTROLLER && keys & KEY_TOUCH && touch.px >= gamepadTouchAreaX1 && touch.px <= gamepadTouchAreaX2 && touch.py >= gamepadTouchAreaY1 && touch.py <= gamepadTouchAreaY2)
        {
            // Check if the player want to enable scope
            if (keysdown & KEY_TOUCH)
            {
                touchCameraStart();
                if (GetDoubleTapTimer() > 0)
                    SetAim();

                SetDoubleTapTimer(11);
            }
            else if (LastTouch.x != -1)
            {
                // Move the camera with the stylus

                int xAngleToAdd = 0;
                int yAngleToAdd = 0;

                if (LastTouch.x != touch.px)
                    xAngleToAdd = LastTouch.x - touch.px;

                if (LastTouch.y != touch.py)
                    yAngleToAdd = touch.py - LastTouch.y;

                xAngleToAdd = fmin(xAngleToAdd, 50);
                yAngleToAdd = fmin(yAngleToAdd, 50);

                // Total of angle added / removed to the camera
                xAngleAdded = abs(xAngleToAdd);
                yAngleAdded = abs(yAngleToAdd);

                // if (xAngleToAdd + yAngleToAdd < 50) // Security to avoid touch screen stylus teleportation
                // The rotation is not applied at once, see smoothTouchCamera()
                touchCameraDragging = true;
                addTouchCameraMovement(xAngleToAdd / 2.0, yAngleToAdd / 2.0);
            }

            LastTouch.x = touch.px;
            LastTouch.y = touch.py;
        }
        else
        {
            LastTouch.x = -1;
            LastTouch.y = -1;
            touchCameraEnd();
        }

        smoothTouchCamera();
    }
}

/**
 * @brief Update gamepad area
 *
 */
void UpdateGamepadArea()
{
    if (isLeftControls)
    {
        gamepadTouchAreaX1 = GAMEPAD_TOUCH_AREA_X1_LEFT_HANDLED;
        gamepadTouchAreaX2 = GAMEPAD_TOUCH_AREA_X2_LEFT_HANDLED;
    }
    else
    {
        gamepadTouchAreaX1 = GAMEPAD_TOUCH_AREA_X1_RIGHT_HANDLED;
        gamepadTouchAreaX2 = GAMEPAD_TOUCH_AREA_X2_RIGHT_HANDLED;
    }
}

/**
 * @brief Scan of input (for input settings)
 *
 */
void ScanForInput()
{
    if (scanForInput)
    {
        for (int i = 0; i < 12; i++)
        {
            if (keysdown & BIT(i))
            {
                bool canChange = true;

                // Check if input is already used
                for (int j = 0; j < INPUT_COUNT; j++)
                {
                    if (inputs[j].value == BIT(i))
                    {
                        canChange = false;
                        break;
                    }
                }

                // If the input is available, change it
                if (canChange)
                {
                    inputs[currentInputScanned].value = BIT(i);
                    inputs[currentInputScanned].nameIndex = i;
                }
                else // If the input is already used, change the input text to already used
                {
                    inputs[currentInputScanned].value = -1;
                    inputs[currentInputScanned].nameIndex = 14;
                }

                // Is the player is in the tutorial, update the text
                if (isInTutorial && inputs[currentInputScanned].value != -1)
                {
                    updateTutorialDialogText();
                }
                scanForInput = false;
                break;
            }
        }
    }
}

/**
 * @brief Check if the input is pressed
 *
 * @param buttonName
 * @return true
 * @return false
 */
bool isKeyDown(enum inputButtons buttonName)
{
    return keysdown & inputs[buttonName].value && inputs[buttonName].value != -1;
}

/**
 * @brief Check if the input is released
 *
 * @param buttonName
 * @return true
 * @return false
 */
bool isKeyUp(enum inputButtons buttonName)
{
    return keysup & inputs[buttonName].value && inputs[buttonName].value != -1;
}

/**
 * @brief Check if the input is held
 *
 * @param buttonName
 * @return true
 * @return false
 */
bool isKey(enum inputButtons buttonName)
{
    return keys & inputs[buttonName].value && inputs[buttonName].value != -1;
}