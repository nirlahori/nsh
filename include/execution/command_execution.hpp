#ifndef COMMAND_EXECUTION_HPP
#define COMMAND_EXECUTION_HPP

#include <string>
#include <cstdio>

class Command_Execution
{

    std::string prompt_fmt;
    std::string prompt_suffix;

    static constexpr int readindex = 0;
    static constexpr int writeindex = 1;
    std::size_t ctrl_term_fd;

public:
    Command_Execution();


    void read_input();
    void start_loop();


};

#endif // COMMAND_EXECUTION_HPP
