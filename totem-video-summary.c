/*
 * Copyright (C) 2015 Victor Toso.
 *
 * Contact: Victor Toso <me@victortoso.com>
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

#include "totem-video-summary.h"

#include <net/grl-net.h>
#include <string.h>

struct _TotemVideosSummaryPrivate
{
  GrlRegistry *registry;
  GrlSource *tmdb_source;
  GrlSource *tvdb_source;
  GrlSource *video_title_parsing_source;
  GrlKeyID tvdb_poster_key;
  GrlKeyID tmdb_poster_key;

  GtkImage *poster;
  GtkLabel *title;
  GtkLabel *summary;
  GtkLabel *released;
  GtkLabel *genre;
  GtkLabel *cast;
  GtkLabel *directors;
  GtkLabel *authors;

  GtkButton *arrow_right;
  GtkButton *arrow_left;

  GList *current_video;
  GList *videos;
};

typedef struct
{
  TotemVideosSummary *totem_videos_summary;
  GrlMediaVideo      *video;

  gchar    *poster_path;
  gboolean  is_tv_show;
} OperationSpec;

typedef struct
{
  gchar    *title;
  gchar    *description;
  gchar    *genre;
  gchar    *performer;
  gchar    *director;
  gchar    *author;
  gchar    *poster_path;
  gboolean  is_tv_show;
} VideoSummaryData;

static gchar *get_data_from_media (GrlData *data, GrlKeyID key);

G_DEFINE_TYPE_WITH_PRIVATE (TotemVideosSummary, totem_videos_summary, GTK_TYPE_GRID);

/* -------------------------------------------------------------------------- *
 * Internal / Helpers
 * -------------------------------------------------------------------------- */

static void
operation_spec_free (OperationSpec *os)
{
  g_clear_object (&os->video);
  g_clear_pointer (&os->poster_path, g_free);
  g_slice_free (OperationSpec, os);
}

static void
video_summary_data_free (gpointer gp)
{
  VideoSummaryData *data = gp;

  g_free (data->title);
  g_free (data->description);
  g_free (data->genre);
  g_free (data->performer);
  g_free (data->director);
  g_free (data->author);
  g_free (data->poster_path);
  g_slice_free (VideoSummaryData, data);
}

static void
totem_videos_summary_set_data_content (TotemVideosSummary *self,
                                       VideoSummaryData   *data)
{
  if (data->title)
    gtk_label_set_text (self->priv->title, data->title);

  if (data->description) {
    gtk_label_set_text (self->priv->summary, data->description);
    gtk_widget_set_visible (GTK_WIDGET(self->priv->summary), TRUE);
  } else {
    gtk_widget_set_visible (GTK_WIDGET(self->priv->summary), FALSE);
  }

  if (data->genre) {
    gtk_label_set_text (self->priv->genre, data->genre);
    gtk_widget_set_visible (GTK_WIDGET(self->priv->genre), TRUE);
  } else {
    gtk_widget_set_visible (GTK_WIDGET(self->priv->genre), FALSE);
  }

  if (data->performer) {
    gtk_label_set_text (self->priv->cast, data->performer);
    gtk_widget_set_visible (GTK_WIDGET(self->priv->cast), TRUE);
  } else {
    gtk_widget_set_visible (GTK_WIDGET(self->priv->cast), FALSE);
  }

  if (data->director) {
    gtk_label_set_text (self->priv->directors, data->director);
    gtk_widget_set_visible (GTK_WIDGET(self->priv->directors), TRUE);
  } else {
    gtk_widget_set_visible (GTK_WIDGET(self->priv->directors), FALSE);
  }

  if (data->author) {
    gtk_label_set_text (self->priv->authors, data->author);
    gtk_widget_set_visible (GTK_WIDGET(self->priv->authors), TRUE);
  } else {
    gtk_widget_set_visible (GTK_WIDGET(self->priv->authors), FALSE);
  }

  if (data->poster_path != NULL) {
    GtkImage *poster;
    GdkPixbuf *srcpixbuf, *dstpixbuf;

    /* Get a scaled pixbuf from img file */
    poster = GTK_IMAGE(gtk_image_new_from_file (data->poster_path));
    srcpixbuf = gtk_image_get_pixbuf (poster);
    dstpixbuf = gdk_pixbuf_scale_simple (srcpixbuf, 226, 333, GDK_INTERP_BILINEAR);
    g_object_unref (poster);

    /* Set new pixbuf */
    gtk_image_set_from_pixbuf (self->priv->poster, dstpixbuf);
    g_object_unref (dstpixbuf);
  }

  /*
  date = grl_media_get_publication_date (GRL_MEDIA (video));
  if (released != NULL) {
    str = g_date_time_format (released, "%Y");
    if (str != NULL) {
      gtk_label_set_text (self->priv->released, str);
      g_free (str);
    }
    gtk_widget_set_visible (GTK_WIDGET(self->priv->released), (str != NULL));
  } else {
    gtk_widget_set_visible (GTK_WIDGET(self->priv->released), FALSE);
  }
  */
}

