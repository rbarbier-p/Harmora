#include <iostream>
#include <iomanip>
#include <vector>
#include <rtmidi/RtMidi.h>
#include <cstdint>
#include "mos.h"


const std::string RESET = "\033[0m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string BLUE = "\033[34m";
const std::string YELLOW = "\033[33m";
const std::string PURPLE = "\033[35m";

const char *COMMANDS[] = {"empty", "update data", "request data", "packet stream", "packet stream",  "debug print", "flush"};
const char *DEVICES[] = {"empty", "led", "potentiometer", "encoder", "switch", "magnetic switch", "on/off switch", "display", "stream"};
const char *VALUES[] = {"emtpy", "released", "pressed", "on", "off", "incremented", "decremented", "display draw char", "display draw circle", "display draw rectangle", "display draw triangle", "display draw line", "display draw icon"};


void midiCallback(double deltatime, std::vector<unsigned char> *message, void *userData)
{
    if (message->size() <= 2)
        return;

    (void)deltatime;

    unsigned char status = message->at(0);
    unsigned char debug = message->at(1);
    bool dataString = false;

    // Detect SysEx
    size_t i = 2;
    if (status == 0xF0)
    {
        if (debug == 0x7D)
        {
            std::cout << PURPLE << "Debug: " << RESET;
            dataString = true;
        }
        else if (debug == 0x7A)
        {
            std::cout << PURPLE << "[MOS Packet]" << RESET << std::endl;
            unsigned char command = message->at(2);
            if (command == M_CMD_DEBUG_PRINT)
                dataString = true;
            std::cout << "COMMAND: " << COMMANDS[command] << std::endl;

            std::cout << "DEVICE: " << DEVICES[message->at(3)] << std::endl;
            std::cout << "ID: " << std::dec << (int)message->at(4) << std::endl;
            std::cout << "VALUE: " << VALUES[message->at(5)] << std::endl;
            std::cout << "LENGTH: " << std::dec << (int)message->at(6) << std::endl;
            std::cout << "DATA: ";
            i = 7;
        }
        else 
            return;
        // for now sysex is not displayed because sometimes it's not text just hex data
        /*
            std::cout << "SysEx: ";
            */

        for (; i < message->size() - 1; i++)
        {
            unsigned char c = message->at(i);
            if (dataString)
            {
                if (c >= 32 && c <= 126)
                    std::cout << (char)c;
            }
            else
            {
                std::cout << std::dec << (int)c;
                if (i < message->size() - 2)
                    std::cout << ", ";
            }
        }

        std::cout << std::endl;
        return;
    }
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
