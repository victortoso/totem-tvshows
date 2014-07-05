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

#define THETVDB_ID           "grl-thetvdb"
#define TOSO_API_KEY         "3F476CEF2FBD0FB0"

static gint gbl_num_tests;

static GrlRegistry *registry;
static GrlKeyID tvdb_poster_key;
static GrlKeyID guest_stars_key;

typedef struct _OperationSpec {
  GList      *list_medias;
  GtkBuilder *builder;
  guint       index_media;
} OperationSpec;

typedef struct _FetchOperation {
  GrlMedia      *media;
  OperationSpec *os;
} FetchOperation;

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
set_media_content (GtkBuilder *builder,
                   GrlMedia   *media)
{
  gchar *img_path;
  GtkWidget *window, *poster, *widget;
  GdkPixbuf *pixbuf;
  const gchar *show, *overview, *title;
  GDateTime *released;
  gchar *str;
  GrlMediaVideo *video;

  video = GRL_MEDIA_VIDEO (media);
  title = grl_media_get_title (media);
  g_message ("set_media_content -> %s", title);
  show = grl_media_video_get_show (video);
  img_path = g_build_filename (g_get_tmp_dir (), title, NULL);

  /* Connect signal handlers to the constructed widgets. */
  widget = GTK_WIDGET (gtk_builder_get_object (builder, "showname-label"));
  gtk_label_set_text (GTK_LABEL (widget), show);

  poster = gtk_image_new_from_file (img_path);
  pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (poster));
  pixbuf = gdk_pixbuf_scale_simple (pixbuf, 226, 333, GDK_INTERP_BILINEAR);
  poster = GTK_WIDGET (gtk_builder_get_object (builder, "poster-image"));
  gtk_image_set_from_pixbuf (GTK_IMAGE (poster), pixbuf);
  g_clear_pointer (&img_path, g_free);

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "summary-data"));
  overview = grl_media_get_description (GRL_MEDIA (video));
  gtk_label_set_text (GTK_LABEL (widget), overview);

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "released-data"));
  released = grl_media_get_publication_date (media);
  if (released != NULL) {
    str = g_date_time_format (released, "%Y");
    gtk_label_set_text (GTK_LABEL (widget), str);
    g_free (str);
  }

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "cast-data"));
  str = get_data_from_media (GRL_DATA (video), GRL_METADATA_KEY_PERFORMER);
  gtk_label_set_text (GTK_LABEL (widget), str);
  g_clear_pointer (&str, g_free);

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "directors-data"));
  str = get_data_from_media (GRL_DATA (video), GRL_METADATA_KEY_DIRECTOR);
  gtk_label_set_text (GTK_LABEL (widget), str);
  g_clear_pointer (&str, g_free);

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "guests-data"));
  str = get_data_from_media (GRL_DATA (video), guest_stars_key);
  if (str != NULL) {
    gtk_label_set_text (GTK_LABEL (widget), str);
    g_clear_pointer (&str, g_free);
  }

  window = GTK_WIDGET (gtk_builder_get_object (builder, "main-window"));
  gtk_widget_show_all (window);
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

  if (index == len - 1)
    gtk_widget_set_visible (right_arrow, FALSE);
  else if (len > 1)
    gtk_widget_set_visible (right_arrow, TRUE);
}

static gboolean
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

static gboolean
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

