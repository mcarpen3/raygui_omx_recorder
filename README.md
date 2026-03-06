# raygui_omx_recorder
- uses raylib@3.0 and raygui@3.0 for raspberry pi 3
- Extends GuiListViewEx -> GuiListViewExSwipe to support file list scrolling with Gestures on touch display
  - Not merged in with raygui3.0 TODO pull request to @raysan5
  - Need to patch GuiListViewExSwipe manually into raygui.h. Instructions in GuiListViewExSwipe.h
- Get packet count `ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of default=noprint_wrappers=1:nokey=1 input.mp4`
- Get packet pts   `ffprobe -show_entries packet=pts_time,stream_index -select_streams [stream_type] -of csv=p=0 [input_file]`
- Show packet duration `ffprobe -show_entries packet=pts_time,duration_time,stream_index -of compact=p=0:nk=1 -v error input.mp4`
