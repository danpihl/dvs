# `src/main_application/axes/` — architecture overview

> **Updated after a `git pull` (main @ `39a6dc9e`, previously documented at
> `23e65b7a`).** Two things to know: (1) `main_application` now depends on
> the external `third_party/LumosAlgo` submodule instead of
> `src/interfaces/cpp/duoplot/` for math/logging types — every `duoplot::`/
> `DUOPLOT_ASSERT`/`#include "duoplot/..."` reference below now reads
> `lumos::`/`LUMOS_ASSERT`/`#include "lumos/..."` in the actual source (see
> `src/main_application/docs/ARCHITECTURE.md`'s dedicated section — this
> also renamed `MatrixFixed<T,R,C>` to `FixedSizeMatrix<T,R,C>`). (2) **Text
> rendering was completely rewritten** — `text_rendering.h/.cpp`'s old
> per-glyph immediate-mode FreeType approach (with hardcoded baseline
> nudges for `-`/`j`/`y`/`g`/`p`/`q`) is gone, replaced by a batched
> glyph-atlas renderer with proper HarfBuzz text shaping, in a new
> `text_rendering_impl/` subdirectory. See
> [`COORDINATE_SYSTEMS.md`](COORDINATE_SYSTEMS.md)'s rewritten text-scale
> section and [`FILE_REFERENCE.md`](FILE_REFERENCE.md) for the new files.
> Everything else in this document (the `AxesInteractor`/`AxesRenderer`
> split, the per-frame lifecycle, the MVP chains) is unchanged.

This module renders and manages one **axes box** — the 3D grid/wall/silhouette
frame, tick numbers, title, legend, and pane background — for a single plot
pane. It owns the camera/view state (rotation, pan, zoom, projection mode)
and all the mouse-interaction logic for manipulating that view. It does
**not** own or render the actual plotted data (lines, scatter points,
surfaces, meshes) — those live under `src/main_application/plot_objects/`
and are driven by `PlotDataHandler`. This module's job is to draw the box
those objects appear inside, and to hand plot-object rendering the
model-view-projection matrix it computed for the current frame.

The only consumer of this module is `PlotPane`
(`src/main_application/plot_pane.h` / `.cpp`) — one `wxGLCanvas`-derived pane
owns exactly one `AxesInteractor` + one `AxesRenderer` pair.

## The two-class split: `AxesInteractor` (state) vs `AxesRenderer` (view)

- **`AxesInteractor`** (`axes_interactor.h/.cpp`) is pure state and math, no
  OpenGL calls. It owns the current `ViewAngles`, `AxesLimits`, the mouse
  interaction mode, and produces `GridVectors` (tick positions) on demand.
  Mouse event handlers on `PlotPane` call into this class
  (`registerMousePressed/Released/DragInput`, `setMouseInteractionType`,
  `setOverriddenMouseInteractionType`) to mutate view state; nothing here
  touches a GPU resource.
- **`AxesRenderer`** (`axes_renderer.h/.cpp`) is the OpenGL-facing half. It
  owns shader-adjacent helper objects (`PlotBoxWalls`, `PlotBoxGrid`,
  `PlotBoxSilhouette`, `PlotPaneBackground`, `LegendRenderer`,
  `PointSelectionBox`, `TextRenderer`, `ZoomRect`) and computes the actual
  `glm::mat4` matrices used for rendering. It is **not incrementally
  stateful across frames in the ways that matter** — `updateStates()` is
  called once per frame with a full snapshot of whatever `AxesInteractor`
  currently holds (plus pane geometry, mouse position, legend entries), and
  every subsequent `render*()` call that frame reads from that snapshot.

This is a soft model/view split (not literally named that in the code), and
it means: to understand "what does the box look like right now," read
`AxesInteractor`'s state; to understand "how is that state turned into
pixels," read `AxesRenderer`.

## Per-frame lifecycle (driven by `PlotPane`)

