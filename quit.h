// quit.h
#ifndef QUIT_H
#define QUIT_H
#include <csetjmp>
#include <thread>

void Quit();  // Declaration of Quit function
bool escapePressed();
static jmp_buf jumpBuffer;
#endif // QUIT_H