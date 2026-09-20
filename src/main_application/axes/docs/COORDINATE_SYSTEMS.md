# Coordinate systems, matrices, and view math

Reference for the exact transforms this module builds. Everything here is
derived from `axes_interactor.cpp`, `axes_renderer.cpp`,
`structures/view_angles.cpp`, `axes_side_configuration.cpp`,
`structures/axes_limits.cpp`, and `grid_numbers.cpp`. Matrix chains are
listed per call site because **they are not identical everywhere** —
several omit the translate step, or use a different subset of scale/rotation
— and conflating them is an easy refactor mistake.

## The three coordinate spaces

1. **Data space** — whatever units the plotted data is in. Bounded by
   `AxesLimits` (`lim_min_`, `lim_max_`, both `Vec3d`).
2. **Canonical box space** — a fixed cube, roughly `[-1, 1]³` (before the
   window-scale/rotation-shrink adjustments described below), that the box
   walls/grid/silhouette geometry is hardcoded in (see e.g.
   `plot_box_walls.cpp`'s `walls_vertices[]`). Data space maps into this
   space by `scale_mat_` (per-axis `1 / (axes_scale/2)`) composed with a
   translate by `-axes_center` when centering is needed.
3. **Screen/NDC space** — standard OpenGL clip space, reached via
   `projection_mat_ * view_mat_ * model_mat_ * ...`.

## `AxesLimits` (`structures/axes_limits.h/.cpp`)

```
getAxesCenter() = (min + max) / 2
getAxesScale()  = (max - min)          // NOT half — callers divide by 2 themselves
```
Every renderer function that needs the box's half-extent computes
`axes_limits_.getAxesScale() / 2.0` locally — `AxesLimits` itself has no
"half scale" accessor. `incrementMinMax(dv)` shifts both min and max by the
same delta (used by panning).

## `ViewAngles` (`structures/view_angles.h/.cpp`)

- `azimuth_` clamped to `[-π, π]`, `elevation_` clamped to `[-π/2, π/2]`
  (`setAngles`). `changeAnglesWithDelta` wraps azimuth around ±π instead of
  clamping (so azimuth rotation is a full loop; elevation is not).
- `angle_limit_` (default 5° = `5·π/180`) is the snap tolerance, settable via
  `setAngleLimit`/`AxesInteractor::setViewAnglesSnapAngle`.
- **Snapped angles** (`snapped_azimuth_`/`snapped_elevation_`) are recomputed
  on every `setAngles` call via `setSnapAngles()`:
  - `isCloseToSnap()` — true only if *both* elevation is near {0, ±90°} AND
    azimuth is near {0, ±90°, ±180°} (within `angle_limit_`).
  - If close, `calcAzimuthSnapAngle`/`calcElevationSnapAngle` round to the
    nearest of those candidate angles; otherwise the snapped value equals the
    raw value (i.e. "snapped" only snaps near axis-aligned views — small
    corrections elsewhere pass through unchanged).
- **Snapping-axis predicates** — `isSnappedAlongX/Y/Z()` each check both
  azimuth and elevation against `angle_limit_` independently; `getSnappingAxis()`
  returns the first of X/Y/Z that matches, else `SnappingAxis::SA_None`.
  Meaning: "along X" = looking down the X axis (elevation flat, azimuth
  ±90°), "along Y" = azimuth 0 or 180°, "along Z" = elevation ±90°
  (looking straight down/up) regardless of azimuth.
- **Look-direction predicates** — `isSnappedLookingAlongPositiveX/NegativeX/
  PositiveY/NegativeY/PositiveZ/NegativeZ()` are finer-grained than the
  axis predicates (they also fix the *sign*/direction) and are what
  `AxesSideConfiguration`'s fuller (currently commented-out) logic was built
  around — only `is_snapped` (an OR of all six) is actually consumed by the
  active code today.
- **Rotation matrices** — three different constructions exist and are
  **not interchangeable**:
  - `getRotationMatrix()` = `rotationMatrixX(elevation) · rotationMatrixY(azimuth)`
    (uses **raw**, unsnapped angles) — has the `// TODO: Inconsistent`
    comment; check current callers before trusting this one matches visual
    box orientation.
  - `getSnappedRotationMatrix()` = `rotationMatrixX(-π/2) · rotationMatrixX(snapped_elevation) · rotationMatrixZ(snapped_azimuth)`
    — this is what panning (`AxesInteractor::changePan`) uses to convert a
    screen-space drag into a data-space translation.
  - The box's actual on-screen rotation (`AxesRenderer::updateStates`'s
    `model_mat_`) uses yet a **third** construction:
    `rotationMatrixZ(-snapped_azimuth) · rotationMatrixX(-snapped_elevation)`
    (negated angles, Z-then-X order) — computed inline in
    `axes_interactor.cpp::registerMouseReleased` and
    `axes_renderer.cpp::updateStates`, not via a `ViewAngles` method at all.
  - `getAngleAxis()`/`getSnappedAngleAxis()` convert the first/second forms
    above to axis-angle representation via `rotationMatrixToAxisAngle` — used
    for... (no current call site found in this module; check
    `main_application` broadly before assuming these are dead).

