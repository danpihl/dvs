# Public API surface — `duoplot::` (C++ interface)

Flat reference of everything client code is meant to call, grouped by header.
For wire-level detail behind any of these, see [`PROTOCOL.md`](PROTOCOL.md).
`namespace duoplot` wraps everything below unless noted; `duoplot::internal`
and `duoplot::gui::` are called out explicitly.

## `duoplot.h`
Umbrella include only — `#include "duoplot/duoplot.h"` pulls in
`control_functions.h` + `gui_api.h` + `plot_functions.h`.

## `plot_functions.h` — plotting calls

All take `const Us&... settings` as trailing variadic arguments accepting any
mix of `properties::*` structs and `properties::PERSISTENT`/`FAST_PLOT`/
`APPENDABLE`/`EXCLUDE_FROM_SELECTION`/`INTERPOLATE_COLORMAP` flags. `T` is the
numeric element type (`float`, `double`, integer types); most functions have
both `Vector<T>`/`Matrix<T>` (owning) and `VectorConstView<T>`/
`MatrixConstView<T>` (non-owning view) overloads — only the owning form is
listed here unless the two diverge in shape.

| Function | Signature (settings omitted) | Wire `Function` |
|---|---|---|
| `plot` | `(Vector<T> x, Vector<T> y)` | `PLOT2` (→`FAST_PLOT2` if `FAST_PLOT`/`APPENDABLE` set) |
| `plot` | `(Vector<T> x, Vector<T> y, Vector<Color> color)` | `PLOT2` (no `FAST_PLOT` support with color) |
| `plot3` | `(Vector<T> x, y, z)` | `PLOT3` (→`FAST_PLOT3` if `FAST_PLOT` set) |
| `plot3` | `(Vector<T> x, y, z, Vector<Color> color)` | `PLOT3` |
| `lineCollection` | `(Vector<T> x, y)` | `LINE_COLLECTION2` |
| `lineCollection3` | `(Vector<T> x, y, z)` | `LINE_COLLECTION3` |
| `plotCollection` | `(std::vector<Vector<T>> x, y)` — ragged batch of polylines | `PLOT_COLLECTION2` |
| `plotCollection3` | `(std::vector<Vector<T>> x, y, z)` | `PLOT_COLLECTION3` |
| `stairs` | `(Vector<T> x, y)` | `STAIRS` |
| `stem` | `(Vector<T> x, y)` | `STEM` |
| `scatter` | `(Vector<T> x, y)` | `SCATTER2` |
| `scatter` | `(Vector<T> x, y, Vector<T> point_sizes)` | `SCATTER2` |
| `scatter` | `(Vector<T> x, y, Vector<Color> color)` | `SCATTER2` |
| `scatter` | `(Vector<T> x, y, Vector<T> point_sizes, Vector<Color> color)` (both arg orders overloaded) | `SCATTER2` |
| `drawPoint` | `(Point2<T> p)` | `SCATTER2` (1 element) |
| `scatter3` | `(Vector<T> x, y, z)` [+ optional `point_sizes`, `color`, both] | `SCATTER3` |
| `drawPoint` | `(Point3<T> p)` | `SCATTER3` (1 element) |
| `surf` | `(Matrix<T> x, y, z)` | `SURF` |
| `surf` | `(Matrix<T> x, y, z, Matrix<Color> color)` | `SURF` |
| `imShow` | `(ImageGray<T> img)` | `IM_SHOW` (1 channel) |
| `imShow` | `(ImageGrayAlpha<T> img)` | `IM_SHOW` (2 channels) |
| `imShow` | `(ImageRGB<T> img)` | `IM_SHOW` (3 channels) |
| `imShow` | `(ImageRGBA<T> img)` | `IM_SHOW` (4 channels) |
| `drawMesh` | `(Vector<Point3<T>> vertices, Vector<IndexTriplet> indices [, Vector<Color> colors])` | `DRAW_MESH` |
| `drawMesh` | `(Vector<T> x, y, z, Vector<IndexTriplet> indices [, Vector<Color> colors])` | `DRAW_MESH_SEPARATE_VECTORS` |
| `drawLine` | `(Line3D<double> line, double t0, double t1)` | `PLOT3` (2-point line) |
| `realTimePlot` | `(T dt, T y, ItemId id)` | `REAL_TIME_PLOT` |
| `drawCubes` | `(VectorConstView<T> x, y, z, T side_length)` | expands to `DRAW_MESH` client-side |

