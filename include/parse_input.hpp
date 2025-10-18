#ifndef PARSE_INPUT_HPP
#define PARSE_INPUT_HPP

#include <string>
#include <list>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cctype>
#include <algorithm>
#include <iostream>
#include <utility>

#include <unistd.h>

#include "command_struct.hpp"

namespace parse{


std::list<std::string> tokenize_command(const std::string& cmdtext){

    std::string delims {"\n "};
    std::string buf (cmdtext);
    std::list<std::string> cmdtokens;

    char* tok {std::strtok(buf.data(), delims.c_str())};
    while(tok){
        cmdtokens.push_back(std::string(tok));
        tok = std::strtok(nullptr, delims.c_str());
    }
    return cmdtokens;
}


std::map<std::string, std::string> extract_env_vars(const std::list<std::string>& tokens, int& curr_token){

    if(tokens.front().find("=") == std::string::npos)
        return {};

    std::map<std::string, std::string> envs;

    std::list<std::string>::const_iterator startptr {tokens.cbegin()};

    char start_char_seq {'\0'};
    while(startptr != tokens.cend()){
        std::string::size_type dlim {(*startptr).find("=")};
        std::string envkey {(*startptr).substr(0, dlim)};
        std::string envvalue {(*startptr).substr(dlim+1)};

        if(dlim != std::string::npos){
            if(envvalue.front() != '\"' && envvalue.front() != '\''){
                envs.insert(std::make_pair(envkey, envvalue));
                curr_token++;
            }
            else if((envvalue.front() == '\"' && envvalue.back() == '\"') || (envvalue.front() == '\'' && envvalue.back() == '\'')){
                envs.insert(std::make_pair(envkey, envvalue));
                curr_token++;
            }
            else{
                start_char_seq = envvalue.front();
                std::ostringstream envstream (std::string(envvalue + " "), std::ios_base::ate);
                startptr = std::next(startptr);
                curr_token++;
                while(startptr != tokens.cend() && (*startptr).back() != start_char_seq){
                    envstream << *startptr << " ";
                    startptr = std::next(startptr);
                    curr_token++;
                }
                if(startptr == tokens.end()){
                    // Second double quotes not in any tokens
                    // Handle error
                    std::terminate();
                }
                if((*startptr).back() == start_char_seq){
                    // Found second token with double quotes
                    envstream << *startptr;
                    curr_token++;
                }
                envs.insert(std::make_pair(envkey, std::move(envstream).str()));
            }
        }
        startptr = std::next(startptr);
    }
    return envs;
}


std::list<command_info> extract_commands(std::list<std::string>& tokens){

    std::list<command_info> cmds_list;
    int ctok{0};
    command_info cinfo;

    std::list<std::string> cmdtokens;
    std::map<std::string, std::string> envs;

    for(const std::string& cmdinput : tokens){
        cmdtokens = tokenize_command(cmdinput);

        envs = extract_env_vars(cmdtokens, ctok);
        cinfo.envs = std::move(envs);

        if(ctok != 0){
            cmdtokens.erase(cmdtokens.begin(), std::next(cmdtokens.begin(), ctok));
        }

        // Check if the input is an assignment, parse it appropriately

        cinfo.execfile = std::exchange(cmdtokens.front(), "");
        cmdtokens.erase(cmdtokens.begin());

        // CRITICAL: EXTRACT ARGUMENTS

        std::move(cmdtokens.begin(), cmdtokens.end(), std::back_inserter(cinfo.cmdargs));
        //expand_cmdline_args(cinfo.cmdargs);
        cmds_list.push_back(cinfo);

        cinfo = {};
        ctok = 0;
    }

    return cmds_list;
}
}


#endif // PARSE_INPUT_HPP
