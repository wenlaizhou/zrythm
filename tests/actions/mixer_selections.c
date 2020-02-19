/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/arranger_selections.h"
#include "actions/create_tracks_action.h"
#include "actions/copy_plugins_action.h"
#include "actions/move_plugins_action.h"
#include "actions/undoable_action.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/master_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/zrythm.h"

#include <glib.h>

/**
 * Bootstraps the test with test data.
 */
static void
rebootstrap ()
{
  char path_str[6000];
  sprintf (
    path_str, "file://%s/%s",
    TESTS_BUILDDIR, "eg-amp.lv2/");
  g_message ("path is %s", path_str);

  plugin_manager_init (PLUGIN_MANAGER);
  LilvNode * path =
    lilv_new_uri (LILV_WORLD, path_str);
  lilv_world_load_bundle (
    LILV_WORLD, path);
  lilv_node_free (path);

  double progress = 0;
  plugin_manager_scan_plugins (
    PLUGIN_MANAGER, 1.0, &progress);
  g_assert_cmpint (PLUGIN_MANAGER->num_plugins, ==, 1);
}

static void
test_move_plugins ()
{
  /* create a track with a plugin */
  g_assert_cmpint (TRACKLIST->num_tracks, ==, 3);
  UndoableAction * action =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO_BUS,
      PLUGIN_MANAGER->plugin_descriptors[0], NULL,
      3, 1);
  undo_manager_perform (UNDO_MANAGER, action);
  g_assert_cmpint (TRACKLIST->num_tracks, ==, 4);

  /* check if track is selected */
  Track * track = TRACKLIST->tracks[3];
  g_assert_cmpint (track->pos, ==, 3);
  g_assert_nonnull (track);
  g_assert_true (
    tracklist_selections_contains_track (
      TRACKLIST_SELECTIONS, track));

  /* check identifiers */
  Channel * ch = track->channel;
  g_assert_nonnull (ch);
  Plugin * pl = ch->plugins[0];
  g_assert_nonnull (pl);
  g_assert_cmpint (pl->id.track_pos, ==, track->pos);
  g_assert_cmpint (pl->id.slot, ==, 0);
  g_assert_true (
    ch == plugin_get_channel (pl));

  /* check that automation tracks are created */
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  g_assert_nonnull (atl);
  g_assert_cmpint (atl->num_ats, ==, 5);
  AutomationTrack * at = atl->ats[3];
  g_assert_nonnull (at);
  g_assert_cmpint (
    at->port_id.owner_type, ==,
    PORT_OWNER_TYPE_PLUGIN);
  g_assert_true (
    at->port_id.flags & PORT_FLAG_PLUGIN_ENABLED);
  at = atl->ats[4];
  g_assert_nonnull (at);
  g_assert_cmpint (
    at->port_id.owner_type, ==,
    PORT_OWNER_TYPE_PLUGIN);
  g_assert_true (
    at->port_id.flags & PORT_FLAG_PLUGIN_CONTROL);

  /* add some automation */
  Position start_pos, end_pos;
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * region =
    automation_region_new (
      &start_pos, &end_pos, track->pos, at->index,
      0);
  track_add_region (
    track, region, at, -1, 1, 0);
  arranger_object_select (
    (ArrangerObject *) region, 1, 0);
  action =
    arranger_selections_action_new_create (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, action);
  g_assert_cmpint (at->num_regions, ==, 1);
  g_assert_cmpint (
    at->regions[0]->id.track_pos, ==, track->pos);
  g_assert_cmpint (
    at->regions[0]->id.at_idx, ==, at->index);
  g_assert_cmpint (
    at->regions[0]->id.idx, ==, 0);
  Position ap1_pos, ap2_pos;
  position_set_to_bar (&ap1_pos, 1);
  position_set_to_bar (&ap2_pos, 2);
  AutomationPoint * ap =
    automation_point_new_float (
      -50.f, 0, &ap1_pos);
  automation_region_add_ap (region, ap);
  arranger_object_select (
    (ArrangerObject *) ap, 1, 0);
  action =
    arranger_selections_action_new_create (
      AUTOMATION_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, action);
  g_assert_cmpint (region->num_aps, ==, 1);
  g_assert_cmpint (region->aps[0]->index, ==, 0);
  g_assert_true (
    region_identifier_is_equal (
      &region->id, &region->aps[0]->region_id));
  ap =
    automation_point_new_float (
      -30.f, 0, &ap2_pos);
  automation_region_add_ap (region, ap);
  arranger_object_select (
    (ArrangerObject *) ap, 1, 0);
  action =
    arranger_selections_action_new_create (
      AUTOMATION_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, action);
  g_assert_cmpint (region->num_aps, ==, 2);
  g_assert_cmpint (region->aps[1]->index, ==, 1);
  g_assert_true (
    region_identifier_is_equal (
      &region->id, &region->aps[1]->region_id));

  /* select plugin and duplicate to next slot in
   * a new track */
  mixer_selections_add_slot (
    MIXER_SELECTIONS, ch, pl->id.slot);
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 1);
  action =
    copy_plugins_action_new (
      MIXER_SELECTIONS, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, action);

  /* check that new track is created and the
   * identifiers are correct */
  g_assert_cmpint (TRACKLIST->num_tracks, ==, 5);
  Track * new_track = TRACKLIST->tracks[4];
  g_assert_nonnull (new_track);
  g_assert_true (
    new_track->type == TRACK_TYPE_INSTRUMENT ||
    new_track->type == TRACK_TYPE_AUDIO_BUS);
  Channel * new_ch = new_track->channel;
  g_assert_nonnull (new_ch);
  Plugin * new_pl = new_ch->plugins[1];
  g_assert_nonnull (new_pl);
  g_assert_null (new_ch->plugins[0]);
  g_assert_cmpint (pl->id.track_pos, ==, 3);
  g_assert_cmpint (pl->id.slot, ==, 0);
  g_assert_cmpint (new_pl->id.track_pos, ==, 4);
  g_assert_cmpint (new_pl->id.slot, ==, 1);
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 1);
  g_assert_cmpint (
    MIXER_SELECTIONS->track_pos, ==, 4);
  g_assert_cmpint (
    MIXER_SELECTIONS->slots[0], ==, 1);

  /* check that the automation was copied
   * correctly */
  atl = track_get_automation_tracklist (new_track);
  g_assert_cmpint (atl->num_ats, ==, 5);
  at = atl->ats[4];
  g_assert_nonnull (at);
  g_assert_cmpint (at->index, ==, 4);
  g_assert_cmpint (at->num_regions, ==, 1);
  region = at->regions[0];
  g_assert_cmpint (region->num_aps, ==, 2);
  g_assert_cmpint (
    region->id.track_pos, ==, new_track->pos);
  g_assert_cmpint (region->id.at_idx, ==, 4);
  g_assert_cmpint (region->id.idx, ==, 0);

  /* copy the new plugin to the slot below */
  action =
    copy_plugins_action_new (
      MIXER_SELECTIONS, new_track, 2);
  undo_manager_perform (UNDO_MANAGER, action);

  /* check that there are 2 plugins in the track
   * now (slot 1 and 2) */
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 1);
  g_assert_cmpint (
    MIXER_SELECTIONS->track_pos, ==, 4);
  g_assert_cmpint (
    MIXER_SELECTIONS->slots[0], ==, 2);
  g_assert_nonnull (new_ch->plugins[2]);
  Plugin * slot2_plugin = new_ch->plugins[2];
  g_assert_cmpint (
    slot2_plugin->id.track_pos, ==, 4);
  g_assert_cmpint (slot2_plugin->id.slot, ==, 2);

  /* check that the automation was copied too */
  g_assert_cmpint (atl->num_ats, ==, 7);
  at = atl->ats[6];
  g_assert_nonnull (at);
  g_assert_cmpint (at->index, ==, 6);
  g_assert_cmpint (at->num_regions, ==, 1);
  region = at->regions[0];
  g_assert_cmpint (region->num_aps, ==, 2);
  g_assert_cmpint (
    region->id.track_pos, ==, new_track->pos);
  g_assert_cmpint (region->id.at_idx, ==, 6);
  g_assert_cmpint (region->id.idx, ==, 0);

  /* select the plugin above too and copy both to
   * the previous track at slot 5 */
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 1);
  g_assert_true (MIXER_SELECTIONS->has_any);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, new_ch, 1);
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 2);
  action =
    copy_plugins_action_new (
      MIXER_SELECTIONS, track, 5);
  undo_manager_perform (UNDO_MANAGER, action);

  /* verify that they were copied there */
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 2);
  g_assert_true (MIXER_SELECTIONS->has_any);
  g_assert_cmpint (
    MIXER_SELECTIONS->track_pos, ==, track->pos);
  g_assert_nonnull (ch->plugins[5]);
  g_assert_nonnull (ch->plugins[6]);
  g_assert_nonnull (new_ch->plugins[1]);
  g_assert_nonnull (new_ch->plugins[2]);
  g_assert_null (new_ch->plugins[5]);
  g_assert_null (new_ch->plugins[6]);

  /* check their identifiers */
  g_assert_cmpint (
    ch->plugins[5]->id.track_pos, ==, track->pos);
  g_assert_cmpint (
    ch->plugins[5]->id.slot, ==, 5);
  g_assert_cmpint (
    ch->plugins[6]->id.track_pos, ==, track->pos);
  g_assert_cmpint (
    ch->plugins[6]->id.slot, ==, 6);

  /* check that the automations were copied too */
  atl = track_get_automation_tracklist (track);
  g_assert_cmpint (atl->num_ats, ==, 9);
  at = atl->ats[8];
  g_assert_nonnull (at);
  g_assert_cmpint (at->index, ==, 8);
  g_assert_cmpint (at->num_regions, ==, 1);
  region = at->regions[0];
  g_assert_cmpint (region->num_aps, ==, 2);
  g_assert_cmpint (
    region->id.track_pos, ==, track->pos);
  g_assert_cmpint (region->id.at_idx, ==, 8);
  g_assert_cmpint (region->id.idx, ==, 0);

  /* move the plugins to slot 7 and 8 in the next
   * track */
  action =
    move_plugins_action_new (
      MIXER_SELECTIONS, new_track, 7);
  undo_manager_perform (UNDO_MANAGER, action);

  /* verify that they were moved there */
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 2);
  g_assert_true (MIXER_SELECTIONS->has_any);
  g_assert_cmpint (
    MIXER_SELECTIONS->track_pos, ==, new_track->pos);
  g_assert_null (ch->plugins[5]);
  g_assert_null (ch->plugins[6]);
  g_assert_nonnull (new_ch->plugins[7]);
  g_assert_nonnull (new_ch->plugins[8]);

  /* verify that the automation was moved too */
  atl = track_get_automation_tracklist (new_track);
  g_assert_cmpint (atl->num_ats, ==, 11);
  at =
    automation_tracklist_get_plugin_at (
      atl, 7, "Gain");
  g_assert_nonnull (at);
  g_assert_cmpint (at->num_regions, ==, 1);
  region = at->regions[0];
  g_assert_cmpint (region->num_aps, ==, 2);
  g_assert_cmpint (
    region->id.track_pos, ==, new_track->pos);
  g_assert_cmpint (region->id.at_idx, ==, at->index);
  g_assert_cmpint (region->id.idx, ==, 0);
  ap = region->aps[0];
  g_assert_true (
    region_identifier_is_equal (
      &region->id, &ap->region_id));

  /* delete these 2 plugins */
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/actions/mixer_selections/"

  rebootstrap ();
  g_test_add_func (
    TEST_PREFIX "test move plugins",
    (GTestFunc) test_move_plugins);

  return g_test_run ();
}
