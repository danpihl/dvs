# Wire protocol reference

This describes the exact byte layout used between a client (this C++ library,
or its C/Python equivalents) and the standalone `main_application` (`duoplot`)
GUI process. All multi-byte values use native/host endianness — the sender
tags the message with an endianness byte, but nothing in this codebase
currently branches on it (no byte-swapping on receive), so client and server
are implicitly assumed to run on the same-endianness host (both are x86/ARM
little-endian in practice).

Source of truth for everything below:
`constants.h`, `enumerations.h`, `communication_header.h`,
`communication_header_object.h`, `internal.h`, `gui_internal.h`.

## Ports

| Port | Protocol | Direction | Purpose | Bound by |
|---|---|---|---|---|
| `9755` (`kTcpPortNum`) | TCP | client → server | plot data + control commands | `main_application` |
| `9758` (`kGuiTcpPortNum`) | TCP | server → client | GUI element state / callbacks | **the client** (unusual: client listens here) |
| `9757` (`kUdpQueryPortNum`) | UDP | client ⇄ server | small ack'd query/response exchanges | `main_application` |

## Magic number & framing (plot channel, port 9755)

Every message sent by `internal::sendHeaderAndData` / `sendHeaderOnly` /
`sendHeaderAndVectorCollection` has this outer envelope:

```
Offset  Size      Field
0       1         is_big_endian  (1 = big endian, 0 = little; see utils.h::isBigEndian — informational only, not acted on by receiver)
1       8         magic number = 0xdeadbeefcafebabe  (kMagicNumber)
9       8         total_num_bytes (uint64_t) — includes ALL of this envelope, i.e. from byte 0
17      variable  CommunicationHeader (serialized, see below)
...     variable  raw payload bytes, one array's worth after another, in the
                  order the header-object append calls implied (see per-
                  function payload layouts below)
```

`kHeaderDataStartOffset = 2*sizeof(uint64_t) + 1 = 17` is exactly the offset
where the `CommunicationHeader` begins — used when reconstructing a header
from a received buffer (`CommunicationHeader(const UInt8ArrayView&)` ctor in
`communication_header.h`).

On the wire, the receiver (`DataReceiver` in `main_application`) reads the
8-byte length field first (bytes 9-16), then reads exactly that many
additional bytes as the rest of the message, and validates the magic number
before parsing further.

There's a soft limit of `kMaxNumBytesForOneTransmission = 1380` bytes,
enforced only on the UDP query path (`sendThroughQueryUdpInterface` throws if
exceeded) — the main TCP plot channel has no such client-side limit.

## `CommunicationHeader` serialization

```
Offset  Size                              Field
0       1                                 num_objects (uint8_t) — count of "object" entries that follow
1       1                                 num_props   (uint8_t) — count of "property" entries that follow
2       sizeof(Function) = 1              function (Function enum, the "command")
3       kTableSize (=UNKNOWN+1, currently 34) object lookup table: index[CommunicationHeaderObjectType] -> position in objects array, or 255 if absent
+34     kTableSize (=UNKNOWN+1, currently 18) property lookup table: index[PropertyType] -> position in props array, or 255 if absent
...     num_objects × (2+1+size)          object entries, each: {type: uint16, size: uint8, data[size]}
...     num_props   × (2+1+size)          property entries, each: {type: uint16, size: uint8, data[size]} — type is always CommunicationHeaderObjectType::PROPERTY (48) here; the *actual* property kind lives inside `data`, see below
...     kNumFlags (= PropertyFlag::UNKNOWN+1, currently 8) bytes  flags array — one byte per PropertyFlag, 1 = set, 0 = unset
```

Notes:
- The lookup tables let the receiver do O(1) "does this header have field X"
  checks (`CommunicationHeader::hasObjectWithType`/`get`) without scanning.
  `255` is the sentinel for "absent" (`kMaxNumObjects`/`kMaxNumProperties` are
  each capped at 10, so a real index never reaches 255).
