/*
 * Copyright (C) 2014. All rights reserved.
 *
 * Author: Victor Toso <me@victortoso.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <grilo.h>
#include <net/grl-net.h>
#include <gtk/gtk.h>

#define THETVDB_ID           "grl-thetvdb"
#define TOSO_API_KEY         "3F476CEF2FBD0FB0"

//static gint num_tests;

static GrlRegistry *registry;
static GrlKeyID tvdb_poster_key;
static GrlKeyID guest_stars_key;

gchar *
get_data_from_media (GrlData *data,
                     GrlKeyID key)
{
  gint i, len;
  gchar *str = NULL;

  len = grl_data_length (data, key);
  for (i = 0; i < len; i++) {
    GrlRelatedKeys *relkeys;
    gchar *tmp;
    const gchar *element;

    relkeys = grl_data_get_related_keys (data, key, i);
    element = grl_related_keys_get_string (relkeys, key);
    g_print ("-----> %s\n", element);

    tmp = str;
    str = (str == NULL) ? g_strdup (element) :
                          g_strconcat (str, ", ", element, NULL);
    g_clear_pointer (&tmp, g_free);
  }
  g_print ("[debug] %s\n", str);
  return str;
}

static void
build_ui (GrlMedia *media,
          gchar    *img_path)
{
  GtkBuilder *builder;
  GtkWidget *window, *poster, *widget;
  GdkPixbuf *pixbuf;
  const gchar *show, *overview;
  gchar *str;
  GrlMediaVideo *video;

  video = GRL_MEDIA_VIDEO (media);
  show = grl_media_video_get_show (video);

  /* Construct a GtkBuilder instance and load our UI description */
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, "totem-shows.glade", NULL);

  /* Connect signal handlers to the constructed widgets. */
  window = GTK_WIDGET (gtk_builder_get_object (builder, "main-window"));
  gtk_window_set_title (GTK_WINDOW (window), show);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  poster = gtk_image_new_from_file (img_path);
  pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (poster));
  pixbuf = gdk_pixbuf_scale_simple (pixbuf, 226, 333, GDK_INTERP_BILINEAR);
  poster = GTK_WIDGET (gtk_builder_get_object (builder, "poster-image"));
  gtk_image_set_from_pixbuf (GTK_IMAGE (poster), pixbuf);
  g_clear_pointer (&img_path, g_free);

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "summary-data"));
  overview = grl_media_get_description (media);
  gtk_label_set_text (GTK_LABEL (widget), overview);

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "cast-data"));
  str = get_data_from_media (GRL_DATA (media), GRL_METADATA_KEY_PERFORMER);
  gtk_label_set_text (GTK_LABEL (widget), str);
  g_clear_pointer (&str, g_free);

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "directors-data"));
  str = get_data_from_media (GRL_DATA (media), GRL_METADATA_KEY_DIRECTOR);
  gtk_label_set_text (GTK_LABEL (widget), str);
  g_clear_pointer (&str, g_free);

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "guests-data"));
  str = get_data_from_media (GRL_DATA (media), guest_stars_key);
  if (str != NULL) {
    gtk_label_set_text (GTK_LABEL (widget), str);
    g_clear_pointer (&str, g_free);
  }

  gtk_widget_show_all (window);
}

static void
fetch_poster_done (GObject *source_object,
                   GAsyncResult *res,
                   gpointer user_data)
{
  const gchar *title;
  gchar *data, *img_path;
  gsize len;
  GError *err = NULL;
  GrlMedia *media;

  grl_net_wc_request_finish (GRL_NET_WC (source_object),
                             res, &data, &len, &err);

  if (err != NULL) {
    g_warning ("Fetch image failed due: %s", err->message);
    g_error_free (err);
    return;
  }

  media = GRL_MEDIA (user_data);
  title = grl_media_get_title (media);
  img_path = g_build_filename (g_get_tmp_dir (), title, NULL);

  if (!g_file_set_contents(img_path, data, len, &err)) {
    g_warning ("Saving image failed due: %s", err->message);
    g_error_free (err);
    return;
  }

  build_ui (media, img_path);
}

static void
resolve_done (GrlSource    *source,
              guint         operation_id,
              GrlMedia     *media,
              gpointer      user_data,
              const GError *error)
{
  const gchar *title;
  gchar *img_path;

  if (error)
    g_error ("Resolve operation failed. Reason: %s", error->message);

  title = grl_media_get_title (media);
  img_path = g_build_filename (g_get_tmp_dir (), title, NULL);
  if (!g_file_test (img_path, G_FILE_TEST_EXISTS)) {
    GrlNetWc *wc;
    const gchar *url;

    url = grl_data_get_string (GRL_DATA (media), tvdb_poster_key);
    wc = grl_net_wc_new ();

    GRL_DEBUG ("url[1] %s", url);
    grl_net_wc_request_async (wc, url, NULL, fetch_poster_done, media);
    g_object_unref (wc);
    g_free (img_path);
  } else
    build_ui (media, img_path);
}

static void
resolve_urls (gint   argc,
              gchar *argv[])
{
  GrlSource *source;
  GrlMediaVideo *video;
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;
  gint i;

  source = grl_registry_lookup_source (registry, THETVDB_ID);
  g_assert (source != NULL);

  caps = grl_source_get_caps (source, GRL_OP_RESOLVE);
  options = grl_operation_options_new (caps);
  grl_operation_options_set_flags (options, GRL_RESOLVE_FULL);

  tvdb_poster_key = grl_registry_lookup_metadata_key (registry, "thetvdb-poster");
  guest_stars_key = grl_registry_lookup_metadata_key (registry, "thetvdb-guest-stars");

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_SHOW,
                                    GRL_METADATA_KEY_SEASON,
                                    GRL_METADATA_KEY_EPISODE,
                                    GRL_METADATA_KEY_TITLE,
                                    GRL_METADATA_KEY_DESCRIPTION,
                                    GRL_METADATA_KEY_PERFORMER,
                                    GRL_METADATA_KEY_DIRECTOR,
                                    GRL_METADATA_KEY_PUBLICATION_DATE,
                                    tvdb_poster_key,
                                    guest_stars_key,
                                    GRL_METADATA_KEY_INVALID);
  for (i = 1; i < argc; i++) {
    g_print ("-> %s\n", argv[1]);
    video = GRL_MEDIA_VIDEO (grl_media_video_new ());
    grl_media_set_url (GRL_MEDIA (video), argv[i]);
    grl_source_resolve (source,
                        GRL_MEDIA (video),
                        keys,
                        options,
                        resolve_done,
                        NULL);
  }
  g_object_unref (options);
  g_object_unref (caps);
  g_list_free (keys);
}

void
config_and_load_plugins (void)
{
  GError *error = NULL;
  GrlConfig *config;

  registry = grl_registry_get_default ();

  /* Add API-KEY to The TVDB source */
  config = grl_config_new (THETVDB_ID, NULL);
  grl_config_set_api_key (config, TOSO_API_KEY);
  grl_registry_add_config (registry, config, &error);
  g_assert_no_error (error);

  grl_registry_load_all_plugins (registry, &error);
  g_assert_no_error (error);
}

gint
main (gint   argc,
      gchar *argv[])
{
  gtk_init (&argc, &argv);
  grl_init (&argc, &argv);

  if (argc == 1) {
    g_print ("Need a url to a file:\n"
             "file:///path/to/the/file.mkv\n");
    return -1;
  }

  config_and_load_plugins ();
  resolve_urls (argc, argv);

  gtk_main ();
  grl_deinit ();

  return 0;
}
