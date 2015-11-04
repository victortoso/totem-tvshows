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

  GrlMediaVideo *video;
  gchar *poster_path;
  gboolean is_tv_show;
};

static gchar *get_data_from_media (GrlData *data, GrlKeyID key);

G_DEFINE_TYPE_WITH_PRIVATE (TotemVideosSummary, totem_videos_summary, GTK_TYPE_GRID);

/* -------------------------------------------------------------------------- *
 * Internal / Helpers
 * -------------------------------------------------------------------------- */

static void
totem_videos_summary_set_content (TotemVideosSummary *grid)
{
  const gchar *title, *description;
  //GDateTime *date;
  gchar *str;

  if (grid->priv->is_tv_show)
    title = grl_media_video_get_episode_title (grid->priv->video);
  else
    title = grl_media_get_title (GRL_MEDIA (grid->priv->video));

  if (title)
    gtk_label_set_text (grid->priv->title, title);

  description = grl_media_get_description (GRL_MEDIA (grid->priv->video));
  if (description != NULL) {
    gtk_widget_set_visible (GTK_WIDGET(grid->priv->summary), TRUE);
    gtk_label_set_text (grid->priv->summary, description);
  } else {
    gtk_widget_set_visible (GTK_WIDGET(grid->priv->summary), FALSE);
  }

  /*
  date = grl_media_get_publication_date (GRL_MEDIA (grid->priv->video));
  if (released != NULL) {
    str = g_date_time_format (released, "%Y");
    if (str != NULL) {
      gtk_label_set_text (grid->priv->released, str);
      g_free (str);
    }
    gtk_widget_set_visible (GTK_WIDGET(grid->priv->released), (str != NULL));
  } else {
    gtk_widget_set_visible (GTK_WIDGET(grid->priv->released), FALSE);
  }
  */

  str = get_data_from_media (GRL_DATA (grid->priv->video), GRL_METADATA_KEY_GENRE);
  if (str != NULL) {
    gtk_label_set_text (grid->priv->genre, str);
    g_clear_pointer (&str, g_free);
    gtk_widget_set_visible (GTK_WIDGET(grid->priv->genre), TRUE);
  } else {
    gtk_widget_set_visible (GTK_WIDGET(grid->priv->genre), FALSE);
  }

  str = get_data_from_media (GRL_DATA (grid->priv->video), GRL_METADATA_KEY_PERFORMER);
  if (str != NULL) {
    gtk_label_set_text (grid->priv->cast, str);
    g_clear_pointer (&str, g_free);
    gtk_widget_set_visible (GTK_WIDGET(grid->priv->cast), TRUE);
  } else {
    gtk_widget_set_visible (GTK_WIDGET(grid->priv->cast), FALSE);
  }

  str = get_data_from_media (GRL_DATA (grid->priv->video), GRL_METADATA_KEY_DIRECTOR);
  if (str != NULL) {
    gtk_label_set_text (grid->priv->directors, str);
    g_clear_pointer (&str, g_free);
    gtk_widget_set_visible (GTK_WIDGET(grid->priv->directors), TRUE);
  } else {
    gtk_widget_set_visible (GTK_WIDGET(grid->priv->directors), FALSE);
  }

  str = get_data_from_media (GRL_DATA (grid->priv->video), GRL_METADATA_KEY_AUTHOR);
  if (str != NULL) {
    gtk_label_set_text (grid->priv->authors, str);
    g_clear_pointer (&str, g_free);
    gtk_widget_set_visible (GTK_WIDGET(grid->priv->authors), TRUE);
  } else {
    gtk_widget_set_visible (GTK_WIDGET(grid->priv->authors), FALSE);
  }

  if (grid->priv->poster_path != NULL) {
    GtkImage *poster;
    GdkPixbuf *srcpixbuf, *dstpixbuf;

    /* Get a scalated pixbuf from img file */
    poster = GTK_IMAGE(gtk_image_new_from_file (grid->priv->poster_path));
    srcpixbuf = gtk_image_get_pixbuf (poster);
    dstpixbuf = gdk_pixbuf_scale_simple (srcpixbuf, 226, 333, GDK_INTERP_BILINEAR);
    g_object_unref (poster);

    /* Clear old image and set new pixbuf to it */
    //FIXME gtk_image_clear (grid->priv->poster);
    gtk_image_set_from_pixbuf (grid->priv->poster, dstpixbuf);
    g_object_unref (dstpixbuf);
  }
}