## Mouse interaction → view state (`AxesInteractor`)

`registerMouseDragInput(axis, x, y, dx, dy)`:
```
dx_mod = 250 · dx / window_width
dy_mod = 250 · dy / window_height
```
then dispatched by effective interaction type (override takes precedence
over the persistent type):

| Type | Call | Gain constant |
|---|---|---|
| `ROTATE` | `changeRotation(dx_mod·g, dy_mod·g, axis)` | `rotation_mouse_gain = 0.01` |
| `ZOOM` | `changeZoom(dy_mod·g, axis)` | `zoom_mouse_gain = 0.005` |
| `PAN` | `changePan(dx_mod·g, dy_mod·g, axis)` | `pan_mouse_gain = 0.005` |
| `POINT_SELECTION` | records `(x, y)` as a pending query point | n/a |

`MouseInteractionAxis` (`X, Y, Z, XY, YZ, XZ, ALL`) is a UI-level "which axes
should this drag affect" restriction — orthogonal to `SnappingAxis` (which
describes the *current view orientation*, not user intent).

- **`changeRotation`**: masks `dx`/`dy` by axis restriction, then calls
  `view_angles_.changeAnglesWithDelta(dx·sa.x, dy·sa.y)` — i.e. `dx` always
  maps to azimuth and `dy` to elevation; `MouseInteractionAxis::X` zeroes
  the elevation term (locks elevation), `::Y` zeroes azimuth (locks azimuth).
- **`changeZoom`**: builds a per-axis mask `sa` — when the view is
  axis-snapped (`SnappingAxis != SA_None`), the mask locks *that* axis (zoom
  affects only the other two); when unsnapped, the mask instead comes from
  the requested `MouseInteractionAxis`. Then:
  ```
  inc_vec = dy · axes_scale · sa
  axes_limits_.min -= inc_vec
  axes_limits_.max += inc_vec
  ```
  (scale grows/shrinks symmetrically around the current center). Also
  recomputes a tick-spacing hint `inc0` via `changeIncrement` (doubles or
  halves toward keeping roughly `num_axes_ticks` grid lines visible) — note
  `inc0` is computed but its only consumers are within this same file; check
  before assuming it feeds `GridVector` generation (it currently doesn't —
  `generateAxisVector` derives its own spacing independently, see below).
- **`changePan`**: builds an axis mask from `MouseInteractionAxis` only
  (panning ignores view-snapping), rotates the screen-space drag vector
  `(-dx, dy, 0)` into data space via `getSnappedRotationMatrix()ᵀ`, scales by
  `axes_scale` and the mask, then calls `axes_limits_.incrementMinMax(v_scaled)`.

## `AxesSideConfiguration` (`axes_side_configuration.h/.cpp`)

Recomputed every frame from the **snapped** view angles; picks which face of
the box (and which side numbers/letters sit on) should be treated as the
far/back side for the current view, so grid lines and tick numbers always
render on the side away from the camera. Active logic (all angles in
radians, azimuth ∈ [-π,π], elevation ∈ [-π/2,π/2]):

