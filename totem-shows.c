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
#include <string.h>

#define LOCAL_METADATA_ID   "grl-local-metadata"

#define THETVDB_ID    "grl-thetvdb"
#define TOSO_API_KEY  "3F476CEF2FBD0FB0"

#define TMDB_ID       "grl-tmdb"
#define TMDB_KEY      "719b9b296835b04cd919c4bf5220828a"

static gint gbl_num_tests;

static GrlRegistry *registry;
static GrlKeyID tvdb_poster_key = NULL;
static GrlKeyID tmdb_poster_key = NULL;

typedef struct _OperationSpec {
  GList      *list_medias;
  GtkBuilder *builder;
  guint       index_media;
} OperationSpec;

typedef struct _FetchOperation {
  GrlMedia      *media;
  OperationSpec *os;
} FetchOperation;

/* -------------------------------------------------------------------------- *
 * Utils functions
 * -------------------------------------------------------------------------- */

/* reduce the tests counter by one and check if we reached it to 0. */
static void
media_failed (void)
{
  gbl_num_tests--;
  if (gbl_num_tests == 0) {
    g_message ("All medias failed. Quit.");
    gtk_main_quit ();
  }
}

/* For GrlKeys that have several values, return all of them in one
 * string separated by comma; */
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

/* check_input_file
 * We accept file-path or tracker-urls.
 */
gchar *
check_input_file (const gchar *input)
{
  GFile *file;
  gchar *uri;

  if (input == NULL)
    return NULL;

  file = g_file_new_for_commandline_arg (input);
  uri = g_file_get_uri (file);
  g_object_unref (file);
  return uri;
}


/* -------------------------------------------------------------------------- *
 * UI functions
 * -------------------------------------------------------------------------- */

static void
set_media_content (GtkBuilder *builder,
                   GrlMedia   *media)
{
  gchar *img_path;
  GtkWidget *poster, *widget;
  GdkPixbuf *srcpixbuf, *dstpixbuf;
  const gchar *show, *overview, *title;
  GDateTime *released;
  gchar *str;
  GrlMediaVideo *video;
  gboolean is_media_show = FALSE;

  video = GRL_MEDIA_VIDEO (media);

  show = grl_media_video_get_show (video);
  is_media_show = (show != NULL) ? TRUE : FALSE;

  title = (is_media_show) ?
    grl_media_video_get_episode_title (GRL_MEDIA_VIDEO (media)) :
    grl_media_get_title (media);

  g_message ("[%s] set_media_content -> %s",
             (is_media_show) ? "Show" : "Movie", title);
  img_path = g_build_filename (g_get_tmp_dir (),
                               (is_media_show) ? show : title,
                               NULL);

  /* Connect signal handlers to the constructed widgets. */
  widget = GTK_WIDGET (gtk_builder_get_object (builder, "showname-label"));
  (is_media_show) ?
    gtk_label_set_text (GTK_LABEL (widget), show) :
    gtk_label_set_text (GTK_LABEL (widget), title);

  /* Get a scalated pixbuf from img file */
  poster = gtk_image_new_from_file (img_path);
  srcpixbuf = gtk_image_get_pixbuf (GTK_IMAGE (poster));
  dstpixbuf = gdk_pixbuf_scale_simple (srcpixbuf, 226, 333, GDK_INTERP_BILINEAR);
  gtk_image_clear (GTK_IMAGE (poster));
  g_object_unref (poster);
  g_clear_pointer (&img_path, g_free);

  /* Clear old image and set new pixbuf to it */
  poster = GTK_WIDGET (gtk_builder_get_object (builder, "poster-image"));
  gtk_image_clear (GTK_IMAGE (poster));
  gtk_image_set_from_pixbuf (GTK_IMAGE (poster), dstpixbuf);
  g_object_unref (dstpixbuf);

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "summary-data"));
  overview = grl_media_get_description (GRL_MEDIA (video));
  if (overview != NULL) {
    gtk_widget_set_visible (widget, TRUE);
    gtk_label_set_text (GTK_LABEL (widget), overview);
  } else {
    gtk_widget_set_visible (widget, FALSE);
  }

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "released-data"));
  released = grl_media_get_publication_date (media);
  if (released != NULL) {
    gtk_widget_set_visible (widget, TRUE);
    str = g_date_time_format (released, "%Y");
    gtk_label_set_text (GTK_LABEL (widget), str);
    g_free (str);
  } else {
    gtk_widget_set_visible (widget, FALSE);
  }

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "genre-data"));
  str = get_data_from_media (GRL_DATA (video), GRL_METADATA_KEY_GENRE);
  if (str != NULL) {
    gtk_widget_set_visible (widget, TRUE);
    gtk_label_set_text (GTK_LABEL (widget), str);
    g_clear_pointer (&str, g_free);
  } else {
    gtk_widget_set_visible (widget, FALSE);
  }

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "cast-data"));
  str = get_data_from_media (GRL_DATA (video), GRL_METADATA_KEY_PERFORMER);
  if (str != NULL) {
    gtk_widget_set_visible (widget, TRUE);
    gtk_label_set_text (GTK_LABEL (widget), str);
    g_clear_pointer (&str, g_free);
  } else {
    gtk_widget_set_visible (widget, FALSE);
  }

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "directors-data"));
  str = get_data_from_media (GRL_DATA (video), GRL_METADATA_KEY_DIRECTOR);
  if (str != NULL) {
    gtk_widget_set_visible (widget, TRUE);
    gtk_label_set_text (GTK_LABEL (widget), str);
    g_clear_pointer (&str, g_free);
  } else {
    gtk_widget_set_visible (widget, FALSE);
  }

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "authors-data"));
  str = get_data_from_media (GRL_DATA (video), GRL_METADATA_KEY_AUTHOR);
  if (str != NULL) {
    gtk_widget_set_visible (widget, TRUE);
    gtk_label_set_text (GTK_LABEL (widget), str);
    g_clear_pointer (&str, g_free);
  } else {
    gtk_widget_set_visible (widget, FALSE);
  }
}