1. **Input phase** (wx mouse/keyboard events, outside this module): calls
   land on `AxesInteractor` — e.g. dragging calls
   `registerMouseDragInput(axis, x, y, dx, dy)`, which dispatches by the
   current `MouseInteractionType` (or an "overridden" one set while a
   modifier key is held) to `changeRotation`/`changeZoom`/`changePan`, each
   mutating `view_angles_`/`axes_limits_` directly. Releasing the mouse after
   a box-zoom drag (see below) commits a new `AxesLimits` via
   `registerMouseReleased`.
2. **Paint phase**, once per frame, in `PlotPane`'s paint handler:
   1. `AxesSideConfiguration` is rebuilt fresh from
      `axes_interactor_.getViewAngles()` — it is **not** persisted state, just
      a pure function of the current (snapped) view angles, recomputed every
      frame (see `axes_side_configuration.cpp`).
   2. `axes_renderer_->updateStates(...)` — pushes the full per-frame
      snapshot in: axes limits, view angles, query point, grid vectors, side
      configuration, projection mode, pane width/height, mouse state, legend
      properties, pane name. This is also where the model rotation matrix
      (`model_mat_`), the "square axes" projection recompute, and the
      "shrink pane while rotating" `window_scale_mat_` are computed (see
      Coordinate Systems doc for the exact math).
   3. `axes_renderer_->render()` — draws the box **chrome**: title, plot box
      walls + grid + silhouette, the zoom-drag rectangle if active, and grid
      tick numbers/axis letters (via `drawGridNumbers`). Background and
      legend are deliberately *not* drawn here.
   4. `axes_renderer_->plotBegin()` — computes the data-space MVP matrix
      (translate-to-center + scale-to-unit-cube + rotation + camera +
      projection) and pushes it into **every plot-type shader's** uniforms
      (`basic_plot_shader`, `img_plot_shader`, `draw_mesh_shader`,
      `scatter_shader`, `plot_2d_shader`, `plot_3d_shader`), and sets up the 6
      GL clip planes (if `clipping_on`). This is the handoff point: whatever
      calls `plotBegin()` next expects `PlotDataHandler`/`PlotObjectBase`
      instances (owned elsewhere, in `plot_objects/`) to issue their draw
      calls using the shaders just configured.
   5. *(external to this module)* `PlotDataHandler` renders each
      `PlotObjectBase` in the pane.
   6. `axes_renderer_->plotEnd()` — disables blending, then renders the
      legend (`renderLegend`) and the pane background
      (`renderBackground`) — background is drawn *last*, after both chrome
      and data, not first.
   7. Optionally `axes_renderer_->renderPointSelection(closest_point)` if a
      point-probe query resolved to a data point that frame (query-point
      logic itself lives in `AxesInteractor`/`PointSelectionBox`).

## Coordinate systems, in one paragraph

Data-space axes limits (`AxesLimits`, arbitrary min/max per axis) are mapped
every frame to a canonical unit cube via translate-by-center then
scale-by-half-range; a separate, fixed camera (`view_mat_`, a `glm::lookAt`
that never moves) looks at that cube from a constant position; box
*orientation* comes entirely from `model_mat_`, built directly from the
current (snapped) view angles' rotation matrix. In other words: **panning and
zooming only ever change what `AxesLimits` window maps onto the unit cube —
the camera itself never moves, and "rotating the view" is actually rotating
the box via `model_mat_`, not the camera.** See
[`COORDINATE_SYSTEMS.md`](COORDINATE_SYSTEMS.md) for the exact matrix chains,
which differ slightly by call site and are easy to get subtly wrong if
touched.

## Rotation snapping

`ViewAngles` (`structures/view_angles.h/.cpp`) tracks both a raw azimuth/
elevation (continuously updated by mouse drag) and a *snapped* pair, rounded
to the nearest axis-aligned view whenever within `angle_limit_` (default 5°,
`setViewAnglesSnapAngle`). Almost every rendering call
(`axes_side_configuration.cpp`, `grid_numbers.cpp`, `plot_box_*`,
`AxesRenderer::updateStates`) uses the **snapped** angles, so the box
visually locks onto clean axis-aligned faces even though the underlying drag
motion is continuous. `getSnappingAxis()` reports which single axis (if any)
the view is currently snapped along — this gates several box-zoom and
number-placement behaviors.

## Box-zoom (drag-to-zoom-to-region)