static void
resolve_poster_done (GObject      *source_object,
                     GAsyncResult *res,
                     gpointer      user_data)
{
  TotemVideosSummary *grid;
  gchar *data;
  gsize len;
  GError *err = NULL;

  grid = TOTEM_VIDEOS_SUMMARY (user_data);
  grl_net_wc_request_finish (GRL_NET_WC (source_object),
                             res, &data, &len, &err);
  if (err != NULL) {
    g_warning ("Fetch image failed due: %s", err->message);
    g_error_free (err);
  } else {
    g_file_set_contents (grid->priv->poster_path, data, len, &err);
  }

  /* Update interface */
  totem_videos_summary_set_content (grid);
}

static void
resolve_metadata_done (GrlSource    *source,
                       guint         operation_id,
                       GrlMedia     *media,
                       gpointer      user_data,
                       const GError *error)
{
  TotemVideosSummary *grid;
  const gchar *title, *poster_url;

  if (error) {
    g_warning ("Resolve operation failed: %s", error->message);
    return;
  }

  grid = TOTEM_VIDEOS_SUMMARY (user_data);

  title = (grid->priv->is_tv_show) ?
    grl_media_video_get_show (GRL_MEDIA_VIDEO (media)) :
    grl_media_get_title (media);

  if (title == NULL) {
    g_warning ("Basic information is missing - no title");
    return;
  }

  poster_url = (grid->priv->is_tv_show) ?
    grl_data_get_string (GRL_DATA (media), grid->priv->tvdb_poster_key) :
    grl_data_get_string (GRL_DATA (media), grid->priv->tmdb_poster_key);

  if (poster_url == NULL) {
    g_debug ("Media without poster");
    totem_videos_summary_set_content (grid);
    return;
  }

  grid->priv->poster_path = g_build_filename (g_get_tmp_dir (), title, NULL);
  if (!g_file_test (grid->priv->poster_path, G_FILE_TEST_EXISTS)) {
    GrlNetWc *wc = grl_net_wc_new ();
    grl_net_wc_request_async (wc, poster_url, NULL, resolve_poster_done, grid);
    g_object_unref (wc);
  } else {
    totem_videos_summary_set_content (grid);
  }
}

static void
resolve_by_tmdb (TotemVideosSummary *grid)
{
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;

  caps = grl_source_get_caps (grid->priv->tmdb_source, GRL_OP_RESOLVE);
  options = grl_operation_options_new (caps);
  grl_operation_options_set_resolution_flags (options, GRL_RESOLVE_NORMAL);

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_DESCRIPTION,
                                    GRL_METADATA_KEY_PERFORMER,
                                    GRL_METADATA_KEY_DIRECTOR,
                                    GRL_METADATA_KEY_AUTHOR,
                                    GRL_METADATA_KEY_GENRE,
                                    GRL_METADATA_KEY_PUBLICATION_DATE,
                                    grid->priv->tmdb_poster_key,
                                    GRL_METADATA_KEY_INVALID);
  grl_source_resolve (grid->priv->tmdb_source,
                      GRL_MEDIA (grid->priv->video),
                      keys,
                      options,
                      resolve_metadata_done,
                      grid);
  g_object_unref (options);
  g_list_free (keys);
}

static void
resolve_by_the_tvdb (TotemVideosSummary *grid)
{
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;

  caps = grl_source_get_caps (grid->priv->tvdb_source, GRL_OP_RESOLVE);
  options = grl_operation_options_new (caps);
  grl_operation_options_set_resolution_flags (options, GRL_RESOLVE_NORMAL);

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_DESCRIPTION,
                                    GRL_METADATA_KEY_PERFORMER,
                                    GRL_METADATA_KEY_DIRECTOR,
                                    GRL_METADATA_KEY_AUTHOR,
                                    GRL_METADATA_KEY_GENRE,
                                    GRL_METADATA_KEY_PUBLICATION_DATE,
                                    GRL_METADATA_KEY_EPISODE_TITLE,
                                    grid->priv->tvdb_poster_key,
                                    GRL_METADATA_KEY_INVALID);
  grl_source_resolve (grid->priv->tvdb_source,
                      GRL_MEDIA (grid->priv->video),
                      keys,
                      options,
                      resolve_metadata_done,
                      grid);
  g_object_unref (options);
  g_list_free (keys);
}

