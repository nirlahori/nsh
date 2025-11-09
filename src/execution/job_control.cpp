#include <list>
#include <algorithm>
#include <string>
#include <string_view>
#include <cstring>

#include "execution/job_control.hpp"
#include "builtin.hpp"


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
    }

bool Job_Control::get_cmdline_opt_args(std::vector<std::string> cmdargs, /*const*/ std::string& filename, std::vector<char*>& argsptrs) noexcept{

    unsigned int index {0};
    argsptrs[index++] = filename.data();
    for(auto& str : cmdargs){
        argsptrs[index++] = str.data();
    }
    argsptrs[index] = nullptr;
    return true;
}

bool Job_Control::get_cmdline_env_args(std::map<std::string, std::string> envmap, std::vector<std::string>&  envargs, std::vector<char*>& envptrs){

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

void Job_Control::set_foreground_pgid(int pgid){

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



void Job_Control::execute_bg_job(job_type job){

    static std::vector<char*> argsptrs;
    static std::vector<std::string> envstrs;
    static std::vector<char*> envptrs;

    int newpgrpid {0};
    std::size_t no_of_pipes {0};
    char filepath[1024];

    jobunit_id++;
    no_of_pipes = job.size() - 1;

    static std::vector<std::vector<int>> pipevec(no_of_pipes);
    if(no_of_pipes > pipevec.size()){
        pipevec.resize(no_of_pipes, std::vector<int>(2));
    }


    for(auto& pipefds : pipevec){
        if(pipe(pipefds.data())){
            std::perror("Error");
            continue;
        }
    }

    std::size_t proc_index {0};
    std::size_t total_procs {job.size()};

    for(command_info& curr_proc : job){

        int pid = fork();
        if(pid == 0){
            connect_processes(no_of_pipes, pipevec, proc_index, total_procs);

            argsptrs.reserve(curr_proc.cmdargs.size() + 2);
            if(!get_cmdline_opt_args(std::move(curr_proc.cmdargs), curr_proc.execfile, argsptrs)){
                // Terminate the process after cleaning up
                std::terminate();
            }
            envstrs.reserve(curr_proc.envs.size());
            envptrs.reserve(curr_proc.envs.size() + 1);
            if(!get_cmdline_env_args(std::move(curr_proc.envs), envstrs, envptrs)){
                // Terminate the process after cleaning up
                std::terminate();
            }

            for(const std::string& path : path_dirs){
                std::string str (path + ((path.ends_with("/")) ? curr_proc.execfile : "/" + curr_proc.execfile));
                std::strncpy(filepath, str.c_str(), str.size() + 1);
                execve(filepath, argsptrs.data(), envptrs.data());
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

    for(std::size_t i{0}; i<no_of_pipes; ++i){
        /*close(pipefds[i][readindex]);
        close(pipefds[i][writeindex]);*/
        close(pipevec[i][readindex]);
        close(pipevec[i][writeindex]);
    }

    siginfo_t waitinfo;
    waitinfo.si_pid = 0;
    for(std::size_t m{0}; m<total_procs; ++m){
        int status = waitid(P_PGID, newpgrpid, &waitinfo, WNOHANG | WEXITED | WSTOPPED);
        if(status == 0){
            if(waitinfo.si_pid == 0){
                break;
            }
        }
        else{
            std::perror("Unknown error occurred\n");
            break;
        }
    }
}


bool Job_Control::tokenize_path_var(std::list<std::string>& path_dirs){


    std::string_view delim {":"};

    char* path_env_val {getenv("PATH")};
    if(!path_env_val)
        return false;

    std::string path_dir_toks;
    path_dir_toks.resize(std::strlen(path_env_val) + 1);

    std::copy(path_env_val, path_env_val + std::strlen(path_env_val) + 1, path_dir_toks.begin());

    char* tok {std::strtok(path_dir_toks.data(), delim.data())};
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

    static std::vector<char*> argsptrs;
    static std::vector<std::string> envstrs;
    static std::vector<char*> envptrs;

    std::size_t chainlist_size {fg_joblist.size()};
    int newpgrpid {0};
    std::size_t no_of_pipes {0};

    static std::vector<std::vector<int>> pipevec;

    char filepath[1024];
    bool all_builtins {true};

    Builtin_Table& builtin_table {Builtin_Table::get_instance()};

    for(std::size_t index{0}; index<chainlist_size; ++index){
        std::list<command_info>& chain_key = *std::next(fg_joblist.begin(), index);
        std::size_t chain_key_size = chain_key.size();
        no_of_pipes = chain_key_size - 1;

        if(no_of_pipes > pipevec.size()){
            pipevec.resize(no_of_pipes, std::vector<int>(2));
        }
        for(auto& pipefds : pipevec){
            if(pipe(pipefds.data())){
                std::perror("Error");
                continue;
            }
        }

        all_builtins = true;

        for(std::size_t j{0}; j<chain_key_size; ++j){

            command_info& curr_proc {*std::next(chain_key.begin(), j)};

            if(builtin_table.is_builtin(curr_proc.execfile)){
                std::list<std::string> arglist (curr_proc.cmdargs.begin(), curr_proc.cmdargs.end());
                builtin_table.execute(curr_proc.execfile, arglist, bgjob_table);
                continue;
            }
            all_builtins = false;

            int pid = fork();
            if(pid == 0){

                //connect_processes(no_of_pipes, pipefds, j, chain_key_size);
                connect_processes(no_of_pipes, pipevec, j, chain_key_size);

                argsptrs.reserve(curr_proc.cmdargs.size() + 2);
                if(!get_cmdline_opt_args(std::move(curr_proc.cmdargs), curr_proc.execfile, argsptrs)){
                    break;
                }

                envstrs.reserve(curr_proc.envs.size());
                envptrs.reserve(curr_proc.envs.size() + 1);
                if(!get_cmdline_env_args(std::move(curr_proc.envs), envstrs, envptrs)){
                    break;
                }

                for(const std::string& path : path_dirs){
                    std::string str (path + ((path.ends_with("/")) ? curr_proc.execfile : "/" + curr_proc.execfile));
                    std::strncpy(filepath, str.c_str(), str.size() + 1);
                    //execve(filepath, procargs, procenvs);
                    execve(filepath, argsptrs.data(), envptrs.data());
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

        for(std::size_t i{0}; i<no_of_pipes; ++i){
            /*close(pipefds[i][readindex]);
            close(pipefds[i][writeindex]);*/
            close(pipevec[i][readindex]);
            close(pipevec[i][writeindex]);
        }

        siginfo_t proc_exit_status_info;
        proc_exit_status_info.si_pid = 0;

        if(!all_builtins){
            for(std::size_t m{0}; m<chain_key_size; ++m){
                if(waitid(P_PGID, newpgrpid, &proc_exit_status_info, WEXITED | WSTOPPED) == -1){
                    std::perror("Error");
                }
            }
        }
        set_foreground_pgid(shell_pgid);
    }
}

void Job_Control::submit_background_jobs(std::list<job_type> bg_jobs){
    bg_joblist = std::move(bg_jobs);
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


    auto first {std::make_move_iterator(bg_joblist.begin())};
    auto last {std::make_move_iterator(bg_joblist.end())};

    for(; first != last; first = std::next(first)){
        execute_bg_job(*first);
    }
}

void Job_Control::connect_processes(int no_of_pipes, const std::vector<std::vector<int>>& pipefds, const int& proc_index, const int& total_proc){

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

bool Job_Control::kill_foreground_job(){


    // Get foreground process group id
    int fgpgid {tcgetpgrp(STDIN_FILENO)};
    if(fgpgid < 0 || fgpgid == getpgrp()){
        return false;
    }

    if(killpg(fgpgid, SIGINT) < 0){
        return false;
    }

    return true;
}
