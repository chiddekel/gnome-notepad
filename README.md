# Gnome-Notepad
<img width="640" height="480" alt="main-window" src="https://github.com/user-attachments/assets/999ee802-81a0-44ee-8a83-3b0e64dd4883" />

A native GNOME text editor built with GTK4, libadwaita, and gtkmm - a from-scratch
port of the Win32 [legacy-notepad](https://github.com/forloopcodes/legacy-notepad)
concept to the GNOME platform, replacing every Windows-specific API with its GNOME
equivalent (RichEdit → GtkTextView, the Registry → GSettings, GDI+ → Cairo/Pango,
comdlg32 → GtkFileDialog/GtkPrintOperation, and so on). See the porting plan for the
full API-by-API mapping and the reasoning behind each decision.

[Project website](https://chiddekel.github.io/gnome-notepad/) ·
[Report an issue](https://github.com/chiddekel/gnome-notepad/issues)

## Features

- Multi-encoding text: UTF-8, UTF-8 BOM, UTF-16 LE/BE, ANSI, with configurable line
  endings (CRLF/LF/CR)
- Find/Replace with wraparound search, Go To Line
- Word wrap, zoom, font selection, Time/Date insertion
- Printing via `GtkPrintOperation`
- Optional background image behind the editor (tile/stretch/fit/fill/9-anchor
  positioning, adjustable opacity)
- Light/dark theme following the system preference, or forced via the View menu
- UI translations: English, Spanish, Portuguese, French, German, Japanese, Korean,
  Arabic, Hindi, Chinese, and Polish (more welcome — see [Translating](#translating)),
  switchable live from the Language menu with no restart required
- Two menu presentations sharing the same actions/state: a GNOME-style hamburger
  menu and a classic Win32-style menu bar (File/Edit/Format/View/Help) — Language
  lives in the hamburger menu only

## Building

Requires a C++17 toolchain, Meson, and GTK4/libadwaita development packages.
gtkmm-4.0/glibmm-2.68/pangomm-2.48/cairomm-1.16/libsigc++-3.0 are *not* required to
be pre-installed: if `pkg-config` can't find them, Meson automatically builds them
as subprojects (see `subprojects/*.wrap`) — the first build will take a while longer
because of this.

```sh
meson setup build
meson compile -C build
meson test -C build      # unit tests + desktop-file-validate + appstreamcli validate
```

Run without installing:

```sh
GSETTINGS_SCHEMA_DIR=build/data NOTEPAD_LOCALEDIR=build/po ./build/src/notepad
```

`NOTEPAD_LOCALEDIR` is only needed for an uninstalled run: `LOCALEDIR` is baked
into the binary as the *installed* prefix's locale directory, so without this
override gettext finds no `.mo` catalogs there and the app (including the
Language menu) silently stays in English.

Or, do the whole thing (configure if needed, compile, test, launch) in one step:

```sh
./deliver.sh                # build, test, then launch
./deliver.sh file.txt       # ...and open file.txt on launch
./deliver.sh --skip-tests   # build and launch, skip the test suite
./deliver.sh --no-run       # build (and test) only, don't launch
```

## Installing

Add the self-hosted Flatpak repo (GPG-signed, updated on every release) and install:

```sh
flatpak remote-add --user --if-not-exists notepad https://chiddekel.github.io/gnome-notepad/notepad.flatpakrepo
flatpak install --user notepad io.github.chiddekel.Notepad
```

`--user` is recommended over a system-wide install/remote-add.

## Building the Flatpak

```sh
flatpak-builder --user --install --force-clean build-flatpak \
  build-aux/flatpak/io.github.chiddekel.Notepad.json
flatpak run io.github.chiddekel.Notepad
```

CI builds this on every push and, on tagged releases, publishes a standalone
`.flatpak` bundle as a GitHub Release plus the self-hosted repo above (see
`.github/workflows/flatpak.yml`).

## Translating

UI strings are marked for translation with gettext; `po/POTFILES.in` lists every
source file that contains one. To update `po/notepad.pot` after changing strings, or
to add a new language:

```sh
ninja -C build notepad-pot        # regenerate po/notepad.pot
ninja -C build notepad-update-po  # merge new/changed strings into every po/*.po
```

Add new locale codes to `po/LINGUAS` (one per line) and create the corresponding
`po/<code>.po`.

The app's description strings shown in GNOME Software (`data/*.desktop.in`'s
`Comment=`, and `data/*.metainfo.xml.in`'s summary/description/screenshot caption)
are translated too, merged in at build time via `i18n.merge_file()` — but since
`xgettext` doesn't extract from those file formats via `POTFILES.in` here, their
four strings are hand-maintained directly in each `po/<code>.po` (and in
`po/notepad.pot`, so `notepad-update-po` doesn't drop them). Keep them in sync by
hand if that English source text ever changes.

## Known differences from legacy-notepad

A few Windows-specific features have no Linux equivalent, or a fundamentally
different one, and were deliberately not replicated 1:1:

- **Icon-library extraction** (`Change Icon...` from `.exe`/`.dll`/`.icl`/`.mun`
  files) has no Linux analog and was dropped entirely.
- **Always-on-top** and **window transparency** are wired up (menu items, state,
  persistence) but are best-effort: GTK4 has no public API for either, and most
  Wayland compositors refuse both outright.
- **Page Setup** isn't a separate dialog/menu item — `GtkPrintOperation`'s print
  dialog already includes page setup.

## License

MIT — see `LICENSE`.
