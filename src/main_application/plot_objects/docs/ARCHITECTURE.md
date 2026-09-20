# `src/main_application/plot_objects/` — architecture overview

> **Updated after a `git pull` (main @ `39a6dc9e`, previously documented at
> `23e65b7a`).** Two things changed here worth knowing before reading
> further: (1) `main_application` now depends on an external submodule,
> `third_party/LumosAlgo`, instead of `src/interfaces/cpp/duoplot/` — every
> `duoplot::`/`#include "duoplot/..."` reference below now reads `lumos::`/
> `#include "lumos/..."` in the actual source (see
> `src/main_application/docs/ARCHITECTURE.md`'s dedicated section for what
> this means and the risk it creates). (2) A new plot type,
> `ScreenSpacePrimitive`, was added, and the `UserSuppliedProperties`
> rewrite that came with this pull introduced a real uninitialized-value
> regression — see the "Known rough edges" section, both added below.

This module implements one C++ class per plottable data type the wire
protocol can carry (see `src/interfaces/cpp/duoplot/docs/PROTOCOL.md`'s
`Function` enum) — `Plot2D`, `Plot3D`, `Scatter2D`, `Surf`, `DrawMesh`,
`ImShow`, etc. Each class knows how to turn a client's raw byte payload into
GPU-ready buffers and render itself inside whatever axes box
`src/main_application/axes/` has currently set up (`AxesRenderer::plotBegin()`
configures the shared shaders' MVP matrix; every class here just calls
`shader.use()` and draws). This is by far the largest of the module docs
written so far: 25 files across ~20 subfolders, ~7500 lines.

**GUI-toolkit note (relevant to the planned wxWidgets → Qt migration): no
wxWidgets usage anywhere in this module** (confirmed by search — zero `wx*`
references in any of the ~25 files). Every class here is pure OpenGL +
plain-data conversion code; the wx boundary is one layer further out, in
`PlotPane`/`PlotDataHandler` (which own and drain these objects) and in
`MainWindow`'s receive-thread/timer plumbing (see the `communication/` and
`axes/` docs). Nothing in this module should need to change for a Qt port.

## The class hierarchy

`PlotObjectBase` (`plot_object_base/`) is the abstract base every
TCP-client-driven plot type derives from:

```
PlotObjectBase
├── Plot2D, Plot3D               (thick anti-aliased lines)
├── FastPlot2D, FastPlot3D       (thin native GL lines, "fast" path)
├── LineCollection2D/3D          (disjoint line segments)
├── PlotCollection2D/3D          (batched ragged multi-polylines)
├── Scatter2D, Scatter3D         (point clouds)
├── Stairs                       (step function)
├── Stem                         (vertical lines + point caps)
├── Surf                         (height-field triangle mesh)
├── DrawMesh                     (arbitrary triangle mesh)
├── ImShow                       (textured quad / image display)
├── ScrollingPlot2D              (REAL_TIME_PLOT backing object)
└── ScreenSpacePrimitive         (screen-space overlay triangles — new, see below)
```

A second, **entirely separate** hierarchy exists for streamed serial/UART
device data (`stream_object_base/`, `stream_objects/{plot2d,scatter,stairs}`)
— see "The stream-object hierarchy is unrelated" below.

All `PlotObjectBase`-derived instances for one pane are owned by
`PlotDataHandler` (`src/main_application/plot_data_handler.h/.cpp`, outside
this module) as `std::vector<PlotObjectBase*> plot_datas_`.

## Two-phase construction: a background-thread / GUI-thread split

This is the single most important architectural fact about this module, and
it's not obvious from reading any one file in isolation:

1. **`convertRawData(hdr, attributes, user_supplied_properties, data_ptr)`**
   — a `static` method every concrete class implements. It does **pure CPU
   work only** (no OpenGL calls): reinterpreting the raw payload bytes as
   the right numeric type (via the `applyConverter`/`Converter` pattern, see
   below), doing whatever per-type geometry math is needed (billboard-quad
   triangulation for `Plot2D`/`Plot3D`, per-quad normal computation for
   `Surf`/`DrawMesh`, run-length dedup for `Plot2D`), and returning a
   heap-allocated `ConvertedDataBase`-derived struct of plain arrays.
   **This runs on `MainWindow`'s background TCP receive thread** — see
   `main_window_receive.cpp::convertPlotObjectData()`, a big `switch` over
   `Function` that calls the matching type's `convertRawData` synchronously
   as part of handling each incoming message, before anything is queued for
   the GUI thread.
