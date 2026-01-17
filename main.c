#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#define FIFO_IMPLEMENTATION
#include "fifo.h"
#include "frame_producer.h"
#define IN_IMG_W 800
#define IN_IMG_H 480
#define FRAME_BUF_SZ (IN_IMG_W * IN_IMG_H * 4)


int main()
{
    /**
      * Create the video frame producer thread
      */
    void *prod_ret;
    uint8_t *frame_data;
    fifo_buffer_t *fifo;
    fifo_init(&fifo, FRAME_BUF_SZ);
    if (NULL == fifo)
    {
        fprintf(stderr, "fifo_init failed to init fifo\n");
        return 1;
    }
    pthread_t prod_thread;
    pthread_create(&prod_thread, NULL, produce_frames, (void *)fifo);
    InitWindow(800, 480, "raygui-control");
    GuiLoadStyle("../raygui/styles/cyber/cyber.rgs");
    // Create a CPU-side image only to initialize the texture
    Image img = {
        .data = MemAlloc(FRAME_BUF_SZ), //RGBA 
        .width = IN_IMG_W,
        .height = IN_IMG_H,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        //.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8
    };

    memset(img.data, 0x00, FRAME_BUF_SZ);
    // Upload to GPU
    Texture2D cameraTex = LoadTextureFromImage(img);
    UnloadImage(img);

    while (!WindowShouldClose())
    {
        fifo_read(fifo, &frame_data);
        UpdateTexture(cameraTex, frame_data);
        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawTexturePro(cameraTex,
            (Rectangle){0, 0, IN_IMG_W, IN_IMG_H},
            (Rectangle){0, 0, IN_IMG_W, IN_IMG_H},
            (Vector2){0, 0},
            0.0f,
            WHITE);
        recorder_states recorder_state = fifo_recorder_state(fifo);
        char *icon;
        switch(recorder_state)
        {
            
            case IDLE:
                icon = "#135#Record";
                break;
            case REC:
                icon = "#133#Stop";
                break;
            case INV:
            default:
                icon = "#137#Wait";
                break;
        } 
        if (GuiButton((Rectangle){5,5,280,100}, icon)) 
        {
            if (CLIENT_REC != fifo_client_state(fifo) && IDLE == recorder_state)
            {
                fifo_client_state_set(fifo, CLIENT_REC);
            }
            else if (CLIENT_STOP != fifo_client_state(fifo) && REC == recorder_state)
            {
                fifo_client_state_set(fifo, CLIENT_STOP);
            }
        }
        EndDrawing();
    }
    CloseWindow();
    pthread_join(prod_thread, &prod_ret);
    printf("prod_thread returned with code %d\n", *(int *)prod_ret);
    fifo_destroy(fifo);
    return 0;
}
