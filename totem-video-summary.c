/*
 * Photos - access, organize and share your photos on GNOME
 * Copyright © 2013 Intel Corporation. All rights reserved.
 * Copyright © 2013, 2014 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "totem-video-summary.h"

#include <net/grl-net.h>
#include <string.h>

struct _TotemVideosSummaryPrivate
{
  GrlRegistry *registry;
  GrlSource *tmdb_source;
  GrlSource *tvdb_source;
  GrlSource *local_metadata_source;
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

static void
totem_videos_summary_set_content (TotemVideosSummary *grid)
{
  const gchar *title, *description;
  //GDateTime *date;
  gchar *str;

  title = (grid->priv->is_tv_show) ?
    grl_media_video_get_episode_title (grid->priv->video) :
    grl_media_get_title (GRL_MEDIA (grid->priv->video));
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
  if (released != NULL)
    {
      str = g_date_time_format (released, "%Y");
      if (str != NULL) {
        gtk_label_set_text (grid->priv->released, str);
        g_free (str);
      }
      gtk_widget_set_visible (GTK_WIDGET(grid->priv->released), (str != NULL));
    }
  else
    {
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
  if (str != NULL)
    {
      gtk_label_set_text (grid->priv->cast, str);
      g_clear_pointer (&str, g_free);
      gtk_widget_set_visible (GTK_WIDGET(grid->priv->cast), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET(grid->priv->cast), FALSE);
    }

  str = get_data_from_media (GRL_DATA (grid->priv->video), GRL_METADATA_KEY_DIRECTOR);
  if (str != NULL)
    {
      gtk_label_set_text (grid->priv->directors, str);
      g_clear_pointer (&str, g_free);
      gtk_widget_set_visible (GTK_WIDGET(grid->priv->directors), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET(grid->priv->directors), FALSE);
    }

  str = get_data_from_media (GRL_DATA (grid->priv->video), GRL_METADATA_KEY_AUTHOR);
  if (str != NULL)
    {
      gtk_label_set_text (grid->priv->authors, str);
      g_clear_pointer (&str, g_free);
      gtk_widget_set_visible (GTK_WIDGET(grid->priv->authors), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET(grid->priv->authors), FALSE);
    }

  if (grid->priv->poster_path != NULL)
    {
      GtkImage *poster;
      GdkPixbuf *srcpixbuf, *dstpixbuf;

      /* Get a scalated pixbuf from img file */
      poster = GTK_IMAGE(gtk_image_new_from_file (grid->priv->poster_path));
      srcpixbuf = gtk_image_get_pixbuf (poster);
      dstpixbuf = gdk_pixbuf_scale_simple (srcpixbuf, 226, 333, GDK_INTERP_BILINEAR);
      g_object_unref (poster);

      /* Clear old image and set new pixbuf to it */
      gtk_image_clear (grid->priv->poster);
      gtk_image_set_from_pixbuf (grid->priv->poster, dstpixbuf);
      g_object_unref (dstpixbuf);
    }
}


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
  self->priv = totem_videos_summary_get_instance_private (self);

#if 0
  GApplication *app;
  priv = self->priv;

  app = g_application_get_default ();

  priv->renderers_mngr = photos_dlna_renderers_manager_dup_singleton ();
  priv->remote_mngr = photos_remote_display_manager_dup_singleton ();
  priv->mode_cntrlr = photos_mode_controller_dup_singleton ();

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_list_box_set_header_func (priv->listbox, photos_dlna_renderers_separator_cb, NULL, NULL);

  g_signal_connect (self, "response", G_CALLBACK (gtk_widget_destroy), NULL);
#endif
}

static void
totem_videos_summary_class_init (TotemVideosSummaryClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->dispose = totem_videos_summary_dispose;
  object_class->finalize = totem_videos_summary_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/totem/grilo/totemvideosummary.ui");
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, poster);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, title);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, summary);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, released);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, genre);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, cast);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, directors);
  gtk_widget_class_bind_template_child_private (widget_class, TotemVideosSummary, authors);
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

  if (err != NULL)
    {
      g_warning ("Fetch image failed due: %s", err->message);
      g_error_free (err);
    }
  else
    g_file_set_contents (grid->priv->poster_path, data, len, &err);

  /* Update interface */
  totem_videos_summary_set_content (grid);
}

