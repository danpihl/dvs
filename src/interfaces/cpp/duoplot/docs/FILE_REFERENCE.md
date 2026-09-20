# File-by-file reference — `src/interfaces/cpp/duoplot/`

Quick-lookup for refactoring: what each file is responsible for, its key
types/functions, and what else in the codebase depends on its exact layout.
For the wire format itself see [`PROTOCOL.md`](PROTOCOL.md); for the bigger
picture see [`ARCHITECTURE.md`](ARCHITECTURE.md).

## Top-level headers

### `duoplot.h`
Umbrella include: pulls in `control_functions.h` + `gui_api.h` +
`plot_functions.h`. Client code normally only includes this one file
(`#include "duoplot/duoplot.h"`).

### `constants.h`
All magic numbers in one place: `kMagicNumber` (0xdeadbeefcafebabe),
`kMaxNumBytesForOneTransmission` (1380, UDP-only limit), `kTcpPortNum` (9755),
`kGuiTcpPortNum` (9758), `kUdpQueryPortNum` (9757),
`kCommunicationHeaderObjectDataSize` (255, per-field cap),
`kMaxNumObjects`/`kMaxNumProperties` (10 each, per-header cap),
`kHeaderDataStartOffset` (17, byte offset where the header begins in a raw
received frame). Touch this file only when changing the wire format itself —
every constant here has a corresponding hardcoded value in the C and Python
interfaces and in `main_application`.

### `enumerations.h`
All wire-format enums: `CommunicationHeaderObjectType`, `PropertyFlag`,
`PropertyType`, `Function`, `DataType`, `DataStructure`, `ColorT`/
`EdgeColorT`/`FaceColorT`/`SilhouetteT` (all under `internal::`), plus
top-level `duoplot::ElementParent`, `DistanceFromType`, `GuiElementType`.
**Enum value = wire value; never reorder or insert in the middle of an
existing enum** — always append before `UNKNOWN`. See `PROTOCOL.md` for the
full enumerated meaning of each.

