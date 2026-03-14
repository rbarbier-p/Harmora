rm -rf midi_monitor
g++ midi_inputs.cpp -o midi_monitor -lrtmidi && ./midi_monitor
