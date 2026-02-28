#include "frame_producer.h"
#include "utl_file.h"
#include "raylib.h"
#include "raygui.h"
#include "controls.h"
#define FIFO_IMPLEMENTATION
#include "fifo.h"
#undef FIFO_IMPLEMENTATION

#define IN_IMG_W 800
#define IN_IMG_H 480
#define FRAME_BUF_SZ (IN_IMG_W * IN_IMG_H * 4)

int main()
{
    // Main globals vars
    void *prod_ret;
    uint8_t *frame_data;
    char **mp4s = NULL;
    int mp4Count, activeItem, scrollIndex;
    float iconRecordRadius;
    Vector2 screenVec2;
    Vector2 iconRecordVec2;
    pthread_t prod_thread;
    ControlAction controlAction;
    fifo_buffer_t *fifo;
    Rectangle sideCtrlRec, listRec;
    ControlState controlState;
    bool controlsActive, mp4sLoaded, prevCtrlsActive, recording;
    Vector2 dragVector;
    recorder_states recorder_state;
    client_states client_state;

    // Initialize vars
    fifo_init(&fifo, FRAME_BUF_SZ);
    if (NULL == fifo)
    {
        fprintf(stderr, "fifo_init failed to init fifo\n");
        return 1;
    }

    pthread_create(&prod_thread, NULL, produce_frames, (void *)fifo);
    mp4Count = 0;
    activeItem = -1;
    scrollIndex = 0;
    controlState = CONTROL_CAMERA;
    controlAction = NONE;
    controlsActive = true;
    mp4sLoaded = false;
    recorder_state = INV;
    client_state = fifo_client_state(fifo);

    
    InitWindow(800, 480, "raygui-control");
    GuiLoadStyle("../raygui/styles/cyber/cyber.rgs");
    GuiSetStyle(DEFAULT, TEXT_SIZE, 30);
    SetTargetFPS(60);
    screenVec2 = (Vector2){ .x = GetScreenWidth(), .y = GetScreenHeight() };
    iconRecordRadius  = 16;
    iconRecordVec2 = (Vector2){ screenVec2.x - iconRecordRadius * 2, iconRecordRadius * 2};

    Texture2D cameraTex = GetVideoImage(FRAME_BUF_SZ, IN_IMG_W, IN_IMG_H, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    sideCtrlRec = GetSideControlRec(controlsActive);
    listRec = GetVidListControlRec(sideCtrlRec.width);

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        switch(controlState)
        {
            case CONTROL_FILES:
                if (mp4sLoaded)
                {
                    activeItem = GuiListViewExSwipe(listRec, (const char **)mp4s, mp4Count, NULL, &scrollIndex, activeItem, &dragVector);
                }
                break;
            case CONTROL_CAMERA:
                
                DrawCameraControl(&cameraTex);
                if (fifo_read(fifo, &frame_data))
                {
                    UpdateTexture(cameraTex, frame_data);
                }
                if (fifo_recorder_state(fifo) == REC)
                {
                    DrawCircleV(iconRecordVec2, iconRecordRadius, RED);
                }
                break;
            default:
                break;
        }

        prevCtrlsActive = controlsActive;
        controlsActive = SideControls(sideCtrlRec, activeItem, &controlAction, controlState, recording);
        
        DrawFPS(screenVec2.x / 2, 4);

        EndDrawing();

        // Update the state
        ControlState prevState = controlState;
        ControlAction lastAction = controlAction;
        UpdateControlState(&controlAction, &controlState);
        recorder_state = fifo_recorder_state(fifo);
        client_state = fifo_client_state(fifo);
        recording = client_state == CLIENT_REC;
        
        // Do controls hidden tasks
        if (prevCtrlsActive != controlsActive)
        {
            sideCtrlRec = GetSideControlRec(controlsActive);
            switch(controlState)
            {
                case CONTROL_FILES:
                    listRec = GetVidListControlRec(sideCtrlRec.width);
                    break;
                case CONTROL_CAMERA:
                default:
                    break;
            }
        }

        // Do new state tasks
        if (prevState != controlState)
        {
            switch(controlState)
            {
                case CONTROL_FILES:
                    mp4sLoaded = false;
                    mp4s = GetMp4s(&mp4Count);
                    mp4sLoaded = true;
                    break;
                default:
                    break;
            }
        };

        // Do current state tasks
        switch(controlState)
        {
            case CONTROL_CAMERA:
            {
                bool cliRec = client_state == CLIENT_REC;
                bool recIdle = recorder_state == IDLE;
                bool cliStop = client_state == CLIENT_STOP;
                bool recRec = recorder_state == REC;

                if (lastAction == CAMERA_REC && !(cliRec) && recIdle)
                {
                    fifo_client_state_set(fifo, CLIENT_REC);
                }
                else if (lastAction == CAMERA_STOP && !(cliStop) && recRec)
                {
                    fifo_client_state_set(fifo, CLIENT_STOP);
                }
            }
            break;
            default:
            break;
        }
    }
    fifo_client_state_set(fifo, CLIENT_EXIT);
    CloseWindow();
    pthread_join(prod_thread, &prod_ret);
    printf("prod_thread returned with code %d\n", *(int *)prod_ret);
    fifo_destroy(fifo);
    ClearDirectoryFiles();
    return 0;
}
