#include "controls.h"

Rectangle GetSideControlRec(bool active)
{
    if (active)
    {
        return (Rectangle){0,0,2*GuiGetStyle(BUTTON, BORDER_WIDTH) + MD_BTN_W, GetScreenHeight() - 2*GuiGetStyle(BUTTON, BORDER_WIDTH)};
    }
    else
    {
        return (Rectangle){0,0,0,0};
    }
}

Rectangle GetVidListControlRec(int sideControlWidth)
{
    return (Rectangle){sideControlWidth, 0, GetScreenWidth() - sideControlWidth, GetScreenHeight()};
} 

void UpdateControlState(ControlAction *action, ControlState *state)
{
    switch(*action)
    {
        case CAMERA:
            *state = CONTROL_CAMERA;
            break;
        case FILES:
            *state = CONTROL_FILES;
            break;
        case PLAY:
        case DELETE:
        case CAMERA_REC:
        case CAMERA_STOP:
        case NONE:
        default:
            break;
    }
    *action = NONE;
}

Texture2D GetVideoImage(int frameBufSz, int width, int height, int pixFmt)
{    
    // Create a CPU-side image only to initialize the texture
    Image img = {
        .data = MemAlloc(frameBufSz), //RGBA 
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = pixFmt
        //.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8
    };

    memset(img.data, 0x00, frameBufSz);
    // Upload to GPU
    Texture2D cameraTex = LoadTextureFromImage(img);
    UnloadImage(img);
    return cameraTex;
}


void DrawCameraControl(Texture2D *cameraTex)
{
    DrawTexturePro(*cameraTex,
        (Rectangle){0, 0, cameraTex->width, cameraTex->height},
        (Rectangle){0, 0, cameraTex->width, cameraTex->height},
        (Vector2){0, 0},
        0.0f,
        WHITE);
};

bool SideControls(Rectangle bounds, bool isActive, int activeItem, ControlAction *act, ControlState state)
{
    bool curActive = false;
    int btnBorder = GuiGetStyle(BUTTON, BORDER_WIDTH);
    ControlAction action = NONE; 
    
    if (!isActive)
    {
        Rectangle btnShow = (Rectangle){btnBorder, GetScreenHeight() - SM_BTN_H - btnBorder, SM_BTN_W, SM_BTN_H};
        curActive = GuiButton(btnShow, ">>") ? true : false;
    }
    else
    {
        Rectangle btnHide = (Rectangle){btnBorder, GetScreenHeight() - MD_BTN_H - btnBorder, MD_BTN_W, MD_BTN_H};
        DrawRectangleLines(bounds.x, bounds.y, bounds.width, bounds.height, BLACK);
        curActive = GuiButton(btnHide, "<<") ? false : true;
        
        Rectangle btnIter = (Rectangle){btnBorder, btnBorder, MD_BTN_W, MD_BTN_H};
        if (state == CONTROL_FILES)
        {
            if (activeItem == -1)
            {
                GuiDisable();
            }

            if (GuiButton(btnIter, "#131#Play"))
            {
               action = PLAY;
            }
            btnIter.y += MD_BTN_H + btnBorder;
            if (GuiButton(btnIter, "#9#Delete"))
            {
                action = DELETE;
            }
            // Renable for nav btns
            GuiEnable();
            btnIter.y += MD_BTN_H + btnBorder;
            if (GuiButton(btnIter, "#169#Camera"))
            {
                action = CAMERA;
            }
        }
        else if (state == CONTROL_CAMERA)
        {
            if (GuiButton(btnIter, "#135#Record"))
            {
                action = CAMERA_REC;
            }
            btnIter.y += MD_BTN_H + btnBorder;
            if (GuiButton(btnIter, "#133#Stop"))
            {
                action = CAMERA_STOP;
            }
            btnIter.y += MD_BTN_H + btnBorder;
            if (GuiButton(btnIter, "#3#Videos"))
            {
                action = FILES;
            }
        }
    }
    *act = action;
    return curActive;
}
