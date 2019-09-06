/*程序功能:
 *
 * 1. 模拟键盘发送CTRL-C或者CTRL-SHIFT-C进行复制操作
 *
 * 2. 通过Xserver获取剪贴板内容
 *
 * 3. 对相关标志变量重设
 *
 * 4. 清除鼠标历史信息
 *
 * */

#include "common.h"

int fd_key = -1;
FILE *fp = NULL;
char *text = NULL;
int NoneText = 0;
char *lastText = NULL;
int CanNewEntrance;

extern char *shmaddr_google;
extern char *shmaddr_baidu;
extern int action;

void send_Ctrl_Shift_C() {

    Display *dpy;
    unsigned int ctrl, shift, c;
    dpy = XOpenDisplay(NULL);

    ctrl = XKeysymToKeycode (dpy, XK_Control_L);
    XTestFakeKeyEvent (dpy, ctrl, True, 0);
    XFlush(dpy);

    shift = XKeysymToKeycode (dpy, XK_Shift_L);
    XTestFakeKeyEvent (dpy, shift, True, 0);
    XFlush(dpy);

    c = XKeysymToKeycode(dpy, XK_C);
    XTestFakeKeyEvent(dpy, c, True, 0);
    XFlush(dpy);

    XTestFakeKeyEvent(dpy, c, False, 0);
    XFlush(dpy);

    XTestFakeKeyEvent(dpy, shift, False, 0);
    XFlush(dpy);

    XTestFakeKeyEvent(dpy, ctrl, False, 0);
    XFlush(dpy);

    XCloseDisplay(dpy);
}

void send_Ctrl_C() {

    Display *dpy;
    unsigned int ctrl, c;
    dpy = XOpenDisplay(NULL);

    ctrl = XKeysymToKeycode (dpy, XK_Control_L);
    XTestFakeKeyEvent (dpy, ctrl, True, 0);
    XFlush(dpy);

    c = XKeysymToKeycode(dpy, XK_C);
    XTestFakeKeyEvent(dpy, c, True, 0);
    XFlush(dpy);

    XTestFakeKeyEvent(dpy, c, False, 0);
    XFlush(dpy);

    XTestFakeKeyEvent(dpy, ctrl, False, 0);
    XFlush(dpy);

    XCloseDisplay(dpy);
}

void notify(int (*history)[4], int *thirdClick, int *releaseButton, int fd[2]) {

    //int Ctrl_Shift_C[] = {KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_C};
    //int Ctrl_C[] = {KEY_LEFTCTRL, KEY_C};
    char appName[100];

    if ( lastText == NULL )  {
        lastText = calloc( TEXTSIZE , 1);
        if ( lastText == NULL ) 
            err_exit("malloc for lastText failed in notify.c");
    }

    int thirdClickTmp = *thirdClick;

    if ( *thirdClick == 1 )
        *thirdClick = 0;
    *releaseButton = 1;

    if ( fd_key < 0 )
        if ((fd_key = open("/dev/input/event3", O_RDWR)) < 0 ) 
            err_exit("opened keyboard device fail");


    /*需每次都执行才能判断当前的窗口是什么*/
    fp = popen("ps -p `xdotool getwindowfocus getwindowpid`\
            | awk '{print $NF}' | tail -n 1", "r");

    memset ( appName, 0, sizeof(appName) );

    if ( fread(appName, sizeof(appName), 1, fp) < 0) {
        fprintf(stderr, "fread error\n");
        return;
    }

    pclose(fp);

    fprintf(stdout, "Focus window application: %s\n", appName);

#if 0
    if ( isApp("screenShotApp", appName) == 1 )
        return;

    if ( isApp("wantToIgnore", appName) == 1 ) {
        printf("忽略此软件\n");
        return;
    }

    if ( isApp("terminal", appName) == 1) {
        printf("send key ctrl-shift-c\n");
        //simulateKey(fd_key, Ctrl_Shift_C, 3);
        //send_Ctrl_Shift_C();
        printf("Send key successful\n");
    }
    else {
        //simulateKey(fd_key, Ctrl_C, 2);
        //send_Ctrl_C();
    }
#endif

    delay();

    if ( text == NULL )
        /*free in forDetectMouse.c*/
        if (( text = calloc(TEXTSIZE, 1)) == NULL)
            err_exit("malloc failed in notify.c");

    memset(text, 0, TEXTSIZE);
    int retval = 0;
    int trytimes = 0;
    while ( trytimes <= 6 ) {

        if ( (retval = getClipboard(text) ) == 1) {
            printf("Not copy event\n");
            action = 0;
            memset(*history, 0, sizeof(*history));
            CanNewEntrance = 0;
            return ;
        }

        if ( strcmp(lastText, text ) != 0 )
            break;
        else
        {
            trytimes++;
            continue;
        }
    }

    if ( strcmp(lastText, text ) == 0 )
    {
        *text = '0';
        action = 0;
        //static int i = 0;
        //printf("same text %d %d %d %d %d\n", i++, (*history)[0], (*history)[1],(*history)[2],(*history)[3]);
        printf("\033[0;34mSame text \033[0m\n");
        if ( strlen(text) < 100)
            printf("\033[0;31m%s\033[0m\n", text);
        memset(*history, 0, sizeof(*history));
        CanNewEntrance = 0;
        return ;
    }

    strcpy(lastText, text);

    /* 只能减小结果获取错误的概率，如果两边翻译都不够快，清零发生在百度谷歌翻译写1之前，
     * 这句是没有意义的，之后点开翻译结果界面获取到的就会有上一次点击文本的内容，不过
     * 一般按切换按钮后是可以重新加载出想要的结果的*/
    memset(shmaddr_google, '0', 10);
    memset(shmaddr_baidu, '0', 10);

    writePipe(text, fd[0]);
    writePipe(text, fd[1]);

    /*管道写完成，可以创建入口图标了*/
    if ( action == DOUBLECLICK || action == SLIDE )
        if ( thirdClickTmp != 1 )
            CanNewEntrance = 1;

    /*清除鼠标记录*/
    memset(*history, 0, sizeof(*history));
}