- **Properties are objects too.** Every property entry's outer `type` field
  is literally `CommunicationHeaderObjectType::PROPERTY`; which property it
  actually is (color, line width, ...) is encoded as the first byte(s) inside
  its `data` payload via a `PropertyBase`-derived struct's own layout (see
  "Property struct layouts" below). The *properties lookup table* is what
  lets the receiver map `PropertyType::COLOR` → index into the properties
  array without inspecting payload bytes.
- `CommunicationHeaderObject::data` is a fixed `uint8_t[255]`
  (`kCommunicationHeaderObjectDataSize = UCHAR_MAX`) — this is a hard per-field
  size ceiling.

### `CommunicationHeaderObjectType` (header "object" tags)

Declared in `enumerations.h`. Numeric value = declaration order (0-based).

```
FUNCTION, NUM_BUFFERS_REQUIRED, NUM_BYTES, DATA_STRUCTURE, BYTES_PER_ELEMENT,
DATA_TYPE, NUM_CHANNELS, NUM_ELEMENTS, HAS_COLOR, HAS_POINT_SIZES,
NUM_VERTICES, NUM_INDICES, NUM_OBJECTS, DIMENSION_2D, HAS_PAYLOAD, AZIMUTH,
ELEVATION, AXIS_MIN_MAX_VEC, VEC3, SCALE_MATRIX, TRANSLATION_VECTOR,
ROTATION_MATRIX, PROJECT_FILE_NAME, SCREENSHOT_BASE_PATH, TITLE_STRING,
LABEL, HANDLE_STRING, INT32, POS2D, FIGURE_NUM, PARENT_NAME, PARENT_TYPE,
ELEMENT_NAME, GUI_ELEMENT_TYPE, PROPERTY, ITEM_ID, NUM_NAMES, UNKNOWN
```

Most are self-explanatory metadata fields attached directly by
`hdr.append(CommunicationHeaderObjectType::X, value)` calls in
`plot_functions.h`/`control_functions.h` (e.g. `DATA_TYPE`, `NUM_ELEMENTS`,
`DIMENSION_2D` for matrix shapes, `HAS_COLOR`/`HAS_POINT_SIZES` as boolean
flags indicating extra trailing payload arrays are present).

### `PropertyType` (what a "property" object actually is)

```
LINE_WIDTH, ALPHA, Z_OFFSET, TRANSFORM, NAME, DISTANCE_FROM, LINE_STYLE,
COLOR, EDGE_COLOR, FACE_COLOR, SILHOUETTE, COLOR_MAP, POINT_SIZE,
BUFFER_SIZE, SCATTER_STYLE, PROPERTY_FLAG, ITEM_ID, UNKNOWN
```