`T` for `imShow` is restricted to `float`, `double`, `uint8_t`
(`static_assert` in each overload).

## `control_functions.h` — session/view/object control

| Function | Signature | Wire `Function` |
|---|---|---|
| `setProperties` | `(ItemId id, const Us&... settings)` | `PROPERTIES_EXTENSION` |
| `setProperties` | `(const std::vector<PropertySet>& sets)` | `PROPERTIES_EXTENSION_MULTIPLE` |
| `setCurrentElement` | `(const std::string& name)` | `SET_CURRENT_ELEMENT` |
| `deletePlotObject` | `(ItemId id)` | `DELETE_PLOT_OBJECT` |
| `setCurrentElementToImageView` | `()` | `CURRENT_ELEMENT_AS_IMAGE_VIEW` |
| `waitForFlush` | `()` | `WAIT_FOR_FLUSH` |
| `flushCurrentElement` | `()` | `FLUSH_ELEMENT` |
| `flushMultipleElements` | `(const Us&... element_names)` (strings) | `FLUSH_MULTIPLE_ELEMENTS` |
| `view` | `(float azimuth_deg, float elevation_deg)` | `VIEW` |
| `axis` | `(Vec3<double> min, Vec3<double> max)` | `AXES_3D` |
| `axis` | `(Vec2<double> min, Vec2<double> max)` | `AXES_2D` |
| `globalIllumination` | `(Vec3<double> light_position)` | `GLOBAL_ILLUMINATION` |
| `showLegend` | `()` | `SHOW_LEGEND` |
| `softClearView` | `()` | `SOFT_CLEAR` |
| `clearView` | `()` | `CLEAR` |
| `disableAutomaticAxesAdjustment` | `()` | `DISABLE_AXES_FROM_MIN_MAX` |
| `disableScaleOnRotation` | `()` | `DISABLE_SCALE_ON_ROTATION` |
| `axesSquare` | `()` | `AXES_SQUARE` |
| `setAxesBoxScaleFactor` | `(Vec3<double> scale)` | `SET_AXES_BOX_SCALE_FACTOR` |
| `setTitle` | `(const std::string& title)` | `SET_TITLE` |
| `setTransform` | `(ItemId id, Matrix<double> scale, rotation, Vec3<double> translation)` | `SET_OBJECT_TRANSFORM` |
| `setTransform` | `(ItemId id, MatrixFixed<double,3,3> scale, rotation, Vec3<double> translation)` | `SET_OBJECT_TRANSFORM` |
| `openProjectFile` | `(const std::string& file_path)` | `OPEN_PROJECT_FILE` |
| `screenshot` | `(const std::string& base_path)` | `SCREENSHOT` |
| `spawn` | `()` — launches `main_application/duoplot` if not already running | *(local only, no wire call)* |

## `plot_properties.h` — property/settings types (`duoplot::properties::`)

