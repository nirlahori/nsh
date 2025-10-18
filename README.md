# nsh
nsh is a unix shell written in C++20. The shell supports command chaining,
foreground and background job control, shell pipelines and some other features.


# Features implemented

    Command Chaining (;) - Execute multiple commands sequentially.
    
    Pipelines (|) - Execute shell pipelines such as "ls -l | wc".
    
    Foreground and Background Job control - Manage multiple jobs simultaneously.
    
    Per-command environment variables - Specify the temporary environment variables when running commands.
    
    Built-in commands - Some essential built-in commands like cd and exit.


# Building

Requirements:

    CMake (min version required: 3.5)
    C++ compiler with support for C++20 standard
    
Instructions:
    
    mkdir <build dir>
    cd <build dir>
    cmake ..
    make
    
The above steps will generate the nsh executable in the build directory.
Run the nsh to start the nsh shell.


# Example usage

    ls -l

    ls -l; who; date

    cat /etc/passwd | cut -d: -f1 | sort | uniq | grep root

    sleep 10 &    

# License
This project is released as a personal learning project and is not intended for production use.
You may freely study, modify, or build upon it.
