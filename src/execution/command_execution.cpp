#include <algorithm>
#include <string>
#include <exception>
#include <csignal>
#include <cassert>
#include <iterator>

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "command_execution.hpp"
#include "../tokens.hpp"
#include "../parse_input.hpp"
#include "../word_control.hpp"
#include "../system_envs.hpp"
#include "job_control.h"



#define set_signal(sigobj, signum, action) \
    sigobj.sa_handler = action; \
    sigaction(SIGINT, &sa_intr, nullptr);




Command_Execution::Command_Execution() : prompt_fmt {"nsh/:"}, prompt_suffix {" => "} {

    // Setup signal handling
    struct sigaction sa_intr, sa_quit, sa_tstp;

    sa_intr.sa_flags = 0;
    if(sigemptyset(&sa_intr.sa_mask) < 0){
        std::puts("Coudn't start shell... unknown error occurred\n");
    }

    sa_quit.sa_flags = 0;
    if(sigemptyset(&sa_quit.sa_mask) < 0){
        std::puts("Coudn't start shell... unknown error occurred\n");
    }

    set_signal(sa_intr, SIGINT, SIG_IGN);
    set_signal(sa_quit, SIGQUIT, SIG_IGN);
}


void Command_Execution::start_loop(){


    environment::init_env();

    char cwdbuf[1024];

    std::string line;
    std::list<std::string> proc_tokens;
    std::list<std::string> cmd_tokens;
    std::list<std::list<std::string>> line_tokens;
    std::list<std::list<command_info>> proc_list;
    std::list<std::list<command_info>> bgjob_list;
    std::list<std::list<command_info>> fgjob_list;

    std::string shell_cwd(1024, '\0'), shell_prompt;

    char filepath[1024];

    if(getcwd(shell_cwd.data(), 1024) != nullptr){
        shell_prompt = prompt_fmt + shell_cwd.c_str() + prompt_suffix;
    }
    else{
        std::terminate();
    }


    Job_Control control_unit;

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

        std::printf("%s", shell_prompt.c_str());
        std::getline(std::cin, line);

        if(line.empty()){
            control_unit.wait_for_background_jobs();
            continue;
        }


        if(!tokenize_job(line, proc_tokens)){
            std::puts("Unknown error occured while parsing line");
            continue;
        }




        for(std::string& cmds : proc_tokens){
            tokenize_cmd(cmds, cmd_tokens);
            line_tokens.push_back(cmd_tokens);
            cmd_tokens.clear();
        }


        int bg_job_start_index {-1};
        int index {0};
        if(std::all_of(line_tokens.begin(), line_tokens.end(), [&bg_job_start_index](std::list<std::string>& elem){
                auto last_token = elem.back();
                //auto start_pos = last_token.find_first_not_of(' ');
                auto end_pos = last_token.find_last_not_of(' ');
                if(last_token[end_pos] == '&'){
                    return true;
                }
                return false;
            })){

            bg_job_start_index = 0;
        }
        else{
            for(const std::list<std::string>& elem : line_tokens){
                auto last_token = elem.back();
                auto end_pos = last_token.find_last_not_of(' ');
                if(last_token[end_pos] == '&'){
                    bg_job_start_index = index;
                    break;
                }
                index++;
            }
        }


        if(bg_job_start_index > -1){
            bool check_bg_job_syntax = std::all_of(std::next(line_tokens.begin(), bg_job_start_index), line_tokens.end(), [&bg_job_start_index](const std::list<std::string>& elem){
                auto last_token = elem.back();
                auto end_pos = last_token.find_last_not_of(' ');
                if(last_token[end_pos] == '&'){
                    return true;
                }
                return false;
            });

            if(!check_bg_job_syntax){
                std::printf("Parse error\n"); // Be clear
                line_tokens.clear();
                proc_tokens.clear();
                cmd_tokens.clear();
                continue;
            }
        }

        if(bg_job_start_index >= 0){
            std::for_each(std::next(line_tokens.begin(), bg_job_start_index), line_tokens.end(), [](std::list<std::string>& cmd){
                std::string& last_token {cmd.back()};
                auto pos = last_token.find_last_of('&');
                last_token.erase(pos - 1);
            });
        }

        for(std::list<std::string>& chain_key : line_tokens){
            proc_list.push_back(parse::extract_commands(chain_key));
        }

        std::for_each(proc_list.begin(), proc_list.end(), [](std::list<command_info>& list){
            std::for_each(list.begin(), list.end(), [](command_info& cinfo){
                wexpand::expand_cmdline_envs(cinfo.envs);
                wexpand::expand_cmdline_args(cinfo.cmdargs);
            });
        });

        if(bg_job_start_index < 0){
            std::move(proc_list.begin(), proc_list.end(), std::back_inserter(fgjob_list));
        }
        else if(bg_job_start_index == 0){
            std::move(proc_list.begin(), proc_list.end(), std::back_inserter(bgjob_list));
        }
        else if(bg_job_start_index > 0){
            std::move(proc_list.begin(), std::next(proc_list.begin(), bg_job_start_index), std::back_inserter(fgjob_list));
            std::move(std::next(proc_list.begin(), bg_job_start_index), proc_list.end(), std::back_inserter(bgjob_list));
        }

        control_unit.submit_foreground_jobs(fgjob_list);
        control_unit.submit_background_jobs(bgjob_list);


        control_unit.run_foreground_jobs();
        control_unit.run_background_jobs();


        proc_tokens.clear();
        cmd_tokens.clear();
        line_tokens.clear();
        proc_list.clear();

        fgjob_list.clear();
        bgjob_list.clear();

        control_unit.wait_for_background_jobs();
    }
}
