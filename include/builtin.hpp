#ifndef BUILTIN_HPP
#define BUILTIN_HPP

#include <map>
#include <string>
#include <cstdarg>
#include <cstdlib>
#include <memory>
#include <list>
#include <vector>
#include <csignal>
#include <algorithm>

#include <unistd.h>
#include <sys/wait.h>

#include "execution/internal/job_control_impl.hpp"


namespace builtin {


struct builtin_base{

public:
    builtin_base() = default;
    virtual void parse(std::list<std::string>&) = 0;
    virtual void invoke(std::list<std::string>&, std::map<std::size_t, background_execution_unit>&) = 0;

    virtual ~builtin_base(){}

};

struct builtin_exit : public builtin_base{

    builtin_exit() : builtin_base() {}
    void parse(std::list<std::string>& tokens){}
    void invoke(std::list<std::string>& arglist, std::map<std::size_t, background_execution_unit>& bgjob_table){
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
    void parse(std::list<std::string>& tokens){}
    void invoke(std::list<std::string>& arglist, std::map<std::size_t, background_execution_unit>& bgjob_table){
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


struct builtin_kill : public builtin_base{


    builtin_kill() : builtin_base() {}

    using signal_t = unsigned int;
    using process_ids_t = std::vector<unsigned int>;
    std::pair<signal_t, process_ids_t> kill_ctx;

    using job_id_t = std::size_t;
    std::vector<job_id_t> jobids;

    constexpr static char help_text[] {
        "kill: usage: kill [signum] pid | jobspec ...\n"
    };


    bool expect_jobid(const std::string& token){

        std::size_t jobid_tokill {0};
        if(token.starts_with("%")){
            try{
                jobids.push_back(std::stoi(token.substr(1)));
            }
            catch(...){
                return false;
            }
        }
        return true;
    }

    bool expect_signal(const std::string& signal){
        try{
            kill_ctx.first = std::stoi(signal);
            if(kill_ctx.first < 0 && kill_ctx.first > SIGRTMAX)
                return false;
        }
        catch(...){
            return false;
        }
        return true;
    }

    bool extract_pids(const std::list<std::string>& tokens){
        process_ids_t pids;
        for(const auto& token : tokens){
            try{
                kill_ctx.second.push_back(std::stoi(token));
            }
            catch(...){
                return false;
            }
        }
        return true;
    }


    void parse(std::list<std::string>& tokens){

        if(tokens.size() == 1){
            std::fprintf(stdout, help_text);
        }
        else{

            if(tokens.size() <= 1){
                std::printf("Error: Invalid Syntax\n");
            }

            else{

                if(!expect_signal(tokens.front())){
                    kill_ctx.first = SIGTERM;
                }
                else{
                    tokens.erase(tokens.begin());
                }
                int index = 0;
                for(const std::string& token : tokens){
                    if(!expect_jobid(token)){
                        break;
                    }
                    index++;
                }
                tokens.erase(tokens.begin(), std::next(tokens.begin(), index));
                if(!extract_pids({std::next(tokens.begin()), tokens.end()})){
                    std::printf("Error: Incorrect process ids\n");
                }
            }
        }
    }
    void invoke(std::list<std::string>& arglist, std::map<std::size_t, background_execution_unit>& bgjob_table){
        parse(arglist);
        // Handle parsing error here


        for(const job_id_t id : jobids){
            if(killpg(bgjob_table[id].pgid, kill_ctx.first) < 0){
                continue;
            }
            if(kill_ctx.first == SIGSTOP || kill_ctx.first == SIGTSTP){
                bgjob_table[id].status = job_status::stopped;
            }
            else if(kill_ctx.first == SIGCONT){
                bgjob_table[id].status = job_status::running;
            }
            else if(kill_ctx.first == SIGTERM || kill_ctx.first == SIGKILL){
                bgjob_table[id].status = job_status::done;
            }
        }
        for(const unsigned int pid : kill_ctx.second){
            kill(pid, kill_ctx.first); // Check for return value
        }

        jobids.clear();
    }
};


struct builtin_jobs : public builtin_base{
    builtin_jobs() : builtin_base() {}
    void parse(std::list<std::string>& tokens){}
    void invoke(std::list<std::string>&, std::map<std::size_t, background_execution_unit>& bgjob_table){

        for(const auto& [jobid, execunit] : bgjob_table){
            std::printf("[%zu] ", execunit.job_id);
            std::printf((execunit.status == job_status::running ? "Running " :
                        execunit.status == job_status::stopped ? "Stopped " : "Done"));
            std::printf("\t\t\t");
            std::printf("%s\n", execunit.job_cmd.c_str());
        }
    }
    ~builtin_jobs(){}
};



struct builtin_fg : public builtin_base{

    builtin_fg() : builtin_base() {}

    int set_fg_job(std::size_t pgrp){
        return tcsetpgrp(STDIN_FILENO, pgrp);
    }

    void parse(std::list<std::string>&){}
    void invoke(std::list<std::string>& arglist, std::map<std::size_t, background_execution_unit>& bgjob_table){

        if(arglist.empty()){
            auto iter = std::prev(bgjob_table.end());

            // Hand over the terminal device to the foreground job
            if(set_fg_job(iter->second.pgid) < 0){
                std::printf("Error executing fg\n");
            }
            else{
                killpg(iter->second.pgid, SIGCONT);

                // Wait for foreground process group to exit
                int no_of_procs = std::count(iter->second.job_cmd.cbegin(), iter->second.job_cmd.cend(), '|') + 1;

                for(int i = 0; i < no_of_procs; i++){
                    if(waitid(P_PGID, iter->second.pgid, nullptr, WEXITED) == -1){
                        std::perror("Error");
                    }
                }

                // Hand over the terminal device to the shell
                if(signal(SIGTTOU, SIG_IGN) == SIG_ERR){
                    std::perror("Error");
                }
                tcsetpgrp(STDIN_FILENO, getpgrp());
                if(signal(SIGTTOU, SIG_DFL) == SIG_ERR){
                    std::perror("Error");
                }

                // Remove job from background job table
                bgjob_table.erase(iter);
            }
        }
        else{
            if(arglist.front().starts_with("%")){
                try{
                    std::size_t jobid = std::stoi(arglist.front().substr(1));
                    auto iter = bgjob_table.find(jobid);
                    if(iter != bgjob_table.end()){
                        if(set_fg_job(bgjob_table[jobid].pgid) < 0){
                            std::printf("Error executing fg\n");
                        }
                        else{
                            killpg(bgjob_table[jobid].pgid, SIGCONT);
                            // Wait for foreground process group to exit
                            int no_of_procs = std::count(iter->second.job_cmd.cbegin(), iter->second.job_cmd.cend(), '|') + 1;

                            for(int i = 0; i < no_of_procs; i++){
                                if(waitid(P_PGID, iter->second.pgid, nullptr, WEXITED) == -1){
                                    std::perror("Error");
                                }
                            }

                            // Hand over the terminal device to the shell
                            if(signal(SIGTTOU, SIG_IGN) == SIG_ERR){
                                std::perror("Error");
                            }
                            tcsetpgrp(STDIN_FILENO, getpgrp());
                            if(signal(SIGTTOU, SIG_DFL) == SIG_ERR){
                                std::perror("Error");
                            }

                            // Remove job from background job table
                            bgjob_table.erase(iter);
                        }
                    }
                    else{
                        std::printf("Error executing fg: No such job\n");
                    }
                }
                catch(...){
                    std::printf("Error executing fg\n");
                }
            }
            else{
                std::printf("Error executing fg: No such job\n");
            }
        }


    }

    ~builtin_fg(){}

};


struct builtin_bg : public builtin_base{
    builtin_bg() : builtin_base() {}

    void parse(std::list<std::string>&){}

    void invoke(std::list<std::string>& arglist, std::map<std::size_t, background_execution_unit>& bgjob_table){
        if(arglist.empty()){
            auto iter = bgjob_table.find(bgjob_table.size());
            killpg(iter->second.pgid, SIGCONT);
            bgjob_table[iter->second.job_id].status = job_status::running;
        }
        else{
            if(arglist.front().starts_with("%")){
                try{
                    std::size_t jobid = std::stoi(arglist.front().substr(1));
                    if(bgjob_table.find(jobid) != bgjob_table.end()){
                        killpg(bgjob_table[jobid].pgid, SIGCONT);
                        bgjob_table[jobid].status = job_status::running;
                    }
                    else{
                        std::printf("Error executing bg\n");
                    }
                }
                catch(...){
                    std::printf("Error executing bg\n");
                }
            }
            else{
                std::printf("Error executing bg\n");
            }
        }
    }
};





using builtin_name_type = std::string;


std::map<builtin_name_type, std::unique_ptr<builtin_base>> builtin_map{};


void init_builtin_map(){
    builtin_map.insert({"exit", std::make_unique<builtin_exit>()});
    builtin_map.insert({"cd", std::make_unique<builtin_cd>()});
    builtin_map.insert({"kill", std::make_unique<builtin_kill>()});
    builtin_map.insert({"jobs", std::make_unique<builtin_jobs>()});
    builtin_map.insert({"fg", std::make_unique<builtin_fg>()});
    builtin_map.insert({"bg", std::make_unique<builtin_bg>()});
}


bool is_builtin(const std::string& cmd){
    return builtin_map.find(cmd) != builtin_map.end();
}

void execute(const std::string& cmd, std::list<std::string>& arglist, std::map<std::size_t, background_execution_unit>& bgjob_table){
    if(is_builtin(cmd)){
        builtin_map[cmd]->invoke(arglist, bgjob_table);
    }
}
}

#endif // BUILTIN_HPP