2. **The constructor** (and `updateWithNewData`/`appendNewData` for types
   that support it) takes that already-converted data and does the actual
   `glGenBuffers`/`glBufferData`/`glGenTextures` GPU upload. **This runs on
   the wx GUI thread**, from `PlotDataHandler::addData()` (called when
   `PlotPane`'s `wxTimer` drains the per-element queue — see the
   `communication/` and `axes/` docs for that pipeline).

The reason for this split: OpenGL calls are only valid on the thread that
owns the GL context (the GUI thread), but geometry conversion — especially
`Plot2D`/`Plot3D`'s per-segment triangulation or `Surf`'s per-quad normal
computation — is CPU-bound work that would otherwise stall rendering if done
synchronously on the GUI thread. Splitting it this way lets that work happen
concurrently with rendering. **When refactoring or porting this module,
preserve this split** — moving GL calls into `convertRawData`, or expensive
math into the constructor, would undo this concurrency benefit (or, worse,
crash on a GL-context-less thread).

## The `applyConverter` / `Converter` / `InputParams` pattern

Declared in `src/main_application/outer_converter.h` (one level up from this
module, shared infrastructure):

```cpp
template <typename O, typename I, typename C>
std::shared_ptr<const O> applyConverter(const uint8_t* data, DataType data_type, C&& converter, const I& input_params);
```

It's a runtime-`DataType`-to-compile-time-template dispatch: a 10-way
`if/else if` over every `DataType` enum value, each branch calling
`converter.convert<T>(data, input_params)` with `T` fixed to the matching
C++ type (`float`, `double`, `int8_t`, ... `uint64_t`). Every concrete plot
type's `.cpp` file repeats the same three-piece boilerplate to use this:

```cpp
namespace {
struct InputParams { /* per-type fields copied out of PlotObjectAttributes */ };
struct ConvertedData : ConvertedDataBase { /* owning raw pointers, ~delete[] in destructor */ };
template <typename T> std::shared_ptr<const ConvertedData> convertData(const uint8_t*, const InputParams&);
struct Converter { template <typename T> auto convert(...) const { return convertData<T>(...); } };
}
```

**This exact boilerplate is duplicated near-verbatim across roughly 16
files** (every TCP-driven type except the trivial ones). It's a strong
candidate for consolidation if this module is ever refactored, but as of
this writing each type still hand-rolls its own copy — check the specific
file rather than assuming a shared implementation exists.

## `PlotObjectBase` responsibilities (the shared 80%)

- **Property/flag assignment** (`assignProperties`, called from the
  constructor; `updateProperties`, called from `postInitialize` on
  live-update) — reads a `UserSuppliedProperties` struct (defined outside
  this module, `src/main_application/user_supplied_properties.h`) and
  resolves each optional property to either the user-supplied value or a
  type-appropriate default. Two properties specifically fall back to
  `ColorPicker` (`src/main_application/color_picker.h`, also outside this
  module) when not explicitly set: `color_` and `face_color_` — this is the
  automatic per-series color cycling every plot function benefits from when
  the client doesn't pass `properties::Color`.
- **Persistent vs. per-message GL usage hint**: `is_updateable_` (from
  `properties::UPDATABLE`/live-update support) selects `GL_DYNAMIC_DRAW` vs.
  `GL_STATIC_DRAW` for buffer allocation (`dynamic_or_static_usage_`).
- **Lazy bounding-box caching**: `getMinMaxVectors()` calls the pure-virtual
  `findMinMax()` exactly once (`min_max_calculated_` guard) and caches the
  result — this feeds the axes' auto-fit-to-data behavior (outside this
  module).
