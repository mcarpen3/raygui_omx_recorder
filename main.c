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
    pthread_t prod_thread;
    ControlAction controlAction;
    fifo_buffer_t *fifo;
    Rectangle sideCtrlRec, listRec;
    ControlState controlState;
    bool controlsActive, mp4sLoaded, prevCtrlsActive;
    Vector2 dragVector;
    //recorder_states recorder_state;

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

    InitWindow(800, 480, "raygui-control");
    GuiLoadStyle("../raygui/styles/cyber/cyber.rgs");
    GuiSetStyle(DEFAULT, TEXT_SIZE, 30);

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
                if (!mp4sLoaded)
                {
                    mp4s = GetMp4s(&mp4Count);
                    printf("GET MP4S LEN: %d\n", mp4Count);
                    mp4sLoaded = true;
                }
                else
                {
                    activeItem = GuiListViewExSwipe(listRec, (const char **)mp4s, mp4Count, NULL, &scrollIndex, activeItem, &dragVector);
                }
                break;
            case CONTROL_CAMERA:
                if (fifo_read(fifo, &frame_data))
                {
                    UpdateTexture(cameraTex, frame_data);
                    DrawCameraControl(&cameraTex);
                }
                break;
            default:
                break;
        }

        prevCtrlsActive = controlsActive;
        controlsActive = SideControls(sideCtrlRec, controlsActive, activeItem, &controlAction, controlState);

        EndDrawing();

        ControlState prevState = controlState;
        UpdateControlState(&controlAction, &controlState);
        if (prevCtrlsActive != controlsActive)
        {
            switch(controlState)
            {
                case CONTROL_FILES:
                    sideCtrlRec = GetSideControlRec(controlsActive);
                    listRec = GetVidListControlRec(sideCtrlRec.width);
                    break;
                default:
                    break;
            }
        }

        if (prevState != controlState)
        {
            // Do new state tasks
            mp4sLoaded = false;
        }
        //recorder_state = fifo_recorder_state(fifo); 
        //if (CLIENT_REC != fifo_client_state(fifo) && IDLE == recorder_state)
        //{
            //fifo_client_state_set(fifo, CLIENT_REC);
        //}
        //else if (CLIENT_STOP != fifo_client_state(fifo) && REC == recorder_state)
        //{
            //fifo_client_state_set(fifo, CLIENT_STOP);
        //}
    }
    fifo_client_state_set(fifo, CLIENT_EXIT);
    CloseWindow();
    pthread_join(prod_thread, &prod_ret);
    printf("prod_thread returned with code %d\n", *(int *)prod_ret);
    fifo_destroy(fifo);
    ClearDirectoryFiles();
    return 0;
}


