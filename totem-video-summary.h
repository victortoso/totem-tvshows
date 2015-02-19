/*
 * Photos - access, organize and share your photos on GNOME
 * Copyright © 2013 Intel Corporation. All rights reserved.
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

#ifndef TOTEM_VIDEOS_SUMMARY_H
#define TOTEM_VIDEOS_SUMMARY_H

#include <gtk/gtk.h>
#include <grilo.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_VIDEOS_SUMMARY (totem_videos_summary_get_type ())

#define TOTEM_VIDEOS_SUMMARY (obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_VIDEOS_SUMMARY, TotemVideosSummary))
#define TOTEM_VIDEOS_SUMMARY_CLASS (klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_VIDEOS_SUMMARY, TotemVideosSummaryClass))
#define TOTEM_IS_VIDEOS_SUMMARY (obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_VIDEOS_SUMMARY))
#define TOTEM_IS_VIDEOS_SUMMARY_CLASS (klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_VIDEOS_SUMMARY))
#define TOTEM_VIDEOS_SUMMARY_GET_CLASS (obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TOTEM_TYPE_VIDEOS_SUMMARY, TotemVideosSummaryClass))

typedef struct _TotemVideosSummary        TotemVideosSummary;
typedef struct _TotemVideosSummaryClass   TotemVideosSummaryClass;
typedef struct _TotemVideosSummaryPrivate TotemVideosSummaryPrivate;

struct _TotemVideosSummary
{
  GtkGrid parent_instance;
  TotemVideosSummaryPrivate *priv;
};

struct _TotemVideosSummaryClass
{
  GtkGridClass parent_class;
};

GType               totem_videos_summary_get_type           (void) G_GNUC_CONST;
GtkWidget          *totem_videos_summary_new                (GtkWindow *parent, GrlMediaVideo *video);

G_END_DECLS

#endif /* TOTEM_VIDEOS_SUMMARY_H */