static void
build_ui (OperationSpec *os)
{
  GtkWidget *widget;
  GtkCssProvider *css;
  gboolean succeed;
  GError *err = NULL;

  if (os->builder != NULL)
    return;

  /* Construct a GtkBuilder instance and load our UI description */
  os->builder = gtk_builder_new ();
  gtk_builder_add_from_file (os->builder, "totem-shows.glade", NULL);

  css = gtk_css_provider_new ();
  succeed = gtk_css_provider_load_from_path (css, "totem-shows.css", &err);
  if (succeed) {
    GdkDisplay *display;
    GdkScreen *screen;

    display = gdk_display_get_default ();
    screen = gdk_display_get_default_screen (display);
    gtk_style_context_add_provider_for_screen (screen,
                                               GTK_STYLE_PROVIDER (css),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_message ("CSS Loaded");
  } else {
    g_warning ("Can't load css: %s", err->message);
    g_error_free (err);
  }
  g_object_unref (css);

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

  /* First Element */
  g_message ("size of list: %d", g_list_length (os->list_medias));
  os->index_media = 0;
  set_media_content (os->builder,
                     GRL_MEDIA (g_list_first (os->list_medias)->data));
  check_ui (os);
}

static void
add_media_to_ui (OperationSpec *os, GrlMedia *media)
{
  os->list_medias = g_list_append (os->list_medias, media);

  if (os->builder == NULL)
    build_ui (os);
  else
    check_ui (os);
}

static void
fetch_poster_done (GObject      *source_object,
                   GAsyncResult *res,
                   gpointer      user_data)
{
  const gchar *title;
  gchar *data, *img_path;
  gsize len;
  GError *err = NULL;
  FetchOperation *fo;

  grl_net_wc_request_finish (GRL_NET_WC (source_object),
                             res, &data, &len, &err);

  fo = (FetchOperation *) user_data;
  title = grl_media_get_title (fo->media);
  img_path = g_build_filename (g_get_tmp_dir (), title, NULL);

  if (err != NULL
      || !g_file_set_contents(img_path, data, len, &err))
    goto fetch_done;

  g_message ("Resolve[2] ok of '%s'", title);
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
resolve_done (GrlSource    *source,
              guint         operation_id,
              GrlMedia     *media,
              gpointer      user_data,
              const GError *error)
{
  const gchar *title;
  gchar *img_path;
  OperationSpec *os;

  if (error)
    g_error ("Resolve operation failed. Reason: %s", error->message);

  os = (OperationSpec *) user_data;

  title = grl_media_get_title (media);
  if (title == NULL) {
    g_warning ("Can't find metdata from media with url: %s",
               grl_media_get_url (media));
    g_object_unref (media);

    gbl_num_tests--;
    if (gbl_num_tests == 0)
      gtk_main_quit ();

    return;
  }

  g_message ("Resolve[1] ok of '%s'", title);
  img_path = g_build_filename (g_get_tmp_dir (), title, NULL);
  if (!g_file_test (img_path, G_FILE_TEST_EXISTS)) {
    GrlNetWc *wc;
    const gchar *url;
    FetchOperation *fo;

    url = grl_data_get_string (GRL_DATA (media), tvdb_poster_key);
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
    g_message ("GoGo build_ui of '%s'", title);
    add_media_to_ui (os, media);
  }
}

/* check_input_file
 * We accept file-path or tracker-urls.
 */
gchar *
check_input_file (const gchar *input)
{
  gchar *filepath;
  gchar *url = NULL;

  if (input == NULL)
    return;

  if (strlen (input) > 8
      && g_ascii_strncasecmp ("file://", input, 7) == 0)
    /* already a url, check if the file exists */
    filepath = g_uri_unescape_string (input + 7, NULL);
  else
    filepath = g_uri_unescape_string (input, NULL);

  if (g_file_test (filepath, G_FILE_TEST_EXISTS)) {
    gchar *filepath_escape = g_uri_escape_string (filepath, "/", FALSE);
    url = g_strdup_printf ("file://%s", filepath_escape);
    g_free (filepath_escape);
  } else
    g_print ("File not found: %s \n", filepath);

  g_free (filepath);
  return url;
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
  OperationSpec *os;

  gbl_num_tests = 0;
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

  os = g_slice_new0 (OperationSpec);
  for (i = 1; i < argc; i++) {
    gchar *tracker_url;

    tracker_url = check_input_file (argv[i]);
    if (tracker_url == NULL)
      continue;

    gbl_num_tests++;
    video = GRL_MEDIA_VIDEO (grl_media_video_new ());
    grl_media_set_url (GRL_MEDIA (video), tracker_url);
    grl_source_resolve (source,
                        GRL_MEDIA (video),
                        keys,
                        options,
                        resolve_done,
                        os);
    g_free (tracker_url);
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
