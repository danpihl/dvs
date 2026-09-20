# C++ interface (`src/interfaces/cpp/duoplot`) — architecture overview

This is the header-only C++ client library for **Duoplot**. It lets a separate
application ("the client") send plot commands and data to the standalone
`main_application` (the `duoplot` GUI process, under `src/main_application/`)
over a custom binary protocol on TCP. There is no shared code between client
and server — they only agree on the wire format described in
[`PROTOCOL.md`](PROTOCOL.md).

The same wire protocol is implemented three times, independently:

- `src/interfaces/cpp/duoplot/` — this library (templated, header-only)
- `src/interfaces/c/duoplot/` — C mirror (structs + `duoplot_`-prefixed functions)
- `src/interfaces/python/duoplot/` — Python mirror (numpy-based)

**Any change to enums, struct layouts, or serialization order in this
directory must be mirrored in the other two, and in the server-side parsing
code in `src/main_application/` (`convertRawData` per plot type, GUI receive
handling). There is no schema/codegen step tying these together** (there is a
`change_enum_names.py` / `python/enum_generator.py` helper for keeping enum
*names* in sync, but struct layouts are hand-maintained). This is the single
biggest hazard when refactoring this code.

## Why header-only / templated

Plot functions like `plot<T>(...)` are templated on the numeric element type
(`float`, `double`, `int32_t`, ...) so the same call compiles for any
supported `DataType`, and the type is written into the wire header
(`typeToDataTypeEnum<T>()`) so the receiving `main_application` knows how to
reinterpret the raw bytes. This trades codegen/reflection for template
instantiation — there is no runtime dispatch on the client side for choosing
serialization strategy.

## Layered structure (bottom-up)

```
┌─────────────────────────────────────────────────────────────┐
│  Public API                                                  │
│  duoplot.h, plot_functions.h, control_functions.h, gui_api.h │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│  Domain types                                                │
│  enumerations.h, item_id.h, plot_properties.h,               │
│  property_set.h, math/ (Vector, Matrix, Vec3, Point3,        │
│  Image*, IndexTriplet, Line/Plane/Triangle)                  │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│  Header serialization                                        │
│  communication_header.h, communication_header_object.h,      │
│  encode_decode_functions.h / _defs.h, fillable_uint8_array.h │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│  Transport                                                   │
│  internal.h (plot channel, port 9755 + UDP 9757)             │
│  gui_internal.h (GUI callback channel, port 9758)            │
└─────────────────────────────────────────────────────────────┘
```

Everything above "Transport" is pure serialization/data-modeling and has no
socket knowledge. `internal.h` and `gui_internal.h` are the only files that
touch BSD sockets.

## Data flow: a plot call end to end

Example: `duoplot::plot(x, y, properties::Color::RED, properties::LineWidth(3))`

1. **`plot_functions.h`** (`plot()`) constructs a `CommunicationHeader` seeded
   with `Function::PLOT2`, appends mandatory fields (`DATA_TYPE`,
   `NUM_ELEMENTS`), then calls `hdr.extend(settings...)` to fold in the
   variadic trailing arguments.
2. **`communication_header.h`** (`CommunicationHeader::extendInternal`) uses
   `static_assert`/`is_same` type-switching (not virtual dispatch) to decide,
   per argument, whether it's a `PropertyFlag` (set a bit in the flags byte
   array), an `ItemId` (appended as a header object), a color enum shorthand
   (`ColorT`/`EdgeColorT`/`FaceColorT`, expanded to the full property struct),
   or a generic `PropertyBase`-derived struct (appended as a "property").
   Some plot functions also swap the `Function` after the fact — e.g. `plot()`
   rewrites `PLOT2` → `FAST_PLOT2` if the `FAST_PLOT`/`APPENDABLE` flag was
   set (see `plot_functions.h`).
3. Each header **object** and **property** is stored as a
   `CommunicationHeaderObject` (`communication_header_object.h`): a
   `{type, size, data[255]}` fixed-capacity struct. A per-type
   `serializeToCommunicationHeaderObject` overload (declared in
   `encode_decode_functions_defs.h`, implemented in
   `encode_decode_functions.h`) knows how to pack each property struct into
   that `data[]` buffer.
4. **`internal::sendHeaderAndData()`** (`internal.h`) computes the total byte
   count, allocates a `FillableUInt8Array`, writes the endianness byte + magic
   number + length prefix, has the header serialize itself
   (`hdr.fillBufferWithData`), then appends the raw payload arrays (`x`, `y`,
   ... — anything with a `.data()`/`.numElements()` pair, i.e. `Vector<T>`,
   `Matrix<T>`, `Image*<T>`, etc.) back to back.
