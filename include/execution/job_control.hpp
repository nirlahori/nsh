#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H


#include <string>
#include <map>
#include <list>

#include <unistd.h>
#include <sys/wait.h>

#include "../command_struct.hpp"
#include "../c_array.hpp"
#include "internal/job_control_impl.hpp"


class Job_Control
{
    std::map<std::size_t, background_execution_unit> bgjob_table;
    std::size_t jobunit_id;

    using job_type = std::list<command_info>;

    std::list<job_type> fg_joblist;
    std::list<job_type> bg_joblist;

    static constexpr int readindex = 0;
    static constexpr int writeindex = 1;
    int shell_pid;
    int shell_pgid;


    bool single_proc_flag {false};

    c_array arglist;



private:

    void set_foreground_pgid(std::size_t pgid);


public:
    void submit_foreground_jobs(const std::list<job_type>& _fg_jobs);
    void run_foreground_jobs();

    void submit_background_jobs(const std::list<job_type>& _bg_jobs);
    void run_background_jobs();

    std::string get_jobunit_desc(const job_type& job);
    void connect_processes(int _no_of_pipes, int pipefd[][2], const int& proc_index, const int& total_proc);

    void wait_for_background_jobs();



public:
    Job_Control();
};

#endif // JOB_CONTROL_H
