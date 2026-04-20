#include <iostream>
#include <iomanip>
#include <vector>
#include <RtMidi.h>
#include <cstdint>


const std::string RESET = "\033[0m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string PURPLE = "\033[35m";


void midiCallback(double deltatime, std::vector<unsigned char> *message, void *userData)
{
    if (message->size() < 3)
        return;

    (void)deltatime;
    (void)userData;

    const unsigned char status = message->at(0);
    if (status != 0xF0) {
        return;
    }

    const unsigned char manufacturer = message->at(1);

    if (manufacturer == 0x7D) {
        // Debug text stream produced by debug_send_string().
        std::cout << PURPLE << "Debug: " << RESET;
        for (size_t i = 2; i + 1 < message->size(); i++) {
            const unsigned char c = message->at(i);
            if (c >= 32 && c <= 126) {
                std::cout << static_cast<char>(c);
            }
        }
        std::cout << std::endl;
        return;
    }

    // Generic SysEx dump for any other manufacturer/debug packet.
    std::cout << YELLOW << "SysEx [mfr 0x"
              << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
              << static_cast<int>(manufacturer)
              << "]" << RESET << " data: ";

    for (size_t i = 2; i + 1 < message->size(); i++) {
        std::cout << std::dec << static_cast<int>(message->at(i));
        if (i + 2 < message->size()) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;
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
