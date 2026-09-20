# File-by-file reference — `src/main_application/axes/`

> **Note (post-`39a6dc9e` pull):** `duoplot::`/`DUOPLOT_ASSERT`/
> `#include "duoplot/..."` throughout this file now read `lumos::`/
> `LUMOS_ASSERT`/`#include "lumos/..."` in the actual source (LumosAlgo
> submodule swap — see `src/main_application/docs/ARCHITECTURE.md`), and
> `MatrixFixed<T,R,C>` is now `FixedSizeMatrix<T,R,C>`. The text-rendering
> entry below was rewritten wholesale. Every other file's changes were
> checked against the diff and are limited to: this rename; switching
> `axes_renderer.cpp`/`grid_numbers.cpp`/`legend_renderer.cpp` from the old
> `shader_collection_.text_shader.use()` + raw `glUniform3f(text_color_uniform,
> ...)` calls to the new `TextRenderer::init()`/`setColor()`/`flush()` API
> (`drawXLetter`/`drawYLetter`/`drawZLetter`/`drawXAxisNumbers`/
> `drawYAxisNumbers`/`drawZAxisNumbers`/`drawGridNumbers` in
> `grid_numbers.cpp` also changed their `TextRenderer` parameter from
> `const TextRenderer&` to `TextRenderer&`, since batching now mutates it)
> — meaning **`ShaderCollection::text_shader` (`shader.h`) is still
> declared but no longer used by any text-drawing code**, effectively dead
> weight now; and one small math-API rename in
> `structures/view_angles.cpp`: the free function `rotationMatrixToAxisAngle()`
> was replaced by a static factory `AxisAngled::fromRotationMatrix()`,
> with `getAngleAxis()`/`getSnappedAngleAxis()` now manually copying a
> dynamic `Matrix<double>` into a `FixedSizeMatrix<double,3,3>` element by
> element before calling it (that factory only accepts the fixed-size
> type). `ShaderCollection` also gained a new `screen_space_shader` member
> — see `plot_objects/`'s docs for the new `ScreenSpacePrimitive` type that
> uses it.

Quick-lookup for refactoring. For the module's overall shape see
[`ARCHITECTURE.md`](ARCHITECTURE.md); for exact matrix/math derivations see
[`COORDINATE_SYSTEMS.md`](COORDINATE_SYSTEMS.md).

## Top level

### `axes.h`
Pure aggregating include (`AxesInteractor`, `AxesRenderer`, and the four
`structures/` headers). Has one stale commented-out include
(`axes_painter.h`, a deleted predecessor file) — harmless, safe to remove in
a cleanup pass.

### `axes_interactor.h` / `axes_interactor.cpp`
The view **state** class — see `ARCHITECTURE.md`. Owns `ViewAngles`,
`AxesLimits` (both current and "default", for `resetView()`), the current
and overridden `MouseInteractionType`, and pending point-selection query
state. Key methods: `registerMousePressed/Released/DragInput` (input →
state), `setViewAngles`/`setAxesLimits` (programmatic view control, called
when a client sends `VIEW`/`AXES_2D`/`AXES_3D` commands — see `plot_pane.cpp`),
`generateGridVectors()` (delegates to free function `generateAxisVector`,
defined in this .cpp, not declared in the header — file-local), `resetView()`.
Also defines, at namespace scope (not member functions): `MouseInteractionAxis`
enum, `QueryPoint` struct, and declares (in the header only) free functions
`findFirstPointInInterval`/`findFirstPointBeforeMin` — these have **no
definition anywhere under `src/main_application/`** (confirmed by search);
they are either dead declarations left from a removed implementation, or
would fail to link if ever called. Do not add a call site without first
writing the definition.
`registerMouseReleased` is the box-zoom commit path — see
`COORDINATE_SYSTEMS.md`'s MVP table; it independently reconstructs a
model/view/projection setup rather than reusing `AxesRenderer`'s.