static void
check_ui (OperationSpec *os)
{
  gint len = g_list_length (os->list_medias);
  gint index = os->index_media;
  GtkWidget *left_arrow, *right_arrow;

  /* Connect signal handlers to the constructed widgets. */
  left_arrow = GTK_WIDGET (gtk_builder_get_object (os->builder, "button-left"));
  right_arrow = GTK_WIDGET (gtk_builder_get_object (os->builder, "button-right"));

  if (index == 0)
    gtk_widget_set_visible (left_arrow, FALSE);
  else
    gtk_widget_set_visible (left_arrow, TRUE);

  if (len <= 1 || index == len - 1)
    gtk_widget_set_visible (right_arrow, FALSE);
  else
    gtk_widget_set_visible (right_arrow, TRUE);
}

static void
on_left (GtkStatusIcon *status_icon,
         GdkEvent      *event,
         gpointer       user_data)
{
  OperationSpec *os;
  GList *it;

  os = (OperationSpec *) user_data;
  os->index_media = (os->index_media == 0) ?
                     g_list_length (os->list_medias) - 1 : os->index_media - 1;

  it = g_list_nth (os->list_medias, os->index_media);
  set_media_content (os->builder, GRL_MEDIA (it->data));
  check_ui (os);
}

static void
on_right (GtkStatusIcon *status_icon,
          GdkEvent      *event,
          gpointer       user_data)
{
  OperationSpec *os;
  GList *it;

  os = (OperationSpec *) user_data;
  os->index_media = (os->index_media + 1) % g_list_length (os->list_medias);

  it = g_list_nth (os->list_medias, os->index_media);
  set_media_content (os->builder, GRL_MEDIA (it->data));
  check_ui (os);
}

/* Creates an empty UI. When grilo's resolve callback is called,
 * the UI will receive the data for the new medias. */
