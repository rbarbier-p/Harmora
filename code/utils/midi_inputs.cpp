#include <iostream>
#include <iomanip>
#include <vector>
#include <rtmidi/RtMidi.h>

const std::string RESET = "\033[0m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string BLUE = "\033[34m";
const std::string YELLOW = "\033[33m";
const std::string PURPLE = "\033[35m";

void midiCallback(double deltatime, std::vector<unsigned char> *message, void *userData)
{
    if (message->size() == 0)
        return;

    (void)deltatime;

    unsigned char status = message->at(0);
    unsigned char debug = message->at(1);

    // Detect SysEx
    if (status == 0xF0)
    {
        if (debug == 0x7D)
        {
            std::cout << PURPLE << "Debug: " << RESET;
            for (size_t i = 2; i < message->size(); i++)
            {
                unsigned char c = message->at(i);
                if (c >= 32 && c <= 126)
                    std::cout << (char)c;
            }
        }
        else if (debug == 0xFA)
        {
            std::cout << PURPLE << "MOS Packet: " << RESET << std::endl;
            std::cout << "CMD: " << message->at(2) << std::endl;
            for (size_t i = 3; i < message->size(); i++)
            {
                unsigned char c = message->at(i);
                if (c >= 32 && c <= 126)
                    std::cout << (char)c;
            }
        }
        else
            std::cout << "SysEx: ";


        std::cout << std::endl;
        return;
    }

    // Regular MIDI message
    /*
    std::cout << "MIDI: ";

    for (unsigned char b : *message)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (int)b << " ";
    }

    std::cout << std::dec << std::endl;
    */
}

int main()
{
    RtMidiIn midiin;

    unsigned int ports = midiin.getPortCount();

    if (ports == 0)
    {
        std::cout << "No MIDI input devices found." << std::endl;
        std::cout << RED << "Try unplugging and plugging back your device, then wait 10 seconds." << RESET << std::endl; 
        return 0;
    }

    std::cout << "Available MIDI inputs: " << std::endl;

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
