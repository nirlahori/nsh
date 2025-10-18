#ifndef JOB_CONTROL_IMPL_HPP
#define JOB_CONTROL_IMPL_HPP


#include <string>
#include <cstdint>
#include <cstdlib>

enum class job_status : std::uint8_t{
    running,
    stopped,
    done
};



struct background_execution_unit{
    std::size_t job_id;
    std::string job_cmd;
    job_status status;
    std::size_t pgid;
};


#endif // JOB_CONTROL_IMPL_HPP