static void
totem_videos_summary_set_basic_content (TotemVideosSummary *self,
                                        GrlMediaVideo      *video)
{
  const gchar *title;
  gboolean is_tv_show = (grl_media_video_get_show (video) != NULL);

  if (is_tv_show)
    title = grl_media_video_get_episode_title (video);
  else
    title = grl_media_get_title (GRL_MEDIA (video));

  if (title)
    gtk_label_set_text (self->priv->title, title);
}

static void
totem_videos_summary_update_ui (TotemVideosSummary *self)
{
  VideoSummaryData *data;
  gboolean visible;

  if (self->priv->videos == NULL) {
    g_warning ("Don't have any video to display");
    return;
  } else if (self->priv->current_video == NULL) {
    self->priv->current_video = self->priv->videos;
  }

  data = self->priv->current_video->data;
  totem_videos_summary_set_data_content (self, data);

  /* For optimization, list is inverted */
  visible = (self->priv->current_video->next != NULL);
  gtk_widget_set_visible (GTK_WIDGET(self->priv->arrow_left), visible);

  visible = (self->priv->current_video->prev != NULL);
  gtk_widget_set_visible (GTK_WIDGET(self->priv->arrow_right), visible);
}

static void
change_video_cb (GtkWidget *button,
                 gpointer   user_data)
{
  TotemVideosSummary *self;
  const gchar *name;
  GList *next_video = NULL;

  self = user_data;
  name = gtk_widget_get_name (button);

  /* List is inverted */
  if (g_str_equal (name, "forward-arrow")) {
    next_video = g_list_previous (self->priv->current_video);
    if (!next_video)
      next_video = g_list_last (self->priv->videos);
  } else {
    next_video = g_list_next (self->priv->current_video);
    if (!next_video)
      next_video = self->priv->videos;
  }

  self->priv->current_video = next_video;
  totem_videos_summary_update_ui (self);
}

static void
add_video_to_summary_and_free (OperationSpec *os)
{
  TotemVideosSummary *self = os->totem_videos_summary;
  VideoSummaryData *data;

  data = g_slice_new0 (VideoSummaryData);
  data->is_tv_show = (grl_media_video_get_show (os->video) != NULL);
  data->description = g_strdup (grl_media_get_description (GRL_MEDIA (os->video)));
  data->genre = get_data_from_media (GRL_DATA (os->video), GRL_METADATA_KEY_GENRE);
  data->performer = get_data_from_media (GRL_DATA (os->video),
                                         GRL_METADATA_KEY_PERFORMER);
  data->director = get_data_from_media (GRL_DATA (os->video),
                                        GRL_METADATA_KEY_DIRECTOR);
  data->author = get_data_from_media (GRL_DATA (os->video),
                                      GRL_METADATA_KEY_AUTHOR);
  data->poster_path = g_strdup (os->poster_path);

  if (data->is_tv_show)
    data->title = g_strdup (grl_media_video_get_episode_title (os->video));
  else
    data->title = g_strdup (grl_media_get_title (GRL_MEDIA (os->video)));

  self->priv->videos = g_list_prepend (self->priv->videos, data);
  totem_videos_summary_update_ui (self);
  operation_spec_free (os);
}

static void
resolve_poster_done (GObject      *source_object,
                     GAsyncResult *res,
                     gpointer      user_data)
{
  OperationSpec *os;
  gchar *data;
  gsize len;
  GError *err = NULL;

  os = user_data;
  grl_net_wc_request_finish (GRL_NET_WC (source_object),
                             res, &data, &len, &err);
  if (err != NULL) {
    g_warning ("Fetch image failed due: %s", err->message);
    g_error_free (err);
  } else {
    g_file_set_contents (os->poster_path, data, len, &err);
  }

  /* Update interface */
  add_video_to_summary_and_free (os);
}

