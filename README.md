# dosrender
Graphics rendering engine/demo for 16-bit DOS with mode 13 hex.

## Features
- [x] Rectangles
  - [x] Arbitrary size
  - [x] Arbitrary border and fill colors
  - [x] Transparency support
  - [x] Out-of-bounds support
- [x] Straight Lines
  - [x] Arbitrary slope
  - [x] Arbitrary color
  - [x] Out-of-bounds support
- [ ] Sprites
- [ ] Text

## Building
The project is written in mostly C89 with some C99 extensions provided by the Watcom compiler, e.g. array designators.
As such, `Open Watcom` (with 16-bit DOS support enabled during installation) is recommended for compiling out of the box,
as it provides appropriate support for real 16-bit compilation and memory management.

Contributions to improve portability across compilers are welcome.
