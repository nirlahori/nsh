#ifndef COMMAND_STRUCT_HPP
#define COMMAND_STRUCT_HPP


#include <vector>
#include <map>
#include <string>

struct command_info{
    std::map<std::string, std::string> envs;
    std::string execfile;
    std::vector<std::string> cmdargs;
    bool builtin;
};


#endif // COMMAND_STRUCT_HPP
