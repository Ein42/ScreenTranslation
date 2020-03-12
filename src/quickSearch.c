#include "common.h"
#include "quickSearch.h"
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include "shortcutListener.h"
#include "sharedMemory.h"

/* int fd[2]; */
char buf[2] = { '\0' };
int InSearchWin = 0;

/* Byte 0: quick search 快捷键标志位(alt-j) <for newWindow.c>
 * Byte 1: 退出窗口快捷键标志位(ctrl-c) <for newWindow.c, 目前被屏蔽了>
 * Byte 2: 翻译窗口打开标志位
 * Byte 4: 搜索窗口存在标志位
 * */
char *shmaddr_keyboard = NULL;

pid_t searchWindow_pid;
pid_t searchWindowMonitor_pid;
pid_t captureShortcutEvent_pid;

static int SIGTERM_SIGNAL = 0;

static void readChild() {

    while( waitpid(captureShortcutEvent_pid, NULL, WNOHANG) > 0);
}

void kill_ourselves() {

    pbgreen ("Kill listenShortcut");

    kill ( captureShortcutEvent_pid, SIGTERM );
    usleep(100000);

    SIGTERM_SIGNAL = 1;
}

void quickSearch()
{

    pbyellow ( "启动 quick search" );

    pid_t pid;

    struct sigaction sa;
    sigemptyset ( &sa.sa_mask );
    sa.sa_handler = kill_ourselves;
    if ( sigaction ( SIGTERM, &sa, NULL) != 0 )
        err_exit_qs("Sigaction error in quickSearch 1");
    if ( sigaction ( SIGINT, &sa, NULL) != 0 )
        err_exit_qs("Sigaction error in quickSearch 1");
    sa.sa_handler = readChild;
    if ( sigaction ( SIGCHLD, &sa, NULL) != 0 )
        err_exit_qs("Sigaction error in quickSearch 1");

    /* socketpair ( AF_UNIX, SOCK_STREAM, 0, fd ); */


    shared_memory_for_keyboard_event(&shmaddr_keyboard);
    memset(shmaddr_keyboard, '0', 100);

    if ( (pid = fork()) < 0) 
        err_exit_qs("Fork error");

    /* 父进程*/
    if ( pid > 0 ) {

        captureShortcutEvent_pid = pid;
        searchWindowMonitor_pid = getpid();

        /* close ( fd[0] ); */

        while ( 1 ) {

            if ( shmaddr_keyboard[QuickSearchShortcutPressed_FLAG] == '1') {

                pmag ( "启动quick search 窗口" );

                //InSearchWin = 1;
                shmaddr_keyboard[SEARCH_WINDOW_OPENED_FLAG] = '1';
                shmaddr_keyboard[QuickSearchShortcutPressed_FLAG] = '0';


                /* 莫得办法，不每次都fork一个进程，窗口除第一次外都无法聚焦*/
                if ( ( pid = fork() ) == 0) {
                    searchWindow();
                    pbblue ( "searchWindow() 退出" );
                    shmaddr_keyboard[QuickSearchShortcutPressed_FLAG] = '0';
                    exit(0);
                } 
                else {

                    searchWindow_pid = pid;

                    pbblue("等待搜索窗口退出");

                    /* wait(pid)*/
                    waitpid(pid, NULL, 0);

                    shmaddr_keyboard[SEARCH_WINDOW_OPENED_FLAG] = '0';
                    pbblue("搜索窗口已退出");
                }
            }

            usleep(10000);

            if ( SIGTERM_SIGNAL ) break;
        }
    } 
    else {

        /* close ( fd[1] ); */

        /* 这又是子进程里的，获取到的变量父进程是用不到的!!!!!!!*/
        //captureShortcutEvent_pid = getpid();

        /* captureShortcutEvent(fd[0]); */
        listenShortcut();
    }

    pbcyan ( "QuickSearch.c 程序退出" );
}