When the view is snapped to a single axis and the interaction mode is
`ZOOM`, dragging draws a 2D rectangle (`ZoomRect`) instead of doing
continuous scroll-zoom. On mouse release,
`AxesInteractor::registerMouseReleased` unprojects the rectangle's screen
corners back into data space (using an independently-reconstructed
model/view/projection setup local to that function — it does not reuse
`AxesRenderer`'s matrices) and commits the result as new `AxesLimits`. This
is the single most mathematically dense function in the module — read it
carefully, in full, before modifying axis-zoom or snapping behavior; it does
not share code with the equivalent per-frame computation in `ZoomRect::render`
or `AxesRenderer::updateStates`'s query-point unprojection, despite doing
conceptually similar work.

## Integration points outside this module

- `PlotPaneSettings` (`src/main_application/project_state/project_settings.h`)
  supplies the static-per-pane configuration `AxesRenderer` is constructed
  with and reads every frame: colors, on/off flags (`grid_on`, `plot_box_on`,
  `axes_numbers_on`, `axes_letters_on`, `clipping_on`), `pane_radius`, and
  `ProjectionMode` (perspective vs orthographic) — this struct is **not**
  part of this module; it's owned by project/config state.
- `ShaderCollection` and the individual `*Shader` uniform-handle structs
  (`shader.h`, one directory up) are constructed elsewhere and passed in by
  reference/copy; this module only calls `.use()` and sets uniforms on them.
- `duoplot::properties::ColorMap` / `ScatterStyle` (from the client-facing
  interface library, `src/interfaces/cpp/duoplot/`) leak into
  `LegendProperties` — the legend needs to know how a given plot object was
  styled in order to draw a matching swatch.

## Known rough edges (read before refactoring)

- `axes.h` has a commented-out `#include "axes/axes_painter.h"` — a deleted
  predecessor file; harmless but stale.
- `AxesRenderer::renderHandle()` and `renderInteractionLetter()` both start
  with an unconditional `return;` before their body (`// TODO: For demo,
  ... is not used`) — the code below is dead but retained. Don't assume
  either function does anything currently.
- `AxesSideConfiguration`'s constructor (`axes_side_configuration.cpp`) has
  roughly 80 lines of commented-out alternate branch logic (keyed off
  `isSnappedLookingAlong{Positive,Negative}{X,Y,Z}()`) below the small
  active implementation. If side/number-placement looks wrong specifically
  on axis-aligned ("snapped") views, that commented block is very likely
  relevant prior art, not dead weight.
- `ViewAngles::getRotationMatrix()` carries a `// TODO: Inconsistent with
  'getSnappedRotationMatrix'!` — the two methods use different rotation-axis
  order/base rotations (`X∘Y` vs `X(-90°)∘X∘Z`). Do not assume they're
  interchangeable or that one is simply the "snapped version" of the other.
- `AxesInteractor::changeZoom`'s snapped-axis branch has a
  `// TODO: Add back` next to a condition that currently always evaluates
  true in practice — treat any snapped-axis zoom peculiarities as suspect
  here first.
- Numerous empirically-tuned magic numbers with no derivation given: mouse
  gain scaling (`250.0f` in `registerMouseDragInput`), the window-scale
  constants (`f = 2.5`, `2.7`) in `registerMouseReleased`/`updateStates`,
  and pixel-offset constants throughout `legend_renderer.cpp` and
  `grid_numbers.cpp` (`150.0f`, `100.0f`, `140.0f`, all commented
  "empirically found"). Treat these as load-bearing visual tuning, not
  arbitrary defaults — change with visual verification, not by inspection.
- `text_rendering.cpp::renderTextFromLeftCenter` hardcodes per-glyph
  vertical/horizontal nudges for `-`, `j`/`y`/`g`, and `p`/`q` — a
  workaround for FreeType baseline behavior with the one bundled font
  (`Roboto-Regular.ttf`), not a general text-layout mechanism. Changing
  fonts would likely require re-tuning these.

See [`COORDINATE_SYSTEMS.md`](COORDINATE_SYSTEMS.md) for the exact
matrix/math reference and [`FILE_REFERENCE.md`](FILE_REFERENCE.md) for a
per-file breakdown.
