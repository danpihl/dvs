# Capability matrix — TCP-driven plot types

> Updated post-`39a6dc9e` pull to add `ScreenSpacePrimitive` (new). See
> `ARCHITECTURE.md`'s top note for the `duoplot::`→`lumos::` rename that
> also applies throughout this file's prose.

Side-by-side comparison of every `PlotObjectBase`-derived class. Use this to
answer "does type X support Y" without re-reading each `.cpp` file. See
[`ARCHITECTURE.md`](ARCHITECTURE.md) for what each column means in general,
and [`FILE_REFERENCE.md`](FILE_REFERENCE.md) for per-file detail.

| Class | `Function` | GL primitive | Shader(s) used | Appendable | Live update (`updateWithNewData`) | Point-picking | Legend type | `preRender` (custom transform) |
|---|---|---|---|---|---|---|---|---|
| `Plot2D` | `PLOT2` | `TRIANGLES` (billboarded thick-line quads) | `plot_2d_shader` | no | yes | no | `LINE` | yes |
| `Plot3D` | `PLOT3` | `TRIANGLES` (billboarded thick-line quads) | `plot_3d_shader` | no | **no** | **yes** | *(none — base default)* | no |
| `FastPlot2D` | `FAST_PLOT2` | `LINE_STRIP` | `basic_plot_shader` | yes | no (only `appendNewData`) | no | `LINE` (**but color always hardcoded red**) | no |
| `FastPlot3D` | `FAST_PLOT3` | `LINE_STRIP` | `basic_plot_shader` | no | no | no | *(none — base default)* | no |
| `LineCollection2D` | `LINE_COLLECTION2` | `LINES` (disjoint segments) | `basic_plot_shader` | no | no | no | `LINE` | no |
| `LineCollection3D` | `LINE_COLLECTION3` | `LINES` (disjoint segments) | `basic_plot_shader` | no | no | no | *(none — base default)* | no |
| `PlotCollection2D` | `PLOT_COLLECTION2` | `LINES` (ragged multi-polyline, exploded to segments) | `basic_plot_shader` | no | no | no | *(none — base default)* | no |
| `PlotCollection3D` | `PLOT_COLLECTION3` | `LINES` (ragged multi-polyline, exploded to segments) | `basic_plot_shader` | no | no | no | *(none — base default)* | no |
| `Scatter2D` | `SCATTER2` | `POINTS` | `scatter_shader` | yes (grows buffer on overflow) | no (only `appendNewData`) | no | `DOT` | no |
| `Scatter3D` | `SCATTER3` | `POINTS` | `scatter_shader` | yes (**drops data on overflow**, does not grow) | no (only `appendNewData`) | **yes** | `DOT` | no |
| `Stairs` | `STAIRS` | `LINE_STRIP` (duplicated points for step shape) | `basic_plot_shader` | no | no | no | *(none — base default)* | no |
| `Stem` | `STEM` | `LINES` (stems) + `POINTS` (caps, fixed size `10.0f`) | `basic_plot_shader` + `scatter_shader` | no | no | no | `LINE` | no |
| `Surf` | `SURF` | `TRIANGLES` (faces) + `LINES` (wireframe overlay) | `draw_mesh_shader` | no | declared but **no-op** (commented out) | no | `POLYGON` (colormap or face/edge color) | yes |
| `DrawMesh` | `DRAW_MESH` / `DRAW_MESH_SEPARATE_VECTORS` | `TRIANGLES`, drawn twice (`GL_LINE`/`GL_FILL` polygon mode toggle for edge vs. face pass) | `draw_mesh_shader` | no | no | no | `POLYGON` (colormap or face/edge color) | yes |
| `ImShow` | `IM_SHOW` | `TRIANGLES` (textured quad, `glDrawElements`) | `img_plot_shader` | no | no | no | *(none — base default)* | yes |
| `ScrollingPlot2D` | `REAL_TIME_PLOT` | `LINE_STRIP` (raw `glDrawArrays`, no `VertexBuffer` wrapper) | `basic_plot_shader` | n/a (always growable ring buffer) | yes (the *only* way it receives data — no constructor-time full dataset) | no | `LINE` | no (commented out) |
| `ScreenSpacePrimitive` | `SCREEN_SPACE_PRIMITIVE` | `TRIANGLES` | `screen_space_shader` (new) | no | no | no | *(none — base default)* | no — and unlike every other row, **does not go through the axes/camera transform at all** (see `ARCHITECTURE.md`) |

## Reading the columns

- **Appendable**: whether the client can set `properties::APPENDABLE` to
  stream additional points into an existing object without replacing it
  (`appendNewData`). Distinct from **live update**, which replaces/refreshes
  existing data in place (`updateWithNewData`) — a type can support one,
  both, or neither.
- **Point-picking**: whether `ConvertedDataBase::getClosestPoint` is
  meaningfully overridden — see `ARCHITECTURE.md`'s point-picking section.
  This is currently 3D-only and only for two of the fifteen types.
- **Legend type**: the `LegendType` (`LINE`/`DOT`/`POLYGON`) a type reports
  via `getLegendProperties()`. "*(none — base default)*" means the class
  never overrides this method, so `PlotObjectBase::getLegendProperties()`'s
  label-only default is used — no colored swatch appears in the legend for
  that plot type today, even though most of them visually have an
  obvious color.
- **`preRender`**: whether `render()` calls `preRender(shader)` to upload
  custom-transform uniforms (`duoplot::setTransform(id, ...)` from the
  client). Types that skip this call do not support per-object transforms
  even though `PlotObjectBase::setTransform()` is always available to set
  `has_custom_transform_` — the transform would simply never be applied at
  draw time.

## Cross-cutting shader usage

| Shader | Used by |
|---|---|
| `basic_plot_shader` | `FastPlot2D`, `FastPlot3D`, `LineCollection2D/3D`, `PlotCollection2D/3D`, `Stairs`, `Stem` (lines), `ScrollingPlot2D` — anything drawing plain, uncolored-by-shader-logic native GL line/point primitives |
| `plot_2d_shader` / `plot_3d_shader` | `Plot2D` / `Plot3D` only — the billboarded-thick-line shaders, each with their own dedicated uniform set (`half_line_width`, `first_length`/`first_point`/`second_point` for dash continuity, `use_dash`) |
| `scatter_shader` | `Scatter2D`, `Scatter3D`, `Stem` (point caps), and (outside this hierarchy) the `stream_objects/scatter` streaming variant |
| `draw_mesh_shader` | `Surf`, `DrawMesh` — shared mesh-face shader, distinguished at draw time only by the `is_edge` uniform toggling between wireframe and fill passes |
| `img_plot_shader` | `ImShow` only |
| `screen_space_shader` (new) | `ScreenSpacePrimitive` only |

## Two-argument color pattern

Nearly every type distinguishes "has an explicit per-vertex/per-point color
array from the client" (`has_color_`) from "use the single resolved
`color_`/`face_color_`/`edge_color_` from properties" — check
`shader.*.base_uniform_handles.has_color_vec` in each `render()`/
`modifyShader()` to see which mode a given draw call is in. `Scatter2D`/
`Scatter3D` additionally support a `has_distance_from_` mode (mutually
exclusive with `has_color_` in practice — see their `modifyShader()`
`if/else if` chains) that colors points by distance from a reference point/
line/plane via a colormap instead of a fixed or per-point color.
