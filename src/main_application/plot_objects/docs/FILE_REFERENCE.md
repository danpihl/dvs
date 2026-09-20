# File-by-file reference — `src/main_application/plot_objects/`

Per-subfolder breakdown. See [`ARCHITECTURE.md`](ARCHITECTURE.md) for the
shared design (two-phase construction, the `Converter`/`applyConverter`
pattern, `PlotObjectBase`'s responsibilities) and
[`CAPABILITY_MATRIX.md`](CAPABILITY_MATRIX.md) for a side-by-side feature
comparison — this document focuses on what's specific to each file.

## Top level

### `plot_objects.h`
Pure aggregating include — pulls in every concrete TCP-driven plot type's
header (not the `stream_object_base`/`stream_objects` hierarchy, which has
its own separate aggregator, `stream_objects/stream_objects.h`).

### `utils.h`
Shared free functions used across most concrete types:
- `getNumDimensionsFromFunction(Function)`: maps a wire `Function` to a
  "how many x/y/z-like arrays does this carry" count — has a `// TODO` note
  admitting the naming is misleading for `IM_SHOW` (an image is "1
  dimension" here despite being 2D pixel data) and suggests renaming to
  "num components."
- `dataTypeToGLInt(DataType)`: maps the wire `DataType` enum to a GL type
  enum (`GL_FLOAT`, `GL_BYTE`, etc.); asserts/throws on `INT64`/`UINT64`
  (`"Haven't found int64/uint64 in opengl enums yet..."` — there is no GL
  equivalent, so these are unsupported for anything that goes through this
  function, though other paths in this module handle 64-bit types directly
  via templates).
- `findMinMaxFromTwoVectors`/`findMinMaxFromThreeVectors`/
  `findMinMaxFromThreeMatrices` (each with an `...Internal<T>` template
  helper): compute a 2D or 3D axis-aligned bounding box directly from a raw
  received-data buffer, dispatching on `DataType` via a hand-written 10-way
  `if/else if` (not `applyConverter`/`Converter` — a second, independent
  dispatch mechanism doing the same kind of thing). Used by most
  `findMinMax()` overrides so they don't need a `ConvertedData` round-trip
  just to compute bounds.

## `plot_object_base/` — the foundation

### `plot_object_base.h`
Declares `isPlotDataFunction(Function)` (a free function — which `Function`
values represent actual plot data vs. control commands; mirrors, but is
independently maintained from, the interface library's own function
categorization — see the interfaces protocol doc), `ConvertedDataBase`
(virtual base with the `getClosestPoint` hook), `PlotObjectAttributes`
(header-derived metadata snapshot — constructed directly from a
`CommunicationHeader`, reading whichever fields are present via
`hasObjectWithType` checks; fields left absent in the header are left
default-initialized, i.e. garbage for POD members — callers must know which
fields a given `Function` actually populates), and `PlotObjectBase` itself
(see `ARCHITECTURE.md` for its responsibilities).

### `plot_object_base.cpp`
`assignProperties` was renamed to `initializeProperties` in the most recent
pull (same role, called once from the constructor) — **see
`ARCHITECTURE.md`'s rough-edges section for a real regression introduced in
that same change**: the line-style setup block was commented out of this
method, leaving `has_line_style_`/`line_style_` uninitialized for every
freshly-constructed object. Implements the constructor (mirrors `PlotObjectAttributes`' header-reading,
somewhat redundantly — both the attributes struct and the base class
independently pull `DATA_TYPE`/`NUM_ELEMENTS`/`ITEM_ID`/etc. out of the same
`CommunicationHeader`), `postInitialize`/`updateProperties` (the live-update
counterpart to the constructor/`assignProperties`), `preRender`,
`modifyShader` (pushes `color_` into four shaders' uniforms —
`basic_plot_shader`, `scatter_shader`, `plot_2d_shader`, `plot_3d_shader` —
notably **not** `draw_mesh_shader` or `img_plot_shader`, which don't use a
single flat vertex color the same way), and `isPlotDataFunction`'s actual
value list (16 `Function`s — cross-check against `CAPABILITY_MATRIX.md`'s
row list, which should match 1:1 with the TCP-driven hierarchy).

## The "line" family

### `plot2d/` (`Plot2D`)
Renders a polyline as a triangle mesh: consecutive points become
screen-facing quads via a `p0`/`p1`/`p2` three-point-per-vertex attribute
layout consumed by `plot_2d_shader` (the shader reconstructs quad geometry
and line-thickness from these three neighboring points — this is why the
shader, not the CPU, does the actual line-width expansion). The CPU-side
`convertData<T>` (in the anonymous namespace) does three things:
deduplicates consecutive identical points (a run of identical `(x,y)` is
collapsed to one), computes cumulative arc length (`length_along`, used for
dash patterns — has a `// TODO: Currently broken, fix` comment on two of the
length-along assignment blocks, so dashed-line rendering may have visible
artifacts), and builds the per-triangle vertex data. Supports per-vertex
color (`has_color_`) and three dash styles resolved to hardcoded gap/dash
sizes (`DASHED`=0.05, `SHORT_DASHED`=0.01, `LONG_DASHED`=0.1). Supports live
update (`updateWithNewData`) by re-running the same conversion and
`updateBufferData`-ing the first five buffers (not the optional color
buffer — check whether color-array updates are actually supported before
relying on it).

### `plot3d/` (`Plot3D`)
Same billboarded-quad-per-segment technique as `Plot2D`, in 3D, via
`plot_3d_shader`. Notably simpler than `Plot2D`: no dash-style handling, no
`length_along`, no live update, no `preRender` call (so custom transforms
silently don't apply — see `ARCHITECTURE.md`). Its `ConvertedData` is the
only one of the "line" family (besides `Scatter3D`) implementing
`getClosestPoint` for point-picking, scanning the raw `px`/`py`/`pz` arrays
(the original, non-triangulated point positions, kept alongside the
triangulated `p0`/`p1`/`p2` buffers specifically to serve picking).

### `fast_plot2d/` (`FastPlot2D`) / `fast_plot3d/` (`FastPlot3D`)
The "cheap" alternative to `Plot2D`/`Plot3D`: no triangulation, no
dedup, just a flat `(x,y[,z])` buffer drawn as `GL_LINE_STRIP` via
`basic_plot_shader` (so line width/dashing are not configurable — you get
whatever width the GL implementation draws native lines at). `FastPlot2D`
supports `APPENDABLE` (`addExpandableBuffer`/`appendNewData`, logs an error
and drops data on overflow rather than growing — see
`CAPABILITY_MATRIX.md`); `FastPlot3D` does not. **`FastPlot2D`'s
hardcoded-red-color bug lives here** — see `ARCHITECTURE.md`'s rough edges.

### `line_collection2/` (`LineCollection2D`) / `line_collection3/`
(`LineCollection3D`)
Structurally near-identical to `FastPlot2D`/`FastPlot3D`'s `convertData`
(same flat x/y[/z] extraction, no dedup, no append support), but rendered
as `GL_LINES` instead of `GL_LINE_STRIP` — i.e. consecutive point pairs
`(0,1), (2,3), (4,5), ...` become independent disjoint segments rather than
a connected path. This is the actual semantic difference the client-facing
`lineCollection`/`lineCollection3` functions rely on (drawing many separate
line segments in one call, e.g. for a wireframe or a batch of unconnected
edges).

### `plot_collection2/` (`PlotCollection2D`) / `plot_collection3/`
(`PlotCollection3D`)
Handles the client's `plotCollection`/`plotCollection3` calls — a *ragged
batch* of independent polylines packed into one message (see the
interfaces protocol doc's `vector_lengths`-prefixed payload format).
`convertRawData` first reads the `vector_lengths` prefix back out
(`memcpy`'d from the front of the payload) to know where each sub-polyline
starts/ends, then `convertData<T>` walks each sub-polyline and explodes its
consecutive-point pairs into `GL_LINES` segments (so an N-point polyline
contributes N-1 independent segments, same non-strip approach as
`LineCollection*`). Bounding box is computed inline during conversion
(`findMinMax()` is a no-op that relies on the constructor having already
set `min_vec_`/`max_vec_` from the `ConvertedData`). No live update, no
append, no legend override (falls back to the label-only default despite
being a line-type plot).

### `stairs/` (`Stairs`)
Converts N data points into `2N-1` step-shaped vertices (`(x0,y0),
(x1,y0), (x1,y1), (x2,y1), ...` — the classic "hold value, then jump"
staircase shape) rendered as one continuous `GL_LINE_STRIP`. No color
support, no live update, no legend override.

### `stem/` (`Stem`)
Two separate `VertexBuffer`s: `vertex_buffer_lines_` (`GL_LINES`, one
vertical segment per point from `y=0` to the data value) and
`vertex_buffer_points_` (`GL_POINTS`, a point cap at each data value's top).
`modifyShader()` **hardcodes the cap point size to `10.0f`**, ignoring the
class's own `point_size_` member entirely (see `ARCHITECTURE.md`).

### `scrolling_plot2d/` (`ScrollingPlot2D`)
The server-side backing object for the client's `realTimePlot(dt, y, id)`
calls (wire `Function::REAL_TIME_PLOT`). Unlike every other type, its
constructor does **not** receive a full dataset — `convertRawData` only
ever decodes a single `(dt, x)` scalar pair per message. It maintains its
own hand-rolled "shift right, prepend at index 0" ring buffer
(`points_ptr_`, `dt_vec_`, sized by `buffer_size_` — the client-settable
`properties::BufferSize`) and re-derives absolute x-positions from
cumulative `dt` on every update. Resizing the buffer at runtime (if
`buffer_size_` changes between updates) is handled in `updateWithNewData`
by reallocating and copying old data forward. Manages its own raw
`GLuint`/`float*` pair directly rather than using `VertexBuffer` (consistent
with the pattern noted in the `axes/` docs — this module has the same
"older raw-GL vs. newer `VertexBuffer`-wrapped" split). Force-sets
`is_updateable_ = true` unconditionally — see `ARCHITECTURE.md`.

## The "point" family

### `scatter/` (`Scatter2D`) / `scatter3/` (`Scatter3D`)
Both render `GL_POINTS` via `scatter_shader`, supporting per-point color
and/or per-point size (mutually combinable — `has_color_ && has_point_sizes_`
is a valid state, with careful pointer-arithmetic in `convertData` to locate
each optional trailing array correctly depending on which combination is
present). Both support `APPENDABLE`, but **handle overflow differently**
(`Scatter2D` grows, `Scatter3D` drops — see `ARCHITECTURE.md`). Both support
a "distance from" colormap-coloring mode (`has_distance_from_`, mutually
exclusive with `has_color_` in the shader uniform logic) and a silhouette
outline effect (`has_silhouette_`, both push `silhouette_color`/
`squared_silhouette_percentage` uniforms identically). **Only `Scatter3D`
implements point-picking** (`getClosestPoint`) — `Scatter2D`'s equivalent
would need to intersect a 2D point cloud with a 3D ray/plane, which isn't
implemented.

## The "surface/mesh" family

### `surf/` (`Surf`)
Converts an `(rows × cols)` grid of `(x,y,z)` matrices into two vertex
buffers: `vertex_buffer_` (2 triangles per grid cell = 6 vertices, with
computed face normals and a "mean height" value used by the shader for
per-face colormap shading) and `vertex_buffer_lines_` (a separate wireframe
line list covering every grid edge, including the final row/column boundary
handled as two extra loops after the main double loop). `render()` does two
passes gated by `has_edge_color_`/`has_face_color_` independently — a
surface can be face-only, edge-only, both, or (if both are `false`) render
nothing. **Live update is unimplemented** — see `ARCHITECTURE.md`.

### `draw_mesh/` (`DrawMesh`)
Handles both wire variants (`DRAW_MESH` — interleaved `Point3<T>` vertices,
and `DRAW_MESH_SEPARATE_VECTORS` — separate x/y/z arrays) via two distinct
`convertData`/`convertDataSeparateVectors` template functions selected in
`convertRawData` by checking `attributes.function`. Structurally very
similar to `Surf`'s face-normal/mean-height computation but operating over
an arbitrary `IndexTriplet` index list instead of an implicit grid — i.e.
this is the general "arbitrary triangle soup" renderer, while `Surf` is
specifically for regular height-field grids. Unlike `Surf`, edge vs. face
rendering here is done as two passes over the **same** vertex buffer using
`glPolygonMode(GL_FRONT_AND_BACK, GL_LINE/GL_FILL)` rather than a separate
dedicated line-list buffer — a different technique for a similar visual
result. No live update, no append.

## `screen_space_primitive/` (`ScreenSpacePrimitive`) — new since the last pass

Added in the `39a6dc9e` pull along with `Function::SCREEN_SPACE_PRIMITIVE`.
See `ARCHITECTURE.md` for the full description — briefly: converts a flat
array of `Point2f`-triple triangles straight from the wire into a
`GL_TRIANGLES` `VertexBuffer`, rendered via a new `screen_space_shader`
with no axes/camera transform applied. Simplest `convertData` in the whole
module (a single reinterpret-and-copy loop, no per-type payload variants,
no color/point-size handling). No live update, no append, no legend
override, no point-picking.

## `im_show/` (`ImShow`)

Structurally the most different member of the hierarchy: instead of a
`VertexBuffer`, it manages a raw VAO/VBO/EBO triple plus a GL texture
directly (`glGenTextures`, `glTexImage2D`, `glGenerateMipmap`), rendering a
single textured quad (`glDrawElements`, 6 indices = 2 triangles) sized to
the image's pixel dimensions. `convertData<T>` repacks the input into a
GL-friendly interleaved channel layout, handling four channel-count cases
(1=gray, 2=gray+alpha, 3=RGB, 4=RGBA) with a special branch for `DOUBLE`
input that narrows to `float` before upload (see `ARCHITECTURE.md`'s rough
edge). No live update, no legend override, no point-picking.

## `stream_object_base/` and `stream_objects/` — the separate serial-stream hierarchy

See `ARCHITECTURE.md`'s "stream-object hierarchy is unrelated" section for
why this exists independently. Files:

- `stream_object_base/stream_object_base.h/.cpp`: `StreamObjectBase` —
  abstract base with `appendNewData` (single object or batch),
  `clear()`, `render()`. Two-line `.cpp` (just constructors).
- `stream_objects/stream_objects.h`: aggregating include for the three
  concrete stream types.
- `stream_objects/conversion_function.h`: `getFloatValue(shared_ptr<BaseObject>)`
  — a 10-way `NumberDataType` switch converting a serial-interface numeric
  object to `float`, used by all three stream types. Structurally the same
  kind of runtime-type dispatch as `applyConverter`, but a completely
  separate, independently-maintained implementation for a different type
  system (`serial_interface::objects::BaseObject`, not
  `duoplot::internal::DataType`).
- `stream_objects/plot2d/`, `stream_objects/scatter/`,
  `stream_objects/stairs/`: `Plot2DStream`, `ScatterStream`, `StairsStream`
  — each a fixed-500-sample ring buffer with nearly identical
  "shift every sample right by one slot, prepend the new one, handle
  32-bit device-timestamp overflow, `glBufferSubData` the whole buffer"
  logic, differing only in vertex layout (`{x,y}` floats vs.
  `TimeAndValue{t,value}` packed struct for `Stairs`) and GL draw mode
  (`GL_LINE_STRIP`, `GL_POINTS`, `GL_LINE_STRIP` respectively). None derive
  from or interact with `PlotObjectBase`/`ConvertedDataBase` at all.

## Cross-references to files outside this module

These are used pervasively by `plot_object_base.h`/most concrete types but
are **not** part of this module — check them directly if you need their
exact behavior:

- `src/main_application/outer_converter.h` — `applyConverter` (see
  `ARCHITECTURE.md`).
- `src/main_application/color_picker.h/.cpp` — `ColorPicker`, the
  automatic color/face-color cycling fallback (tiny, 23-line header:
  `getNextColor`/`getNextFaceColor`/`getNextEdgeColor`/`reset`, each backed
  by an independent rotating index).
- `src/main_application/user_supplied_properties.h/.cpp` — the
  `UserSuppliedProperties` struct every constructor/`updateProperties` call
  reads from; each optional field follows a `{data, has_default_value}`
  pattern so `PlotObjectBase` can tell "explicitly set by the client" apart
  from "using the type's default."
- `src/main_application/plot_data_handler.h/.cpp` — owns the
  `vector<PlotObjectBase*>` for one pane, is what actually calls
  `new Plot2D(...)` etc. on the GUI thread (see `ARCHITECTURE.md`'s
  two-phase construction section).
- `src/main_application/main_window_receive.cpp` —
  `convertPlotObjectData()`, the `Function`-keyed switch that calls each
  type's `convertRawData` on the background receive thread.
- `src/main_application/shader.h` — `ShaderCollection` and every
  `*_shader` referenced throughout this module's `render()`/`modifyShader()`
  methods; not documented here.
- `src/main_application/axes/legend_properties.h` — `LegendProperties`,
  `LegendType` (documented from the *renderer's* side in the `axes/` docs;
  this module is what actually populates one per plot object).