- **`preRender(shader)`**: uploads the custom-transform uniforms
  (rotation/translation/scale matrices) shared by every shader that supports
  per-object transforms — called from most (not all — see the capability
  matrix) concrete `render()` implementations.
- **`modifyShader()`**: pushes the resolved base color into every plot
  shader's uniform (called once, not per-frame, when the object is created
  or its properties change) — several subclasses override this to push
  additional type-specific uniforms (point size, scatter mode, silhouette).
- **Default legend**: `getLegendProperties()` returns a label-only
  `LegendProperties` (no color/type set) unless overridden — several
  concrete types never override this (see the capability matrix), meaning
  they get no meaningful legend swatch.

## Point-picking (`ConvertedDataBase::getClosestPoint`)

`ConvertedDataBase` declares a virtual `getClosestPoint(const Line3D<double>&)`
returning the closest data point to a query ray plus its distance; the base
implementation just logs `"Called base function!"` and returns a placeholder.
**Only `Plot3D` and `Scatter3D` override it** (brute-force linear scan over
their converted point arrays) — point-selection/picking is effectively a
3D-only feature in this module today. The query ray itself comes from
`AxesRenderer::getLine()` (see the `axes/` docs' per-frame lifecycle,
step 2ii) — this is the module's one direct integration point with the axes
box's mouse-interaction system, beyond just rendering inside it.

## The stream-object hierarchy is unrelated