| Field | Value | Condition |
|---|---|---|
| `xy_plane_z_value` | `-1` else `+1` | elevation ≥ 0 |
| `xz_plane_y_value` | `+1` else `-1` | −π/2 ≤ azimuth ≤ π/2 |
| `yz_plane_x_value` | `+1` else `-1` | azimuth ≥ 0 |
| `z_axes_numbers_y_value` | `+1` else `-1` | azimuth > 0 |
| `z_axes_numbers_x_value` | `-1` else `+1` | −π/2 ≤ azimuth < π/2 |
| `y_axes_numbers_x_value` | = `yz_plane_x_value` | (copied) |
| `y_axes_numbers_z_value` | = `xy_plane_z_value` | (copied) |
| `is_snapped` | OR of all six `ViewAngles::isSnappedLookingAlong*` predicates | |

A large commented-out block (see `ARCHITECTURE.md` rough edges) contains a
more elaborate version of this table keyed off the six directional snap
predicates and `perspective_projection` — currently unused, but likely the
right starting point if visual bugs appear specifically on snapped/axis-
aligned views.

## The MVP chains, by call site

All use `glm::mat4`. `projection_mat_` is `orth_projection_mat_` or
`persp_projection_mat_` depending on `use_perspective_proj_`. `view_mat_` is
constant: `glm::lookAt((0,-6,0), (0,0,0), (0,0,1))` — the camera never
moves; **all view changes happen in `model_mat_`/`scale_mat_`/
`window_scale_mat_`/translate, not the camera.**

