/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include "actions/edit_track_action.h"
#include "actions/undo_manager.h"
#include "audio/audio_track.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/master_track.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/backend/events.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"

void
track_init_loaded (Track * track)
{
  track->channel =
    project_get_channel (track->channel_id);

  int i;
  for (i = 0; i < track->num_regions; i++)
    track->regions[i] =
      project_get_region (
        track->region_ids[i]);
  for (i = 0; i < track->num_chords; i++)
    track->chords[i] =
      project_get_chord (
        track->chord_ids[i]);

  track->widget = track_widget_new (track);

  if (track->type != TRACK_TYPE_CHORD)
    automation_tracklist_init_loaded (
      &track->automation_tracklist);
}

void
track_init (Track * track)
{
  track->visible = 1;
  track->handle_pos = 1;
}

/**
 * Returns a new track for the given channel with
 * the given label.
 */
Track *
track_new (Channel * channel, char * label)
{
  Track * track =
    calloc (1, sizeof (Track));

  track_init (track);
  track->name = label;
  track->channel = channel;
  track->channel_id = channel->id;

  switch (channel->type)
    {
    case CT_MIDI:
      instrument_track_init (track);
      break;
    case CT_AUDIO:
      audio_track_init (track);
      break;
    case CT_BUS:
      bus_track_init (track);
      break;
    case CT_MASTER:
      master_track_init (track);
      break;
    }

  project_add_track (track);

  return track;
}

/**
 * Sets recording and connects/disconnects the JACK ports.
 */
void
track_set_recording (Track *   track,
                     int       recording)
{
  Channel * channel =
    track_get_channel (track);

  if (!channel)
    {
      ui_show_notification_idle (
        "Recording not implemented yet for this "
        "track.");
      return;
    }
  if (channel->type == CT_AUDIO)
    {
      /* TODO connect L and R audio ports for recording */
      if (recording)
        {
          port_connect (
            AUDIO_ENGINE->stereo_in->l,
            channel->stereo_in->l);
          port_connect (
            AUDIO_ENGINE->stereo_in->r,
            channel->stereo_in->r);
        }
      else
        {
          port_disconnect (
            AUDIO_ENGINE->stereo_in->l,
            channel->stereo_in->l);
          port_disconnect (
            AUDIO_ENGINE->stereo_in->r,
            channel->stereo_in->r);
        }
    }
  else if (channel->type == CT_MIDI)
    {
      /* find first plugin */
      Plugin * plugin = channel_get_first_plugin (channel);

      if (plugin)
        {
          /* Connect/Disconnect MIDI port to the plugin */
          for (int i = 0; i < plugin->num_in_ports; i++)
            {
              Port * port = plugin->in_ports[i];
              if (port->type == TYPE_EVENT &&
                  port->flow == FLOW_INPUT)
                {
                  g_message ("%d MIDI In port: %s",
                             i, port->label);
                  if (recording)
                    port_connect (
                      AUDIO_ENGINE->midi_in, port);
                  else
                    port_disconnect (
                      AUDIO_ENGINE->midi_in, port);
                }
            }
        }
    }

  track->recording = recording;

  EVENTS_PUSH (ET_TRACK_STATE_CHANGED,
               track);
}

/**
 * Sets track muted and optionally adds the action
 * to the undo stack.
 */
void
track_set_muted (Track * track,
                 int     mute,
                 int     trigger_undo)
{
  UndoableAction * action =
    edit_track_action_new_mute (track,
                                mute);
  g_message ("setting mute to %d",
             mute);
  if (trigger_undo)
    {
      undo_manager_perform (UNDO_MANAGER,
                            action);
    }
  else
    {
      edit_track_action_do (
        (EditTrackAction *) action);
    }
}

/**
 * Sets track soloed, updates UI and optionally
 * adds the action to the undo stack.
 */
void
track_set_soloed (Track * track,
                  int     solo,
                  int     trigger_undo)
{
  UndoableAction * action =
    edit_track_action_new_solo (track,
                                solo);
  if (trigger_undo)
    {
      undo_manager_perform (UNDO_MANAGER,
                            action);
    }
  else
    {
      edit_track_action_do (
        (EditTrackAction *) action);
    }
}

/**
 * Returns the last region in the track, or NULL.
 */
Region *
track_get_last_region (
  Track * track)
{
  int i;
  Region * last_region = NULL, * r;
  Position tmp;
  position_init (&tmp);

  if (track->type == TRACK_TYPE_AUDIO)
    {
      AudioTrack * at = (AudioTrack *) track;
      for (i = 0; i < at->num_regions; i++)
        {
          r = (Region *) at->regions[i];
          if (position_compare (
                &r->end_pos,
                &tmp) > 0)
            {
              last_region = r;
              position_set_to_pos (
                &tmp, &r->end_pos);
            }
        }
    }
  else if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      InstrumentTrack * at =
        (InstrumentTrack *) track;
      for (i = 0; i < at->num_regions; i++)
        {
          r = (Region *) at->regions[i];
          if (position_compare (
                &r->end_pos,
                &tmp) > 0)
            {
              last_region = r;
              position_set_to_pos (
                &tmp, &r->end_pos);
            }
        }
    }
  return last_region;
}

/**
 * Returns the last region in the track, or NULL.
 */
AutomationPoint *
track_get_last_automation_point (
  Track * track)
{
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  if (!atl)
    return NULL;

  int i;
  AutomationPoint * last_ap = NULL, * ap;
  AutomationTrack * at;
  Position tmp;
  position_init (&tmp);

  for (int i = 0; i < atl->num_automation_tracks;
       i++)
    {
      at = atl->automation_tracks[i];

      for (int j = 0; j < at->num_automation_points;
           j++)
        {
          ap = at->automation_points[j];

          if (position_compare (
                &ap->pos,
                &tmp) > 0)
            {
              last_ap = ap;
              position_set_to_pos (
                &tmp, &ap->pos);
            }
        }
    }

  return last_ap;
}

