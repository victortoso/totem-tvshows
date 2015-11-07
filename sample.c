#include <glib.h>
#include "totem-video-summary.h"

struct {
    char *url;
    char *title;
    char *episode_title;
    int season;
    int episode;
} videos[] = {
    /* Series */
    { LOCAL_PATH "House.S01E01.mkv", "House.S01E01.mkv", NULL, 1, 1 },
    { LOCAL_PATH "Breaking.Bad.S01E01.mkv", "Breaking.Bad.S01E01.mkv", NULL, 1, 1 },
    { LOCAL_PATH "Limitless.S01E01.mkv", "Limitless.S01E01.mkv", NULL, 1, 1 }
};

#define THETVDB_ID  "grl-thetvdb"
#define THETVDB_KEY "3F476CEF2FBD0FB0"

#define TMDB_ID  "grl-tmdb"
#define TMDB_KEY "719b9b296835b04cd919c4bf5220828a"

#define LUA_FACTORY_ID "grl-lua-factory"

static void
setup_grilo(void)
{
  GrlConfig *config;
  GrlRegistry *registry;
  GError *error = NULL;

  registry = grl_registry_get_default ();

  /* For metadata in the filename */
  grl_registry_load_plugin_by_id (registry, LUA_FACTORY_ID, &error);

  /* For Movies and Series */
  config = grl_config_new (THETVDB_ID, NULL);
  grl_config_set_api_key (config, THETVDB_KEY);
  grl_registry_add_config (registry, config, &error);
  g_assert_no_error (error);
  grl_registry_load_plugin_by_id (registry, THETVDB_ID, &error);
  g_assert_no_error (error);

  config = grl_config_new (TMDB_ID, NULL);
  grl_config_set_api_key (config, TMDB_KEY);
  grl_registry_add_config (registry, config, &error);
  grl_registry_load_plugin_by_id (registry, TMDB_ID, &error);
  g_assert_no_error (error);
}

gint main(gint argc, gchar *argv[])
{
    GtkSettings *gtk_settings;
    TotemVideosSummary *tvs;
    GrlMediaVideo *video;
    GtkWidget *win;
    gint i;

    gtk_init (&argc, &argv);
    grl_init (&argc, &argv);

    gtk_settings = gtk_settings_get_default ();
    g_object_set (G_OBJECT (gtk_settings), "gtk-application-prefer-dark-theme", TRUE, NULL);

    setup_grilo();
    tvs = totem_videos_summary_new ();
    g_return_val_if_fail (TOTEM_IS_VIDEOS_SUMMARY (tvs), 1);

    for (i = 0; i < G_N_ELEMENTS (videos); i++) {
      video = GRL_MEDIA_VIDEO(grl_media_video_new());
      grl_media_set_url (GRL_MEDIA (video), videos[i].url);
      grl_media_set_title (GRL_MEDIA (video), videos[i].title);
      totem_videos_summary_add_video (tvs, video);
      g_object_unref (video);
    }

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (win), 800, 600);
    gtk_window_set_title(GTK_WINDOW(win), "Totem TVSHOWS");
    gtk_container_add (GTK_CONTAINER (win), GTK_WIDGET(tvs));
    gtk_widget_show (GTK_WIDGET(tvs));
    gtk_widget_show (win);
    gtk_main ();
    g_object_unref (tvs);
    g_object_unref (win);
    return 0;
}
