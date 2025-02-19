# Mac make file.
cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(jitBrainsForth VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)



# Add the executable
add_executable(jitBrainsForth
        main.cpp
        jitContext.cpp
        jitContext.h
        ForthDictionary.cpp
        ForthDictionary.h
        StackManager.h
        JitGenerator.h
        interpreter.h
        utility.h
        quit.cpp
        Compiler.h
        CompilerUtility.h
        jitLabels.h
        StringStorage.h
        tests.h
        StringStorage.h
)

# Copy start.f to the build directory
file(COPY ${CMAKE_SOURCE_DIR}/start.f DESTINATION ${CMAKE_BINARY_DIR})

# Add the include path for asmjit
target_include_directories(jitBrainsForth PRIVATE /usr/local/include)

find_package(asmjit CONFIG REQUIRED)

# Link the AsmJit library to your project
target_link_libraries(jitBrainsForth PRIVATE asmjit::asmjit)
