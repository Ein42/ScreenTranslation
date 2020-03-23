/*
 * 程序功能：
 * 1. 检测鼠标动作，捕捉双击，单击和区域选择事件，
 *    并设置action为相应的值:DOUBLECLICK,SINGLECLICK, SLIDE
 *
 * 2. Fork一个子进程，父进程通过管道将剪贴板内容送入子进程
 *
 * 3. 重定向子进程标准输入为管道读取端，使exec执行的python翻译
 *    程序直接从管道中读取翻译源数据
 *
 * 4. 第3个子进程用于检测Primary Selection变化，以排除窗口拖动
 *    和重设窗口大小值事件, 防止翻译入口图标误弹
 * */


#include <poll.h>
#include "common.h"
#include "quickSearch.h"
#include "detectMouse.h"
#include "cleanup.h"
#include "configControl.h"
#include "windowData.h"
#include <bsd/unistd.h>

#define TIMOUE_OUT (1000)

extern char *shmaddr;
extern char *shmaddr_selection;
extern int InNewWin;
extern int BAIDU_TRANS_EXIT_FALG;
extern int GOOGLE_TRANS_EXIT_FLAG;
extern int CanNewWin;
extern int CanNewEntrance;

extern char *shmaddr_searchWin;
extern char *shmaddr_keyboard;
extern char *shmaddr_pic;
extern int shmid_searchWin ;

/* 来自notify.c text是从剪贴板直接复制过来的字符串*/
extern char *text; 

extern int quickSearchFlag;

pid_t baidu_translate_pid;
pid_t google_translate_pid;
pid_t check_selectionEvent_pid;
pid_t fetch_data_from_mysql_pid;
pid_t detect_tran_pic_action_pid;

int mousefd;
int mouseNotRelease;
extern int action;

extern volatile sig_atomic_t SIGTERM_NOTIFY;

