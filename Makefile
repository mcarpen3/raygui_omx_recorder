FFMPEG_LIBS= libavdevice \
			 libavformat \
			 libavfilter \
			 libavcodec \
			 libswresample \
			 libswscale \
			 libavutil \

RAYLIB=/home/pi/raylib
RAYGUI=/home/pi/raygui
RAYLIB_LIBS= -lm \
			 -lpthread \
			 -lbrcmGLESv2 \
			 -lbrcmEGL \
			 -lvcos \
			 -lvchiq_arm \
			 -lbcm_host \
             -latomic \

CC      = gcc
CFLAGS += -Wall -Wextra -ggdb 
CFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS)) $(CFLAGS)
CFLAGS := -I/opt/vc/include -I$(RAYLIB)/src -I$(RAYGUI)/src $(CFLAGS)
LDLIBS := $(shell pkg-config --libs $(FFMPEG_LIBS)) $(LDLIBS)
LDLIBS := $(LDLIBS) $(RAYLIB_LIBS) -L/opt/vc/lib
OBJS = main.o raygui_impl.o sig_event.o frame_producer.o save_video.o controls.o utl_file.o
main: $(OBJS) $(RAYLIB)/src/libraylib.a 
	$(CC) -o $@ $(OBJS) $(RAYLIB)/src/libraylib.a $(LDLIBS)
sig_event.o: sig_event.c sig_event.h
controls.o: controls.c controls.h
utl_file.o: utl_file.c utl_file.h
frame_producer.o: frame_producer.c frame_producer.h
save_video.o: save_video.c save_video.h
output_video_dir: CFLAGS += -DOUTPUT_VID_DIR=\"OutputVideos\"
output_video_dir: main
raygui_impl.o: CFLAGS += -DRAYGUI_IMPLEMENTATION
raygui_impl.o: raygui_impl.c
clean:
	rm *.o
