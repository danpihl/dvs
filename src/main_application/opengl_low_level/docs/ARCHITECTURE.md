# `src/main_application/opengl_low_level/` — architecture overview

> **Note (post-`39a6dc9e` pull):** `vertex_buffer.h`'s
> `#include "duoplot/math/math.h"` now reads `#include "lumos/math.h"` —
> part of the repo-wide swap to the external `third_party/LumosAlgo`
> submodule, see `src/main_application/docs/ARCHITECTURE.md`. Also,
> `opengl_header.h`'s Linux branch no longer defines
> `GL_GLEXT_PROTOTYPES` itself — that macro is now defined globally via
> CMake (`linux.cmake`) so it's already active before `gl.h` pulls in
> `glext.h`; the `#define` was replaced with a comment explaining why. No
> other change in this module.

The thin, shared OpenGL plumbing layer: a platform-specific GL header
shim, and a small RAII wrapper class around a VAO + its VBOs. Two headers,
no `.cpp` files (fully inline), ~250 lines total. Used pervasively —
`VertexBuffer` alone appears in ~19 files across `axes/` and `plot_objects/`.

**GUI-toolkit note (relevant to the planned wxWidgets → Qt migration): no
wxWidgets usage anywhere in this module.** This is raw OpenGL (desktop GL /
`GL_*` calls) with zero windowing-toolkit dependency — it doesn't know or
care whether the GL context it's drawing into was created by a
`wxGLCanvas` (current) or a `QOpenGLWidget`/`QOpenGLContext` (Qt). Both
files should carry over to a Qt port essentially unchanged, provided
whatever creates the GL context (currently `PlotPane : public wxGLCanvas`,
outside this module — see the axes docs) is what actually gets replaced.

## `opengl_header.h` — platform GL header shim

Pure preprocessor dispatch, no code: includes the right platform's GL
headers based on a compile definition set by CMake
(`src/CMakeLists.txt`, not this module) — `PLATFORM_LINUX_M` pulls in
`GL/{glut,gl,glx,glext}.h` with `GL_GLEXT_PROTOTYPES` defined (needed on
Linux to get extension function prototypes without a loader library);
`PLATFORM_APPLE_M` pulls in `<OpenGL/gl3.h>` (macOS's Core Profile GL3
header — no GLUT needed there). If neither macro is defined, this header
resolves to an empty file and nothing downstream that expects `GL*` symbols
will compile — there is no `#error` guard for an unrecognized platform.
Every other file in this module (and most of `axes/`) includes this header
transitively rather than a platform GL header directly, which is what
keeps the rest of the codebase platform-agnostic.

## `vertex_buffer.h` — VAO/VBO RAII wrapper

`OGLPrimitiveType` — a `uint64_t` enum mirroring the subset of GL primitive
draw modes this codebase uses (`POINTS, LINES, LINE_LOOP, LINE_STRIP,
TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN, QUADS`), each value literally equal
to the corresponding `GL_*` constant — exists so call sites can pass a
typed enum instead of a raw `GLenum`, but the values are wire-compatible
with GL's own if ever needed.

`VertexBuffer` — owns one Vertex Array Object plus a growable list of
Vertex Buffer Objects bound to sequential attribute indices (unless an
explicit index is given). Not copyable-safe (no copy constructor/assignment
declared, but it holds raw `GLuint` handles — copying one would double-free
on destruction; nothing currently stops that from compiling). Two
construction paths: default constructor (VAO id `0`, must call `init()`
before use) or `VertexBuffer(primitive_type)` (generates and binds the VAO
immediately). `~VertexBuffer()`/`clear()` delete all owned buffers and the
VAO (`clear()` additionally resets internal state so the object can be
reused as if freshly default-constructed).

Buffer-management methods, all templated on `T` (constrained by
`static_assert` to `float`, `int`, or `int32_t` — anything else fails to
compile with `"Only float and int supported for now!"`):

- `addBuffer(data, num_elements, num_dimensions[, usage])` — creates a new
  VBO, uploads `data` immediately, and binds it to the **next sequential**
  attribute index (`vertex_buffers_.size() - 1` at the time of the call —
  i.e. attribute index is implicit, determined by call order, not
  explicitly chosen). Two overloads: one defaulting `usage` to
  `GL_STATIC_DRAW`, one taking `usage` explicitly.
- `addBuffer(data, num_elements, num_dimensions, usage, input_idx)` — same,
  but binds to an **explicitly given** attribute index instead of the
  next-sequential one. Mixing this overload with the sequential one on the
  same `VertexBuffer` instance risks attribute-index collisions since
  `vertex_buffers_.size()` isn't reconciled against manually-chosen indices.
- `addExpandableBuffer<T>(total_num_elements, num_dimensions)` — allocates
  an uninitialized `GL_DYNAMIC_DRAW` buffer of the given capacity (data
  pointer `NULL`) for later per-frame updates via `updateBufferData` — this
  is the pattern most `axes/` renderer classes use for geometry that's
  rebuilt every frame (grid lines, silhouette, legend swatches) instead of
  managing raw buffers themselves.
- `updateBufferData(buffer_idx, data, num_elements, num_dimensions[, offset_in_elements])` —
  `glBufferSubData` into an already-allocated buffer; the offset overload
  allows partial updates starting mid-buffer.
- `getBufferData(buffer_idx, out_data, num_elements, num_dimensions)` —
  read-back via `glGetBufferSubData`; **hardcoded to `float`** regardless of
  what type the buffer at `buffer_idx` actually holds — calling this on an
  `int`/`int32_t` buffer will silently reinterpret the bytes as floats. No
  current caller does this today, but it's a trap if one is added.
- `render(num_elements[, primitive_type][, offset])` — three overloads:
  bare (draws using the type passed at construction), with an explicit
  `OGLPrimitiveType` override for that one call, and with both an offset
  and an override. Wraps VAO bind → `glDrawArrays` → unbind (VAO `0`).

## Integration pattern

Every OpenGL-drawing class in `axes/` that predates a shared-buffer
refactor manages its own raw `GLuint`/`float*` pair directly (see the
`axes/` `FILE_REFERENCE.md`'s cross-cutting note: `PlotBoxGrid`,
`PlotBoxSilhouette`, `PlotBoxWalls`, `PlotPaneBackground`), while newer/
simpler ones (`LegendRenderer`, `PointSelectionBox`) use `VertexBuffer`
directly. **There is no migration in progress to unify these** — both
patterns are live and idiomatic in different parts of the same directory;
don't assume one is "the old way" being phased out without checking git
history first.

## Known rough edges

- No `#error` fallback in `opengl_header.h` if neither `PLATFORM_LINUX_M`
  nor `PLATFORM_APPLE_M` is defined — a misconfigured build silently
  produces an empty header and confusing downstream compile errors instead
  of a clear platform-support message.
- `VertexBuffer` is not safely copyable (raw owning GL handles, no copy
  constructor deleted or defined) — relies on call sites never copying an
  instance; not enforced by the type system.
- `getBufferData` is hardcoded to `float` regardless of the buffer's actual
  uploaded type.
- Mixing the sequential-index and explicit-index `addBuffer` overloads on
  one instance can silently collide on attribute indices — no bounds/
  conflict checking is done.