`stream_object_base/` (`StreamObjectBase`) and
`stream_objects/{plot2d,scatter,stairs}/` (`Plot2DStream`, `ScatterStream`,
`StairsStream`) form a **second, independent class hierarchy** that has
nothing to do with the TCP client protocol described above. These objects
are fed by `src/main_application/serial_interface/` (`objects::BaseObject`,
a UART/serial device data stream), configured via `SubscribedStreamSettings`
(`project_state/project_settings.h`), and each maintains a fixed 500-sample
"shift and prepend" ring buffer, re-uploading the whole buffer via
`glBufferSubData` on every new sample. All three implementations
(`Plot2DStream`/`ScatterStream`/`StairsStream`) are near-identical copies of
this same buffer-shifting logic with only the vertex layout and draw mode
differing — another clear duplication candidate. They share only
`ShaderCollection`/`RGBTripletf` with the main hierarchy; they do not derive
from `PlotObjectBase` and are not constructed via `convertRawData`/
`PlotDataHandler` at all (check `PlotPane`/wherever `SubscribedStreamSettings`
is consumed for how these get instantiated and driven — outside this
module's scope).

## `ScreenSpacePrimitive` (new plot type, `plot_objects/screen_space_primitive/`)

Wired to a new wire `Function::SCREEN_SPACE_PRIMITIVE` value. Structurally
the simplest member of the hierarchy: `convertData<T>` reinterprets the raw
payload directly as an array of `std::array<Point2f, 3>` (i.e. flat 2D
triangles, no z-coordinate at all), and `render()` draws them as
`GL_TRIANGLES` via a new dedicated `screen_space_shader` (added to
`ShaderCollection`, see `src/main_application/docs/ARCHITECTURE.md`).
**Unlike every other plot type, it does not participate in the axes/camera
transform** — `findMinMax()` unconditionally returns a fixed `[-1,1]³` box
rather than computing real bounds from the triangle data, and nothing in
its `render()` applies the per-pane MVP matrix the way `plotBegin()` sets
up for every other type (see the `axes/` docs' per-frame lifecycle). This
strongly suggests it's meant for screen-space UI overlays (e.g. custom
markers/icons drawn directly in normalized device coordinates) rather than
data-space plotting — check current client-library call sites (if any
exist yet in `third_party/LumosAlgo`'s `lumos::plotting::` or
`src/interfaces/cpp/duoplot/`) to confirm the intended use before assuming
it behaves like a normal plot type. Its `convertData` also has a dead local
variable (`tp`, declared then unused — `triangles`, a `VectorConstView`
built from the same pointer, is what's actually read) and a commented-out
`memcpy` alternative left in place.

## Known rough edges (found while reading, worth flagging before refactoring)

- **New regression: `has_line_style_`/`line_style_` are left uninitialized
  for every freshly-constructed plot object.** The `UserSuppliedProperties`
  rewrite (switching from a hand-rolled `OptionalParameter<T>` to real
  `std::optional<T>`, part of the same pull that swapped in LumosAlgo — see
  the note at the top of this file) renamed
  `PlotObjectBase::assignProperties` to `initializeProperties` and, in the
  process, **commented out the block that sets `has_line_style_`/
  `line_style_`** in that method (the equivalent block in `updateProperties`,
  the live-update path, was left intact). Since `PlotObjectBase`'s
  constructor calls `initializeProperties` unconditionally, and neither
  member has a default member initializer in the header
  (`plot_objects/plot_object_base/plot_object_base.h`: plain
  `LineStyle line_style_; bool has_line_style_;`), **every plot object
  constructed via the normal (non-live-update) path now reads these two
  members uninitialized** — this is undefined behavior, not just a wrong
  default. `Plot2D` is the type that actually consumes
  `has_line_style_`/`line_style_` (for dashed-line rendering) — its
  dash/solid line-style behavior on freshly-plotted (as opposed to
  updated) objects is now driven by garbage memory. Worth a one-line fix
  (restore the commented block, or give both members default member
  initializers) before this is relied upon.

- **`FastPlot2D` hardcodes its color to red** (`color_ = RGBTripletf(1.0f,
  0.0f, 0.0f); // TODO: Remove`, in the constructor, *after* the base class
  already resolved the correct user-supplied-or-picked color) — every
  `FAST_PLOT2` line renders red regardless of what the client requested.
  This looks like an accidental leftover from debugging, not intentional
  behavior.
- **`Surf::updateWithNewData` is entirely commented out** — the method
  exists (and overrides the virtual), but its body is a no-op wrapped in a
  block comment. Live/streaming updates to an existing `Surf` object do not
  actually work despite the API surface suggesting they should.
- **Inconsistent buffer-overflow handling for appendable scatter plots**:
  `Scatter2D::appendNewData` grows its buffer (reads existing data out,
  doubles `buffer_size_`, reallocates, copies back in) when it would
  overflow; `Scatter3D::appendNewData` in the same situation just logs
  `"Buffer overflow!"` and silently drops the new data. The two sibling
  classes handle the identical situation differently.
- **`ScrollingPlot2D` force-sets `is_updateable_ = true`** in both its
  constructor and `updateWithNewData`, each site marked
  `// TODO: Temporary hack` — this bypasses the normal
  `UserSuppliedProperties`-driven flag that every other type respects.
- **`Stem::modifyShader()` hardcodes point size to `10.0f`**, ignoring the
  `point_size_` property that `Scatter2D`/`Scatter3D` both honor from
  `properties::PointSize`.
- **`ImShow` narrows `DOUBLE`-typed images to `float` before texture
  upload** (`gl_data_type = GL_FLOAT` for both `FLOAT` and `DOUBLE` inputs;
  the `DOUBLE` branch of `convertData` explicitly casts down) — a client
  sending double-precision image data silently loses precision with no
  warning.
- **`Plot3D` has no `updateWithNewData`, `appendNewData`, or
  `getLegendProperties` override** — unlike `Plot2D`, it doesn't support
  live updates, and it falls back to `PlotObjectBase`'s default legend
  (label only, no line color swatch), even though a 3D line plot
  conceptually needs the same `LegendType::LINE` swatch `Plot2D` sets.
- **Heavy structural duplication**: beyond the `Converter`/`InputParams`
  pattern noted above, `plot_objects/utils.h`'s `findMinMaxFromTwoVectors`/
  `findMinMaxFromThreeVectors`/`findMinMaxFromThreeMatrices` each repeat the
  same 10-way `DataType` dispatch independently rather than sharing one
  generic implementation parameterized on vector count.

See [`CAPABILITY_MATRIX.md`](CAPABILITY_MATRIX.md) for a side-by-side
comparison of every plot type's rendering primitive, shader, and feature
support, and [`FILE_REFERENCE.md`](FILE_REFERENCE.md) for a per-file/
per-subfolder breakdown.
