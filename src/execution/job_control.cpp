#include <list>
#include <algorithm>
#include <string>
#include <string_view>
#include <cstring>

#include "../../include/execution/job_control.hpp"
#include "../../include/builtin.hpp"

#define set_signal(sigobj, signum, action) \
    sigobj.sa_handler = action; \
    sigaction(signum, &sa_intr, nullptr);



Job_Control::Job_Control() :
    bgjob_table(),
    jobunit_id{0},
    fg_joblist(),
    bg_joblist(),
    shell_pid{getpid()},
    shell_pgid{getpgrp()}
    {
        if(!tokenize_path_var(path_dirs)){
            // Cannot proceed forward in case of error in getting directory list in $PATH
            std::exit(EXIT_FAILURE);
        }

        builtin::init_builtin_map();

        // Setup signal handling
        struct sigaction sa_intr, sa_tstp;

        sa_intr.sa_flags = 0;
        if(sigemptyset(&sa_intr.sa_mask) < 0){
            std::puts("Coudn't start shell... unknown error occurred\n");
        }

        set_signal(sa_intr, SIGINT, SIG_IGN);






    }


void Job_Control::set_foreground_pgid(std::size_t pgid){

    if(pgid != shell_pgid){
        tcsetpgrp(STDIN_FILENO, pgid);
    }
    else{
        if(signal(SIGTTOU, SIG_IGN) == SIG_ERR){
            std::perror("Error");
        }
        tcsetpgrp(STDIN_FILENO, pgid);
        if(signal(SIGTTOU, SIG_DFL) == SIG_ERR){
            std::perror("Error");
        }
    }
}

// void Job_Control::handle_sigint(Job_Control *job_context){




// }

// void Job_Control::handle_interrupt(int signum, siginfo_t *info, void *context){

//     switch (signum){
//     case SIGINT:


//         break;
//     default:
//         break;
//     }

// }



bool Job_Control::tokenize_path_var(std::list<std::string>& path_dirs){


    std::string_view delim {":"};

    char* path_env_val {getenv("PATH")};
    if(!path_env_val)
        return false;

    char path_dir_toks[std::strlen(path_env_val) + 1];

    memcpy(path_dir_toks, path_env_val, std::strlen(path_env_val) + 1);

    char* tok {std::strtok(path_dir_toks, delim.data())};
    while(tok){
        path_dirs.push_back(std::string(tok));
        tok = std::strtok(nullptr, delim.data());
    }
    return true;
}

void Job_Control::submit_foreground_jobs(const std::list<job_type>& _fg_jobs){
    fg_joblist = _fg_jobs;
}

void Job_Control::run_foreground_jobs(){

    std::size_t chainlist_size {fg_joblist.size()};
    std::size_t newpgrpid {0};
    std::size_t no_of_pipes {0};

    char filepath[1024];
    bool all_builtins {true};

    for(std::size_t index{0}; index<chainlist_size; ++index){
        std::list<command_info>& chain_key = *std::next(fg_joblist.begin(), index);
        std::size_t chain_key_size = chain_key.size();
        no_of_pipes = chain_key_size - 1;
        int pipefds[no_of_pipes][2];

        for(std::size_t k{0}; k<no_of_pipes; ++k){
            if(pipe(pipefds[k]) == -1){
                std::perror("Error");
                continue;
            }
        }

        all_builtins = true;

        for(std::size_t j{0}; j<chain_key_size; ++j){

            command_info& curr_proc {*std::next(chain_key.begin(), j)};

            if(builtin::is_builtin(curr_proc.execfile)){
                std::list<std::string> arglist (curr_proc.cmdargs.begin(), curr_proc.cmdargs.end());
                builtin::execute(curr_proc.execfile, arglist, bgjob_table);
                continue;
            }
            all_builtins = false;

            int pid = fork();
            if(pid == 0){

                connect_processes(no_of_pipes, pipefds, j, chain_key_size);

                char** procargs = arglist.get_cmdline_opt_args(curr_proc.cmdargs, curr_proc.execfile);
                char** procenvs = arglist.get_cmdline_env_args(curr_proc.envs);


                for(const std::string& path : path_dirs){
                    std::string str (path + ((path.ends_with("/")) ? curr_proc.execfile : "/" + curr_proc.execfile));
                    std::strncpy(filepath, str.c_str(), str.size() + 1);
                    execve(filepath, procargs, procenvs);
                    std::memset(filepath, '\0', std::strlen(filepath));
                }
                std::perror("Error");
                std::exit(EXIT_FAILURE);

            }
            else{
                if(j == 0){
                    newpgrpid = pid;
                    if(newpgrpid < 0){
                        std::perror("Error");
                        std::exit(EXIT_FAILURE);
                    }
                }
                if(setpgid(pid, newpgrpid) < 0){
                    std::perror("Error");
                    std::exit(EXIT_FAILURE);
                }
            }
        }

        set_foreground_pgid(newpgrpid);

        for(int i{0}; i<no_of_pipes; ++i){
            close(pipefds[i][readindex]);
            close(pipefds[i][writeindex]);
        }

        siginfo_t proc_exit_status_info;
        proc_exit_status_info.si_pid = 0;

        if(!all_builtins){
            for(int m{0}; m<chain_key_size; ++m){
                if(waitid(P_PGID, newpgrpid, &proc_exit_status_info, WEXITED | WSTOPPED) == -1){
                    std::perror("Error");
                }
            }
        }
        set_foreground_pgid(shell_pgid);
    }
}