static void
resolve_metadata_done (GrlSource *source,
                      guint operation_id,
                       GrlMedia *media,
                       gpointer  user_data,
                       const GError *error)
{
  TotemVideosSummary *grid;
  const gchar *title, *poster_url;

  if (error)
    {
      g_warning ("Resolve operation failed: %s", error->message);
      return;
    }

  grid = TOTEM_VIDEOS_SUMMARY (user_data);

  title = (grid->priv->is_tv_show) ?
    grl_media_video_get_show (GRL_MEDIA_VIDEO (media)) :
    grl_media_get_title (media);

  if (title == NULL)
    {
      g_warning ("Basic information is missing - no title");
      return;
    }

  poster_url = (grid->priv->is_tv_show) ?
    grl_data_get_string (GRL_DATA (media), grid->priv->tvdb_poster_key) :
    grl_data_get_string (GRL_DATA (media), grid->priv->tmdb_poster_key);

  if (poster_url == NULL)
    {
      g_debug ("Media without poster");
      totem_videos_summary_set_content (grid);
      return;
    }

  grid->priv->poster_path = g_build_filename (g_get_tmp_dir (), title, NULL);
  if (!g_file_test (grid->priv->poster_path, G_FILE_TEST_EXISTS))
    {
      GrlNetWc *wc = grl_net_wc_new ();
      grl_net_wc_request_async (wc, poster_url, NULL, resolve_poster_done, grid);
      g_object_unref (wc);
    }
  else
    {
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
resolve_by_local_metadata_done (GrlSource *source,
                                guint operation_id,
                                GrlMedia *media,
                                gpointer  user_data,
                                const GError *error)
{
  TotemVideosSummary *grid;

  if (error != NULL)
    {
      g_warning ("local-metadata failed: %s", error->message);
      return;
    }

  grid = TOTEM_VIDEOS_SUMMARY (user_data);

  if (grl_media_video_get_show (grid->priv->video) != NULL)
    {
      grid->priv->is_tv_show = TRUE;
      totem_videos_summary_set_content (grid);
      resolve_by_the_tvdb (grid);
    }
  else if (grl_media_get_title (media) != NULL)
    {
      grid->priv->is_tv_show = FALSE;
      totem_videos_summary_set_content (grid);
      resolve_by_tmdb (grid);
    }
  else
    {
      g_warning ("local-metadata can't define it as movie or series: %s",
                 grl_media_get_url (media));
    }
}

static void
resolve_by_local_metadata (TotemVideosSummary *grid)
{
  GrlOperationOptions *options;
  GList *keys;
  GrlCaps *caps;

  caps = grl_source_get_caps (grid->priv->local_metadata_source, GRL_OP_RESOLVE);
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
  grl_source_resolve (grid->priv->local_metadata_source,
                      GRL_MEDIA (grid->priv->video),
                      keys,
                      options,
                      resolve_by_local_metadata_done,
                      grid);
  g_object_unref (options);
  g_list_free (keys);
}

static gboolean
totem_videos_summary_media_init (TotemVideosSummary *grid)
{
  const gchar *url;

  url = grl_media_get_url (GRL_MEDIA (grid->priv->video));
  if (url == NULL || !g_file_test (url, G_FILE_TEST_EXISTS))
    return FALSE;

  resolve_by_local_metadata (grid);
  return TRUE;
}

TotemVideosSummary *
totem_videos_summary_new (GrlMediaVideo *video)
{
  TotemVideosSummary *grid;
  GrlConfig *config;
  GrlSource *source;
  GrlRegistry *registry;
  GError *error = NULL;

  grid = g_object_new (TOTEM_TYPE_VIDEOS_SUMMARY, NULL);
  registry = grl_registry_get_default();
  grid->priv->registry = registry;

  config = grl_config_new ("grl-thetvdb", NULL);
  grl_config_set_api_key (config, "3F476CEF2FBD0FB0");
  grl_registry_add_config (registry, config, &error);
  g_assert_no_error (error);

  config = grl_config_new ("grl-tmdb", NULL);
  grl_config_set_api_key (config, "719b9b296835b04cd919c4bf5220828a");
  grl_registry_add_config (registry, config, &error);
  g_assert_no_error (error);

  /* FIXME: Only load the plugins we need */
  grl_registry_load_all_plugins (registry, &error);
  g_assert_no_error (error);

  source = grl_registry_lookup_source (registry, "grl-tmdb");
  g_return_val_if_fail (source != NULL, NULL);
  grid->priv->tmdb_source = source;

  source = grl_registry_lookup_source (registry, "grl-thetvdb");
  g_return_val_if_fail (source != NULL, NULL);
  grid->priv->tvdb_source = source;

  source = grl_registry_lookup_source (registry, "grl-local-metadata");
  g_return_val_if_fail (source != NULL, NULL);
  grid->priv->local_metadata_source = source;

  grid->priv->tvdb_poster_key = grl_registry_lookup_metadata_key (registry, "thetvdb-poster");
  grid->priv->tmdb_poster_key = grl_registry_lookup_metadata_key (registry, "tmdb-poster");

  /* FIXME: Disable movie/series if plugin is not loaded correctly or return NULL in case of
   * all of them. */
  g_return_val_if_fail (grid->priv->tvdb_poster_key != GRL_METADATA_KEY_INVALID, NULL);
  g_return_val_if_fail (grid->priv->tmdb_poster_key != GRL_METADATA_KEY_INVALID, NULL);

  grid->priv->video = g_object_ref (video);
  if (totem_videos_summary_media_init (grid) == FALSE) {
    g_clear_object (&grid);
  }

  return g_object_ref_sink (grid);
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
    g_print ("-----> %s\n", element);

    tmp = str;
    str = (str == NULL) ? g_strdup (element) : g_strconcat (str, ", ", element, NULL);
    g_clear_pointer (&tmp, g_free);
  }
  g_print ("[debug] %s\n", str);
  return str;
}
