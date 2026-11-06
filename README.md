# vfs.pdf

A Kodi VFS add-on that opens a PDF as a folder holding a picture of each page:

    pdf://<the PDF's path, URL encoded>/0001.jpg

so anything in Kodi that shows pictures can show a PDF's pages. It was written
for game manuals, which [script.game.manuals](https://github.com/sunlollyking/script.game.manuals)
shows with it.

Only the `pdf://` protocol is registered, not the `.pdf` extension. Kodi adds
an add-on's extensions to its video, music and picture lists, so claiming
`.pdf` would put every PDF into every media listing; an add-on that wants a
PDF's pages builds the `pdf://` path itself.

Pages are drawn by [Poppler](https://poppler.freedesktop.org) onto white, as
JPEGs whose longer side is 2560 pixels: enough to zoom in on small print while
staying inside the texture size most devices allow.

## Build

Like any Kodi binary add-on, through Kodi's `cmake/addons`; `depends/common`
builds FreeType, libjpeg-turbo, libpng, OpenJPEG, zlib and Poppler's C++ frontend
statically, with iconv bundled on platforms that need it. The bundled versions
are FreeType 2.15.0 and Poppler 26.10.0. Building Poppler requires CMake 3.28 or
newer and a compiler with C++23 support.
Bundled Poppler retains native font configuration on Windows and Android;
Linux and Apple default to generic fonts without Fontconfig. This fallback is
sufficient for game manuals. A system Poppler is used instead if one is found,
its own dependencies coming from pkg-config.

Poppler must be built with OpenJPEG. Scanned manuals mostly keep their pages
as JPEG 2000, and without it those pages are drawn blank.

PNG support is required explicitly, using libpng 1.6.58 and zlib, so Poppler's
PNG export is available consistently. The add-on still exports pages as JPEGs.
Brotli and HarfBuzz are disabled to avoid additional dependencies: PDF streams
using `BrotliDecode` cannot be decoded, and HarfBuzz font subsetting is unavailable.
FreeType's optional zlib, bzip2, PNG, HarfBuzz, Brotli and Apple HVF dependencies
are disabled; in particular, this excludes WOFF2 and PNG bitmap fonts.

The page renderer's test needs Poppler and libjpeg but not Kodi:

    cmake -S tests -B build-tests && cmake --build build-tests
    ctest --test-dir build-tests

It renders a PDF it builds itself. Linux, including LibreELEC, has been built
and run. The webOS ARM add-on cross-builds with GCC 16.2.0, and its static
renderer tests pass under QEMU ARM; no physical webOS device was tested.
Native macOS ARM64 dependency/add-on builds and renderer tests passed (2/2).
Full iOS/tvOS ARM64 cross-builds passed with deployment target 12.0 retained;
runtime on 12.0 was not verified. Windows/UWP and Android builds of these
upgraded dependencies remain unverified.

## Licence

GPL-2.0-or-later, as Poppler.