void Job_Control::submit_background_jobs(const std::list<job_type>& _bg_jobs){
    bg_joblist = _bg_jobs;
}


std::string Job_Control::get_jobunit_desc(const job_type& job){

    std::size_t job_size = job.size();
    std::size_t index = 0;

    std::string jobunit_desc;
    for(const command_info& info : job){

        std::string proc_args;
        for(const std::string& args : info.cmdargs)
            proc_args += args;
        if(index == job_size - 1){
            jobunit_desc += (info.execfile + " " + proc_args + " ");
        }
        else{
            jobunit_desc += (info.execfile + " " + proc_args + " | ");
        }
        index++;
    }
    return jobunit_desc;
}

void Job_Control::run_background_jobs(){

    //std::size_t chainlist_size {bg_joblist.size()};
    std::size_t newpgrpid {0};
    std::size_t no_of_pipes {0};

    char filepath[1024];

    for(const job_type& job : bg_joblist){

        jobunit_id++;
        no_of_pipes = job.size() - 1;

        int pipefds[no_of_pipes][2];

        for(std::size_t k{0}; k<no_of_pipes; ++k){
            if(pipe(pipefds[k]) == -1){
                std::perror("Error");
                continue;
            }
        }

        std::size_t proc_index {0};
        std::size_t total_procs {job.size()};
        for(const command_info& curr_proc : job){

            int pid = fork();
            if(pid == 0){
                connect_processes(no_of_pipes, pipefds, proc_index, total_procs);

                char** procargs = arglist.get_cmdline_opt_args(curr_proc.cmdargs, curr_proc.execfile);
                char** procenvs = arglist.get_cmdline_env_args(curr_proc.envs);

                for(const std::string& path : path_dirs){
                    std::string str (path + ((path.ends_with("/")) ? curr_proc.execfile : "/" + curr_proc.execfile));
                    std::strncpy(filepath, str.c_str(), str.size() + 1);
                    execve(filepath, procargs, procenvs);
                    std::memset(filepath, '\0', std::strlen(filepath));
                }
                std::perror("Error");
                std::exit(EXIT_FAILURE);
            }
            else{

                if(proc_index == 0){
                    newpgrpid = pid;
                    if(newpgrpid < 0){
                        std::perror("Error");
                    }
                }
                if(setpgid(pid, newpgrpid) < 0){
                    std::perror("Error");
                }
                // Print the status of the job
            }
            proc_index++;
        }


        background_execution_unit unit {jobunit_id, get_jobunit_desc(job), job_status::running, newpgrpid};
        bgjob_table.insert({unit.job_id, std::move(unit)});

        for(int i{0}; i<no_of_pipes; ++i){
            close(pipefds[i][readindex]);
            close(pipefds[i][writeindex]);
        }

        siginfo_t waitinfo;
        waitinfo.si_pid = 0;
        for(int m{0}; m<total_procs; ++m){
            int status = waitid(P_PGID, newpgrpid, &waitinfo, WNOHANG | WEXITED | WSTOPPED);
            if(status < 0){
                // Handle error properly
            }
            else if(status == 0){
                if(waitinfo.si_pid == 0){
                    break;
                }
            }
            else{
                std::perror("unknown error occurred\n");
                break;
            }
        }
    }
}

