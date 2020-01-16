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

#include "config.h"

#ifdef HAVE_SDL

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/engine_sdl.h"
#include "audio/master_track.h"
#include "audio/mixer.h"
#include "audio/port.h"
#include "project.h"
#include "utils/ui.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

static void
sdl_callback (
  void *  user_data,
  Uint8 * buf,
  int     len)
{
  AudioEngine * self = (AudioEngine *) user_data;
  if (!self->run)
    return;

  nframes_t num_frames =
    AUDIO_ENGINE->block_length;
  /*g_message (*/
    /*"processing for num frames %u (len %d)",*/
    /*num_frames, len);*/
  engine_process (self, num_frames);

  memset (buf, 0, (size_t) len);
  float * float_buf = (float *) buf;
  for (nframes_t i = 0; i < num_frames; i++)
    {
      float_buf[i * 2] =
        self->monitor_out->l->buf[i];
      float_buf[i * 2 + 1] =
        self->monitor_out->r->buf[i];
    }
}

/**
 * Set up Port Audio.
 */
int
engine_sdl_setup (
  AudioEngine * self,
  int           loading)
{
  g_message ("Setting up SDL...");

  if (SDL_Init (
        SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE) < 0)
    {
      g_critical (
        "Failed to initialize SDL: %s",
        SDL_GetError ());
      return -1;
    }

  SDL_AudioSpec req_specs;
  memset (&req_specs, 0, sizeof (req_specs));
  req_specs.freq = 48000;
  req_specs.format = AUDIO_F32SYS;
  req_specs.channels = 2;
  req_specs.samples = 1024;
  req_specs.callback =
    (SDL_AudioCallback) sdl_callback;
  req_specs.userdata = self;

  int num_out_devices = SDL_GetNumAudioDevices (0);
  for (int i = 0; i < num_out_devices; i++)
    {
      g_message (
        "Output audio device %d: %s",
        i, SDL_GetAudioDeviceName (i, 0));
    }

  SDL_AudioSpec actual_specs;
  self->dev =
    SDL_OpenAudioDevice (
      NULL, 0, &req_specs, &actual_specs,
      SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
  if (self->dev == 0)
    {
      g_critical (
        "Couldn't open audio device: %s",
        SDL_GetError ());
      return -1;
    }
  g_message (
    "[SDL] Opened output device %d",
    (int) self->dev);

  /* Set audio engine properties */
  self->sample_rate =
    (sample_rate_t) actual_specs.freq;
  self->block_length = actual_specs.samples;

  g_message (
    "Setting sample rate to %u and buffer size to "
    "%d", self->sample_rate, self->block_length);

  g_warn_if_fail (
    TRANSPORT && TRANSPORT->beats_per_bar > 1);

  engine_update_frames_per_tick (
    self,
    TRANSPORT->beats_per_bar,
    TRANSPORT->bpm,
    self->sample_rate);

  /* create ports */
  Port * monitor_out_l, * monitor_out_r;

  if (loading)
    {
    }
  else
    {
      monitor_out_l =
        port_new_with_data (
          INTERNAL_PA_PORT,
          TYPE_AUDIO,
          FLOW_OUTPUT,
          "SDL Stereo Out / L",
          NULL);
      monitor_out_r =
        port_new_with_data (
          INTERNAL_PA_PORT,
          TYPE_AUDIO,
          FLOW_OUTPUT,
          "SDL Stereo Out / R",
          NULL);

      self->monitor_out =
        stereo_ports_new_from_existing (
          monitor_out_l,
          monitor_out_r);
    }

  g_message ("SDL set up");

  return 0;
}

void
engine_sdl_activate (
  AudioEngine * self)
{
  g_message ("Activating SDL...");

  engine_realloc_port_buffers (
    self, AUDIO_ENGINE->block_length);

  /* start playing */
  SDL_PauseAudioDevice (self->dev, 0);

  switch (SDL_GetAudioDeviceStatus (self->dev))
    {
      case SDL_AUDIO_STOPPED:
        g_message ("SDL audio stopped");
        break;
      case SDL_AUDIO_PLAYING:
        g_message ("SDL audio playing");
        break;
      case SDL_AUDIO_PAUSED:
        g_message("SDL audio paused");
        break;
      default:
        g_critical (
          "[SDL] Unknown audio device status");
        break;
    }

  g_message ("SDL activated");
}

/**
 * Tests if PortAudio is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_sdl_test (
  GtkWindow * win)
{
  return 0;
}

/**
 * Closes Port Audio.
 */
void
engine_sdl_tear_down (
  AudioEngine * self)
{
  SDL_CloseAudioDevice (self->dev);
  SDL_Quit ();
}

#endif