Each maps 1:1 to a struct in `plot_properties.h` (see "Property struct
layouts" below), all deriving from `internal::PropertyBase` which stores its
own `PropertyType` tag as its first logical field (serialized by
`encode_decode_functions.h`'s `serializeToCommunicationHeaderObject`
overloads — not shown inline here, but every property's `data[]` begins with
that tag byte followed by the struct's actual fields).

### `PropertyFlag` (boolean flags, not objects)

```
PERSISTENT, APPENDABLE, INTERPOLATE_COLORMAP, UPDATABLE, FAST_PLOT,
EXCLUDE_FROM_SELECTION, SELECTABLE, UNKNOWN
```

Stored as one byte each in the header's flags array (not as
`CommunicationHeaderObject`s). Client-visible constants:
`properties::PERSISTENT`, `properties::INTERPOLATE_COLORMAP`,
`properties::FAST_PLOT`, `properties::APPENDABLE`,
`properties::EXCLUDE_FROM_SELECTION`; `UPDATABLE`/`SELECTABLE` are exposed
under `properties::not_ready::` (i.e. reserved/unfinished — do not treat as
stable if found in use).

Special client-side behavior: setting `FAST_PLOT` or `APPENDABLE` on a 2D
`plot()` call rewrites the header's `Function` from `PLOT2` to `FAST_PLOT2`
before sending (similarly `PLOT3` → `FAST_PLOT3` for `FAST_PLOT` only); this
happens in `plot_functions.h`, purely client-side — the flag byte is *also*
still sent.

### `Function` (the command/message type — `uint8_t`)

Declared in `enumerations.h`, one byte on the wire. Grouped here by purpose
(the enum itself is one flat list, order = numeric value):

**Plot-data functions** (carry a bulk payload after the header):
`PLOT2, PLOT3, REAL_TIME_PLOT, LINE_COLLECTION2, LINE_COLLECTION3, STEM,
FAST_PLOT2, FAST_PLOT3, PLOT_COLLECTION2, PLOT_COLLECTION3, SCATTER2,
SCATTER3, DRAW_MESH, DRAW_MESH_SEPARATE_VECTORS, SURF, IM_SHOW, STAIRS`

**Object lifecycle / targeting:**
`SET_CURRENT_ELEMENT, CREATE_NEW_ELEMENT, DELETE_PLOT_OBJECT,
CURRENT_ELEMENT_AS_IMAGE_VIEW, NEW_ELEMENT, PROPERTIES_EXTENSION,
PROPERTIES_EXTENSION_MULTIPLE, SET_OBJECT_TRANSFORM`

**View / axes / render control (header-only, no payload):**
`VIEW, AXES_2D, AXES_3D, CLEAR, SOFT_CLEAR, HOLD_ON, HOLD_OFF, SET_TITLE,
DISABLE_AXES_FROM_MIN_MAX, SET_AXES_BOX_SCALE_FACTOR, SHOW_LEGEND,
DISABLE_SCALE_ON_ROTATION, AXES_SQUARE, GLOBAL_ILLUMINATION`

**Flush / sync / misc app control:**
`WAIT_FOR_FLUSH, FLUSH_ELEMENT, FLUSH_MULTIPLE_ELEMENTS, OPEN_PROJECT_FILE,
SCREENSHOT, IS_BUSY_RENDERING, GET_FLOAT_PARAMETER,
QUERY_FOR_SYNC_OF_GUI_DATA, POSITION`

**GUI element control (client → app, header-only):**
`SET_GUI_ELEMENT_LABEL, SET_GUI_ELEMENT_ENABLED, SET_GUI_ELEMENT_DISABLED,
SET_GUI_ELEMENT_MIN_VALUE, SET_GUI_ELEMENT_MAX_VALUE, SET_GUI_ELEMENT_VALUE,
SET_GUI_ELEMENT_STEP`

**Legacy/unused-by-this-library primitives** (present in the enum,
not referenced anywhere in `cpp/duoplot/*.h` at the time of writing — check
`main_application` before assuming these are dead):
`DRAW_LINE3D, DRAW_ARROW, PLANE_XY, PLANE_XZ, PLANE_YZ, GRID_ON, GRID_OFF,
CUBE, SPHERE, QUIVER, QUIVER3, DRAW_LINE_BETWEEN_POINTS_3D,
POLYGON_FROM_4_POINTS, DRAW_TRIANGLES_3D, DRAW_TRIANGLE_3D, DRAW_TILES`

`UNKNOWN` is the sentinel/default.

### `DataType` (numeric element type tag)

```
FLOAT, DOUBLE, INT8, INT16, INT32, INT64, UINT8, UINT16, UINT32, UINT64, UNKNOWN
```

Set via `internal::typeToDataTypeEnum<T>()` (`communication_header.h`), which
uses `std::is_same`/`std::is_signed`/`sizeof` to map a C++ type to this enum.
Byte size for a given `DataType` is recovered on decode via
`dataTypeToNumBytes()`.

### Color-name shorthand enums

`ColorT`, `EdgeColorT`, `FaceColorT`, `SilhouetteT` (all `uint8_t`,
`RED/GREEN/BLUE/CYAN/MAGENTA/YELLOW/BLACK/WHITE/GRAY`, plus `NONE` for
`EdgeColorT`/`FaceColorT`). These never go on the wire directly — passing one
to `hdr.extend(...)` expands it client-side into the corresponding full
property struct (`properties::Color`/`EdgeColor`/`FaceColor`/`Silhouette`)
before serialization (see `CommunicationHeader::extendInternal` in
`communication_header.h`).

## Property struct layouts (`plot_properties.h`)

All derive from `PropertyBase` (carries a `PropertyType`). Layout of each
struct's data fields (serialization order matches declaration order unless a
custom `serializeToCommunicationHeaderObject` says otherwise —
check `encode_decode_functions.h` when in doubt):