static void
resolve_by_video_title_parsing_done (GrlSource *source,
                                     guint operation_id,
                                     GrlMedia *media,
                                     gpointer  user_data,
                                     const GError *error)
{
  TotemVideosSummary *grid;

  if (error != NULL) {
    g_warning ("video-title-parsing failed: %s", error->message);
    return;
  }

  grid = TOTEM_VIDEOS_SUMMARY (user_data);

  if (grl_media_video_get_show (GRL_MEDIA_VIDEO(media)) != NULL) {
    grid->priv->is_tv_show = TRUE;
    totem_videos_summary_set_content (grid);
    resolve_by_the_tvdb (grid);
  } else if (grl_media_get_title (media) != NULL) {
    grid->priv->is_tv_show = FALSE;
    totem_videos_summary_set_content (grid);
    resolve_by_tmdb (grid);
  } else {
    g_warning ("video type is not defined: %s", grl_media_get_url (media));
  }
}

static void
resolve_by_video_title_parsing (TotemVideosSummary *grid)
{
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;

  caps = grl_source_get_caps (grid->priv->video_title_parsing_source, GRL_OP_RESOLVE);
  options = grl_operation_options_new (caps);
  grl_operation_options_set_resolution_flags (options, GRL_RESOLVE_NORMAL);

  keys = grl_metadata_key_list_new (GRL_METADATA_KEY_TITLE,
                                    GRL_METADATA_KEY_EPISODE_TITLE,
                                    GRL_METADATA_KEY_SHOW,
                                    GRL_METADATA_KEY_SEASON,
                                    GRL_METADATA_KEY_EPISODE,
                                    GRL_METADATA_KEY_INVALID);

  /* We want to extract all metadata from file's name */
  grl_data_set_boolean (GRL_DATA (grid->priv->video),
                        GRL_METADATA_KEY_TITLE_FROM_FILENAME,
                        TRUE);
  grl_source_resolve (grid->priv->video_title_parsing_source,
                      GRL_MEDIA (grid->priv->video),
                      keys,
                      options,
                      resolve_by_video_title_parsing_done,
                      grid);
  g_object_unref (options);
  g_list_free (keys);
}

static gboolean
totem_videos_summary_media_init (TotemVideosSummary *grid)
{
  const gchar *url;

  url = grl_media_get_url (GRL_MEDIA (grid->priv->video));
  if (url == NULL) {
    g_warning ("Video does not have url: can't initialize totem-video-summary");
    return FALSE;
  }

  if (!g_file_test (url, G_FILE_TEST_EXISTS)) {
    g_warning ("Video file does not exist");
    return FALSE;
  }

  resolve_by_video_title_parsing (grid);
  return TRUE;
}

/* For GrlKeys that have several values, return all of them in one
 * string separated by comma; */
static gchar *
get_data_from_media (GrlData *data,
                     GrlKeyID key)
{
  gint i, len;
  gchar *str = NULL;

  /* FIXME: Use GString instead */
  len = grl_data_length (data, key);
  for (i = 0; i < len; i++) {
    GrlRelatedKeys *relkeys;
    gchar *tmp;
    const gchar *element;

    relkeys = grl_data_get_related_keys (data, key, i);
    element = grl_related_keys_get_string (relkeys, key);

    tmp = str;
    str = (str == NULL) ? g_strdup (element) : g_strconcat (str, ", ", element, NULL);
    g_clear_pointer (&tmp, g_free);
  }
  return str;
}

/* -------------------------------------------------------------------------- *
 * External
 * -------------------------------------------------------------------------- */

TotemVideosSummary *
totem_videos_summary_new (GrlMediaVideo *video)
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

  self->priv->video = g_object_ref (video);
  if (!totem_videos_summary_media_init (self))
    g_clear_object (&self);

  return self;
}

/* -------------------------------------------------------------------------- *
 * Object
 * -------------------------------------------------------------------------- */

static void
totem_videos_summary_dispose (GObject *object)
{
  //TotemVideosSummary *self = TOTEM_VIDEOS_SUMMARY (object);

  G_OBJECT_CLASS (totem_videos_summary_parent_class)->dispose (object);
}

static void
totem_videos_summary_finalize (GObject *object)
{
  TotemVideosSummary *self = TOTEM_VIDEOS_SUMMARY (object);

  g_clear_object (&self->priv->video);
  g_clear_pointer (&self->priv->poster_path, g_free);

  G_OBJECT_CLASS (totem_videos_summary_parent_class)->finalize (object);
}

static void
totem_videos_summary_init (TotemVideosSummary *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  self->priv = totem_videos_summary_get_instance_private (self);
}

static void
totem_videos_summary_class_init (TotemVideosSummaryClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->dispose = totem_videos_summary_dispose;
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
}
