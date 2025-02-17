// quit.cpp


#include <iostream>
#include "quit.h"
#include "interpreter.h"
#include <thread>


bool escapePressed()
{

    return false;
}


std::thread Quit() {
    //
    sm.resetDS();
    sm.resetLS();
    sm.resetSS();
    sm.resetRS();


    while (true)
    {
        try
        {

            interactive_terminal();
        }

        catch (const std::exception& e)
        {
            std::cerr << "Runtime error: " << e.what() << std::endl;
            // Reset context and stack as required
        }
    }
}