### `item_id.h`
Defines `enum class ItemId : uint16_t` with exactly 255 literal members
(`ID0`..`ID254`) plus `UNKNOWN = 0xFFFF`, and a `duoplot::properties::` alias
for each (`properties::ID0`, etc., so client code writes
`properties::ID3` rather than `ItemId::ID3`). This is how a client refers back
to a previously-plotted object (for `setProperties`, `deletePlotObject`,
`setTransform`, `realTimePlot`'s per-series id). Almost entirely boilerplate
— if this ever needs to grow past 255 IDs it requires a wire-format change
(currently serialized as a single `uint16_t`, so room exists, but the
enumerator list itself would need generating rather than hand-editing).

## Header serialization layer

### `communication_header.h`
The central `CommunicationHeader` class — this is what every plot/control
function builds up and sends. Key pieces:
- `CommunicationHeaderObjectLookupTable` / `PropertyLookupTable`: fixed-size
  `uint8_t[]` arrays mapping enum value → index, sentinel 255 = absent.
- `CommunicationHeader::Array<N>`: a fixed-capacity array type (backs both
  the objects and properties arrays) — throws on overflow past `N`
  (`kMaxNumObjects`/`kMaxNumProperties`, both 10).
- `extend(settings...)` / `extendInternal(...)`: the variadic entry point
  every plot/control function calls to fold in trailing arguments. Uses
  `static_assert`+`is_same` chains (not virtual dispatch or concepts) to
  route each argument type to the right handling: `PropertyFlag` → flags
  array bit; `ItemId` → header object; `ColorT`/`EdgeColorT`/`FaceColorT` →
  expanded to full color property struct; `properties::ScatterStyle`/
  `ColorMap`/`LineStyle` → `appendEnumProperty`; anything else deriving from
  `PropertyBase` → `appendProperty`. **Adding a new settable type means
  adding a branch here** (and to the `static_assert` allow-lists in both
  `extendInternal` overloads — they're currently duplicated, a
  refactor target if you're touching this).
- `append<U>(type, data)`: appends a raw header **object** (not a property).
- `CommunicationHeader(const UInt8ArrayView&)`: the deserializing
  constructor, used to reconstruct a header from received bytes — must stay
  byte-for-byte consistent with `fillBufferWithData`.
- `fillBufferWithData(FillableUInt8Array&)`: the serializer (has a `// TODO:
  Rename to "serialize"` comment — naming is inconsistent with
  `encode_decode_functions.h`'s `serializeToCommunicationHeaderObject`).
- `numBytes()`: must exactly match what `fillBufferWithData` writes — these
  two functions are easy to let drift if you add a new field to the header.
- `templateToObjectType<T>()`/`value<T>()`/`valueOr<T>()`: currently only
  meaningfully support `T = ItemId` (see `ARCHITECTURE.md` rough edges).

Depends on: `communication_header_object.h`, `encode_decode_functions.h`,
`plot_properties.h`, `fillable_uint8_array.h`, `constants.h`, `enumerations.h`.

### `communication_header_object.h`
Defines `CommunicationHeaderObject` — the `{type: CommunicationHeaderObjectType,
size: uint8_t, data: uint8_t[255]}` struct that is the atomic unit of both
"objects" and "properties" in a header. Contains:
- A long list of **converting constructors** — `CommunicationHeaderObject(type,
  const U&)` overloads for every serializable payload shape (primitive
  numeric types, `Dimension2D`, `pair<Vec3,Vec3>`, `ItemId`, `DataType`,
  `Vec3<double>`, `MatrixFixed<double,3,3>`, `properties::Label`). Adding a
  new object type to put directly into a header (as opposed to a `PropertyBase`
  property) means adding a constructor overload here.
- A parallel set of **`as<T>()` template specializations** — the decode side
  for every type above, plus every property type (`Label`, `Alpha`,
  `LineWidth`, `ColorInternal`, `EdgeColor`, `FaceColor`, `Silhouette`,
  `ScatterStyle`, `ColorMap`, `LineStyle`, `PointSize`, `BufferSize`,
  `DistanceFrom`, `ZOffset`, `Transform`, `PropertyFlag`). **Every
  constructor overload above should have a matching `as<T>()` here** — check
  both when adding a type.

### `encode_decode_functions_defs.h` / `encode_decode_functions.h`
Forward declarations (`_defs.h`) and implementations (no `_defs` suffix) of
`numBytes(const T&)`, `serializeToCommunicationHeaderObject(obj, const T&)`,
and `deserializeFromCommunicationHeaderObject(T&, const obj)` for every
property/object type. This is the actual byte-packing logic that
`CommunicationHeaderObject`'s constructors/`as<T>()` and
`CommunicationHeader::appendProperty` delegate to. Split into `_defs.h` +
implementation to break a circular include between `communication_header.h`
and `communication_header_object.h` (both need these declared before either
class body is complete). **If you need to see the actual byte-for-byte
packing of a specific property, this is the file to read** —
`communication_header_object.h`'s `as<T>()` table just tells you *which*
property types exist, not their exact layout.

### `fillable_uint8_array.h`
Two small, dependency-free helper types used everywhere a raw byte buffer
needs building or viewing:
- `UInt8ArrayView`: non-owning `{const uint8_t* data, size_t size}` — the
  type passed to the socket-send functions.
- `FillableUInt8Array`: owns a `new[]`-allocated buffer, exposes
  `fillWithStaticType<T>(T)` (memcpy + advance) and
  `fillWithDataFromPointer<T>(T*, count)`, asserts it never overflows its
  declared size. No bounds *shrinking* — you must size it exactly right
  upfront (typically via a paired `numBytes()` call), which is why
  `CommunicationHeader::numBytes()` must stay in sync with
  `fillBufferWithData()`.

### `utils.h`
Grab-bag of small free functions: generic (unused by current wire code, but
present) `fillBufferWithObjects`/`fillObjectsFromBuffer` template pack
helpers; `isBigEndian()` (used only to set the informational endianness byte
— see `PROTOCOL.md` note that nothing currently branches on it);
`ackValid(const char[256])` (validates the 5-byte `"ack#\0"` UDP ack).

## Transport layer

### `internal.h`
The plot-channel (port 9755) and UDP query-channel (port 9757) transport.
Key pieces:
- `UdpClient`: thin RAII wrapper around a UDP socket (`socket`/`sendto`/
  `recvfrom`/`close`).
- `initializeTcpSocket` / `sendThroughTcpInterfaceClient`: lazily opens
  (function-local `static` file descriptor + `static bool` init flag) **one
  persistent TCP connection per process**, reused for every subsequent plot
  call. This means a client that never plots anything never opens a socket,
  and a client that fails to connect once (`main_application` not running)
  silently keeps trying to write to a bad fd on every call thereafter (logs a
  warning on the *first* failed connect only — see `DUOPLOT_LOG_WARNING` in
  `initializeTcpSocket`, not repeated on subsequent sends).
- `sendThroughTcpInterface`: an alternate, non-persistent variant that opens
  a fresh socket, sends, and closes it (`getSendFunction()` never returns
  this one currently — it's effectively unused/legacy since
  `getSendFunction()` hardcodes `sendThroughTcpInterfaceClient`).
- `sendThroughQueryUdpInterface` / `receiveFromQueryUdpInterface`: the UDP
  ack'd query path (see `PROTOCOL.md`).
- `getSendFunction()`: returns the `SendFunctionType` (a
  `std::function<void(UInt8ArrayView, uint64_t)>`) actually used by every
  plot/control function — currently always `sendThroughTcpInterfaceClient`.
  If you ever need to inject a mock transport for testing, this is the seam.
- `countNumBytes` / `fillBuffer` / `fillBufferWithCollection`: variadic
  helpers used by the `sendHeaderAndData`/`sendHeaderAndVectorCollection`
  overloads below to size and fill the outer buffer across multiple payload
  arrays.
- `sendHeaderAndData(send_fn, hdr, elem1, ...elemN)`: the main entry point
  used by nearly every plot function — builds the full envelope (endianness
  byte + magic + length + header + concatenated payloads) and calls
  `send_fn`.
- `sendHeaderAndVectorCollection(...)`: variant for `plotCollection`/
  `plotCollection3` — additionally writes a `vector_lengths` array before the
  per-object payloads (see `PROTOCOL.md`).
- `sendHeaderOnly(send_fn, hdr)`: for control functions with no payload.
- `isDuoplotRunning()`: shells out to `popen("ps -ef | grep duoplot")` and
  string-matches process lines. Used by `spawn()` and the GUI heartbeat
  thread. Fragile by nature (relies on the process being invoked with a
  literal `duoplot` in its command line) — don't rely on it for anything
  more than a best-effort check.

Uses raw POSIX headers directly (`<sys/socket.h>`, `<netinet/in.h>`, etc.) —
**this file is not portable to non-POSIX platforms (e.g. Windows) as-is.**

### `gui_internal.h`
The GUI-callback channel (port 9758) transport plus per-widget-type internal
state classes. See `ARCHITECTURE.md` and `PROTOCOL.md` for the reversed
client-as-server model and byte layouts. Key pieces:
- `InternalGuiElementHandle`: abstract base for all internal widget state,
  holds `handle_string_`/`type_`, and implements the client→app control
  calls (`setLabel`, `setEnabled`, `setDisabled`, `setMinValue`,
  `setMaxValue`, `setValue`, `setStepSize`) by sending header-only messages
  over the *normal* plot channel (port 9755, via `getSendFunction()`) —
  despite living in the "GUI" file, these particular calls do NOT use the
  9758 socket. Pure-virtual `updateState(UInt8ArrayView)` is the decode hook
  each concrete subclass implements.
- `SliderInternal`, `ButtonInternal`, `CheckboxInternal`, `TextLabelInternal`,
  `ListBoxInternal`, `EditableTextInternal`, `DropdownMenuInternal`,
  `RadioButtonGroupInternal`: one per `GuiElementType`, each with its own
  `updateState` byte-layout (documented in `PROTOCOL.md`).
- `getGuiElementHandles()`: process-global `map<string, shared_ptr<InternalGuiElementHandle>>` registry, keyed by handle string.
- `initTcpSocket()`: binds+listens on `kGuiTcpPortNum` (9758) —
  **this file's client acts as a TCP server**, unlike everything else in this
  library.
- `ReceivedGuiData`: move-only owning buffer (manual `new[]`/`delete[]`,
  no smart pointer) for one received GUI message.
- `receiveGuiData()`: blocking `accept()` + length-prefixed read loop on the
  9758 socket.
- `populateGuiElementWithData(type, handle_string, data)`: creates-or-updates
  the registry entry for a handle string; errors (logged, not thrown) if a
  handle string already exists under a different `GuiElementType`.
- `waitForSyncForAllGuiElements()`: parses the one-time full-sync message
  (see `PROTOCOL.md`) and populates the registry for every existing GUI
  element the app currently has.
- `queryForSyncOfGuiData()`: sends the `QUERY_FOR_SYNC_OF_GUI_DATA` request
  (over port 9755) that triggers the app to send the full sync above.
- `updateGuiState(ReceivedGuiData)`: parses one single-element update
  message and forwards to `populateGuiElementWithData`.

## Domain / API layer

### `item_id.h` — see above (Header serialization layer section covers it
logically but it's really a plain domain type; listed once).

### `plot_properties.h`
All the settable "property" structs client code actually constructs:
`PropertyBase` (the common base, holds a `PropertyType`), `LineWidth`,
`Alpha`, `ZOffset`, `Transform` (with three constructor forms: fixed-size
`MatrixFixed<double,3,3>`, or dynamic `Matrix<double>` with a runtime
3x3 size assertion), `Label` (the 100-char-capped string-carrier type reused
for titles/handles/paths, not just the `NAME` property), `LineStyle` (enum),
`ScatterStyle` (enum), `Color`/`EdgeColor`/`FaceColor`/`Silhouette` (each with
a converting constructor from their respective `*T` shorthand enum, and
`static constexpr` named-color members so client code can write
`properties::Color::RED`), `ColorMap` (enum), `PointSize`, `DistanceFrom`
(private constructor + named static factories `x/y/z/xy/xz/yz/xyz` — the only
way to construct one), `BufferSize`. Also the flag constants
(`properties::PERSISTENT`, `FAST_PLOT`, `APPENDABLE`,
`EXCLUDE_FROM_SELECTION`, `INTERPOLATE_COLORMAP`, and the
`not_ready::UPDATABLE`/`not_ready::SELECTABLE` reserved pair). At the bottom,
`internal::ColorInternal` (the actual wire-serialized form of `Color`, tagged
`PropertyType::COLOR`) and `internal::Dimension2D`.

### `property_set.h`
`PropertySet`: a small standalone class (does NOT reuse `CommunicationHeader`)
for building one entry of a `setProperties(vector<PropertySet>)` batch call —
pairs an `ItemId` with an arbitrary list of properties, and knows how to
compute its own serialized size (`getTotalSize`) and pack itself
(`fillBuffer`) into a caller-provided buffer. See `PROTOCOL.md`'s
"PropertySet payload" section for the exact byte layout. Used only from
`control_functions.h::setProperties(const std::vector<PropertySet>&)`.

### `plot_functions.h`
The bulk-data plotting API — every function documented in `PROTOCOL.md`'s
"Payload layout per plot-producing function" table. Organized as many
overload sets, roughly one per `Function` enum value, each with variants for
owning (`Vector<T>`/`Matrix<T>`) vs. view (`VectorConstView<T>`/
`MatrixConstView<T>`) inputs, and with/without an accompanying color/point-size
vector. Notable non-trivial functions:
- `drawCubes`: pure client-side geometry expansion (12 triangles per cube,
  hand-unrolled per-face vertex math) that ends up calling the `drawMesh`
  wire path — no dedicated `Function` enum value of its own.
- `drawLine`: builds a 2-point line from a `Line3D` + two parameter values
  via `line.eval(t)`.
- `realTimePlot`: the only function that takes a single scalar (not a
  vector) — packs `{dt, y}` into a 2-element `Vector<T>` and tags it with an
  `ItemId` so the server can accumulate a running series per id.
See `API_REFERENCE.md` for the full flat function list with signatures.

### `control_functions.h`
The non-bulk-data / view-and-session control API — every function documented
in `PROTOCOL.md`'s "Control-function wire shapes" table
(`setProperties`, `setCurrentElement`, `deletePlotObject`, `view`, `axis`,
`clearView`, `setTitle`, `setTransform`, `openProjectFile`, `screenshot`,
`spawn`, etc.). Also hosts `flushMultipleElements`'s variadic
string-collecting helper (`internal::flushMultipleElementsInternal`).
`spawn()` is the one function here that doesn't touch the wire protocol at
all — it's a local `system()` call gated on `isDuoplotRunning()`.

### `gui_api.h`
The public GUI-widget API: `SliderHandle`, `ButtonHandle`, `CheckboxHandle`,
`TextLabelHandle`, `ListBoxHandle`, `EditableTextHandle`, `DropdownMenuHandle`,
`RadioButtonGroupHandle` — each a thin wrapper around a
`shared_ptr<internal::XInternal>` from `gui_internal.h`, exposing typed
getters and (where applicable) setters that delegate to the base class's
wire-sending methods. `registerGuiCallback(handle_string, fn)` (overloaded
per handle type) stores callbacks in one of the per-type
`map<string, XCallbackFunction>` registries (`internal::getXCallbacks()`).
`getGuiElementHandle<T>(handle_string)` (explicit specializations, one per
handle type) looks up the registry, validates the stored `GuiElementType`
matches `T`, and returns the typed wrapper — throws if not found or
mismatched. `callGuiCallbackFunction(ReceivedGuiData)` parses one incoming
message's header (type/handle/payload-size) and dispatches to the matching
callback if one is registered. `startGuiReceiveThread()` is the actual
"turn on the GUI feature" entry point client code calls once — see
`ARCHITECTURE.md`'s GUI channel section for what its three background threads
do. Note: `TextLabelCallbackFunction`/its registration overload is fully
commented out — `TextLabelHandle` exists and has state, but has no
callback-registration path currently wired up.

## `math/` — supporting linear algebra / geometry / image library

Not part of the wire protocol itself, but it's what every plot function's
input types come from. Roughly:
- `lin_alg/vector_dynamic/`, `lin_alg/matrix_dynamic/`: `Vector<T>`,
  `VectorConstView<T>`, `Matrix<T>`, `MatrixConstView<T>` — dynamically-sized,
  own or view external memory, expose `.data()`/`.numElements()`/`.size()`
  (the interface `sendHeaderAndData` etc. rely on duck-typing against).
- `lin_alg/matrix_fixed/`: `MatrixFixed<T, Rows, Cols>` — used for the fixed
  3x3 transform matrices.
- `lin_alg/vector_low_dim/`: `Vec2<T>`, `Vec3<T>`, `Vec4<T>` — small
  fixed-size math vectors (as opposed to `Vector<T>`, the dynamic array type;
  the naming is easy to confuse when reading call sites).
- `geometry/`: `Line2D<T>`, `Line3D<T>`, `Plane<T>`, `Triangle<T>` — used by
  e.g. `drawLine`.
- `image/`: `ImageGray<T>`, `ImageGrayAlpha<T>`, `ImageRGB<T>`,
  `ImageRGBA<T>` (plus `*ConstView` variants) — backing types for `imShow`.
- `structures/index_triplet.h`: `IndexTriplet` — three vertex indices, one
  mesh triangle.
- `transformations/`: `AxisAngle` — not currently referenced from the
  plot/control API surface at the time of writing; check current usage
  before assuming it's live.
- `math.h` / `pre_defs.h` / `lin_alg.h`: aggregating includes.
- `misc/forward_decl.h`, `misc/math_macros.h`: forward declarations and
  shared macros (e.g. assertion helpers used across the math types).

This math library also has a point of coupling worth knowing: `Point2<T>`,
`Point3<T>` (used by `drawPoint`, `drawMesh`'s vertex overloads) and
`PointXY`/`PointXZ`/`PointYZ<T>` (used by `DistanceFrom::xy/xz/yz`) are
referenced throughout `plot_functions.h`/`plot_properties.h` but are thin
aliases/wrappers over the `Vec2`/`Vec3` types rather than a separate
hierarchy — check `math/lin_alg/vector_low_dim/` if you need their exact
definitions.

## Logging / timing (not protocol-related)

### `logging.h`
Self-contained logging macros (`DUOPLOT_LOG_INFO()`, `_WARNING()`, `_ERROR()`,
`_DEBUG()`, `_TRACE()`, `_FATAL()`, `DUOPLOT_ASSERT(cond)`,
`DUOPLOT_EXIT(cond)`, plain `DUOPLOT_PRINT()`) — all return an
`std::ostringstream&`-yielding temporary (`Log` class) so call sites read as
`DUOPLOT_LOG_WARNING() << "message" << value;`. `DUOPLOT_ASSERT` raises
`SIGABRT` on failure (not a normal C++ exception) — keep this in mind before
wrapping any interface call in a `try/catch` expecting assertion failures to
be catchable. Global settings (`useColors`, `showFile`, `showLineNumber`,
`showFunction`, `showThreadId`) are process-wide mutable statics guarded by a
mutex.

### `timing.h`
Two tiny helpers: `timing::getTimeNow()` and `timing::timePointsToMsDouble()`
wrapping `std::chrono::high_resolution_clock`. Not used by the protocol path
itself; available for client-side instrumentation.