| Struct | `PropertyType` | Fields |
|---|---|---|
| `LineWidth` | `LINE_WIDTH` | `uint8_t data` |
| `Alpha` | `ALPHA` | `float data` |
| `ZOffset` | `Z_OFFSET` | `float data` |
| `Transform` | `TRANSFORM` | `MatrixFixed<double,3,3> scale`, `rotation`, `Vec3<double> translation` |
| `Label` | `NAME` | `uint8_t length`, `char data[101]` (100 chars max + null terminator; `Label` is also reused for non-property strings like titles/handle-strings/paths tagged under other `CommunicationHeaderObjectType`s, e.g. `TITLE_STRING`, `HANDLE_STRING`, `ELEMENT_NAME`, `PROJECT_FILE_NAME`, `SCREENSHOT_BASE_PATH`) |
| `Color` (plain struct, not a `PropertyBase`) | n/a — wrapped in `internal::ColorInternal` (`PropertyType::COLOR`) before sending | `uint8_t red, green, blue` |
| `EdgeColor` | `EDGE_COLOR` | `uint8_t use_color, red, green, blue` (`use_color=0` means "NONE") |
| `FaceColor` | `FACE_COLOR` | `uint8_t use_color, red, green, blue` |
| `Silhouette` | `SILHOUETTE` | `uint8_t red, green, blue`, `float percentage` (0.0–1.0) |
| `ColorMap` (enum) | `COLOR_MAP` | `uint8_t`: `JET, HSV, MAGMA, VIRIDIS, PASTEL, JET_SOFT, JET_BRIGHT, UNKNOWN` |
| `PointSize` | `POINT_SIZE` | `uint8_t data` |
| `DistanceFrom` | `DISTANCE_FROM` | `Vec3<double> pt`, `double min_dist, max_dist`, `DistanceFromType` (see below) — constructed only via static factories `::x/::y/::z/::xy/::xz/::yz/::xyz` |
| `BufferSize` | `BUFFER_SIZE` | `uint16_t data` |
| `LineStyle` (enum) | `LINE_STYLE` | `uint8_t`: `SOLID, DASHED, SHORT_DASHED, LONG_DASHED` |
| `ScatterStyle` (enum) | `SCATTER_STYLE` | `uint8_t`: `SQUARE, CIRCLE, DISC, PLUS, CROSS` |
| `PropertyFlag` (enum, not a struct) | `PROPERTY_FLAG` | goes into the flags byte array, not a props entry (see above) |
| `ItemId` (enum) | `ITEM_ID` | `uint16_t`, appended as a header **object** (`CommunicationHeaderObjectType::ITEM_ID`), not a property |

`DistanceFromType` (`enumerations.h`, top-level `duoplot::` namespace, not
`internal::`): `X, Y, Z, XY, XZ, YZ, XYZ`.

`Dimension2D` (`internal::`, used for `DIMENSION_2D` objects, e.g. matrix
shapes for `surf`/`imShow`): `{uint32_t rows, cols}`.

## Payload layout per plot-producing function (`plot_functions.h`)

All payloads follow the header. Multiple arrays are concatenated back to
back in the exact order passed to `sendHeaderAndData(hdr, arg1, arg2, ...)`.
Element counts come from the corresponding `NUM_ELEMENTS`/`NUM_VERTICES`/
`NUM_INDICES` header objects; per-element byte size comes from `DATA_TYPE`.