| Type | Construct as | Notes |
|---|---|---|
| `LineWidth` | `LineWidth(uint8_t width)` | |
| `Alpha` | `Alpha(float a)` | |
| `ZOffset` | `ZOffset(float z)` | |
| `Transform` | `Transform(scale3x3, rotation3x3, translation3)` | fixed or dynamic 3x3 overloads |
| `Label` | `Label(const char* name)` | max 100 chars; also underlies titles/handles/paths |
| `Color` | `Color(r,g,b)` or `Color::RED/GREEN/BLUE/CYAN/MAGENTA/YELLOW/BLACK/WHITE/GRAY` | not itself a `PropertyBase`; wrapped as `ColorInternal` on send |
| `EdgeColor` | `EdgeColor(r,g,b)`, `EdgeColor(use_color)`, or named constants incl. `EdgeColor::NONE` | |
| `FaceColor` | same shape as `EdgeColor`, incl. `FaceColor::NONE` | |
| `Silhouette` | `Silhouette(r,g,b[,percentage])` or named constants (no `NONE`) | `percentage` defaults to 0.1 |
| `ColorMap` (enum) | `ColorMap::JET/HSV/MAGMA/VIRIDIS/PASTEL/JET_SOFT/JET_BRIGHT` | |
| `PointSize` | `PointSize(uint8_t size)` | |
| `DistanceFrom` | `DistanceFrom::x/y/z(val, min_dist, max_dist)`, `::xy/xz/yz(point, min, max)`, `::xyz(point3, min, max)` | no public constructor — factories only |
| `BufferSize` | `BufferSize(uint16_t size)` | used with streaming/appendable plots |
| `LineStyle` (enum) | `LineStyle::SOLID/DASHED/SHORT_DASHED/LONG_DASHED` | |
| `ScatterStyle` (enum) | `ScatterStyle::SQUARE/CIRCLE/DISC/PLUS/CROSS` | |
| Flags | `properties::PERSISTENT`, `FAST_PLOT`, `APPENDABLE`, `EXCLUDE_FROM_SELECTION`, `INTERPOLATE_COLORMAP` | pass directly as a trailing settings argument |
| Reserved flags | `properties::not_ready::UPDATABLE`, `not_ready::SELECTABLE` | not stable — treat as unfinished |

## `item_id.h` — `duoplot::ItemId` / `duoplot::properties::ID0..ID254`

Fixed 255-value enum (`ID0`..`ID254`, plus `ItemId::UNKNOWN`) used to tag and
later refer back to a specific plotted object — passed to `setProperties`,
`deletePlotObject`, `setTransform`, and as the per-series key for
`realTimePlot`. Reference as `duoplot::properties::ID7`, not `ItemId::ID7`
(both work, but the `properties::` aliases are the intended call-site form).

## `property_set.h` — `duoplot::PropertySet`

```cpp
PropertySet(ItemId id, const Us&... props)
```
Bundles an `ItemId` with a list of properties for use in a single batched
`setProperties(const std::vector<PropertySet>&)` call — e.g.:
```cpp
setProperties({{properties::ID0, transform0, properties::Color::RED},
               {properties::ID1, transform1, properties::Color::GREEN}});
```

## `gui_api.h` — interactive GUI widgets (`duoplot::gui::`)

Call `duoplot::gui::startGuiReceiveThread()` once at startup to enable this
subsystem (spawns background threads — see `ARCHITECTURE.md`).

| Handle type | Getters | Setters |
|---|---|---|
| `SliderHandle` | `getMinValue/getMaxValue/getStepSize/getValue/getNormalizedValue` | `setEnabled/setDisabled/setMinValue/setMaxValue/setValue/setStepSize` |
| `ButtonHandle` | `getIsPressed` | `setLabel` |
| `CheckboxHandle` | `getIsChecked` | `setLabel` |
| `TextLabelHandle` | `getLabel` | `setLabel` |
| `ListBoxHandle` | `getElements`, `getSelectedElement` | — |
| `EditableTextHandle` | `getText`, `getEnterPressed` | — |
| `DropdownMenuHandle` | `getElements`, `getSelectedElement` | — |
| `RadioButtonGroupHandle` | `getButtons`, `getSelectedButtonIdx` | — |

```cpp
template <typename T> T getGuiElementHandle(const std::string& handle_string);
```
Explicit specializations exist for each handle type above; throws if the
handle string is unknown or registered under a different `GuiElementType`.

```cpp
void registerGuiCallback(const std::string& handle_string,
                          std::function<void(const XHandle&)> callback);
```
Overloaded for every handle type except `TextLabelHandle` (its callback path
is commented out in source — see `ARCHITECTURE.md` rough edges). Registering
twice for the same handle string logs a warning and overwrites.

## Everything else (`internal::`)

`enumerations.h`, `constants.h`, `communication_header*.h`,
`encode_decode_functions*.h`, `fillable_uint8_array.h`, `internal.h`,
`gui_internal.h`, `utils.h`, `logging.h`, `timing.h` are all
`duoplot::internal::` (or free-standing, e.g. `UInt8ArrayView`/
`FillableUInt8Array`) — not part of the intended client-facing surface.
Client code should never need to name these directly; see
[`FILE_REFERENCE.md`](FILE_REFERENCE.md) if you're modifying them.
