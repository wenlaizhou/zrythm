/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Timeline panel.
 */

#ifndef __GUI_WIDGETS_MAIN_NOTEBOOK_H__
#define __GUI_WIDGETS_MAIN_NOTEBOOK_H__

#include <gtk/gtk.h>

#define MAIN_NOTEBOOK_WIDGET_TYPE \
  (main_notebook_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MainNotebookWidget, main_notebook_widget,
  Z, MAIN_NOTEBOOK_WIDGET, GtkNotebook)

typedef struct _TimelinePanelWidget
  TimelinePanelWidget;
typedef struct _EventViewerWidget
  EventViewerWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_MAIN_NOTEBOOK \
  MW_CENTER_DOCK->main_notebook

typedef struct _MainNotebookWidget
{
  GtkNotebook           parent_instance;

  /** Event viewr + timeline panel. */
  GtkPaned *
    timeline_plus_event_viewer_paned;

  TimelinePanelWidget * timeline_panel;
  EventViewerWidget *   event_viewer;
  GtkStack *            end_stack;
} MainNotebookWidget;

void
main_notebook_widget_setup (
  MainNotebookWidget * self);

/**
 * Prepare for finalization.
 */
void
main_notebook_widget_tear_down (
  MainNotebookWidget * self);

/**
 * @}
 */

#endif
