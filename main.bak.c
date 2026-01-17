#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#define FIFO_IMPLEMENTATION
#include "fifo.h"
#include "frame_producer.h"
#define IN_IMG_W 800
#define IN_IMG_H 480


int main()
{
    /**
      * Create the video frame producer thread
      */
    void *prod_ret;
    uint8_t *frame_data;
    fifo_buffer_t *fifo = malloc(sizeof(fifo_buffer_t));
    fifo_init(fifo, IN_IMG_W * IN_IMG_H * 4);
    pthread_t prod_thread;
    pthread_create(&prod_thread, NULL, produce_frames, (void *)fifo);
    InitWindow(800, 480, "raygui-control");
    GuiLoadStyle("../raygui/styles/cyber/cyber.rgs");
    bool showMessageBox = false; 
    // Create a CPU-side image only to initialize the texture
    Image img = {
        .data = MemAlloc(IN_IMG_W * IN_IMG_H * 4),   // RGBA
        .width = IN_IMG_W,
        .height = IN_IMG_H,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        //.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8
    };

    memset(img.data, 0x00, IN_IMG_W * IN_IMG_H * 4);
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

        if (record && !record_pressed) {
            record_pressed = true;
        } else if (!record_pressed) {
            record_pressed = true;
            if (GuiButton((Rectangle){5, 5, 140, 50}, "#135#Record"))
                showMessageBox = true;
            if (showMessageBox)
            { 
                int result = GuiMessageBox((Rectangle){85, 70, 250, 100},
                    "#191#Message Box", "Hi! This is a message!", "Nice;Cool");
                if (result >= 0)
                    showMessageBox = false;
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
            
