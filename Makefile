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
main: fifo.h main.c save_video.c $(RAYLIB)/src/libraylib.a sig_event.o frame_producer.o
sig_event.o: sig_event.c
frame_producer.o: frame_producer.c
clean:
	rm *.o
