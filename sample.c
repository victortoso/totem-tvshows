#include <glib.h>
#include "totem-video-summary.h"

gint main(gint argc, gchar *argv[])
{
    TotemVideosSummary *tvs;
    GrlMediaVideo *video;

    gtk_init (&argc, &argv);
    grl_init (&argc, &argv);

    video = GRL_MEDIA_VIDEO(grl_media_video_new());
    grl_media_set_url (GRL_MEDIA (video), argv[1]);

    tvs = totem_videos_summary_new (video);
    g_object_unref (video);
    if (TOTEM_IS_VIDEOS_SUMMARY (tvs))
        g_print ("Sucesso\n");
    g_object_unref (tvs);
    return 0;
}
