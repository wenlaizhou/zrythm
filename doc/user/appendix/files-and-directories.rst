.. SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
.. SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Files and Directories
=====================

User path
---------
When Zrythm is launched for the first time, it will
also ask you to select a directory for saving your
user data. See :term:`Zrythm user path` for details.

Installed Files
---------------
When Zrythm is installed, it installs the following
files by default.

Desktop Entry
~~~~~~~~~~~~~
A `freedesktop-compliant configuration file <https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html>`_
that describes how a particular program should be
launched, how it appears in menus, etc. This is
normally installed at
:file:`/usr/share/applications/zrythm.desktop`.

GLib Schema
~~~~~~~~~~~
This file contains the schema for user
configurations. It is normally installed at
:file:`/usr/share/glib-2.0/schemas/org.zrythm.Zrythm.gschema.xml`.

Desktop Icon
~~~~~~~~~~~~
This is used by the desktop environment. Normally
installed at
:file:`/usr/share/icons/hicolor/scalable/apps/zrythm.svg`.

Localization Files
~~~~~~~~~~~~~~~~~~
Localization files are installed under
:file:`/usr/share/locale`. For example,
:file:`/usr/share/locale/ko/LC_MESSAGES/zrythm.mo`
for Korean.

Samples
~~~~~~~
Default audio samples used by Zrythm (such as the
metronome). Normally installed under
:file:`/usr/share/zrythm/samples`.

Fonts
~~~~~
Unless your distribution provides packages for the
required fonts, fonts required by Zrythm will
normally be installed under
:file:`/usr/share/fonts/zrythm`.

GtkSourceView Stylesheets
~~~~~~~~~~~~~~~~~~~~~~~~~
Zrythm will install CSS stylesheets to be used by
some text input widgets under
:file:`/usr/share/zrythm/sourceview-styles`.

Themes
~~~~~~
Default themes (including icon themes) will
normally be installed under
:file:`/usr/share/zrythm/themes`.

.. hint:: Zrythm provides ways to override some of
   these files with user-specified files in the
   :term:`Zrythm user path`. See
   :doc:`../theming/intro` for details.

User Files
----------
Cached Plugin Descriptors
~~~~~~~~~~~~~~~~~~~~~~~~~
Plugin descriptor cache for faster scanning.
This is found at
:file:`cached_plugin_descriptors.yaml` under the
:term:`Zrythm user path`.

Plugin Collections
~~~~~~~~~~~~~~~~~~
This file contains your plugin collections.
It is found at
:file:`plugin_collections.yaml` under the
:term:`Zrythm user path`.

Log File
--------
Zrythm will write to a log file on each run inside
the :file:`log` subdirectory under the
:term:`Zrythm user path`. The filename will contain
the current date and time, for example
:file:`log_2020-06-26_15-34-19.log`.

This log file is useful for debugging crashes and
other problems.