5. **`internal::sendThroughTcpInterfaceClient()`** lazily opens (and keeps
   open, via function-local `static`) one TCP connection per client process
   to `127.0.0.1:9755`, and writes the length-prefixed blob.
6. On the server side (`main_application`), `DataReceiver` reads the frame,
   parses the `CommunicationHeader` back out, and — keyed by whatever the last
   `setCurrentElement()` call set as the target — routes the typed payload
   into the right `PlotObjectBase::convertRawData()` for rendering. That side
   is out of scope for this doc set; see `src/main_application/`.

Control-only calls (no bulk payload — `setTitle`, `view`, `clearView`, ...)
skip step 4's payload step and call `internal::sendHeaderOnly()` instead.

## The GUI callback channel is a separate, reversed connection

Interactive GUI widgets (sliders, buttons, checkboxes, ...) need the
*application* to notify the *client* of user interaction. This can't reuse
the port-9755 connection (client is the initiator there), so:

- The **client** binds and listens on TCP port `kGuiTcpPortNum` (9758) —
  see `gui_internal.h::initTcpSocket()`. This is unusual: for this one
  channel the client library acts as the server.
- `gui::startGuiReceiveThread()` (`gui_api.h`) spawns three background
  threads: one to ask `main_application` for an initial full GUI-state sync
  (`queryForSyncOfGuiData`, sent over the normal 9755 channel), one blocking
  receive loop that updates local `InternalGuiElementHandle` state and
  invokes any registered callback (`callGuiCallbackFunction`), and one
  heartbeat loop that just polls `isDuoplotRunning()` (currently a no-op
  besides that check — see TODOs in that function).
- Each GUI element type (`SliderInternal`, `ButtonInternal`,
  `CheckboxInternal`, `TextLabelInternal`, `ListBoxInternal`,
  `EditableTextInternal`, `DropdownMenuInternal`, `RadioButtonGroupInternal`
  in `gui_internal.h`) owns its own hand-rolled binary `updateState()`
  decoder — see [`PROTOCOL.md`](PROTOCOL.md) for the exact byte layouts.
- Public-facing `*Handle` wrapper classes (`gui_api.h`) hold a
  `shared_ptr<InternalGuiElementHandle>` and expose typed getters/setters;
  `getGuiElementHandle<T>()` does a `dynamic_pointer_cast` + `GuiElementType`
  check to hand back the right wrapper.

## Query/ack channel (UDP, port 9757)

A third, minor channel: `sendThroughQueryUdpInterface()` /
`receiveFromQueryUdpInterface()` in `internal.h` send a small UDP datagram and
expect a 5-byte `"ack#\0"` reply (validated by `utils.h::ackValid`). Used
for synchronous query-style round trips distinct from the fire-and-forget
plot channel. Lightly used; treat as legacy/special-purpose rather than a
primary path when refactoring.

## Notable rough edges (useful to know before refactoring)

- `CommunicationHeader::fillBufferWithData` has a `// TODO: Rename to
  "serialize"` comment — the name predates the rest of the (de)serialize
  naming convention.
- `CommunicationHeader::templateToObjectType<T>()` only supports `ItemId` and
  asserts otherwise (`// TODO: Ugly`) — `valueOr`/`value` are effectively
  single-purpose despite being templates.
- `gui_api.h` has a commented-out `TextLabelCallbackFunction` / callback
  registration path — text labels have state (`TextLabelInternal`) but no
  wired-up callback registration.
- `EditableTextHandle`'s "enter pressed" is a one-shot event flag with a
  known unresolved reset-timing issue (see comment in
  `getGuiElementHandle<EditableTextHandle>`).
- `gui_api.h::startGuiReceiveThread`'s heartbeat thread doesn't currently act
  on `duoplot` going away (logging calls are commented out) — see its TODO.
- Wire size limits are small and easy to hit silently: labels/titles cap at
  100 chars (`properties::Label::kMaxLength`), a single `CommunicationHeaderObject`
  caps at 255 bytes (`kCommunicationHeaderObjectDataSize`), and a header can
  hold at most 10 objects and 10 properties (`kMaxNumObjects`/`kMaxNumProperties`
  in `constants.h`).
- `ItemId` is a fixed enum of 255 literal values (`ID0`..`ID254`) rather than
  an arbitrary integer type — see `item_id.h`. This bounds how many
  independently-addressable plot objects/properties can exist per element.

See [`PROTOCOL.md`](PROTOCOL.md) for exact wire formats,
[`FILE_REFERENCE.md`](FILE_REFERENCE.md) for a per-file breakdown, and
[`API_REFERENCE.md`](API_REFERENCE.md) for the public function/type surface.
