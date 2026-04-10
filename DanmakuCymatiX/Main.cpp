#include <iostream>
#include "bass.h"

int main() 
{
    // ======================================================
    // TEST
	// ======================================================
    
    //  Initialize Soundcard (-1: default device, 44100hz)
    if (BASS_Init(-1, 44100, 0, 0, NULL)) 
    {
        std::cout << "[SUCCESS] BASS 64-bit is initialized successfully.\n";
        BASS_Free(); // Cleanup
    }
    else 
    {
        std::cout << "[ERROR] BASS could not be initialized. Error Code: " << BASS_ErrorGetCode() << "\n";
    }

    std::cin.get();
    return 0;
}