void *DetectMouse(void *arg) {

    pbblue ( "启动线程DetectMouse" );
    /* ConfigData *cd = ((struct Arg*)arg)->cd; */

    setpgid ( getpid(), getppid() );

    struct sigaction sa;
    int retval ;
    char buf[3];
    struct timeval old, now, whenTimeout;
    double oldtime = 0;
    double newtime = 0;
    double lasttime = 0;
    double inTimeout = 0;
    int releaseButton = 1;
    int thirdClick;

    int fd_google[2], fd_baidu[2], fd_mysql[2];
    int fd_python[3];
    int status;
    pid_t pid_google = -1, pid_baidu = -1, pid_mysql = -1;
    pid_t retpid;

    if ( (status = pipe(fd_google)) != 0 ) {
        fprintf(stderr, "create pipe fail (google)\n");
        exit(1);
    }

    if ( (status = pipe(fd_baidu)) != 0 ) {
        fprintf(stderr, "create pipe fail (baidu)\n");
        exit(1);
    }

    if ( (status = pipe(fd_mysql)) != 0 ) {
        fprintf(stderr, "create pipe fail (mysql)\n");
        exit(1);
    }

    if ( ( pid_google = fork() ) == -1 ) {
        fprintf(stderr, "fork fail\n");
        exit(1);
    }

    /*只能放在主进程中执行*/
    if ( pid_google > 0 ) {

        if ( ( pid_baidu = fork() ) == -1 ) {
            fprintf(stderr, "fork fail\n");
            exit(1);
        }

        /*让子进程中pid_google=-1,防止子进程执行到pid_google>0的代码段*/
        if (pid_baidu == 0)
            pid_google = -1;
        else if ( pid_baidu > 0 )
            baidu_translate_pid = pid_baidu;
    }

    if ( pid_google > 0 ) {

        if ( ( retpid = fork() ) == -1) 
            err_exit ("Fork check_selectionEvent failed");

        if ( retpid == 0) {

            pid_google = -1;
            setproctitle ( "%s", "Check Selection Changed" );
            checkSelectionChanged();
        }
        else {
            /* 用于后期退出时清理进程*/
            check_selectionEvent_pid = retpid;
        }
    }

    /* 咱再fork一个用于离线翻译的*/
    if ( pid_google > 0 ) {

        if ( ( retpid = fork() ) == -1) 
            err_exit ("Fork check_selectionEvent failed");

        /* In child process*/
        if ( retpid == 0 ) {

            pid_google = -1;
            pid_mysql = 0;
        }
        else if ( retpid > 0) {
            /* 用于后期退出时清理进程*/

            fetch_data_from_mysql_pid = retpid;
        }
    }

    /* 来来来，加个检测截图识别的进程*/
    if ( pid_google > 0 ) {

        if ( ( retpid = fork() ) == -1) 
            err_exit ("Fork detectTranPicAction() failed");

        /* In child process*/
        if ( retpid == 0 ) {

            pid_google = -1;
            detect_tran_pic_action_pid = 0;

            setproctitle ( "%s", "Check Tran Pic Action" );

            /* 启动截图识别检测进程*/
            detectTranPicAction();
        }
        else if ( retpid > 0) {
            /* 用于后期退出时清理进程*/
            detect_tran_pic_action_pid = retpid;
        }
    }

    if ( pid_google > 0 ) {

        google_translate_pid = pid_google;

        /*父进程:关闭读端口*/
        close(fd_google[0]);
        close(fd_baidu[0]);

        fd_python[0] = fd_google[1];
        fd_python[1] = fd_baidu[1];
        fd_python[2] = fd_mysql[1];

        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        if ( sigaction(SIGCHLD, &sa, NULL) == -1) {
            pbred("sigaction exec failed (DetectMouse.c -> SIGTERM)");
            perror("sigaction");
            quit();
        }

        // 打开鼠标设备
        mousefd = open("/dev/input/mice", O_RDONLY );
        if ( mousefd < 0 ) {
            pbred("Failed to open mice, Try to execute as superuser\n\
                    or add current user to group which /dev/input/mice belong to\n");
            quit();
        }

        int history[4] = { 0 };
        int i = 0, n = 0, m = 0;
        struct pollfd pfd;
        int timeout = 20;
        pfd.fd = mousefd;
        pfd.events = POLLIN|POLLPRI;

        while(1) {

            retval = poll ( &pfd, 1, timeout );

            if ( SIGTERM_NOTIFY ) break;

            /* 任何一端翻译程序终止即退出取词翻译*/
            if ( BAIDU_TRANS_EXIT_FALG ) {
                pbred("百度翻译子进程已退出");
                pbred("准备退出取词翻译程序");
                SIGTERM_NOTIFY = 1;
                break;
                /* quit(); */
            } 

            if ( GOOGLE_TRANS_EXIT_FLAG ) {

                pbred("谷歌翻译子进程已退出");
                pbred("准备退出取词翻译程序");
                SIGTERM_NOTIFY = 1;
                break;
                /* quit(); */
            }

            if ( shmaddr_searchWin[TEXT_SUBMIT_FLAG] == '1') {

                pbcyan ( "Quick Search 完毕" );

                if ( text == NULL )
                    if (( text = calloc(TEXTSIZE, 1)) == NULL)
                        err_exit("malloc failed in notify.c");

                shmaddr_searchWin[TEXT_SUBMIT_FLAG] = '0';
                strcpy ( text,  &shmaddr_searchWin[SUBMIT_TEXT] );
                writePipe ( &shmaddr_searchWin[SUBMIT_TEXT], fd_python[0] );
                writePipe ( &shmaddr_searchWin[SUBMIT_TEXT], fd_python[1] );
                writePipe ( &shmaddr_searchWin[SUBMIT_TEXT], fd_python[2] );
                CanNewWin = 1;
            }

            if ( shmaddr_pic[0] == FINFLAG ) {

                pbcyan ( "Tran pic 完毕" );

                if ( text == NULL )
                    if (( text = calloc(TEXTSIZE, 1)) == NULL)
                        err_exit("malloc failed in notify.c");

                shmaddr_pic[0] = CLEAR;
                strcpy ( text,  &shmaddr_pic[ACTUALSTART] );
                writePipe ( &shmaddr_pic[ACTUALSTART], fd_python[0] );
                writePipe ( &shmaddr_pic[ACTUALSTART], fd_python[1] );
                writePipe ( &shmaddr_pic[ACTUALSTART], fd_python[2] );
                CanNewEntrance = 1;
            }

            if ( shmaddr_keyboard[RECALL_PREVIOUS_TRAN] == '1' ) {

                pbcyan ( "打开上一次翻译内容" );

                if ( text == NULL )
                    if (( text = calloc(TEXTSIZE, 1)) == NULL)
                        err_exit("malloc failed in notify.c");

                shmaddr_keyboard[RECALL_PREVIOUS_TRAN] = CLEAR;

                if ( strlen ( text ) ) {
                    writePipe ( text, fd_python[0] );
                    writePipe ( text, fd_python[1] );
                    writePipe ( text, fd_python[2] );
                    CanNewWin = 1;
                }
                else { pbred ( "上一次翻译内容为空，此次不调出窗口" );}
            }

            /*超时*/
            if(retval==0) {

                gettimeofday( &whenTimeout, NULL );
                inTimeout = (whenTimeout.tv_usec + whenTimeout.tv_sec*1000000) / 1000;

                /* 超时自动清零history*/
                if ( abs (inTimeout - oldtime) > 700 && ! isAction(history, i, ALLONE))
                    if ( history[0] | history[1] |  history[2] | history[3]) {
                        memset(history, 0, sizeof(history));
                        releaseButton = 1;
                        action = 0;
                    }

                continue;
            }

            if ( shmaddr_keyboard[SELECT_EXCLUDE_FLAG] == '1' ) {
                checkApp ( selectApp() );
                shmaddr_keyboard[SELECT_EXCLUDE_FLAG] = '0';
            }

            if(read(mousefd, buf, 3) <= 0) {
                continue;
            }

            /*打开翻译窗口后不再判断鼠标动作
             * NOTE: 这句要放在读鼠标设备的语句之后,防止窗口关闭后旧数据
             * 被读进history误判，而把任何时候的数据都读完了,
             * 其他代码逻辑才不会被旧数据影响*/
            /* if ( InNewWin == 1 ) */
            /* continue; */

            /*循环写入鼠标数据到数组*/
            history[i++] = buf[0] & 0x07;
            if ( i == 4 )
                i = 0;

            /*m为最后得到的鼠标键值*/
            m = previous(i);
            n = previous(m);

            /*LOG*/
            /* int j = previous(n), x = previous(j); */
            /* printf("%d %d %d %d\n", history[m], history[n], history[j], history[x]); */

            /*没有按下按键并活动鼠标,标志releaseButton=1*/
            if ( history[m] == 0 && history[n] == 0 ) {
                releaseButton = 1;

                /* 由于拼音打字和复制操作也会触发selection changed event
                 * 但是一般拖动窗口等操作之前都是移动鼠标，并且没有按下任何
                 * 按键，正好可以将剪贴板变化的事件标志给清空*/
                shmaddr_selection[0] = '0';

                action = 0;
            }

            /* 此变量用于newNormalWindow，
             * 作用：拖动窗口标志变量(仅当拖动窗口时起效)
             * 详见newNormalWindow*/
            if ( isAction(history, i, ALLONE) ) mouseNotRelease = 1;


            /*按下左键*/
            /* 此处不要改变1 0的顺序，因为m n下标出现0 1可能是区域选择
             * 事件(SLIDE),这将导致SLIDE被一直误判*/
            if ( history[m] == 1 && history[n] == 0 ) {
                if ( releaseButton ) {

                    gettimeofday(&old, NULL);

                    /* lasttime为双击最后一次的按下按键时间;
                     * 如果上次双击时间到现在不超过600ms，则断定为3击事件;
                     * 3击会选中一整段，或一整句，此种情况也应该复制文本*/
                    if (abs(lasttime - ((old.tv_usec + old.tv_sec*1000000) / 1000)) < 800 \
                            && lasttime != 0 && action == DOUBLECLICK) {

                        thirdClick = 1;
                        notify(&history, &thirdClick, &releaseButton, fd_python);
                    }
                    else { /*不是3击事件则按单击处理，更新oldtime*/
                        oldtime = (old.tv_usec + old.tv_sec*1000000) / 1000;
                        thirdClick = 0;
                        action = SINGLECLICK;

                        //printf("\033[0;35m单击事件 \033[0m\n");
                    }
                    releaseButton = 0;

                    /*非3击事件，则为单击，更新oldtime后返回检测鼠标新一轮事件*/
                    if ( !thirdClick)
                        continue;
                }
            }

            /*检测检测是否可能为双击,以及判断时间间隔(应跳过确定的3击事件)*/
            if ( isAction(history, i, DOUBLECLICK) && !thirdClick )  {
                releaseButton = 1;
                gettimeofday( &now, NULL );
                newtime = (now.tv_usec + now.tv_sec*1000000) / 1000;

                /*双击超过700ms的丢弃掉*/
                if ( abs (newtime - oldtime) > 700)  {
                    gettimeofday(&old, NULL);
                    oldtime = (old.tv_usec + old.tv_sec*1000000) / 1000;
                    memset(history, 0, sizeof(history));
                    pbred("超时丢弃");
                    continue;
                }

                /*更新最后一次有效双击事件的发生时间*/
                lasttime = newtime;

                notify(&history, &thirdClick, &releaseButton, fd_python);
                continue;
            }

            if ( isAction( history, i, SLIDE ) ) {
                mouseNotRelease = 0;
                notify(&history, &thirdClick, &releaseButton, fd_python);
            }

        } /*while loop*/

    } /*if pid_google > 0*/

    /*child process for google translate*/
    if (pid_google == 0){ 

        pbgreen("执行谷歌翻译程序");

        close(fd_google[1]); /*关闭写端口*/

        /*重映射标准输入为管道读端口*/
        if ( fd_google[0] != STDIN_FILENO) {
            if ( dup2( fd_google[0], STDIN_FILENO ) != STDIN_FILENO) {
                fprintf(stderr, "dup2 error (google)");
                close(fd_google[0]);
                exit(1);
            }
        }

        char * const cmd[3] = {"tranen","-s", (char*)0};
        if ( execv( "/usr/bin/tranen", cmd ) < 0) {
            fprintf(stderr, "Execv error (google)\n");
            perror("Execv error(google):");
            exit(1);
        }
        printf("detectMouse.c (google)子进程已经退出...\n");
        exit(1);
    }

    if ( pid_baidu == 0 ) {
        
        close(fd_baidu[1]); /*关闭写端口*/

        /*重映射标准输入为管道读端口*/
        if ( fd_baidu[0] != STDIN_FILENO) {
            if ( dup2( fd_baidu[0], STDIN_FILENO ) != STDIN_FILENO) {
                fprintf(stderr, "dup2 error (baidu)");
                close(fd_baidu[0]);
                exit(1);
            }
        }

        pbgreen("执行百度翻译程序");
        char * const cmd[3] = {"bing","-s", (char*)0};
        if ( execv( "/usr/bin/bing", cmd ) < 0) {
            fprintf(stderr, "Execv error (baidu)\n");
            perror("Execv error(baidu):");
            exit(1);
        }
        pbgreen("detectMouse.c (baidu)子进程已经退出...\n");
        exit(1);

    }

    if ( pid_mysql == 0 ) {

        close(fd_mysql[1]); /*关闭写端口*/

        /*重映射标准输入为管道读端口*/
        if ( fd_mysql[0] != STDIN_FILENO) {
            if ( dup2( fd_mysql[0], STDIN_FILENO ) != STDIN_FILENO) {
                fprintf(stderr, "dup2 error (mysql)");
                close(fd_mysql[0]);
                exit(1);
            }
        }

        pbgreen("Execute the offline translation process\n");
        char * const cmd[3] = {"fetchDict","-s", (char*)0};
        if ( execv( "/usr/bin/fetchDict", cmd ) < 0) {
            fprintf(stderr, "Execv error (mysql)\n");
            perror("Execv error(mysql):");
            exit(1);
        }
        printf("detectMouse.c (mysql)子进程已经退出...\n");
        exit(1);

    }
    pbcyan ( "DetectMouse 退出" );
    /* pthread_exit(NULL); */

    return NULL;
}

