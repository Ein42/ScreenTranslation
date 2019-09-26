#ifndef __COMMON_H__
#define __COMMON_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gtk/gtkwindow.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <time.h>
#include <linux/uinput.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

/*for gdk_x11_window_get_xid()*/
#include <gdk/gdkx.h> 

#include <X11/extensions/XTest.h>

#define TEXTSIZE (1024*1024)
#define SINGLECLICK (1)
#define DOUBLECLICK (2)
#define SLIDE (3)
#define EMPTYFLAG ('5')
#define EXITFLAG ('4')
#define NULLCHAR ('3')
#define ERRCHAR ('2')
#define FINFLAG ('1')
#define CLEAR   ('0')
#define ACTUALSTART (10)
#define BAIDUSIZE (6)
#define GOOGLESIZE (3)
#define LINELEN (28)

typedef struct Google {
    double width;
    double height;
}Google;

Google gw;

typedef struct Baidu {
    double width;
    double height;
    char *audio[2];
}Baidu;

Baidu bw;

typedef struct WinData{

    GtkWidget *window;
    GtkWidget *layout;
    GtkWidget *button;
    GtkWidget *view;

    GdkWindow *gdkwin;

    GtkTextIter *iter;
    GtkTextBuffer *buf;

    GtkWidget *volume;
    GtkWidget *scroll;

    GtkWidget *image;
    GtkWidget *oldImage;

    GdkPixbuf *srcBackgroundImage;

    gint width;
    gint height;
    gint forceResize;

    gint lineHeight;
    gint phonPos;

    gint hadRedirect;

    gint hadShowGoogleResult;

    int index_google[2];
    char **storage;

    int switchEvent;

}WinData;

void show_utf8_prop(Display *dpy, Window w, Atom p, char *text);

int getClipboard(char *text);
void delay();
void writePipe(char *text, int fd);
void handler(int signo);
int isApp(char *appName, char *name);
int previous( int n );
int isAction(int history[], int last, int action);
void quit();
void sync_key( int *fd, struct input_event *event, int *len);
void press(int fd, int keyCode);
void release(int fd, int keyCode);
void simulateKey(int fd,  int key[], int len);
void err_exit(char *buf);
void *GuiEntrance(void *arg);
void *DetectMouse(void *arg);
int shared_memory_for_google_translate(char **addr);
int shared_memory_for_baidu_translate(char **addr);
int shared_memory_for_selection(char **addr);
void *newNormalWindow();
void adjustStr(char *p[3], int len, char *storage[3]);
int adjustStrForScrolledWin(int len, char *source);
void notify(int (*history)[4], int *thirdClick, int *releaseButton, int fd[2]);

void separateDataForBaidu(int *index, int len);
void adjustStrForBaidu(int len, char *source, int addSpace, int copy);
int countLines ( int len, char *source );
int countCharNums ( char *source );
GtkWidget* newVolumeBtn () ;
GtkWidget* insertVolumeIcon( GtkWidget *window, GtkWidget *layout, WinData *wd ) ;

int mp3play (GtkWidget *button, gpointer *data);
GtkWidget * getImageWidgetWithBG( gint width, gint height  ) ;
GtkWidget *syncImageSize ( GtkWidget *window, gpointer *data ) ;

GtkWidget *newSwitchButton ( WinData *win );

int getLinesOfGoogleTrans ( int *index_google );

void separateGoogleDataSetWinSize ( int *index_google );

struct clickDate {
    GtkWidget *window;
    GtkWidget *button;
};

struct Arg {
    int argc;
    char **argv;
    char *addr_google;
    char *addr_baidu;
};


#define GETEKYDIR ("/tmp")
#define PROJECTID  (2333)
#define PROJECTID2  (2334)
#define SHMSIZE (1024*1024)

#define PhoneticFlag ( shmaddr_baidu[1] - '0' )
#define NumZhTranFlag ( shmaddr_baidu[2] - '0')
#define NumEnTranFlag ( shmaddr_baidu[3] - '0')
#define OtherWordFormFlag ( shmaddr_baidu[4] - '0')
#define NumAudioFlag ( shmaddr_baidu[5] - '0')

#define SourceInput ((char *)( baidu_result[0] ))
#define Phonetic ((char *)( baidu_result[1] ))
#define ZhTrans ((char *)( baidu_result[2] ))
#define EnTrans ((char *)( baidu_result[3] ))
#define OtherWordForm ((char *)( baidu_result[4] ))
#define Audios ((char *)( baidu_result[5] ))


/* For newWindow.c*/

int destroyNormalWin(GtkWidget *window, WinData *win);
int waitForContinue();
void getIndex(int *index, char *addr);
void get_paragraph();
void clearMemory ();
void initMemoryBaidu();
void initMemoryGoogle();
void printDebugInfo();
int  newScrolledWin();
void setFontProperties(GtkTextBuffer *buf, GtkTextIter *iter);
void changeDisplay(GtkWidget *button, gpointer *arg);
void displayGoogleTrans(GtkWidget *button, gpointer *arg);
void displayBaiduTrans(GtkTextBuffer *buf, GtkTextIter* iter, gpointer *arg);
void syncScrolledWinWithConfigEvent ( GtkWidget *window, GdkEvent *event, gpointer *wd );
void syncNormalWinForConfigEvent( GtkWidget *window, GdkEvent *event, gpointer scroll );
void adjustWinSize(GtkWidget *button, gpointer *arg, int which);
void setWinSizeForNormalWin ( int maxlen, int lines, char *addr );
void showGoogleScrolledWin(GtkTextBuffer *gtbuf, GtkTextIter *iter, WinData *wd);
int getMaxLenOfBaiduTrans() ;
int getLinesOfBaiduTrans () ;

gboolean key_press ( GtkWidget *window, GdkEventKey *event, gpointer *data );

/* End For newWindow.c*/


void quickSearch();
void tranSelect();
void checkSelectionChanged() ;

int isEmpty( char *buf );

#endif
