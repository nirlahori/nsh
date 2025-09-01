#ifndef COMMAND_EXECUTION_HPP
#define COMMAND_EXECUTION_HPP

#include <string>
#include <list>
#include <iostream>
#include <cstdio>
#include <iterator>


class Command_Execution
{

    std::string prompt_fmt;
    std::string prompt_suffix;

public:
    Command_Execution();


    void read_input();
    void start_loop();

};

#endif // COMMAND_EXECUTION_HPP
