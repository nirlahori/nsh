#ifndef COMMAND_STRUCT_HPP
#define COMMAND_STRUCT_HPP


#include <vector>
#include <map>
#include <string>
#include <list>

#include <cstring>
#include <cstdlib>

struct command_info{
    std::map<std::string, std::string> envs;
    std::string execfile;
    std::vector<std::string> cmdargs;

    int output_fd {1};
    int input_fd {0};

    std::string output_filename;
    std::string input_filename;
};


struct job_info{
    std::list<std::list<command_info>> chainlist;
};

#endif // COMMAND_STRUCT_HPP
