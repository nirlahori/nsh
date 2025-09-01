#ifndef TOKENS_HPP
#define TOKENS_HPP

#include <string>
#include <cstring>
#include <list>

bool tokenize_input(const std::string& input, std::list<std::string>& tokens){

    std::string delims {";\n"};
    std::string buf{input.begin(), input.end()};

    char* tok {std::strtok(buf.data(), delims.c_str())};
    while(tok){
        tokens.push_back(std::string(tok));
        tok = std::strtok(nullptr, delims.c_str());
    }



    return true;
}




#endif // TOKENS_HPP
