# jitBrainsForth


Deprecated this version as the C++ side was not stable, it is not fun when the slightest code change breaks your application.


Forth Jit (MacOSX Intel variant) written in cooperation with the JetBrains AI.
This is the Mac OSX Intel Variant

Status: compiles and runs, you need OSX command line tools and CMake. I use CLion.
Currently still being adjusted (from the Windows version) to meet the requirements below.

MacOS Intel we use ASMJIT to compile code for a 64bit intel cpu.
The calling convention to C is to use RDI, RSI for parameters etc.
Our machine stack must be 16 byte aligned. 
This requires adjustment every time since every call uses 8 bytes.
We must use relative calls, or indirect them.

Other differences
The C compilers are slightly different
On the Mac we handle the SIGINT signal (ctrl/c) to interupt a long running command.
On the Mac we need to enter raw terminal mode, and handle the arrow keys etc.



There is also a Windows version of this Forth on this Github.

The project does not accept pull requests.

