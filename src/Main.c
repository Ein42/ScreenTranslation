#include "common.h"


int main(int argc, char **argv)
{

    pid_t pid;

    if ( (pid = fork()) < 0)
        err_exit("fork err in main");

    void (*quickSearchProcess)(void);
    quickSearchProcess = quickSearch;

    void (*tranSelectProcess)(void);
    tranSelectProcess = tranSelect;

    /* 启动取词翻译进程和quickSearch进程*/
    if ( pid > 0 )
        tranSelectProcess();
    else
        quickSearchProcess();
}