static OperationSpec *
build_ui (void)
{
  GtkWidget *widget;
  OperationSpec *os;

  os = g_slice_new0 (OperationSpec);
  os->list_medias = NULL;

  /* Construct a GtkBuilder instance and load our UI description */
  os->builder = gtk_builder_new ();
  gtk_builder_add_from_file (os->builder, "totem-shows.glade", NULL);

  /* Connect signal handlers to the constructed widgets. */
  widget = GTK_WIDGET (gtk_builder_get_object (os->builder, "main-window"));
  gtk_window_set_title (GTK_WINDOW (widget), "Totem TV Shows");
  g_signal_connect (widget, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  widget = GTK_WIDGET (gtk_builder_get_object (os->builder, "button-left"));
  g_signal_connect (widget, "clicked",
                    G_CALLBACK (on_left), os);

  widget = GTK_WIDGET (gtk_builder_get_object (os->builder, "button-right"));
  g_signal_connect (widget, "clicked",
                    G_CALLBACK (on_right), os);

  os->index_media = 0;
  check_ui (os);

  widget = GTK_WIDGET (gtk_builder_get_object (os->builder, "main-window"));
  gtk_widget_show_all (widget);
  return os;
}

static void
add_media_to_ui (OperationSpec *os, GrlMedia *media)
{
  if (os->list_medias == NULL)
    set_media_content (os->builder, media);

  os->list_medias = g_list_append (os->list_medias, media);
  check_ui (os);
}

/* -------------------------------------------------------------------------- *
 * Interacting with Grilo
 * -------------------------------------------------------------------------- */

static void
fetch_poster_done (GObject      *source_object,
                   GAsyncResult *res,
                   gpointer      user_data)
{
  const gchar *str;
  gchar *data, *img_path;
  gsize len;
  GError *err = NULL;
  FetchOperation *fo;

  grl_net_wc_request_finish (GRL_NET_WC (source_object),
                             res, &data, &len, &err);

  fo = (FetchOperation *) user_data;
  str = grl_media_video_get_show (GRL_MEDIA_VIDEO (fo->media));
  if (str == NULL)
    str = grl_media_get_title (fo->media);

  img_path = g_build_filename (g_get_tmp_dir (), str, NULL);

  if (err != NULL
      || !g_file_set_contents (img_path, data, len, &err))
    goto fetch_done;

  g_message ("Resolve[2] ok of '%s'", str);
  add_media_to_ui (fo->os, fo->media);

fetch_done:
  if (err != NULL) {
    g_warning ("Fetch image failed due: %s", err->message);
    g_error_free (err);
    g_object_unref (fo->media);
  }
  g_clear_pointer (&img_path, g_free);
  g_slice_free (FetchOperation, fo);
  return;
}

static void
resolve_media_done (GrlSource    *source,
                    guint         operation_id,
                    GrlMedia     *media,
                    gpointer      user_data,
                    const GError *error)
{
  const gchar *show;
  const gchar *title;
  const gchar *url;
  gchar *img_path;
  OperationSpec *os;

  if (error) {
    g_error ("Resolve operation failed. Reason: %s", error->message);
    g_object_unref (media);
    media_failed ();
    return;
  }

  os = (OperationSpec *) user_data;

  show = grl_media_video_get_show (GRL_MEDIA_VIDEO (media));
  title = grl_media_get_title (media);

  /* Get url to the poster of movie/series */
  url = (show != NULL) ?
    grl_data_get_string (GRL_DATA (media), tvdb_poster_key) :
    grl_data_get_string (GRL_DATA (media), tmdb_poster_key);

  if ((title == NULL && show == NULL) || url == NULL) {
    g_warning ("Can't find metdata from media with url: %s",
               grl_media_get_url (media));
    g_object_unref (media);
    media_failed ();
    return;
  }

  g_message ("POSTER URL for '%s' is '%s'",
             (show != NULL) ? show : title,
             url);

  img_path = g_build_filename (g_get_tmp_dir (),
                               (show != NULL) ? show : title,
                               NULL);
  if (url != NULL && !g_file_test (img_path, G_FILE_TEST_EXISTS)) {
    GrlNetWc *wc;
    FetchOperation *fo;

    wc = grl_net_wc_new ();
    fo = g_slice_new0 (FetchOperation);
    fo->media = media;
    fo->os = os;

    GRL_DEBUG ("url[1] %s", url);
    grl_net_wc_request_async (wc, url, NULL, fetch_poster_done, fo);
    g_object_unref (wc);
    g_free (img_path);
  } else {
    g_free (img_path);
    add_media_to_ui (os, media);
  }
}

static void
resolve_show (GrlMediaVideo *video,
              OperationSpec *os)
{
  GrlSource *source = NULL;
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;

  g_message ("TV SHOW: %s", grl_media_video_get_show (video));
  source = grl_registry_lookup_source (registry, THETVDB_ID);
  if (source == NULL) {
    g_critical ("Can't find the tvdb source");
    media_failed ();
    return;
  }

  tvdb_poster_key = grl_registry_lookup_metadata_key (registry, "thetvdb-poster");
  g_assert (tvdb_poster_key != GRL_METADATA_KEY_INVALID);

  caps = grl_source_get_caps (source, GRL_OP_RESOLVE);
  options = grl_operation_options_new (caps);
  grl_operation_options_set_flags (options, GRL_RESOLVE_NORMAL);

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_DESCRIPTION,
                                    GRL_METADATA_KEY_PERFORMER,
                                    GRL_METADATA_KEY_DIRECTOR,
                                    GRL_METADATA_KEY_AUTHOR,
                                    GRL_METADATA_KEY_GENRE,
                                    GRL_METADATA_KEY_PUBLICATION_DATE,
                                    GRL_METADATA_KEY_EPISODE_TITLE,
                                    tvdb_poster_key,
                                    GRL_METADATA_KEY_INVALID);
  grl_source_resolve (source,
                      GRL_MEDIA (video),
                      keys,
                      options,
                      resolve_media_done,
                      os);
  g_object_unref (options);
  g_list_free (keys);
}

