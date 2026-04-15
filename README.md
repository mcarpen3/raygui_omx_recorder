# raygui_omx_recorder
- uses raylib@3.0 and raygui@3.0 for raspberry pi 3
- Extends GuiListViewEx -> GuiListViewExSwipe to support file list scrolling with Gestures on touch display
  - Not merged in with raygui3.0 TODO pull request to @raysan5
  - Need to patch GuiListViewExSwipe manually into raygui.h. Instructions in GuiListViewExSwipe.h
- Uses Makefile and raspberry pi 3b+ with screen [XYZStudy 800x480 LCD](https://www.amazon.com/dp/B083TG7Y9B?ref=ppx_yo2ov_dt_b_fed_asin_title)
    - See Makefile for dependencies.
    - Touchscreen works OOB on Raspberry PI 3B+ with Raspbian buster.
    - Unstable on newer OS. Will work with some overlay tricks I hope.
## Random ffmpeg cmdline tricks
- Get packet count `ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of default=noprint_wrappers=1:nokey=1 input.mp4`
- Get packet pts   `ffprobe -show_entries packet=pts_time,stream_index -select_streams [stream_type] -of csv=p=0 [input_file]`
- Show packet duration `ffprobe -show_entries packet=pts_time,duration_time,stream_index -of compact=p=0:nk=1 -v error input.mp4`
## Screenshots
- ![Screenshot0](https://github.com/mcarpen3/raygui_omx_recorder/blob/main/screenshots/scshot_20260415152459.png?raw=true)
- ![Screenshot1](https://github.com/mcarpen3/raygui_omx_recorder/blob/main/screenshots/scshot_20260415152515.png?raw=true)
- ![Screenshot2](https://github.com/mcarpen3/raygui_omx_recorder/blob/main/screenshots/scshot_20260415152525.png?raw=true)
- ![Screenshot3](https://github.com/mcarpen3/raygui_omx_recorder/blob/main/screenshots/scshot_20260415152530.png?raw=true)
- ![Screenshot4](https://github.com/mcarpen3/raygui_omx_recorder/blob/main/screenshots/scshot_20260415152539.png?raw=true)
