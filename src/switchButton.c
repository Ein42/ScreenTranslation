#include "common.h"


GtkWidget *newSwitchButton ( WinData *win )
{
    GtkWidget *button = gtk_button_new ();

    GdkPixbuf *src = gdk_pixbuf_new_from_file("/home/rease/.stran/Switch.png", NULL);
    GdkPixbuf *dst = gdk_pixbuf_scale_simple(src, 20, 20, GDK_INTERP_BILINEAR);

    GtkWidget *image = gtk_image_new_from_pixbuf(dst);

    gtk_button_set_image(GTK_BUTTON(button), image);

    g_object_ref ( image );

    g_object_unref ( src );
    g_object_unref ( dst );

    win->image = image;

    return button;
}

GtkWidget *newOfflineButton ( WinData *win ) {

    GtkWidget *button = gtk_button_new();

    GdkPixbuf *src = gdk_pixbuf_new_from_file("/home/rease/.stran/offline.png", NULL);
    GdkPixbuf *dst = gdk_pixbuf_scale_simple(src, 20, 20, GDK_INTERP_BILINEAR);

    GtkWidget *image = gtk_image_new_from_pixbuf(dst);

    gtk_button_set_image(GTK_BUTTON(button), image);

    g_object_ref ( image );

    g_object_unref ( src );
    g_object_unref ( dst );

    win->offlineButton = button;

    return button;
}
