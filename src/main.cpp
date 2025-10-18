#include "execution/command_execution.hpp"
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>



int main(int argc, char* argv[]){


    Command_Execution cmdexec;

    cmdexec.start_loop();
    return 0;
}