| Function | Header objects (beyond FUNCTION) | Payload order |
|---|---|---|
| `plot`/`PLOT2` (or `FAST_PLOT2` if `FAST_PLOT`/`APPENDABLE` set) | `DATA_TYPE`, `NUM_ELEMENTS`, optional `HAS_COLOR` | `x, y[, color]` |
| `plot3`/`PLOT3` (or `FAST_PLOT3`) | same + no `HAS_POINT_SIZES` | `x, y, z[, color]` |
| `lineCollection`/`LINE_COLLECTION2` | `DATA_TYPE`, `NUM_ELEMENTS` | `x, y` |
| `lineCollection3`/`LINE_COLLECTION3` | same | `x, y, z` |
| `plotCollection`/`PLOT_COLLECTION2` | `DATA_TYPE`, `NUM_OBJECTS`, `NUM_ELEMENTS` (sum) | `vector_lengths` (uint16 per sub-vector, via `sendHeaderAndVectorCollection`), then each sub-vector's `x` data concatenated, then each sub-vector's `y` data concatenated |
| `plotCollection3`/`PLOT_COLLECTION3` | same + z | `vector_lengths`, then `x`s, `y`s, `z`s |
| `stairs`/`STAIRS` | `DATA_TYPE`, `NUM_ELEMENTS` | `x, y` |
| `stem`/`STEM` | `DATA_TYPE`, `NUM_ELEMENTS` | `x, y` |
| `scatter`/`SCATTER2` | `DATA_TYPE`, `NUM_ELEMENTS`, optional `HAS_COLOR`, optional `HAS_POINT_SIZES` | `x, y[, point_sizes][, color]` (order of point_sizes vs color depends on overload — see source) |
| `drawPoint(Point2)`/`SCATTER2` | `NUM_ELEMENTS=1` | `x, y` (single-element vectors) |
| `scatter3`/`SCATTER3` | `DATA_TYPE`, `NUM_ELEMENTS`, optional `HAS_COLOR`, optional `HAS_POINT_SIZES` | `x, y, z[, point_sizes][, color]` |
| `drawPoint(Point3)`/`SCATTER3` | `NUM_ELEMENTS=1` | `x, y, z` |
| `surf`/`SURF` | `DATA_TYPE`, `NUM_ELEMENTS`, `DIMENSION_2D` (rows/cols), optional `HAS_COLOR` | `x, y, z[, color]` (each a `Matrix<T>`/`Matrix<Color>`) |
| `imShow`/`IM_SHOW` | `DATA_TYPE`, `NUM_CHANNELS` (1=Gray,2=GrayAlpha,3=RGB,4=RGBA), `NUM_ELEMENTS`, `DIMENSION_2D` | `img` (packed pixel buffer) |
| `drawMesh(vertices,indices[,colors])`/`DRAW_MESH` | `DATA_TYPE`, `NUM_VERTICES`, `NUM_INDICES`, `NUM_ELEMENTS=NUM_INDICES`, optional `HAS_COLOR` | `vertices` (`Point3<T>` array), `indices` (`IndexTriplet` array)[, `colors`] |
| `drawMesh(x,y,z,indices[,colors])`/`DRAW_MESH_SEPARATE_VECTORS` | same shape, `NUM_VERTICES` = `x.size()` | `x, y, z, indices[, colors]` |
| `drawLine`/`PLOT3` internally builds 2-point `x,y` (see source; note: it currently only fills `x,y`, not `z`, for a `Line3D` — check before relying on 3D behavior) | `DATA_TYPE=DOUBLE`, `NUM_ELEMENTS=2` | `x, y` |
| `realTimePlot`/`REAL_TIME_PLOT` | `DATA_TYPE`, `NUM_ELEMENTS=1`, `ITEM_ID` | `data` = `[dt, y]` as one 2-element vector |
| `drawCubes` | expands client-side into a `drawMesh` call (12 triangles per cube) — no distinct wire function | (delegates to `DRAW_MESH`) |

`IndexTriplet` (`math/structures/index_triplet.h`): three vertex indices
forming one triangle (mesh payloads are always triangle soups).

## Control-function wire shapes (`control_functions.h`, all header-only — `sendHeaderOnly`)

