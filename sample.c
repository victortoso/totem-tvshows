#include <glib.h>
#include "totem-video-summary.h"

struct {
    char *url;
    char *title;
    char *episode_title;
    int season;
    int episode;
} videos[] = {
    { LOCAL_PATH "House.S01E01.mkv", "House.S01E01.mkv", NULL, 1, 1 },
};

gint main(gint argc, gchar *argv[])
{
    TotemVideosSummary *tvs;
    GrlMediaVideo *video;
    GtkWidget *win;

    gtk_init (&argc, &argv);
    grl_init (&argc, &argv);

    video = GRL_MEDIA_VIDEO(grl_media_video_new());
    grl_media_set_url (GRL_MEDIA (video), videos[0].url);
    grl_media_set_title (GRL_MEDIA (video), videos[0].title);

    tvs = totem_videos_summary_new (video);
    g_object_ref_sink (video);
    g_object_unref (video);
    if (tvs != NULL) {
        if (TOTEM_IS_VIDEOS_SUMMARY (tvs)) {
            g_print ("Sucesso\n");
        } else {
            g_print ("Fail: is not tvs\n");
            return 1;
        }

        win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(win), "Totem TVSHOWS");
        gtk_container_add (GTK_CONTAINER (win), GTK_WIDGET(tvs));
        gtk_widget_show (GTK_WIDGET(tvs));
        gtk_widget_show (win);
        gtk_main ();
        g_object_unref (tvs);
        g_object_unref (win);
    } else
        g_print ("Fail: is null\n");
    return 0;
}
