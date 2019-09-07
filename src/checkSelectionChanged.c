#include "common.h"
#include <assert.h>
#include <X11/extensions/Xfixes.h>

extern char *shmaddr_selection;
extern int action;

void WatchSelection(Display *display, const char *bufname);

Display *display;

/* 此函数用不上了，采用sigterm无法kill掉此进程，必须用sigkil，
 * 但sigkill无法捕捉*/
void clean() {

    printf("close display\n");
    if ( display )
        XCloseDisplay ( display );

    /* 手动置空，双重保险*/
    display = NULL;

    exit(0);
}

void checkSelectionChanged(int writefd, int readfd)
{
    display = XOpenDisplay(NULL);
    unsigned long color = BlackPixel(display, DefaultScreen(display));
    XCreateSimpleWindow(display, DefaultRootWindow(display), 0,0, 1,1, 0, color, color);

    struct sigaction sa;
    sa.sa_handler =  clean;
    sigemptyset ( &sa.sa_mask );

    if ( sigaction(SIGTERM, &sa, NULL) == -1) {
        printf("\033[0;31msigaction err(checkSelectionChanged -> SIGTERM)\033[0m\n");
        perror("sigaction");
        quit();
    }

    if ( sigaction(SIGINT, &sa, NULL) == -1) {
        printf("\033[0;31msigaction err(checkSelectionChanged -> SIGTERM)\033[0m\n");
        perror("sigaction");
        quit();
    }

    const char buf[] = "PRIMARY";
    WatchSelection ( display, buf);
}

void WatchSelection(Display *display, const char *bufname)
{
    int event_base, error_base;
    XEvent event;
    Atom bufid = XInternAtom(display, bufname, False);

    assert( XFixesQueryExtension(display, &event_base, &error_base) );
    XFixesSelectSelectionInput(display, DefaultRootWindow(display), bufid, XFixesSetSelectionOwnerNotifyMask);

    while ( 1 ) {

        XNextEvent ( display, &event );

        if (event.type == event_base + XFixesSelectionNotify &&
                ((XFixesSelectionNotifyEvent*)&event)->selection == bufid) {

            shmaddr_selection[0] = '1';
            printf("\033[0;31mSelection change: write finish flag: 1 \033[0m\n");
        }

        if ( action == CLEAR ) {
            shmaddr_selection[0] = '0';
            printf("\033[0;31mAction is clean, set flag to 0 \033[0m\n");
        }

    }
}