void Job_Control::connect_processes(int no_of_pipes, int pipefds[][2], const int& proc_index, const int& total_proc){

    if(no_of_pipes > 0){

        if(proc_index == 0){
            dup2(pipefds[proc_index][writeindex], STDOUT_FILENO);
            close(pipefds[proc_index][readindex]);
            close(pipefds[proc_index][writeindex]);

            for(int pfd = 1; pfd < no_of_pipes; ++pfd){
                close(pipefds[pfd][readindex]);
                close(pipefds[pfd][writeindex]);
            }
        }
        else if(proc_index == (total_proc - 1)){
            dup2(pipefds[proc_index-1][readindex], STDIN_FILENO);
            close(pipefds[proc_index-1][writeindex]);
            close(pipefds[proc_index-1][readindex]);

            int read_pipefd_index = proc_index - 2;
            if(read_pipefd_index >= 0){
                for(int pfd = read_pipefd_index; pfd >= 0; --pfd){
                    close(pipefds[pfd][writeindex]);
                    close(pipefds[pfd][readindex]);
                }
            }
        }
        else{
            dup2(pipefds[proc_index-1][readindex], STDIN_FILENO);
            dup2(pipefds[proc_index][writeindex], STDOUT_FILENO);

            close(pipefds[proc_index-1][writeindex]);
            close(pipefds[proc_index-1][readindex]);
            close(pipefds[proc_index][readindex]);
            close(pipefds[proc_index][writeindex]);


            int write_pipefd_index = proc_index + 1;
            int read_pipefd_index = proc_index - 2;

            if(read_pipefd_index >= 0){
                for(int pfd = read_pipefd_index; pfd >= 0; --pfd){
                    close(pipefds[pfd][readindex]);
                    close(pipefds[pfd][writeindex]);
                }
            }

            for(int pfd = write_pipefd_index; pfd < no_of_pipes; ++pfd){
                close(pipefds[pfd][readindex]);
                close(pipefds[pfd][writeindex]);
            }
        }

    }
}

void Job_Control::wait_for_background_jobs(){

    std::vector<std::size_t> to_be_removed;


    siginfo_t waitinfo;
    int stopped_proc{0}, term_proc{0};
    int status {0};

    for(auto& [jobid, unit] : bgjob_table){

        int no_of_procs = std::count(unit.job_cmd.cbegin(), unit.job_cmd.cend(), '|') + 1;

        stopped_proc = 0;
        term_proc = 0;

        waitinfo.si_pid = 0;
        while((status = waitid(P_PGID, unit.pgid, &waitinfo, WNOHANG | WEXITED)), status == 0){
            if(waitinfo.si_pid != 0){
                term_proc++;
                waitinfo.si_pid = 0;
            }
            else{
                break;
            }
        }

        waitinfo.si_pid = 0;
        while((status = waitid(P_PGID, unit.pgid, &waitinfo, WNOHANG | WSTOPPED)), status == 0){
            if(waitinfo.si_pid != 0){
                stopped_proc++;
                waitinfo.si_pid = 0;
            }
            else{
                break;
            }
        }

        if(term_proc == no_of_procs){
            to_be_removed.push_back(jobid);
            unit.status = job_status::done;
        }
        else if(stopped_proc == no_of_procs){
            unit.status = job_status::stopped;
        }
        else if(term_proc + stopped_proc == no_of_procs){
            unit.status = job_status::stopped;
        }
    }

    for(const auto& id : to_be_removed){
        bgjob_table.erase(id);
    }

    if(bgjob_table.empty()){
        jobunit_id = 0;
    }
    else{
        jobunit_id = jobunit_id - to_be_removed.size();
    }
}
