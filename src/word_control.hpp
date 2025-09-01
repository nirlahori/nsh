#ifndef WORD_CONTROL_HPP
#define WORD_CONTROL_HPP

#include <vector>
#include <string>
#include <map>


namespace wexpand{


constexpr char single_quote {'\''};
constexpr char double_quote {'\"'};
constexpr char chdollar {'$'};


bool expand_word(std::string& word){

    if(word == "$" || word.length() == 1){
        return true;
    }

    std::string str{};

    if(word.front() == chdollar){
        if(word.at(1) != single_quote && word.at(1) != double_quote){
            str = word.substr(1);
            char* ch {std::getenv(str.c_str())};
            word = (ch) ? ch : str;
        }
        else{
            word = word.substr(2, word.length()-3).c_str();
        }
    }
    else if(word.front() == single_quote){
        word = word.substr(1, word.length()-2).c_str();
    }
    else if(word.front() == double_quote){
        if(word[1] == chdollar){
            str = word.substr(2, word.length()-3);
            char* ch {std::getenv(str.c_str())};
            word = (ch) ? ch : str;
        }
        else{
            word = word.substr(1, word.length()-2).c_str();
        }
    }
    return true;
}


void expand_cmdline_args(std::vector<std::string>& cmdargs){
    for(auto& args : cmdargs){
        expand_word(args);
    }
}


void expand_cmdline_envs(std::map<std::string, std::string>& envs){
    for(auto& [name, value] : envs){
        expand_word(value);
    }
}

}








#endif // WORD_CONTROL_HPP