static void
resolve_movie (GrlMediaVideo *video,
               OperationSpec *os)
{
  GrlSource *source = NULL;
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;

  g_message ("MOVIE: %s", grl_media_get_title (GRL_MEDIA (video)));

  source = grl_registry_lookup_source (registry, TMDB_ID);
  if (source == NULL) {
    g_critical ("Can't find the tvdb source");
    media_failed ();
    return;
  }

  tmdb_poster_key = grl_registry_lookup_metadata_key (registry, "tmdb-poster");
  g_assert (tmdb_poster_key != GRL_METADATA_KEY_INVALID);

  caps = grl_source_get_caps (source, GRL_OP_RESOLVE);
  options = grl_operation_options_new (caps);
  grl_operation_options_set_flags (options, GRL_RESOLVE_NORMAL);

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_DESCRIPTION,
                                    GRL_METADATA_KEY_PERFORMER,
                                    GRL_METADATA_KEY_DIRECTOR,
                                    GRL_METADATA_KEY_AUTHOR,
                                    GRL_METADATA_KEY_GENRE,
                                    GRL_METADATA_KEY_PUBLICATION_DATE,
                                    tmdb_poster_key,
                                    GRL_METADATA_KEY_INVALID);
  grl_source_resolve (source,
                      GRL_MEDIA (video),
                      keys,
                      options,
                      resolve_media_done,
                      os);
  g_object_unref (options);
  g_list_free (keys);
}

static void
resolve_urls_done (GrlSource    *source,
                   guint         operation_id,
                   GrlMedia     *media,
                   gpointer      user_data,
                   const GError *error)
{
  OperationSpec *os;
  GrlMediaVideo *video;

  if (error != NULL) {
    g_message ("local-metadata failed: %s", error->message);
    media_failed ();
    return;
  }

  os = (OperationSpec *) user_data;
  video = GRL_MEDIA_VIDEO (media);

  if (grl_media_video_get_show (video) != NULL) {
    /* resolve the tv show with the tvdb source */
    resolve_show (video, os);
  } else if (grl_media_get_title (media) != NULL) {
    /* resolve the movie with the tmdb source */
    resolve_movie (video, os);
  } else {
    g_message ("local-metadata can't define it as movie or series: %s",
               grl_media_get_url (media));
    media_failed ();
  }
}

