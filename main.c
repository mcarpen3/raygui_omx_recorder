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
    char *output_vid_dir;
    char **mp4s = NULL;
    char *player_filename;
    int mp4Count, activeItem, scrollIndex, iconRecordDur, rc;
    float iconRecordRadius;
    double lastSeconds;
    Vector2 screenVec2;
    Vector2 iconRecordVec2;
    pthread_t prod_thread;
    ControlAction controlAction;
    fifo_buffer_t *fifo;
    Rectangle sideCtrlRec, listRec;
    ControlState controlState;
    ControlSubState controlSubState;
    bool controlsActive, mp4sLoaded, prevCtrlsActive, bRecordingIcon;
    Vector2 dragVector;
    recorder_states recorder_state;
    client_states client_state;
    player_states player_state;

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
    player_state = fifo_player_state(fifo);
    controlSubState = CONTROL_CAMERA_NONE;
    bRecordingIcon = true;
    iconRecordDur = 2;
#ifndef OUTPUT_VID_DIR
    output_vid_dir = "OutputVideos";
#else
    output_vid_dir = OUTPUT_VID_DIR;
#endif   

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
        recorder_state = fifo_recorder_state(fifo);
        client_state = fifo_client_state(fifo);
        player_state = fifo_player_state(fifo);

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
            case CONTROL_PLAYER:
            case CONTROL_CAMERA:

                DrawCameraControl(&cameraTex);
                if (player_state != PLAYER_PAUSE && fifo_read(fifo, &frame_data))
                {
                    UpdateTexture(cameraTex, frame_data);

                }
                if (fifo_recorder_state(fifo) == REC)
                {
                    if (bRecordingIcon)
                    {
                        DrawCircleV(iconRecordVec2, iconRecordRadius, RED);
                    }
                    if ((int)(GetTime() - lastSeconds) >= iconRecordDur)
                    {
                        bRecordingIcon = !bRecordingIcon;
                        lastSeconds = GetTime(); 
                    }
                }
                break;
            case CONTROL_DELETE:
                DrawText("Delete video?", screenVec2.x / 2, screenVec2.y / 2, 24.0f, RED);
                break;
            default:
                break;
        }

        prevCtrlsActive = controlsActive;
        controlsActive = SideControls(sideCtrlRec, activeItem, &controlAction, controlState, controlSubState);
        
        DrawFPS(screenVec2.x / 2, 4);

        EndDrawing();

        // Update the state
        ControlState prevState = controlState;
        ControlAction lastAction = controlAction;
        UpdateControlState(&controlAction, &controlState);
                
        switch(client_state)
        {
            case CLIENT_REC:
                controlSubState = CONTROL_CAMERA_REC;
                break;
            case CLIENT_IDLE:
            case CLIENT_STOP:
                controlSubState = CONTROL_CAMERA_IDLE;
                break;
            case CLIENT_PLAY:
                {
                    if (lastAction == PLAYER_PLAY_ACT)
                    {
                        controlSubState = CONTROL_PLAYER_PLAY;
                    }
                    else if (lastAction == PLAYER_PAUSE_ACT)
                    {
                        controlSubState = CONTROL_PLAYER_PAUSE;
                    }
                } break;
            default:
                controlSubState = CONTROL_CAMERA_NONE;
                break;
        }
        
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
                    mp4s = GetMp4s(&mp4Count, output_vid_dir);
                    mp4sLoaded = true;
                    break;

                case CONTROL_PLAYER:
                {
                    player_filename = mp4s[activeItem];
                    char *player_filename_path = make_message("%s/%s", output_vid_dir, player_filename);

                    fifo_player_set_filename(fifo, player_filename_path);
                    printf("player_filename_path = %s\n", player_filename_path);
                    if (player_filename_path) free(player_filename_path);
                    controlSubState = CONTROL_PLAYER_PLAY;
                    
                } break;

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
                    lastSeconds = GetTime();
                }
                else if (lastAction == CAMERA_STOP && !(cliStop) && recRec)
                {
                    fifo_client_state_set(fifo, CLIENT_STOP);
                }
            }
            break;
            case CONTROL_PLAYER:
            {
                bool cliPlay = client_state == CLIENT_PLAY;
                bool recIdle = recorder_state == IDLE;
                bool playerPause = player_state == PLAYER_PAUSE;
                bool playerPlay = player_state == PLAYER_PLAY;
                bool playerRw = player_state == PLAYER_RW;
                bool playerFf = player_state == PLAYER_FF;
                
                if (lastAction == PLAY && !(cliPlay) && recIdle)
                {
                    fifo_client_state_set(fifo, CLIENT_PLAY);
                }
                if (lastAction == PLAYER_PAUSE_ACT && !(playerPause))
                {
                    fifo_player_state_set(fifo, PLAYER_PAUSE);
                }
                if (lastAction == PLAYER_PLAY_ACT && !(playerPlay))
                {
                    fifo_player_state_set(fifo, PLAYER_PLAY);
                }
                if (lastAction == PLAYER_FF_ACT && !(playerFf))
                {
                    fifo_player_state_set(fifo, PLAYER_FF);
                }
                if (lastAction == PLAYER_RW_ACT && !(playerRw))
                {
                    fifo_player_state_set(fifo, PLAYER_RW);
                }
            }
            break;
            case CONTROL_FILES:
            {
                bool cliIdle = client_state == CLIENT_IDLE;
                bool recPlay = recorder_state == REC_PLAY;

                if (lastAction == FILES && !(cliIdle) && recPlay)
                {
                    fifo_client_state_set(fifo, CLIENT_IDLE);
                }
                if (lastAction == DELETE_YES)
                {
                    printf("delete %s?\n", mp4s[activeItem]); 
                    rc = remove(make_message("%s/%s", output_vid_dir, mp4s[activeItem]));
                    if (rc != 0)
                    {
                        fprintf(stderr, "ERROR deleting %s, status code %d\n", mp4s[activeItem], rc);
                        perror("ERROR");
                    }
                    else
                    {
                        mp4s = GetMp4s(&mp4Count, output_vid_dir);
                    }
                }
            }
            break;
            case CONTROL_DELETE:
            {
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