/**
 * Wrapper.
 */
void
track_setup (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      instrument_track_setup (
        (InstrumentTrack *) track);
      break;
    case TRACK_TYPE_MASTER:
      master_track_setup (
        (MasterTrack *) track);
      break;
    case TRACK_TYPE_AUDIO:
      audio_track_setup (
        (AudioTrack *) track);
      break;
    case TRACK_TYPE_CHORD:
      break;
    case TRACK_TYPE_BUS:
      bus_track_setup (
        (BusTrack *) track);
      break;
    }
}

/**
 * Wrapper.
 */
void
track_add_region (Track * track,
                  Region * region)
{
  g_warn_if_fail (
    (track->type == TRACK_TYPE_INSTRUMENT ||
    track->type == TRACK_TYPE_AUDIO) &&
    region->id >= 0);

  region_set_track (region, track);
  track->region_ids[track->num_regions] =
    region->id;
  array_append (track->regions,
                track->num_regions,
                region);

  EVENTS_PUSH (ET_REGION_CREATED,
               region);
}

/**
 * Wrapper.
 */
void
track_remove_region (Track * track,
                     Region * region,
                     int       delete)
{
  if (CLIP_EDITOR->region == region)
    {
      CLIP_EDITOR->region = NULL;
      EVENTS_PUSH (ET_CLIP_EDITOR_REGION_CHANGED,
                   NULL);
    }
  if (TL_SELECTIONS)
    {
      array_delete (
        TL_SELECTIONS->regions,
        TL_SELECTIONS->num_regions,
        region);
    }
  if (MW_TIMELINE->start_region == region)
    MW_TIMELINE->start_region = NULL;

  array_delete (track->regions,
                track->num_regions,
                region);
  int size = track->num_regions + 1;
  array_delete (track->region_ids,
                size,
                region->id);

  if (delete)
    {
      project_remove_region (region);
      region_free (region);
    }

  EVENTS_PUSH (ET_REGION_REMOVED, track);
}

/**
 * Wrapper for each track type.
 */
void
track_free (Track * track)
{
  int i;

  /* remove automation points, curves, tracks,
   * lanes*/
  automation_tracklist_free_members (
    &track->automation_tracklist);

  /* remove regions */
  for (i = 0; i < track->num_regions; i++)
    track_remove_region (
      track, track->regions[i], 1);

  /* remove chords */
  for (i = 0; i < track->num_chords; i++)
    chord_track_remove_chord (
      track, track->chords[i]);

  switch (track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      instrument_track_free (
        (InstrumentTrack *) track);
      break;
    case TRACK_TYPE_MASTER:
      master_track_free (
        (MasterTrack *) track);
      break;
    case TRACK_TYPE_AUDIO:
      audio_track_free (
        (AudioTrack *) track);
      break;
    case TRACK_TYPE_CHORD:
      chord_track_free (
        (ChordTrack *) track);
      break;
    case TRACK_TYPE_BUS:
      bus_track_free (
        (BusTrack *) track);
      break;
    }

  if (track->widget && GTK_IS_WIDGET (track->widget))
    gtk_widget_destroy (
      GTK_WIDGET (track->widget));

  free (track);
}

/**
 * Returns the automation tracklist if the track type has one,
 * or NULL if it doesn't (like chord tracks).
 */
AutomationTracklist *
track_get_automation_tracklist (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_CHORD:
      break;
    case TRACK_TYPE_BUS:
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_MASTER:
        {
          ChannelTrack * bt = (ChannelTrack *) track;
          return &bt->automation_tracklist;
        }
    }

  return NULL;
}

/**
 * Wrapper for track types that have fader automatables.
 *
 * Otherwise returns NULL.
 */
Automatable *
track_get_fader_automatable (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_CHORD:
      break;
    case TRACK_TYPE_BUS:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_INSTRUMENT:
        {
          ChannelTrack * bt = (ChannelTrack *) track;
          return channel_get_fader_automatable (bt->channel);
        }
    }

  return NULL;
}

/**
 * Returns the channel of the track, if the track type has
 * a channel,
 * or NULL if it doesn't.
 */
Channel *
track_get_channel (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_CHORD:
      break;
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_BUS:
        {
          ChannelTrack * bt = (ChannelTrack *) track;
          return bt->channel;
        }
    }

  return NULL;
}

/**
 * Returns the region at the given position, or NULL.
 */
Region *
track_get_region_at_pos (
  Track *    track,
  Position * pos)
{
  if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      InstrumentTrack * it = (InstrumentTrack *) track;
      for (int i = 0; i < it->num_regions; i++)
        {
          Region * r = (Region *) it->regions[i];
          if (position_compare (pos,
                                &r->start_pos) >= 0 &&
              position_compare (pos,
                                &r->end_pos) <= 0)
            return r;
        }
    }
  else if (track->type == TRACK_TYPE_AUDIO)
    {
      AudioTrack * at = (AudioTrack *) track;
      for (int i = 0; i < at->num_regions; i++)
        {
          Region * r = (Region *) at->regions[i];
          if (position_compare (pos,
                                &r->start_pos) >= 0 &&
              position_compare (pos,
                                &r->end_pos) <= 0)
            return r;
        }
    }
  return NULL;
}

/**
 * Wrapper to get the track name.
 */
char *
track_get_name (Track * track)
{
  return track->name;
}
