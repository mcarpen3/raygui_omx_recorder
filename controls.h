#pragma once
#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "raygui.h"
#include <string.h>
#define PHI 1.618033989
#define SM_BTN_W  80
#define MD_BTN_W 160
#define SM_BTN_H (SM_BTN_W / PHI)
#define MD_BTN_H (MD_BTN_W / PHI)

// button action types
typedef enum {
    NONE,
    PLAY,
    DELETE,
    CAMERA,
    FILES,
    CAMERA_REC,
    CAMERA_STOP,
} ControlAction;

typedef enum {
    CONTROL_CAMERA,
    CONTROL_FILES,
} ControlState;

// list of video files
//int GuiListViewExSwipe(Rectangle bounds, const char **text, int count, int *focus, int *scrollIndex, int active, Vector2 *lastDragVector);

// side bar of button controls
bool SideControls(Rectangle bounds, bool isActive, int activeItem, ControlAction *act, ControlState state);

Rectangle GetSideControlRec(bool active);
Rectangle GetVidListControlRec(int sideControlWidth);
void UpdateControlState(ControlAction *action, ControlState *state);
Texture2D GetVideoImage(int frameBufSz, int width, int height, int pixFmt);
void DrawCameraControl(Texture2D *cameraTex);
