#include "common.h"
#include "quickSearch.h"
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

int fd[2];
char buf[2] = { '\0' };
int InSearchWin = 0;
int childExitFlag = 0;

/* Byte 0: quick search 快捷键标志位(alt-j) <for newWindow.c>
 * Byte 1: 退出窗口快捷键标志位(ctrl-c) <for newWindow.c, 目前被屏蔽了>
 * Byte 2: 翻译窗口打开标志位
 * Byte 4: 搜索窗口存在标志位
 * */
char *shmaddr_keyboard = NULL;

pid_t searchWindow_pid;
pid_t searchWindowMonitor_pid;
pid_t captureShortcutEvent_pid;

void kill_ourselves() {

    pred("KILL  captureShortcutEvent(), PID %d \n", captureShortcutEvent_pid);
    //kill ( captureShortcutEvent_pid, SIGTERM );

    /* 万一失败就不好了，多来几个*/
    kill ( captureShortcutEvent_pid, SIGKILL );
    kill ( captureShortcutEvent_pid, SIGKILL );
    kill ( captureShortcutEvent_pid, SIGKILL );
    kill ( captureShortcutEvent_pid, SIGKILL );
    kill ( captureShortcutEvent_pid, SIGKILL );
    exit(0);
}

void readChildProcessInfo(int signo) {

    while ( waitpid(searchWindow_pid, NULL, WNOHANG) > 0)
        childExitFlag = 1;
}

void quickSearch()
{
    pid_t pid;

    struct sigaction sa;
    sa.sa_handler = kill_ourselves;
    sigemptyset ( &sa.sa_mask );
    if ( sigaction ( SIGTERM, &sa, NULL) != 0 )
        err_exit_qs("Sigaction error in quickSearch 1");

    socketpair ( AF_UNIX, SOCK_STREAM, 0, fd );


    shared_memory_for_keyboard_event(&shmaddr_keyboard);
    memset(shmaddr_keyboard, '0', 100);

    if ( (pid = fork()) < 0) 
        err_exit_qs("Fork error");

    /* 父进程*/
    if ( pid > 0 ) {

        captureShortcutEvent_pid = pid;
        searchWindowMonitor_pid = getpid();

        sa.sa_handler = readChildProcessInfo;
        sigemptyset ( &sa.sa_mask );
        if ( sigaction ( SIGCHLD, &sa, NULL) != 0 )
            err_exit_qs("Sigaction error in quickSearch 2");

        close ( fd[0] );

        while ( 1 ) {

            if ( shmaddr_keyboard[QuickSearchShortcutPressed_FLAG] == '1') {

                //InSearchWin = 1;
                shmaddr_keyboard[SEARCH_WINDOW_OPENED_FLAG] = '1';
                shmaddr_keyboard[QuickSearchShortcutPressed_FLAG] = '0';


                /* 莫得办法，不每次都fork一个进程，窗口除第一次外都无法聚焦*/
                if ( ( pid = fork() ) == 0) {
                    searchWindow();
                    exit(0);
                } 
                else {

                    searchWindow_pid = pid;

                    printf("等待搜索窗口退出\n");

                    /* 父进程等待子进程退出*/
                    while ( ! childExitFlag )
                        usleep(10000);


                    childExitFlag = 0;
                    shmaddr_keyboard[SEARCH_WINDOW_OPENED_FLAG] = '0';
                    printf("搜索窗口已退出 shmaddr_keyboard[SEARCH_WINDOW_OPENED_FLAG=]%c\n",\
                            shmaddr_keyboard[SEARCH_WINDOW_OPENED_FLAG]);
                }
            }

            usleep(10000);
        }
    } 
    else {

        close ( fd[1] );

        /* 这又是子进程里的，获取到的变量父进程是用不到的!!!!!!!*/
        //captureShortcutEvent_pid = getpid();

        captureShortcutEvent(fd[0]);
    }
}
