Building cairomm-1.16.x (or later) with Visual Studio 2017 or later
=
* You will need Visual Studio 2017 (Visual Studio 15) or later. Building with 
Visual Studio 2015 or earlier is no longer supported due to C++17 requirements.
For Visual Studio 2017, using the latest 15.9.x is recommended.

* Using/Building the latest cairo 1.17.x release is recommended, due to 
numerous improvements on the Windows support, and supports building with Visual
Studio using Meson. Cairo can be built with Visual Studio 2015 or later, with
DLL/shared builds being recommended. Otherwise, you can install the latest Win32
GTK+ Development files from ftp://ftp.gnome.org/pub/GNOME/binaries/win32/gtk+/.
By any means, add the paths to headers and import libraries to Visual Studio, if
they are not already in `$(PREFIX)`, which is
`$(srcroot)\..\vs$(VSVER)\$(Platform)` by default.

* You will also need libsigc++-3.x built with Visual Studio 2017 or later

## Building with NMake
* In a Visual Studio command prompt, do the following:
```
cd $(srcroot)\MSVC_NMake

# Build
nmake /f Makefile.vc CFG=[release|debug] <PREFIX=...>

# Build the tests (the Boost C++ libraries are required; Boost must
# be built with the same compiler with the same release/debug
# configuration)

# BOOST_DLL=1 means that one is using the DLL builds of Boost to
# build the tests.
nmake /f Makefile.vc CFG=[release|debug] <PREFIX=...> <BOOST_DLL=1> tests

# Copy the built DLL and .lib and headers to $(PREFIX) in their
# appropriate locations
nmake /f Makefile.vc CFG=[release|debug] <PREFIX=...> install
```
* Notice that for the built DLL and .lib the Visual Studio version is no
longer `vc$(VSVER)0`, but is named like the following (Visual Studio version),
to follow what is done in other C++ libraries such as Boost:
  * 2017: `VSVER=15`, `cairomm-vc141-1_16.[dll|pdb|lib]`
  * 2019: `VSVER=16`, `cairomm-vc142-1_16.[dll|pdb|lib]`
  * 2022: `VSVER=17`, `cairomm-vc143-1_16.[dll|pdb|lib]`

* If using the old-style naming, i.e. `vc150`, is desired, such as when
rebuilding things that use cairomm is inconvenient, pass in `USE_COMPAT_LIBS=1`
in the NMake commandline. Note that this option is only recommended when
absolutely needed, and libsigc++-3.x should be built with this option enabled 
too.

## Building with Meson
Building cairomm with Meson for Visual Studio builds is now supported, with the
following notes:
* Please see `$(srcroot)/README.md` for more instructions on Meson builds.
* It is recommended that cairo-1.17.x is used, since it can be built with Meson
with Visual Studio, with the proper pkg-config files that Meson builds will try
to use first, which is best supported.
* Pass in the option `-Dmsvc14x-parallel-installable=false` to the Meson
configure command line to avoid having the toolset version in the final DLL and
.lib filenames; again, this is only recommended if it is inconvenient to
re-build the dependent code, and this option should be used when building items
that depend on cairomm, if this option is provided.
* Building from a GIT checkout is possible, which will enable maintainer mode by
default.  In maintainer mode, you will need the following items (their `bin`
directory must be in `%PATH%`):
```
mm-common
Doxygen   # Unless -Dbuild-documentation=false is used
LLVM      # Unless -Dbuild-documentation=false is used; likely needed by Doxygen
Graphviz  # Unless -Dbuild-documentation=false is used
```

Cedric Gustin
08/18/2006

Armin Burgmeier
09/29/2010

Chun-wei Fan
09/10/2015, updated 06/29/2023