### `axes_renderer.h` / `axes_renderer.cpp`
The OpenGL-facing **view** class — see `ARCHITECTURE.md`. Constructed once
per pane with a `ShaderCollection`, `PlotPaneSettings`, and tab background
color; owns `PlotBoxWalls`, `PlotBoxSilhouette`, `PlotBoxGrid`,
`PlotPaneBackground`, `LegendRenderer`, `PointSelectionBox`, `TextRenderer`.
Public surface: `updateStates(...)` (per-frame snapshot in, see
`ARCHITECTURE.md` for the full parameter list), `render()` (chrome),
`plotBegin()`/`plotEnd()` (bracket data rendering — see lifecycle in
`ARCHITECTURE.md`), `renderPointSelection`, `renderHelpPane` (declared,
check current definition/usage — not covered in detail here),
`activateGlobalIllumination`/`resetGlobalIllumination`, `setTitle`,
`getAxesBoxScaleFactor`/`setAxesBoxScaleFactor`, `setScaleOnRotation`,
`setAxesSquare`. Private render helpers (`renderBackground`,
`renderPlotBox`, `renderBoxGrid`, `enableClipPlanes`, `renderLegend`,
`renderHandle`, `renderTitle`, `renderInteractionLetter`, `renderViewAngles`,
`setClipPlane`) are all called from `render()`/`plotEnd()`/`plotBegin()` —
see `ARCHITECTURE.md`'s dead-code note on `renderHandle`/
`renderInteractionLetter` (both short-circuit with an unconditional
`return;`). `getLine()` exposes `line_`, the unprojected 3D ray computed in
`updateStates()` when a query point is pending — consumed by whatever does
point-picking against plot geometry (outside this module).

### `axes_side_configuration.h` / `.cpp`
`AxesSideConfiguration` — a plain struct (not a class with invariants),
rebuilt from scratch every frame in its constructor from the current
`ViewAngles` + a `perspective_projection` bool (currently unused by the
active logic branch — only referenced inside the commented-out alternate
implementation). See `COORDINATE_SYSTEMS.md` for the exact per-field
derivation table and the large dead/commented alternate implementation
worth reading before touching snapped-view visual bugs.

