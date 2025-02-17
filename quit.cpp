#include <iostream>
#include <csignal>
#include <csetjmp> // Required for setjmp and longjmp

#include "quit.h"
#include "interpreter.h"



// Standard exception messages
const char *exception_messages[] = {
    "Unknown error", // 0
    "Stack underflow", // 1
    "Stack overflow", // 2
    "Invalid memory access", // 3
    "Division by zero", // 4
    "Invalid word", // 5
    "Invalid execution token", // 6
    "Undefined behavior", // 7
    "ERROR: EXEC Attempted to execute NULL XT", // 8
    "Break on CTRL/C" // 9
};

static jmp_buf quit_env; // Jump buffer for error handling

// Function to raise an exception
void raise_c(int eno) {
    size_t num_exceptions = sizeof(exception_messages) / sizeof(exception_messages[0]);

    // Ensure the error number is within bounds
    if (eno < 0 || eno >= num_exceptions) {
        eno = 0; // Default to "Unknown error"
    }

    fprintf(stderr, "FORTH RUNTIME ERROR: %s (Error %d)\n", exception_messages[eno], eno);

    // Instead of exiting, jump back to Quit()'s saved state
    longjmp(quit_env, 1); // Jump back to the point saved by setjmp
}

// Signal handler for SIGINT
void handle_sigint(int sig) {
    raise_c(9); // Raise the SIGINT-specific error
}

// The Quit function with setjmp for recovery
void Quit() {


    // Set up the signal handler for SIGINT
    signal(SIGINT, handle_sigint);

    // Main program loop
    while (true) {
        // Save the current execution state for recovery
        if (setjmp(quit_env) == 0) {
            // Initial path: this runs the first time (or after recovery resumes here)
            sm.resetDS();
            sm.resetLS();
            sm.resetSS();
            sm.resetRS();


            // Run the main interactive loop
            try {
                interactive_terminal(); // Main interpreter loop
            } catch (const std::exception &e) {
                // Handle runtime exceptions gracefully
                std::cerr << "Runtime error: " << e.what() << std::endl;

                // Reset stacks so we recover cleanly
                sm.resetDS();
                sm.resetLS();
                sm.resetSS();
                sm.resetRS();
            }

        } else {
            // Recovery path: longjmp returns here when an exception or signal occurs


            sm.resetDS(); // Reset data stack
            sm.resetLS(); // Reset local stack
            sm.resetSS(); // Reset state stack
            sm.resetRS(); // Reset return stack

            // The `while(true)` ensures the loop continues after recovery
        }
    }
}