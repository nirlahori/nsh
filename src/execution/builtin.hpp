#ifndef BUILTIN_HPP
#define BUILTIN_HPP

#include <map>
#include <string>
#include <cstdarg>
#include <cstdlib>
#include <memory>
#include <list>
#include <filesystem>


#include <unistd.h>


namespace builtin {


struct builtin_base{

public:
    builtin_base() = default;
    virtual void parse(const std::list<std::string>&) = 0;
    virtual void invoke(const std::list<std::string>&, const std::map<std::string, std::string>&) = 0;
};

struct builtin_exit : public builtin_base{

    builtin_exit() : builtin_base() {}
    void parse(const std::list<std::string>& tokens){}
    void invoke(const std::list<std::string>& arglist, const std::map<std::string, std::string>& envlist){
        if(arglist.empty())
            std::exit(0); // Need to exit with status of last executed command
        std::exit(std::stoi(arglist.front()));
    }
};

struct builtin_cd : public builtin_base{

private:

    std::string_view home_char {"~"};

public:
    builtin_cd() : builtin_base() {}
    void parse(const std::list<std::string>& tokens){}
    void invoke(const std::list<std::string>& arglist, const std::map<std::string, std::string>& envlist){
        if(arglist.empty() || arglist.front().c_str() == home_char){
            char* cwd {getenv("HOME")};
            if(chdir((cwd) ? cwd : "/") == -1){
                std::perror("Error");
            }
        }
        else{
            if(chdir(arglist.front().c_str()) == -1){
                std::perror("Error");
            }
        }
    }
};





using builtin_name_type = std::string;


std::map<builtin_name_type, std::unique_ptr<builtin_base>> builtin_map{};


void init_builtin_map(){
    builtin_map.insert({"exit", std::make_unique<builtin_exit>()});
    builtin_map.insert({"cd", std::make_unique<builtin_cd>()});
}


bool is_builtin(const std::string& cmd){
    return builtin_map.find(cmd) != builtin_map.end();
}

void execute(const std::string& cmd, std::list<std::string>& arglist, std::map<std::string, std::string>& envlist){
    if(is_builtin(cmd)){
        builtin_map[cmd]->invoke(std::move(arglist), std::move(envlist));
    }
}
}

#endif // BUILTIN_HPP
