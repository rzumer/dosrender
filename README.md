# dosrender
Graphics rendering engine/demo for 16-bit DOS with mode 13 hex.

## Features
- [x] Lines
  - [x] Arbitrary slope
  - [x] Arbitrary color
  - [x] Out-of-bounds support
  - [ ] Transformation support
    - [x] Scaling/Mirroring
    - [x] 2D Rotation
    - [x] Shear
    - [ ] 3D Perspective
- [x] Rectangles
  - [x] Arbitrary size
  - [x] Arbitrary border and fill colors
  - [x] Transparency support
  - [x] Out-of-bounds support
  - [ ] Transformation support
    - [x] Scaling/Mirroring
    - [ ] 2D Rotation
    - [ ] 3D Perspective
- [x] Free-form polygons
  - [x] Arbitrary sides
  - [x] Arbitrary border color
  - [x] Out-of-bounds support
  - [ ] Arbitrary fill color
  - [x] Transformation support
    - [x] Scaling/Mirroring
    - [x] 2D/3D Rotation
    - [x] Shear
- [ ] Sprites
- [ ] Text

## Building
The project is written in mostly C89 with some C99 extensions provided by the Watcom compiler, e.g. array designators.
As such, `Open Watcom` (with 16-bit DOS support enabled during installation) is recommended for compiling out of the box,
as it provides appropriate support for real 16-bit compilation and memory management.

Contributions to improve portability across compilers are welcome.
