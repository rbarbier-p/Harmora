#include <iostream>
#include <iomanip>
#include <vector>
#include <rtmidi/RtMidi.h>

void midiCallback(double deltatime, std::vector<unsigned char> *message, void *userData)
{
    std::cout << "dt=" << deltatime << "  ";

    if (message->size() == 0)
        return;

    unsigned char status = message->at(0);

    // Detect SysEx
    if (status == 0xF0)
    {
        std::cout << "SysEx: ";

        for (unsigned char b : *message)
        {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << (int)b << " ";
        }

        std::cout << std::dec;

        // Attempt ASCII decode (useful for debug logs)
        std::cout << " | ";

        for (size_t i = 1; i < message->size(); i++)
        {
            unsigned char c = message->at(i);
            if (c >= 32 && c <= 126)
                std::cout << (char)c;
        }

        std::cout << std::endl;
        return;
    }

    // Regular MIDI message
    std::cout << "MIDI: ";

    for (unsigned char b : *message)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (int)b << " ";
    }

    std::cout << std::dec << std::endl;
}

int main()
{
    RtMidiIn midiin;

    unsigned int ports = midiin.getPortCount();

    if (ports == 0)
    {
        std::cout << "No MIDI input devices found.\n";
        return 0;
    }

    std::cout << "Available MIDI inputs:\n";

    for (unsigned int i = 0; i < ports; i++)
    {
        std::cout << i << ": " << midiin.getPortName(i) << std::endl;
    }

    std::cout << "Select port: ";
    unsigned int port;
    std::cin >> port;

    if (port >= ports)
    {
        std::cout << "Invalid port\n";
        return 1;
    }

    midiin.openPort(port);

    // Do not ignore SysEx
    midiin.ignoreTypes(false, false, false);

    midiin.setCallback(&midiCallback);

    std::cout << "Listening... press Enter to quit.\n";

    std::cin.ignore();
    std::cin.get();

    return 0;
}