| Function | Header objects |
|---|---|
| `setProperties(id, ...)` / `PROPERTIES_EXTENSION` | `ITEM_ID`, then `extend(settings...)` (properties/flags as usual). Rejects `APPENDABLE`/`FAST_PLOT` flags client-side (logged warning, no send). |
| `setProperties(vector<PropertySet>)` / `PROPERTIES_EXTENSION_MULTIPLE` | none beyond function; payload is a hand-packed byte buffer — see "PropertySet payload" below. **This is the one exception that sends a payload despite being conceptually "header only"** — implemented via `sendHeaderAndData`, not `sendHeaderOnly`. |
| `setCurrentElement(name)` / `SET_CURRENT_ELEMENT` | `ELEMENT_NAME` (a `Label`) |
| `deletePlotObject(id)` / `DELETE_PLOT_OBJECT` | `ITEM_ID` |
| `setCurrentElementToImageView()` / `CURRENT_ELEMENT_AS_IMAGE_VIEW` | none |
| `waitForFlush()` / `WAIT_FOR_FLUSH` | none |
| `flushCurrentElement()` / `FLUSH_ELEMENT` | none |
| `flushMultipleElements(...)` / `FLUSH_MULTIPLE_ELEMENTS` | `NUM_NAMES`; payload = `name_lengths` (uint8 vector) + concatenated name chars (this one uses `sendHeaderAndData`, not header-only, despite living alongside the header-only calls) |
| `view(az, el)` / `VIEW` | `AZIMUTH` (float), `ELEVATION` (float) |
| `axis(min3, max3)` / `AXES_3D` | `AXIS_MIN_MAX_VEC` = `pair<Vec3<double>,Vec3<double>>` (48 bytes: min.x,y,z then max.x,y,z as doubles) |
| `axis(min2, max2)` / `AXES_2D` | same shape, z padded to -1.0/1.0 |
| `globalIllumination(pos)` / `GLOBAL_ILLUMINATION` | `VEC3` |
| `showLegend()` / `SHOW_LEGEND` | none |
| `softClearView()` / `SOFT_CLEAR` | none |
| `clearView()` / `CLEAR` | none |
| `disableAutomaticAxesAdjustment()` / `DISABLE_AXES_FROM_MIN_MAX` | none |
| `disableScaleOnRotation()` / `DISABLE_SCALE_ON_ROTATION` | none |
| `axesSquare()` / `AXES_SQUARE` | none |
| `setAxesBoxScaleFactor(vec3)` / `SET_AXES_BOX_SCALE_FACTOR` | `VEC3` |
| `setTitle(str)` / `SET_TITLE` | `TITLE_STRING` (a `Label`) |
| `setTransform(id, scale, rotation, translation)` / `SET_OBJECT_TRANSFORM` | `ROTATION_MATRIX` (9 doubles), `TRANSLATION_VECTOR` (3 doubles), `SCALE_MATRIX` (9 doubles), `ITEM_ID` |
| `openProjectFile(path)` / `OPEN_PROJECT_FILE` | `PROJECT_FILE_NAME` (a `Label`) |
| `screenshot(base_path)` / `SCREENSHOT` | `SCREENSHOT_BASE_PATH` (a `Label`) |
| `spawn()` | not a wire call — shells out to `./main_application/duoplot &` if `isDuoplotRunning()` is false (checked via `ps -ef \| grep duoplot`) |

### `PropertySet` payload (used by `setProperties(vector<PropertySet>)`)

Hand-packed, not going through `CommunicationHeader`'s object/property
mechanism at all (see `property_set.h::PropertySet::fillBuffer`):

```
Offset 0: uint8_t                  num_property_sets
For each PropertySet:
  uint8_t                          num_properties_in_this_set
  sizeof(ItemId) (=2) bytes        item id this set applies to
  For each property in the set:
    uint8_t                        size of this property's data
    <size> bytes                   the property's serialized CommunicationHeaderObject data (same per-type layout as in the main header's property entries)
```

This whole blob is sent as a single `Vector<uint8_t>` payload after a
`PROPERTIES_EXTENSION_MULTIPLE` header with no header objects.

## UDP query/ack protocol (port 9757)

