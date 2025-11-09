#ifndef COMMAND_EXECUTION_HPP
#define COMMAND_EXECUTION_HPP

#include <string>
#include <cstdio>
#include <csignal>

#include "execution/job_control.hpp"

class Command_Execution
{

    std::string prompt_fmt;
    std::string prompt_suffix;

    Job_Control control_unit;

    static sig_atomic_t sigflag;

public:
    Command_Execution();

    static void handle_interrupt(int signum, siginfo_t* info, void* context);
    static void handle_sigint(Job_Control* job_context);

    void read_input();
    void start_loop();
};


#endif // COMMAND_EXECUTION_HPP
