/*GuiEntrance.c
 * 程序功能:
 *  创建翻译入口图标*/

#include "common.h"
#include "expanduser.h"

extern char *shmaddr_google;
extern char *shmaddr_searchWin;
extern int action;
extern int timeout_id_1;
extern int timeout_id_2;
extern int CanNewWin;
extern int CanNewEntrance;

extern char *baidu_result[BAIDUSIZE] ;
extern char *google_result[GOOGLESIZE];

extern char *shmaddr_google;
extern char *shmaddr_baidu;

extern char audioOnline_en[512];
extern char audioOnline_uk[512];

int HadDestroied = 1;
int quickSearchFlag = 0;
static int aboveWindow = 0;

int quit_entry(void *arg);
int quit_test(void *arg);
void setNewWinFlag(GtkWidget *button, GtkWidget *window);
void leave_event();
void enter_event();

struct clickDate {
    GtkWidget *window;
    GtkWidget *button;
};

void *GuiEntrance(void *arg) {

    aboveWindow = 0;

    /*等待鼠标事件到来创建入口图标*/
    while(1) {

        if (CanNewEntrance) {
            printf("\nDetect mouse action, creating icon entry "
                    "CanNewEntrance = %d action=%d\n", CanNewEntrance, action);

            if ( shmaddr_google[0] == EMPTYFLAG) {
                printf("空字符串,返回继续等待...\n");
                shmaddr_google[0] = CLEAR;
                action = 0;
                CanNewWin = 0;
                CanNewEntrance = 0;
                pthread_exit(NULL);
                return (void*)0;
            }
            break;
        }

        /*
           if ( shmaddr_searchWin[20] == '1' ) {
           quickSearchFlag = 1;
           pthread_exit(NULL);
           return (void*)0;
           } */

        usleep(1000);
    }

    GtkWidget *window;

    /*入口图标销毁标志置0，表示处于显示状态*/
    HadDestroied = 0;
    CanNewEntrance = 0;
    action = 0;

    gtk_init(NULL, NULL);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);

    /*设置窗口基本属性*/
    gtk_window_set_title(GTK_WINDOW(window), "");
    gtk_window_set_default_size(GTK_WINDOW(window), 15,15);
    gtk_window_set_deletable(GTK_WINDOW(window), FALSE); 
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_TOOLBAR); 
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE); 
    GtkWidget *button = gtk_button_new();


    /*TODO:添加文件存在性检测*/
    /*添加图标*/

    GdkPixbuf *src = gdk_pixbuf_new_from_file(expanduser("/home/$USER/.stran/tran.png"), NULL);
    GdkPixbuf *dst = gdk_pixbuf_scale_simple(src, 30, 30, GDK_INTERP_BILINEAR);
    GtkWidget *image = gtk_image_new_from_pixbuf(dst);

    g_object_unref(src);
    g_object_unref(dst);

    //GtkWidget *image = gtk_image_new_from_file("/home/usernamee/.stran/tran.png");
    gtk_button_set_image(GTK_BUTTON(button), image);
    gtk_container_add(GTK_CONTAINER(window), button);
    gtk_widget_set_app_paintable(window, TRUE);
    gtk_widget_set_opacity(window, 0.7);
    GdkScreen *screen = gtk_widget_get_screen(window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    gtk_widget_set_visual(window, visual);


    /*连接鼠标点击事件*/
    g_signal_connect(button, "clicked",G_CALLBACK(setNewWinFlag), window);

    /*监听鼠标进入和离开窗口事件*/
    g_signal_connect(GTK_BUTTON(button), "leave", G_CALLBACK(leave_event), NULL);
    g_signal_connect(GTK_BUTTON(button), "enter", G_CALLBACK(enter_event), NULL);

    /*移动入口图标防止遮挡视线*/
    gint cx, cy;
    gtk_window_get_position(GTK_WINDOW(window), &cx, &cy);
    gtk_window_move(GTK_WINDOW(window), cx-30, cy-50);
    gtk_widget_show_all(window);

    /*添加超时和单击销毁图标回调函数*/
    struct clickDate cd;
    cd.window = window;
    cd.button = button;
    timeout_id_1 = g_timeout_add(1200, quit_entry, &cd);
    timeout_id_2 = g_timeout_add(600, quit_test, &cd);

    gtk_main();

    pthread_exit(NULL);
}

void setNewWinFlag(GtkWidget *button, GtkWidget *window) {

    if ( shmaddr_google[0] == EMPTYFLAG )
        CanNewWin = 0;
    else 
        CanNewWin = 1;

    HadDestroied = 1;

    /*退出时注意注销超时回调函数，否则下一次创建的
     * 入口图标可能刚好创建就超时导致不显示*/
    g_source_remove(timeout_id_2);
    g_source_remove(timeout_id_1);

    gtk_widget_destroy(button);
    gtk_widget_destroy(window);
    gtk_main_quit();
}

int quit_test(void *arg) {

    struct clickDate * cd;
    cd = (struct clickDate*)arg;
    GtkWidget *button = cd->button;
    GtkWidget *window = cd->window;

    //printf("单击检测\n");

    /*入口图标已在quit_entry中销毁,返回FALSE不再调用此函数*/
    if ( HadDestroied )
        return FALSE;

    /*不在窗口上的单击定义为销毁窗口命令*/
    if (!HadDestroied && !aboveWindow && \
            (action == SINGLECLICK || action == DOUBLECLICK) ) {

        if ( action == SINGLECLICK  && !aboveWindow) {
            printf("GuiEntrance: 单击销毁\n");

            clearMemory();

            CanNewWin = 0;

            /*单击销毁action置0
             * 双击销毁则可能新选中了文本，再新建一个入口*/
            if ( action == SINGLECLICK )
                action = 0;
            else if (action == DOUBLECLICK)
                CanNewEntrance = 1;

            HadDestroied = 1;

            g_source_remove(timeout_id_2);
            g_source_remove(timeout_id_1);

            gtk_widget_destroy(button);
            gtk_widget_destroy(window);
            gtk_main_quit();

            return FALSE;
        }
    }

    return TRUE;
}

int quit_entry(void *arg) {

    struct clickDate * cd;
    cd = (struct clickDate*)arg;
    GtkWidget *button = cd->button;
    GtkWidget *window = cd->window;

    /*入口图标已经在quit_test中销毁时返回FALSE不再调用*/
    if ( HadDestroied )
        return FALSE;

    if ( button &&  window && !HadDestroied && !aboveWindow) {

        printf("GuiEntrance: 超时销毁\n");

        /*如果超时销毁的时候恰好又遇到双击选中文本
         * 也应该新建入口图标*/
        if ( action == DOUBLECLICK)
            CanNewEntrance = 1;

        clearMemory();

        action  = 0;
        CanNewWin = 0;
        HadDestroied = 1;

        g_source_remove(timeout_id_1);
        g_source_remove(timeout_id_2);
        gtk_widget_destroy(button);
        gtk_widget_destroy(window);
        gtk_main_quit();
        return FALSE;
    }

    return TRUE;
}

void leave_event() {
    printf("Leaved window\n");
    aboveWindow = 0;
}

void enter_event() {
    printf("Enter window\n");
    aboveWindow = 1;
}