static void
resolve_metadata_done (GrlSource    *source,
                       guint         operation_id,
                       GrlMedia     *media,
                       gpointer      user_data,
                       const GError *error)
{
  TotemVideosSummary *self;
  OperationSpec *os = user_data;
  const gchar *title, *poster_url;

  if (error) {
    g_warning ("Resolve operation failed: %s", error->message);
    operation_spec_free (os);
    return;
  }

  self = os->totem_videos_summary;

  if (os->is_tv_show)
    title = grl_media_video_get_show (GRL_MEDIA_VIDEO (media));
  else
    title = grl_media_get_title (media);

  if (title == NULL) {
    g_warning ("Basic information is missing - no title");
    operation_spec_free (os);
    return;
  }

  if (os->is_tv_show)
    poster_url = grl_data_get_string (GRL_DATA (media), self->priv->tvdb_poster_key);
  else
    poster_url = grl_data_get_string (GRL_DATA (media), self->priv->tmdb_poster_key);

  if (poster_url != NULL) {
    os->poster_path = g_build_filename (g_get_tmp_dir (), title, NULL);
    if (!g_file_test (os->poster_path, G_FILE_TEST_EXISTS)) {
      GrlNetWc *wc = grl_net_wc_new ();
      grl_net_wc_request_async (wc, poster_url, NULL, resolve_poster_done, os);
      g_object_unref (wc);
      return;
    }
  }

  add_video_to_summary_and_free (os);
}

static void
resolve_by_tmdb (OperationSpec *os)
{
  TotemVideosSummary *self;
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;

  self = os->totem_videos_summary;

  caps = grl_source_get_caps (self->priv->tmdb_source, GRL_OP_RESOLVE);
  options = grl_operation_options_new (caps);
  grl_operation_options_set_resolution_flags (options, GRL_RESOLVE_NORMAL);

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_DESCRIPTION,
                                    GRL_METADATA_KEY_PERFORMER,
                                    GRL_METADATA_KEY_DIRECTOR,
                                    GRL_METADATA_KEY_AUTHOR,
                                    GRL_METADATA_KEY_GENRE,
                                    GRL_METADATA_KEY_PUBLICATION_DATE,
                                    self->priv->tmdb_poster_key,
                                    GRL_METADATA_KEY_INVALID);
  grl_source_resolve (self->priv->tmdb_source,
                      GRL_MEDIA (os->video),
                      keys,
                      options,
                      resolve_metadata_done,
                      os);
  g_object_unref (options);
  g_list_free (keys);
}

static void
resolve_by_the_tvdb (OperationSpec *os)
{
  TotemVideosSummary *self;
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;

  self = os->totem_videos_summary;
  caps = grl_source_get_caps (self->priv->tvdb_source, GRL_OP_RESOLVE);
  options = grl_operation_options_new (caps);
  grl_operation_options_set_resolution_flags (options, GRL_RESOLVE_NORMAL);

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_DESCRIPTION,
                                    GRL_METADATA_KEY_PERFORMER,
                                    GRL_METADATA_KEY_DIRECTOR,
                                    GRL_METADATA_KEY_AUTHOR,
                                    GRL_METADATA_KEY_GENRE,
                                    GRL_METADATA_KEY_PUBLICATION_DATE,
                                    GRL_METADATA_KEY_EPISODE_TITLE,
                                    self->priv->tvdb_poster_key,
                                    GRL_METADATA_KEY_INVALID);
  grl_source_resolve (self->priv->tvdb_source,
                      GRL_MEDIA (os->video),
                      keys,
                      options,
                      resolve_metadata_done,
                      os);
  g_object_unref (options);
  g_list_free (keys);
}

static void
resolve_by_video_title_parsing_done (GrlSource    *source,
                                     guint         operation_id,
                                     GrlMedia     *media,
                                     gpointer      user_data,
                                     const GError *error)
{
  TotemVideosSummary *self;
  OperationSpec *os = user_data;

  if (error != NULL) {
    g_warning ("video-title-parsing failed: %s", error->message);
    operation_spec_free (os);
    return;
  }

  self = os->totem_videos_summary;

  if (grl_media_video_get_show (GRL_MEDIA_VIDEO(media)) != NULL) {
    os->is_tv_show = TRUE;
    totem_videos_summary_set_basic_content (self, os->video);
    resolve_by_the_tvdb (os);
    return;
  }

  if (grl_media_get_title (media) != NULL) {
    os->is_tv_show = FALSE;
    totem_videos_summary_set_basic_content (self, os->video);
    resolve_by_tmdb (os);
    return;
  }

  g_warning ("video type is not defined: %s", grl_media_get_url (media));
  operation_spec_free (os);
}

static void
resolve_by_video_title_parsing (OperationSpec *os)
{
  TotemVideosSummary *self;
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;

  self = os->totem_videos_summary;
  caps = grl_source_get_caps (self->priv->video_title_parsing_source, GRL_OP_RESOLVE);
  options = grl_operation_options_new (caps);
  grl_operation_options_set_resolution_flags (options, GRL_RESOLVE_NORMAL);

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_TITLE,
                                    GRL_METADATA_KEY_EPISODE_TITLE,
                                    GRL_METADATA_KEY_SHOW,
                                    GRL_METADATA_KEY_SEASON,
                                    GRL_METADATA_KEY_EPISODE,
                                    GRL_METADATA_KEY_INVALID);

  /* We want to extract all metadata from file's name */
  grl_data_set_boolean (GRL_DATA (os->video),
                        GRL_METADATA_KEY_TITLE_FROM_FILENAME,
                        TRUE);
  grl_source_resolve (self->priv->video_title_parsing_source,
                      GRL_MEDIA (os->video),
                      keys,
                      options,
                      resolve_by_video_title_parsing_done,
                      os);
  g_object_unref (options);
  g_list_free (keys);
}