static gboolean
resolve_urls (gchar         *strv[],
              OperationSpec *os)
{
  GrlSource *source;
  GrlMediaVideo *video;
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;
  gint i, strv_len;

  gbl_num_tests = 0;
  source = grl_registry_lookup_source (registry, LOCAL_METADATA_ID);
  if (source == NULL) {
    g_critical ("Can't find local-metadata source");
    return FALSE;
  }

  caps = grl_source_get_caps (source, GRL_OP_RESOLVE);
  options = grl_operation_options_new (caps);
  grl_operation_options_set_flags (options, GRL_RESOLVE_NORMAL);

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_TITLE,
                                    GRL_METADATA_KEY_EPISODE_TITLE,
                                    GRL_METADATA_KEY_SHOW,
                                    GRL_METADATA_KEY_SEASON,
                                    GRL_METADATA_KEY_EPISODE,
                                    GRL_METADATA_KEY_INVALID);

  strv_len = g_strv_length (strv);
  for (i = 0; i < strv_len; i++) {
    gchar *tracker_url;

    tracker_url = check_input_file (strv[i]);
    if (tracker_url == NULL) {
      g_message ("IGNORED: %s", strv[i]);
      continue;
    }

    video = GRL_MEDIA_VIDEO (grl_media_video_new ());

    /* We want to extract all metadata from file's name */
    grl_data_set_boolean (GRL_DATA (video),
                          GRL_METADATA_KEY_TITLE_FROM_FILENAME,
                          TRUE);
    grl_media_set_url (GRL_MEDIA (video), tracker_url);

    gbl_num_tests++;
    grl_source_resolve (source,
                        GRL_MEDIA (video),
                        keys,
                        options,
                        resolve_urls_done,
                        os);
    g_free (tracker_url);
  }

  g_object_unref (options);
  g_list_free (keys);
  return (gbl_num_tests > 0) ? TRUE : FALSE;
}

/* -------------------------------------------------------------------------- *
 * Main
 * -------------------------------------------------------------------------- */

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

  /* Add API-KEY to The TMDB source */
  config = grl_config_new (TMDB_ID, NULL);
  grl_config_set_api_key (config, TMDB_KEY);
  grl_registry_add_config (registry, config, &error);
  g_assert_no_error (error);

  grl_registry_load_all_plugins (registry, &error);
  g_assert_no_error (error);
}

static gchar **
read_input (gint   argc,
            gchar *argv[])
{
  GError *error = NULL;
  gchar **strv = NULL;
  GOptionContext *context = NULL;
  GOptionEntry entries[] = {
    { G_OPTION_REMAINING, '\0', 0,
      G_OPTION_ARG_FILENAME_ARRAY, &strv,
      "List of paths to tv shows",
      NULL },
    { NULL }
  };

  context = g_option_context_new ("OPERATION PARAMETERS...");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_add_group (context, grl_init_get_option_group ());
  g_option_context_set_summary (context,
                                "\t<list of paths to vides of tv shows\n");
  g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  return strv;
}

gint
main (gint   argc,
      gchar *argv[])
{
  gchar **strv;
  OperationSpec *os;
  gboolean resolving;

  gtk_init (&argc, &argv);
  grl_init (&argc, &argv);

  strv = read_input (argc, argv);
  if (strv == NULL) {
    g_critical ("No valid input.");
    return 1;
  }

  os = build_ui ();
  if (os == NULL) {
    g_critical ("Can't create the UI.");
    g_strfreev (strv);
    return 2;
  }

  config_and_load_plugins ();
  resolving = resolve_urls (strv, os);

  if (resolving)
    gtk_main ();

  grl_deinit ();
  g_strfreev (strv);
  g_object_unref (os->builder);
  g_list_free_full (os->list_medias, g_object_unref);
  g_slice_free (OperationSpec, os);

  return 0;
}
