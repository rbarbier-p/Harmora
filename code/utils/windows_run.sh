rm -rf midi_monitor
g++ -g -I/ucrt64/include/rtmidi midi_monitor.cpp -o midi_monitor -lrtmidi
./midi_monitor