/* For GrlKeys that have several values, return all of them in one
 * string separated by comma; */
static gchar *
get_data_from_media (GrlData *data,
                     GrlKeyID key)
{
  gint i, len;
  GString *s;

  len = grl_data_length (data, key);
  if (len <= 0)
    return NULL;

  s = g_string_new ("");
  for (i = 0; i < len; i++) {
    GrlRelatedKeys *relkeys;
    const gchar *element;

    relkeys = grl_data_get_related_keys (data, key, i);
    element = grl_related_keys_get_string (relkeys, key);

    if (i > 0)
      g_string_append (s, ", ");
    g_string_append (s, element);
  }
  return g_string_free (s, FALSE);
}

/* -------------------------------------------------------------------------- *
 * External
 * -------------------------------------------------------------------------- */

TotemVideosSummary *
totem_videos_summary_new (void)
{
  TotemVideosSummary *self;
  GrlSource *source;
  GrlRegistry *registry;

  self = g_object_new (TOTEM_TYPE_VIDEOS_SUMMARY, NULL);
  registry = grl_registry_get_default();
  self->priv->registry = registry;

  /* Those plugins should be loaded as requirement */
  source = grl_registry_lookup_source (registry, "grl-tmdb");
  g_return_val_if_fail (source != NULL, NULL);
  self->priv->tmdb_source = source;

  source = grl_registry_lookup_source (registry, "grl-thetvdb");
  g_return_val_if_fail (source != NULL, NULL);
  self->priv->tvdb_source = source;

  source = grl_registry_lookup_source (registry, "grl-video-title-parsing");
  g_return_val_if_fail (source != NULL, NULL);
  self->priv->video_title_parsing_source = source;

  self->priv->tvdb_poster_key = grl_registry_lookup_metadata_key (registry, "thetvdb-poster");
  self->priv->tmdb_poster_key = grl_registry_lookup_metadata_key (registry, "tmdb-poster");

  /* FIXME: Disable movie/series if plugin is not loaded correctly or return NULL in case of
   * all of them. */
  g_return_val_if_fail (self->priv->tvdb_poster_key != GRL_METADATA_KEY_INVALID, NULL);
  g_return_val_if_fail (self->priv->tmdb_poster_key != GRL_METADATA_KEY_INVALID, NULL);

  return self;
}

gboolean
totem_videos_summary_add_video (TotemVideosSummary *self,
                                GrlMediaVideo      *video)
{
  const gchar *url;
  OperationSpec *os;

  g_return_val_if_fail (TOTEM_IS_VIDEOS_SUMMARY (self), FALSE);
  g_return_val_if_fail (video != NULL, FALSE);

  url = grl_media_get_url (GRL_MEDIA (video));
  if (url == NULL) {
    g_warning ("Video does not have url: can't initialize totem-video-summary");
    return FALSE;
  }

  if (!g_file_test (url, G_FILE_TEST_EXISTS)) {
    g_warning ("Video file does not exist");
    return FALSE;
  }

  os = g_slice_new0 (OperationSpec);
  os->totem_videos_summary = self;
  os->video = g_object_ref (video);
  resolve_by_video_title_parsing (os);
  return TRUE;
}

/* -------------------------------------------------------------------------- *
 * Object
 * -------------------------------------------------------------------------- */

static void
totem_videos_summary_finalize (GObject *object)
{
  TotemVideosSummary *self = TOTEM_VIDEOS_SUMMARY (object);

  if (self->priv->videos) {
    g_list_free_full (self->priv->videos, video_summary_data_free);
    self->priv->videos = NULL;
  }

  G_OBJECT_CLASS (totem_videos_summary_parent_class)->finalize (object);
}

static void
totem_videos_summary_init (TotemVideosSummary *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  self->priv = totem_videos_summary_get_instance_private (self);

  g_signal_connect (self->priv->arrow_right, "clicked", G_CALLBACK (change_video_cb), self);
  g_signal_connect (self->priv->arrow_left, "clicked", G_CALLBACK (change_video_cb), self);
}

static void
totem_videos_summary_class_init (TotemVideosSummaryClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->finalize = totem_videos_summary_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/totem/grilo/totem-video-summary.ui");
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, poster);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, title);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, summary);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, released);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, genre);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, cast);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, directors);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, authors);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, arrow_left);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, arrow_right);
}
