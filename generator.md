# FORTH Interpreter and Compiler

This version of FORTH uses [AsmJit](https://github.com/asmjit/asmjit) to generate new words. 
This document explains how the interpreter and compiler work together to process and execute FORTH words.

## Interpreter

The FORTH interpreter processes each word and performs the following steps:

1. **Lookup in Dictionary**: If the word is found in the dictionary 
2. and we are in execution mode, the word is executed.
2. **Number Check**: If the word is not found in the dictionary, it is checked to see if it is a number.
    - If it is a number, the number is pushed onto the data stack.

## Compiler

The `:` (colon) word causes the interpreter to enter compiler mode. The steps in compiler mode are as follows:

1. **New Word Creation**: The `:` word takes the next word it encounters as a new word name and adds that to the dictionary.
2. The compiler works through the rest of the words up until the semicolon as it does so :-
3. **Immediate Words**: The compiler runs any immediate words it encounters.
3. **Generator Functions**: For words that have a `generatorFunc`, the compiler runs this function to generate inline code.
4. **Compiled Functions**: If there is no `generatorFunc` for a word, the compiler compiles a call to its `compiledFunc`.
5. **Number Handling**: When the compiler encounters a number, it compiles code to push that number onto the stack.
6. **End Compilation**: Compilation ends when the compiler encounters the `;` (semicolon) word.

This dual-mode operation—interpreting and compiling—allows for flexible execution and 
efficient code generation in FORTH.