Request: `sendThroughQueryUdpInterface(data)` — a raw `UInt8ArrayView`, no
special envelope, capped at `kMaxNumBytesForOneTransmission` (1380) bytes.

Reply: exactly 5 bytes, ASCII `"ack#"` + `'\0'` (`utils.h::ackValid` checks
`data[0..4] == 'a','c','k','#','\0'`). Anything else throws
`std::runtime_error` client-side.

`receiveFromQueryUdpInterface()` (receive-only variant): expects the first
`sizeof(size_t)` bytes of the datagram to be a `size_t` value and returns it
raw — no ack framing. Used rarely; check current callers before assuming this
path is exercised.

## GUI callback channel protocol (port 9758, server → client)

Unlike the plot channel, **the client binds and listens** on this port
(`gui_internal.h::initTcpSocket`, `SO_REUSEADDR` set). `main_application`
connects out to the client and pushes messages. Framing per message:

```
Offset 0: uint64_t   num_expected_bytes (length of everything after this field)
Offset 8: ...        payload (read in a loop until num_expected_bytes bytes received)
```

No magic number or endianness byte on this channel (unlike the 9755 channel).

### Payload shape — single element update (`updateGuiState` / `callGuiCallbackFunction`)

```
Offset 0: uint8_t              GuiElementType (see enumerations.h)
Offset 1: uint8_t               handle_string_length
Offset 2: <handle_string_length> bytes   handle_string (ASCII, not null-terminated)
next:     uint32_t              payload_size
next:     <payload_size> bytes  type-specific state blob (see table below)
```

### Payload shape — full sync (`waitForSyncForAllGuiElements`, sent once after `queryForSyncOfGuiData`)

```
Offset 0: uint8_t     num_gui_objects
Repeated num_gui_objects times, each in the "single element update" shape above
(type, handle_string_length, handle_string, payload_size, payload)
```

### Type-specific state blobs (`gui_internal.h::*Internal::updateState`)

| `GuiElementType` | Blob layout |
|---|---|
| `Slider` | `int32_t min_value, max_value, step_size, value` (16 bytes, fixed) |
| `Button` | `uint8_t is_pressed` (1 byte) |
| `Checkbox` | `uint8_t is_checked` (1 byte) |
| `TextLabel` | `uint8_t label_length`, then `label_length` bytes of label text |
| `ListBox` | `uint8_t selected_length`, `selected_length` bytes selected text, `uint16_t num_elements`, then per element: `uint8_t elem_length` + `elem_length` bytes |
| `DropdownMenu` | same shape as `ListBox` |
| `EditableText` | `uint8_t enter_pressed` (as a byte), `uint8_t text_length`, then `text_length` bytes of text |
| `RadioButtonGroup` | `int32_t selected_idx`, `uint16_t num_buttons`, then per button: `uint8_t button_length` + `button_length` bytes |

Client → server direction on this same logical channel (but physically sent
over the *plot* channel, port 9755, as ordinary header-only `Function`
values) uses these functions instead — see `gui_internal.h::InternalGuiElementHandle`:
`SET_GUI_ELEMENT_LABEL` (`LABEL` + `HANDLE_STRING` objects),
`SET_GUI_ELEMENT_ENABLED`/`_DISABLED` (`HANDLE_STRING` only),
`SET_GUI_ELEMENT_MIN_VALUE`/`_MAX_VALUE`/`_VALUE`/`_STEP`
(`HANDLE_STRING` + `INT32`).

## Cross-checking against the other two language bindings

When changing anything in this document, also check:
- `src/interfaces/c/duoplot/enumerations.h`, `communication_header*.h` — must
  have matching numeric enum values and struct byte layouts.
- `src/interfaces/python/duoplot/enums.py`, `internal.py`,
  `serialization.py` — same requirement, plus `python/enum_generator.py` /
  root `change_enum_names.py` which may auto-generate parts of `enums.py`.
- `src/main_application/main_window_receive.cpp` and each
  `plot_objects/*/{*.h,*.cpp}`'s `convertRawData` — the receiving side that
  must decode whatever this document describes.
