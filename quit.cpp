#include <iostream>

#include "quit.h"
#include "interpreter.h"




void Quit() {


    // Run the main interactive loop
    while (true) {
        try {
            interactive_terminal();
        } catch (const std::exception& e) {
            // Handle runtime exceptions gracefully
            std::cerr << "Runtime error: " << e.what() << std::endl;
            sm.resetDS();
            sm.resetLS();
            sm.resetSS();
            sm.resetRS();
        }
    }
}