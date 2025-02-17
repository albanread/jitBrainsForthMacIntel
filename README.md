# jitBrainsForth

Forth Jit (MacOSX Intel variant) written in cooperation with the JetBrains AI.

This is the Mac OSX Intel Variant

Currently still being adjusted (from the Windows version) to meet the requirements below.

MacOS Intel we use ASMJIT to compile code for a 64bit intel cpu.
The calling convention to C is to use RDI, RSI for parameters etc.
Our machine stack must be 16 byte aligned, so we need to adjust the stack before calling a function, since the call uses 8 bytes.

There is also a Windows version of this Forth on this Github.