| Call site | Chain | Notes |
|---|---|---|
| `AxesRenderer::plotBegin()` (data rendering) | `proj · view · model · scale_mat · window_scale_mat · t_mat` | `t_mat` = translate by `-axes_center`; `scale_mat` = `1/(axes_scale/2)` per axis. This is the matrix hand to plot-object shaders. |
| `renderBoxGrid()` | `proj · view · model · scale_mat · window_scale_mat` | Same scale, **no** `t_mat` — box grid geometry is already centered at origin in its own local space. |
| `renderPlotBox()` (walls) | `proj · view · model · window_scale_mat` | No `scale_mat` at all — walls span a fixed `[-1,1]` shell scaled only by `window_scale_mat`. |
| Silhouette render (inside `render()`) | `proj · view · model · scale_mat(=identity 1.0) · window_scale_mat` | `scale_mat_` is explicitly reset to identity first — silhouette, like walls, isn't scaled by axes extent. |
| `renderPointSelection()` | `view · model · local_scale_mat · window_scale_mat · t_mat` for unprojection, then `orth_projection_mat_ · view_mat_` for the on-screen box render | Two different matrices used for two different purposes in the same function — projecting the 3D point vs. drawing the fixed-size on-screen marker. |
| `renderBackground()` | `orth_projection_mat_ · view_mat_ · rotation_mat(90° about X)` | Always orthographic regardless of `use_perspective_proj_`; background is a screen-aligned quad, not a data-space object. |
| `renderLegend()` | `orth_projection_mat_ · view_mat_` | Always orthographic; legend is screen-space UI, not data. |
| `drawGridNumbers()` (per axis) | `view · model · scale_mat_axis` where `scale_mat_axis` divides by scale on **only one axis at a time** (x, y, or z) | Three separate `view_model_x/y/z` matrices — tick numbers for the X axis are positioned using a matrix that only compensates for X-axis scale, etc. |
| `AxesInteractor::registerMouseReleased()` (box-zoom unprojection) | independently reconstructs `rot_mat = rotationMatrixZ(-snapped_az) · rotationMatrixX(-snapped_el)`, its own `window_scale_mat` (different constants than `AxesRenderer`'s), and `view_mat = lookAt((0,-6,0),(0,0,0),(0,0,1))` | Does **not** call into `AxesRenderer` or share its matrices — a parallel, hand-rolled reconstruction. If `AxesRenderer`'s window-scale formula changes, this needs updating to match or box-zoom will unproject against the wrong geometry. |
| `ZoomRect::render()` | uses matrices passed in by `AxesRenderer::render()` (`view_mat_`, `model_mat_ · window_scale_mat_`, `projection_mat_`) | Unlike `registerMouseReleased`, this one *is* fed `AxesRenderer`'s live matrices — the drag-preview rectangle and the final committed zoom are computed via two different code paths that happen to need the same inputs. |

## "Shrink while rotating" (`window_scale_mat_`)

Only applied in orthographic mode when `scale_on_rotation_` is true
(`AxesRenderer::updateStates`):
```
az = |sin(2·snapped_azimuth)|^0.6 · 0.7
el = |sin(2·snapped_elevation)|^0.7 · 0.5
s  = sqrt(az² + el²)
window_scale_mat_.diag(0..2) = scale_vector_ - s   // scale_vector_ default (2.5,2.5,2.5)
```
This shrinks the box slightly mid-rotation (when neither snapped angle is at
a multiple of 90°) purely so the rotating box doesn't clip the viewport edges
in orthographic projection; it's a no-op in perspective mode or when
`scale_on_rotation_` is false. `setAxesBoxScaleFactor`/`getAxesBoxScaleFactor`
control the baseline `scale_vector_` (client-settable via
`duoplot::setAxesBoxScaleFactor`, see the interfaces docs).

## Clip planes (`AxesRenderer::enableClipPlanes`)

Six planes, one per axis-bound (`x0,x1,y0,y1,z0,z1`), each built via
`planeFromThreePoints` from three corner points offset by a small margin
`f = 0.1` around `axes_center ± axes_scale/2`, then optionally inverted
(`Planed(-a,-b,-c,d)`) so the plane's positive half-space is "inside the box."
Uniforms are set on **six** shaders individually
(`basic_plot_shader`, `img_plot_shader`, `draw_mesh_shader`, `scatter_shader`,
`plot_2d_shader`, `plot_3d_shader`) — adding a new plot-type shader that
needs clipping means adding it here too; there's no shared/looped list.

## Grid tick generation (`AxesInteractor::generateGridVectors` → `generateAxisVector`)

Per axis, independently:
```
d = max - min
d_inc = 2^floor(log2(d / num_axes_ticks))     // power-of-two step size
val = min - (min mod d_inc)                    // align first candidate to a multiple of d_inc
while val < max: collect (val - offset) if val > min; val += d_inc
```
`offset` is `axes_center` for that axis (so returned tick values are
relative to center, matching how `PlotBoxGrid`/`drawGridNumbers` consume
them — they re-add the center where needed, e.g. `gv.x.data[k] + axes_center.x`
in `grid_numbers.cpp`). Hard-capped at `GridVector::kMaxNumGridNumbers = 30`
via `DUOPLOT_ASSERT` — an axis whose range doesn't converge within
`num_axes_ticks * 3` iterations logs `"ERROR: Number of lines grew a lot!"`
and breaks out (not a hard failure, but a sign the requested range/tick
count combination is degenerate).

## Box/wall/silhouette geometry construction pattern

`PlotBoxWalls`, `PlotBoxSilhouette`, and `PlotBoxGrid` all share one pattern:
a hardcoded template vertex array for a unit cube's three visible-face
triangles/lines, with one coordinate per vertex group left at a `0.0f`
placeholder. Each frame, `setIndices(first, last, dimension_idx, val)`
overwrites just that placeholder dimension across the relevant vertex range
with the current `AxesSideConfiguration` value (`xy_plane_z_value` etc.) —
this is how the box "flips" which face is drawn as the far side without
regenerating the whole vertex buffer. `PlotBoxGrid` additionally has to
rebuild its buffer's *content* (not just flip a plane) every frame from
`GridVectors`, since the number/position of grid lines changes with zoom.

## Legend layout (`LegendRenderer::render`)

Box size is derived from the widest label's measured text width
(`calculateStringSize`) plus empirically-chosen margins (`text_x_offset`,
`text_x_margin` = `150/axes_width`), then scaled by `scale_factor_`
(client-settable legend scale). Per-entry vertical spacing is a fixed
`legend_element_z_delta = 140/axes_height`. Three `LegendType`s render
differently: `LINE` (a short line segment swatch), `DOT` (a single point,
respecting `ScatterStyle`), `POLYGON` (either a flat quad with edge/face
color, or — if `has_color_map` — a colored ring swatch built by
`renderColorMapLegend`, a 6-segment fan gradient plus an outline).

## Text rendering scale (`text_rendering.cpp` + `text_rendering_impl/`) — rewritten post-`39a6dc9e`

**This subsystem was completely rewritten** in the most recent pull (see
`ARCHITECTURE.md`'s top note) from a per-glyph immediate-mode FreeType
renderer to a batched glyph-atlas renderer with HarfBuzz text shaping. The
pixel-to-NDC scale relationship is unchanged in form:
```
x_scale = scale * kTextScaleParameter(=1000) / axes_width
y_scale = scale * kTextScaleParameter / axes_height
```
still calibrated against `Roboto-Regular.ttf` loaded at 68px
(`label_text_store::init(kFontPath, 68)`, called from `TextRenderer::init()`)
— so this constant still needs recalibrating if the font or base pixel size
ever changes. **What changed underneath:**

- **String-width measurement now uses real HarfBuzz shaping**
  (`TextRenderer::calculateStringSize`): a `hb_buffer_t` is built from the
  UTF-8 text, `hb_shape()`'d against the loaded `hb_font_t`, and the glyph
  advances summed (`x_advance / 64.0f` — HarfBuzz reports advances in
  26.6 fixed-point). This replaces the old naive "sum each character's
  raw FreeType advance" loop, and is now kerning/ligature-aware. **Text
  height, however, is now only an approximation** — `ascender / 64.0f`
  from the FreeType face's global ascender metric, not a per-string
  measured bounding box like the old implementation computed. Height
  will no longer vary between e.g. an all-caps string and a string with
  descenders, which the old per-glyph-bbox approach did capture.
- **The old hardcoded per-glyph baseline nudges are gone.** The previous
  implementation special-cased `-`, `j`/`y`/`g`, and `p`/`q` with manual
  vertical/horizontal offsets to compensate for FreeType baseline quirks
  (see the pre-pull note this replaced). The new renderer positions every
  glyph from its own atlas-stored bearing/size (`GlyphData::bearing`/
  `size` in `font_atlas.h`), so those hacks are no longer needed —
  **verify text baseline alignment visually if you're touching this area**,
  since the fix relies on the atlas glyph metrics being correct rather
  than empirically-tuned per-character constants.
- **Rendering is now batched, not immediate.** `renderTextFromCenter`/
  `RightCenter`/`LeftCenter` no longer issue a draw call each — they just
  push a `Label{text, loc, color, x_scale, y_scale}` onto
  `pending_labels_`. Nothing actually renders until an explicit
  **`flush()`** call, which hands every pending label to
  `label_text_store::add_text()`, builds one combined GPU vertex/index
  buffer (`set_buffers()`), and draws the whole batch in one
  `paint_text()` call (`glDrawElements`, presumably — the actual buffer
  layout lives in `text_rendering_impl/buffers/`). **Callers must call
  `flush()` once after all their `renderTextFrom*` calls for that pass, or
  nothing appears** — confirmed call sites: `AxesRenderer::render()` (for
  grid-number/axis-letter text) and `LegendRenderer::render()` (for legend
  labels), each flushing its own batch independently rather than sharing
  one global end-of-frame flush.
- **`TextRenderer` is now a value type designed to be copied.** It holds a
  `std::shared_ptr<TextRenderContext>` (the actual GL resources — font
  atlas, shader program, init flag) so copies share one underlying GL
  context's resources; `init()` must be called once per GL context (i.e.
  once per `PlotPane`, since each pane owns its own `wxGLCanvas` context)
  and is idempotent (`if (ctx_->initialized) return true;`).
  `pending_labels_` is deliberately *not* copied on copy-construction/
  assignment — only the shared context is.
- **The text shader is now a small inline GLSL pair** (`kVertSrc`/
  `kFragSrc`, raw string literals in `text_rendering.cpp`), compiled and
  linked directly in `TextRenderer::init()` rather than going through the
  shared `ShaderBase`/`ShaderCollection` machinery every other shader in
  this codebase uses (see `src/main_application/docs/ARCHITECTURE.md`'s
  `shader.h`/`.cpp` section) — this text shader is not a member of
  `ShaderCollection` and doesn't support custom transforms, clip planes,
  or any of the other `BaseUniformHandles` uniforms. A one-off,
  self-contained shader pipeline living outside the codebase's usual
  shader-management pattern.
- **New third-party dependency: HarfBuzz** (`hb.h`/`hb-ft.h`), vendored/
  linked per-platform in `CMakeLists.txt` (macOS: a vendored static build
  under `src/externals/harfbuzz/build/`; Linux: resolved via
  `pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)`).

See `FILE_REFERENCE.md` for the new `text_rendering_impl/` file list
(`font_atlas`, `label_text_store`, and the `buffers/` GL-object wrapper
set).
