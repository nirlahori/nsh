#ifndef C_ARRAY_HPP
#define C_ARRAY_HPP

#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <map>

class c_array{

    constexpr static std::size_t max_args = 1024;
    constexpr static std::size_t max_arg_length = 1024;

    constexpr static std::size_t max_env = 1024;
    constexpr static std::size_t max_env_length = 1024;


    char* c_strs_options[max_args];
    char* options_cleanup[max_args];

    char* c_strs_envs[max_env];
    char* envs_cleanup[max_env];

    std::size_t null_opt_index {0};
    char* nullindex_optptr {nullptr};

    std::size_t null_env_index {0};
    char* nullindex_envptr {nullptr};


public:
    c_array(){
        for(std::size_t i{0}; i<max_args; ++i){
            c_strs_options[i] = static_cast<char*>(std::malloc(max_arg_length * sizeof(char)));
            options_cleanup[i] = c_strs_options[i];
        }

        for(std::size_t i{0}; i<max_env; ++i){
            c_strs_envs[i] = static_cast<char*>(std::malloc(max_env_length * sizeof(char)));
            envs_cleanup[i] = c_strs_envs[i];
        }

    }


    bool get_cmdline_opt_args(std::vector<std::string> cmdargs, /*const*/ std::string& filename, std::vector<char*>& argsptrs) noexcept{

        unsigned int index {0};
        argsptrs[index++] = filename.data();
        for(auto& str : cmdargs){
            argsptrs[index++] = str.data();
        }
        argsptrs[index] = nullptr;
        return true;
    }


/*   char** get_cmdline_opt_args(const std::vector<std::string>& cmdargs, const std::string& filename) noexcept{

        int index{0};
        std::strncpy(c_strs_options[index++], filename.c_str(), filename.length() + 1);

        if(cmdargs.empty()){
            nullindex_optptr = c_strs_options[index];
            c_strs_options[index] = nullptr;
            null_opt_index = index;
            return c_strs_options;
        }


        //c_strs_options[index++];
        for(const auto& str : cmdargs){
            std::strncpy(c_strs_options[index++], str.c_str(), str.length() + 1);
        }
        nullindex_optptr = c_strs_options[index];
        c_strs_options[index] = nullptr;
        null_opt_index = index;
        return c_strs_options;
    }*/

    /*char** get_cmdline_env_args(const std::map<std::string, std::string>& envmap){

        int index{0};
        for(const auto& [name, value] : envmap){
            std::string str (name + "=" + value);
            std::strncpy(c_strs_envs[index++], str.c_str(), str.length() + 1);
        }

        nullindex_envptr = c_strs_envs[index];
        c_strs_envs[index] = nullptr;
        null_env_index = index;
        return c_strs_envs;
    }*/


    bool get_cmdline_env_args(std::map<std::string, std::string> envmap, std::vector<std::string>&  envargs, std::vector<char*>& envptrs){

        int index{0};
        for(const auto& [name, value] : envmap){
            std::string str (name + "=" + value);
            envargs[index] = std::move(str);
            envptrs[index] = envargs[index].data();
            index++;
        }
        envptrs[index] = nullptr;
        return true;
    }


    // char** get_cmdline_opt_args(){
    //     return c_strs_options;
    // }

    char** get_cmdline_env_args(){
        return c_strs_envs;
    }

    void reset_options(){
        c_strs_options[null_opt_index] = nullindex_optptr;
    }

    void reset_envs(){
        c_strs_envs[null_env_index] = nullindex_envptr;
    }


    ~c_array(){
        for(std::size_t i{0}; i<max_args; ++i){
            std::free(c_strs_options[i]);
        }

        for(std::size_t i{0}; i<max_env; ++i){
            std::free(c_strs_envs[i]);
        }


    }
};










#endif // C_ARRAY_HPP
