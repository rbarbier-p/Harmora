- [ ] make new clean implementation by regrouping: working.c + (debug, device_manager, mcu, midi, usb, main)
    -> clean and simple implementation that i actually know

NO CDC SUPPORT:
main3.c (removed) -> send mcu messages | works in midiview but not reaper
CDC SUPPORT:
working.c -> send a lot of sysex messages | works in midiview and reaper (if not only sysex messages)
separated files -> sends nothing | works no where 
midi_usb.c -> works in reaper and midiview | as a midi device but MCU stops working ?

- working.c MCU working in REAPER
    -> splitted files same main doesn't work at all

-> trying to find the main i was using: "harmora v1", playing chords

- USB
    -> CDC
        -> handle usb reset
    -> MIDI
        -> MCU/sysex input/ouput
        -> MIDI