### `grid_numbers.h` / `.cpp`
Free function `drawGridNumbers(...)` plus three static-like (translation
unit local) helpers `drawXAxisNumbers`/`drawYAxisNumbers`/`drawZAxisNumbers`
and `drawXLetter`/`drawYLetter`/`drawZLetter`. Not a class — this is pure
procedural rendering code called once per frame from
`AxesRenderer::render()`. Each axis's numbers/letter use a *different*
one-axis-scaled view-model matrix (`view_model_x/y/z` — see
`COORDINATE_SYSTEMS.md`'s MVP table) and are skipped entirely when that
axis is snapped (unless in perspective mode, where numbers are always
shown). Uses `formatNumber` (from `misc/misc.h`, outside this module) to
render tick values to 3 significant figures. Several `cond`/`cond2`
branches select left/right/center text anchoring based on azimuth
range — some are dead (commented out) alternates left in place; read
current logic carefully rather than trusting variable names alone (e.g.
`cond` in `drawXAxisNumbers` is computed but never used by the active code
path).

### `legend_properties.h`
Plain struct `LegendProperties` (label, `LegendType`, colors, colormap,
point size, scatter style) — the per-legend-entry data `PlotDataHandler` (or
whatever assembles the legend list, outside this module) must fill in per
plotted object. Has a top-of-file TODO: `"This should be filled in
plot_object_base and the individual object types"` — i.e. the intended
design is for each `PlotObjectBase` subclass to know how to describe its own
legend entry, which may not be fully wired up yet; check current call sites
in `plot_objects/` before assuming every plot type populates every field
correctly. `LegendType` enum: `LINE, POLYGON, DOT`.

### `legend_renderer.h` / `.cpp`
`LegendRenderer` — draws the legend box (edge + inner-fill quad via two
`VertexBuffer`s) and, per entry, a swatch (`LINE`/`DOT`/`POLYGON`, with a
colormap-ring special case for `POLYGON` when `has_color_map`) plus the
label text. See `COORDINATE_SYSTEMS.md` for the box-sizing formula.
`kMaxNumPoints = 100` bounds the shared scratch buffers (`points_`,
`colors_`) used to build each swatch's geometry before upload —
`renderColorMapLegend`'s `num_segments * 7` vertices (6 segments here) must
stay under that cap if `num_segments` is ever made configurable.
`setLegendScaleFactor` clamps to `1.0f` if the requested factor is `≤ 0.1f`
(silent fallback, not an error).

### `plot_box_grid.h` / `.cpp`
`PlotBoxGrid` — renders the *interior* grid lines on all three planes
(XY/XZ/YZ). Allocates a fixed `float* grid_points_` sized for
`GridVector::kMaxNumGridNumbers * 2 * 2 * 3` vertices *per plane*, times 3
planes (see constructor) — i.e. capacity is fixed at construction time and
assumes the 30-tick cap in `GridVectors`; raising that cap requires updating
this allocation too. `fillXYGrid`/`fillXZGrid`/`fillYZGrid` each write two
line-segments per tick value (one for each perpendicular grid direction).
`render()` re-fills the whole buffer and re-uploads via
`glBufferSubData` every frame — no dirty-tracking.

### `plot_box_silhouette.h` / `.cpp`
`PlotBoxSilhouette` — draws the box's outline edges (as `GL_LINES`) on
whichever three faces `AxesSideConfiguration` selects as visible. Static
template array `silhouette_vertices[]` (24 vertices, one 0.0f placeholder
dimension per face group) is copied into an owned `data_array_` at
construction, then `setIndices()` overwrites the placeholder dimension per
frame — see `COORDINATE_SYSTEMS.md`'s shared-geometry-pattern note. Fixed
vertex count (`3U * 8U = 24`) — this file does not use `GridVectors` at all
(unlike `PlotBoxGrid`), since a box silhouette is always exactly 3 faces ×
4 edges regardless of tick density.

### `plot_box_walls.h` / `.cpp`
`PlotBoxWalls` — draws the (optionally) filled box walls as `GL_TRIANGLES`
(2 triangles per face × 3 faces = 18 vertices, `walls_vertices[]` template).
Same placeholder-dimension-overwrite pattern as the silhouette. Named index
constants (`kXYFirstIdx`/`kXYLastIdx`/`kXYChangeDimension`, etc., declared
`static constexpr` in the header) define which vertex ranges belong to which
face and which coordinate axis gets overwritten — keep these in sync with
`walls_vertices[]`'s actual layout if either changes.

### `plot_pane_background.h` / `.cpp`
`PlotPaneBackground` — draws a rounded-rectangle-ish pane background (4
quads at the corners, `num_corner_segments_`/`num_corner_vertices_` declared
in the header but **not actually used to build a rounded corner arc** in
the current `.cpp` — the render function only ever emits 4 flat rectangle
corners, not a segmented curve; treat the "rounded corner" as aspirational/
unfinished unless you find it wired up elsewhere). `radius_` (constructor
parameter, from `PlotPaneSettings::pane_radius`) scales how far the corner
rectangles are inset. Always draws in the fixed `orthographic × lookAt`
matrix (see `COORDINATE_SYSTEMS.md`) — this is screen-space chrome, not a
data-space object.

### `point_selection_box.h` / `.cpp`
`PointSelectionBox` — draws a small triangle-pair "flag" marker at a
clicked/queried data point, flipped up or down (`draw_up`) depending on
which half of the screen the point falls in (avoids the marker rendering
off-screen). Uses a single `VertexBuffer` (`pane_vao_`) updated in place
each call — no persistent per-point state beyond the current render call's
`closest_point`.

### `structures/axes_limits.h` / `.cpp`
`AxesLimits` — see `COORDINATE_SYSTEMS.md`. Also tracks a `tick_begin_`
(`setTickBegin`/`getTickBegin`) that is set but — check current callers —
does not appear to be read anywhere in this module's active rendering path;
verify before assuming it's load-bearing.

### `structures/axes_settings.h` / `.cpp`
`AxesSettings` — a different, smaller struct than `PlotPaneSettings` (which
lives in `project_state/project_settings.h`, outside this module) despite
the similar name and overlapping fields (both have color fields, on/off
flags). `AxesSettings` is what `AxesInteractor` is constructed with (mostly
just for `num_axes_ticks`, used by grid generation); `PlotPaneSettings` is
what `AxesRenderer` is constructed with. **Don't confuse the two when
tracing where a rendering setting actually comes from** — check which class
reads which struct. `AxesSettings`'s constructor also has an extensive block
of commented-out "Light" theme color values alongside the active "Dark"
theme — a light-mode palette was evidently drafted but never wired to a
toggle.

### `structures/grid_vectors.h`
Pure data: `MouseInteractionType` enum (`ROTATE, PAN, ZOOM,
POINT_SELECTION, UNCHANGED, UNKNOWN`), `GridVector` (fixed
`std::array<double, 30>` plus `num_valid_values`/`min_value`/`max_value`/
`grid_spacing`/`range`), `GridVectors` (one `GridVector` per axis). See
`COORDINATE_SYSTEMS.md` for how these are populated.

### `structures/view_angles.h` / `.cpp`
`ViewAngles` and `SnappingAxis` enum. See `COORDINATE_SYSTEMS.md` for the
full behavioral reference — this is the module's most subtle piece of
state; read that doc's `ViewAngles` section in full before modifying
anything here, especially the three different rotation-matrix constructions.

### `text_rendering.h` / `.cpp` — rewritten post-`39a6dc9e`, see note below

**Completely rewritten** in the most recent pull; the description that used
to be here (a `characters` map keyed by ASCII char, `initFreetype()` with a
disabled early-return, per-glyph immediate-mode drawing) no longer matches
the source and has been removed from this document. See
`COORDINATE_SYSTEMS.md`'s "Text rendering scale" section for the full
replacement writeup. Briefly: `TextRenderer` (`text_rendering.h`) is now a
small copyable value type holding a `shared_ptr<TextRenderContext>`
(GL-resource ownership) plus a `pending_labels_` batch; its `.cpp` owns the
inline text shader source, the batching (`renderTextFrom*`/`flush()`), and
HarfBuzz-based `calculateStringSize()`. The actual glyph-atlas/GPU-buffer
work now lives in `text_rendering_impl/` (below), which this file depends
on but does not itself implement.

### `text_rendering_impl/` — new subsystem (glyph atlas + batched text buffers)

Added in the same pull as the `text_rendering.cpp` rewrite; not present in
the previously-documented version of this module.

- **`font_atlas.h`/`.cpp`** — `font_atlas`, `GlyphData`. Owns the FreeType
  face (`FT_Face`) and a HarfBuzz font (`hb_font_t*`, built from the same
  face via `hb_ft_font_create`-style setup, kept alive for shaping calls)
  plus a GPU texture atlas. Two-phase: `init(font_path, pixel_size)` loads
  the font and creates the FreeType/HarfBuzz handles only;
  `build_atlas(glyph_ids)` is called separately once the set of glyphs
  actually needed is known, and packs them into one GPU texture
  (`texture_id`), recording each glyph's atlas UV rect and bitmap
  size/bearing in `glyph_map`, keyed by **FreeType glyph index, not Unicode
  codepoint** (important: HarfBuzz shaping already resolves codepoints to
  glyph indices, including for ligatures/substitutions, so codepoint-keyed
  lookup would be wrong for shaped text).
- **`label_text_store.h`/`.cpp`** — `label_text_store`, `label_text`. The
  batch accumulator: `init()` sets up the `font_atlas`; `add_text(label,
  loc, color, angle, x_size, y_size)` appends one `label_text` entry
  (supports a rotation angle — `label_angle` — which `TextRenderer`
  currently always passes as `0.0f`, so per-label rotation is plumbed
  through but not yet exposed at the `TextRenderer` API level);
  `set_buffers()` walks every accumulated label
  (`get_buffer()`/`rotate_pt()` internally, building per-glyph quads —
  `rotate_pt` is presumably how a non-zero `label_angle` would rotate each
  glyph's quad about the label's origin) and uploads one combined
  vertex/index buffer via `gBuffers`; `paint_text()` binds the atlas
  texture and issues the actual draw call for the whole batch.
- **`buffers/`** — a small, self-contained OpenGL object wrapper set,
  independent of `opengl_low_level/vertex_buffer.h`'s `VertexBuffer`
  (yet another buffer-management pattern alongside the two already noted
  in this module's cross-cutting notes below — raw `GLuint` pairs,
  `VertexBuffer` the wrapper class, and now this):
  - `VertexBuffer.h`/`.cpp`, `IndexBuffer.h`/`.cpp` — thin `GLuint`-owning
    RAII wrappers around one VBO/one EBO respectively (bind/unbind,
    upload).
  - `VertexBufferLayout.h`/`.cpp` — describes a vertex format (attribute
    count/type/stride) to configure a `VertexArray` from, decoupling
    layout description from buffer creation (a pattern distinct from this
    codebase's other buffer helper, `opengl_low_level/vertex_buffer.h`,
    which bakes the layout into each `addBuffer` call instead).
  - `VertexArray.h`/`.cpp` — owns one VAO, binds a `VertexBuffer` +
    `VertexBufferLayout` pair to it.
  - `gBuffers.h`/`.cpp` — the top-level object `label_text_store` actually
    uses: bundles one `VertexArray` + `VertexBuffer` + `IndexBuffer`
    together as the single GPU-side representation of "one batch of text
    quads."

### `zoom_rect.h` / `.cpp`
`ZoomRect` — draws the live drag-preview rectangle during a box-zoom drag,
projected onto whichever plane the view is currently snapped to
(`SnappingAxis`). Falls back to printing `"Shouldn't end up here!"` to
stdout if rendered while `SnappingAxis::SA_None` (i.e. this should never be
called while unsnapped — `AxesInteractor::registerMouseDragInput`'s `ZOOM`
case has a commented-out guard for this, see `ARCHITECTURE.md`'s
`changeZoom` TODO — meaning it's theoretically possible, if that guard is
ever restored differently, for this fallback path to be hit). Static
module-level mutable arrays (`rect_vertices[]`, `rect_color[]`) are
overwritten in place each render call rather than using instance members —
harmless as long as only one `ZoomRect` instance ever renders per frame
(true today, one per `AxesRenderer`/pane), but not thread-safe and not
reentrant if that assumption ever changes.

## Cross-cutting notes

- Every `Plot*` OpenGL helper class in this directory (`PlotBoxGrid`,
  `PlotBoxSilhouette`, `PlotBoxWalls`, `PlotPaneBackground`) follows the same
  resource pattern: raw `new[]`-allocated `float*` buffer + a
  `GLuint vertex_buffer_`/`vertex_buffer_array_` pair, freed manually in the
  destructor, uploaded once in the constructor (`GL_DYNAMIC_DRAW`) and
  updated per frame via `glBufferSubData`. None of them use the shared
  `VertexBuffer` wrapper (`opengl_low_level/vertex_buffer.h`) that
  `LegendRenderer`/`PointSelectionBox` use instead — a pre-existing
  inconsistency in this module, not something introduced by one file; worth
  knowing before assuming there's one canonical way buffers are managed here.
- Everything in this directory does `using namespace duoplot;` at file
  scope (pulling in the client-interface math library's `Vec2d`/`Vec3d`/
  `Matrixd`/etc.) — be alert to name collisions if adding new types.
