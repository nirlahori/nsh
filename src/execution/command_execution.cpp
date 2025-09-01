#include <algorithm>
#include <string>
#include <exception>

#include "command_execution.hpp"
#include "builtin.hpp"
#include "../tokens.hpp"
#include "../parse_input.hpp"
#include "../word_control.hpp"

#include <unistd.h>

Command_Execution::Command_Execution() : prompt_fmt {"nsh/:"}, prompt_suffix {" => "} {}


void Command_Execution::start_loop(){


    char cwdbuf[1024];

    builtin::init_builtin_map();

    std::string line;
    std::list<std::string> tokens;

    std::string shell_cwd(1024, '\0'), shell_prompt;

    if(getcwd(shell_cwd.data(), 1024) != nullptr){
        shell_prompt = prompt_fmt + shell_cwd.c_str() + prompt_suffix;
    }
    else{
        std::terminate();
    }

    while(true){


        if(getcwd(cwdbuf, 1024) == nullptr){
            std::perror("Error");
        }

        if(shell_cwd != cwdbuf){
            shell_cwd.assign(cwdbuf, std::strlen(cwdbuf));
            if(shell_cwd == getenv("HOME")){
                shell_prompt = prompt_fmt + "~" + prompt_suffix;
            }
            else{
                shell_prompt = prompt_fmt + shell_cwd + prompt_suffix;
            }
        }

        std::fprintf(stdout, shell_prompt.c_str());
        std::getline(std::cin, line);

        if(line.empty())
            continue;

        tokenize_input(line, tokens);
        std::list<command_info> cmds {parse::extract_commands(tokens)};

        std::for_each(cmds.begin(), cmds.end(), [](command_info& cinfo){
            wexpand::expand_cmdline_envs(cinfo.envs);
            wexpand::expand_cmdline_args(cinfo.cmdargs);
        });


        for(auto& cmd : cmds){
            if(builtin::is_builtin(cmd.execfile)){
                std::list<std::string> arglist (cmd.cmdargs.begin(), cmd.cmdargs.end());
                builtin::execute(cmd.execfile, arglist, cmd.envs);
            }
        }


        // for(const auto& cmd : cmds){
        //     std::printf("cmd exec: %s\n", cmd.execfile.c_str());
        //     std::printf("cmd environment variables: \n");
        //     for(const auto& [name, value] : cmd.envs)
        //         std::cout << "  name: " << name << " value: " << value << std::endl;
        //     std::printf("cmd cmdline args: \n  ");
        //     std::copy(cmd.cmdargs.begin(), cmd.cmdargs.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
        // }
        tokens.clear();
    }
}
