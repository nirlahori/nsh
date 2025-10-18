#ifndef SYSTEM_ENVS_HPP
#define SYSTEM_ENVS_HPP


#include <map>
#include <string>
#include <string_view>

#include <unistd.h>

extern char** environ;

namespace environment{

using name_t = std::string;
using value_t = std::string;

std::map<name_t, value_t> envmap;



bool init_env(){

    char** env = environ;
    while(env && *env){
        std::string_view val (*env);
        std::string_view::size_type pos = val.find("=");
        envmap.insert(std::pair{val.substr(0, pos), val.substr(pos + 1)});
        env++;
    }
    return true;
}


bool register_new_env(std::string_view name, std::string_view value){

    if(setenv(name.data(), value.data(), 1) < 0){
        return false;
    }
    envmap.insert(std::pair{name.data(), value.data()});
    return true;
}






}







#endif // SYSTEM_ENVS_HPP
