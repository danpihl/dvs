# `src/main_application/serial_interface/` — architecture overview

> **Note (post-`39a6dc9e` pull):** `main_application` now depends on the
> external `third_party/LumosAlgo` submodule instead of
> `src/interfaces/cpp/duoplot/` — see
> `src/main_application/docs/ARCHITECTURE.md`'s dedicated section. This
> module itself has no dependency on either (its own `definitions.h`/
> `object_types.h` protocol is completely separate from the TCP wire
> protocol, as noted below), so it's unaffected beyond the portability fix
> noted just below.

This module is a second, completely independent data-input path into
`main_application`, alongside the TCP client protocol documented in
`src/interfaces/cpp/duoplot/docs/PROTOCOL.md`. It reads a byte-stuffed
framed protocol off a UART/serial port (e.g. from a microcontroller),
extracts individual typed values, and feeds them onward to whatever GUI
elements or streaming plot objects have subscribed to a given topic. 8
files, ~920 lines.

**GUI-toolkit note (relevant to the planned wxWidgets → Qt migration): no
wxWidgets usage anywhere in this module** (confirmed by search) — this is
POSIX termios serial I/O plus plain framing/parsing code. However, **this
module has a more serious portability problem than any other one documented
so far**: see "Known rough edges" below — `serial_port.h`/`.cpp` are
effectively macOS-only as currently written, and are compiled
unconditionally on every platform.

## What this module is *not*

It has no knowledge of the TCP client protocol, `CommunicationHeader`, or
anything in `src/interfaces/`. Its own framing/type system
(`ObjectType`, `NumberDataType`, `TopicId`) is entirely separate from the
interface library's `Function`/`DataType`/`ItemId` enums, despite looking
superficially similar. Don't conflate the two when tracing a bug — check
which protocol is actually in play first.

## Pipeline overview

```
SerialPort (termios read/write)
  → SerialInterface (background thread, ring buffer, frame extraction)
    → RawDataFrame (one de-stuffed-boundary, still-escaped payload)
      → BufferedReader (de-escapes + deserializes as it reads)
        → objects::BaseObject subclasses (Float/Double/Int8../UInt64/...)
          → consumed by MainWindow (main_window_serial.cpp, outside this module)
            → routed by TopicId to subscribed PlotPanes / GUI elements
```

### `SerialPort` (`serial_port.h/.cpp`)

Thin wrapper around a POSIX `termios` file descriptor: opens the port
non-blocking (`O_RDWR | O_NOCTTY | O_NDELAY`), configures it in
`reconfigurePort` (raw mode — canonical/echo/signal processing all
disabled, 8N1, no flow control, `VMIN=0`/`VTIME=20` for a bounded-wait
non-blocking-ish read), and exposes `numAvailableBytes()` (via
`ioctl(fd, TIOCINQ, ...)`), `readBytes`, `writeBytes`, `flushPort`,
`isValid()`. No framing knowledge at this layer — just raw bytes in/out.

### `SerialInterface` (`serial_interface.h/.cpp`)

Owns one `SerialPort` and a fixed **8 MiB** (`kBufferSize`) heap-allocated
ring buffer (`buffer_`), read into by a dedicated background
`std::thread` (`readSerialDataThreadFunction`, spawned by `start()`) that
polls `numAvailableBytes()` and reads into the ring buffer every 10 ms
(`kReadPeriodUs`), handling the wraparound-at-buffer-end case explicitly in
`readSerialData()`. `start()` also does an initial synchronous drain-and-
discard of whatever's already buffered on the port before starting the
background thread (clearing stale data from before the application
connected).

`extractRawDataFrames()` is the framing state machine — called from
**outside this thread** (see "Threading model" below) to pull complete
frames out of whatever's accumulated in the ring buffer since the last
call. It's a two-state scanner (`searching_for_start_of_frame_`/
`searching_for_end_of_frame_`) walking `tail_idx_` forward through the
ring buffer looking for `kStartOfFrameByte` (`0x7E`) then
`kEndOfFrameByte` (`0x7F`); finding a second start-of-frame byte before an
end-of-frame byte discards the in-progress frame and restarts the search
(logged, not treated as fatal). Each complete frame becomes one
`RawDataFrame`. `NUM_BYTES_AVAILABLE_TO_READ` (a macro in `definitions.h`)
is the shared ring-buffer distance calculation used throughout — the
classic "handle wraparound with one ternary" ring-buffer idiom.

`publishData()` is the write direction — used by GUI elements configured
with `publish_to_serial: true` (see the `project_state/` docs'
`GuiElementSettings::id`/`publish_to_serial` fields) to send a value back
out over the same serial link.

### `RawDataFrame` / `BufferedReader` (`raw_data_frame.h/.cpp`)

`RawDataFrame` is a small owning byte-buffer type: given the ring buffer,
its size, and a start/end index pair, it computes the frame length
(handling the wraparound case with two `memcpy`s) and copies **only the
bytes strictly between** the start-of-frame and end-of-frame markers —
**note it does not de-escape anything at construction time**; the copied
bytes may still contain `kEscapeByte` (`0x7D`) sequences. Move-only
(copy disabled), heap-`new[]`-allocated, freed in the destructor.

`BufferedReader` wraps a `RawDataFrame` (borrowed, non-owning `const
uint8_t* const`) and does the **actual de-escaping**, one field at a time,
via three macros in `definitions.h`:
- `READ_ESCAPED_BYTE` — single byte, unescape-if-needed.
- `READ_ESCAPED_UINT16` — two bytes, each independently unescaped (used
  only for `TopicId`/`uint16_t` reads).
- `READ_ESCAPED_DATA` — generic `sizeof(T)`-byte loop, each byte
  independently unescaped; this is what every `Number` subclass's
  constructor uses via `BufferedReader::read<T>(value)`.

Escaping scheme (byte-stuffing, classic HDLC/PPP-style): any occurrence of
`kEscapeByte` in the original data is followed by `(original_byte ^
kEscapeXOR)`; unescaping XORs it back. This is what lets the sender embed
`0x7E`/`0x7F`/`0x7D` byte values inside frame payloads without them being
mistaken for frame boundaries — but it means **frame boundary detection
itself, in `extractRawDataFrames()`, operates on still-escaped bytes**, so
it's relying on the sender never emitting a raw (unescaped)
`0x7E`/`0x7F` inside a payload — check the encoder side (not in this
repository/module) to confirm it escapes payload bytes correctly before
trusting this framing under adversarial or noisy-line conditions.

### `object_types.h`

The `objects::` namespace: `BaseObject` (topic id, `ObjectType`,
timestamp), `Number` (adds `NumberDataType`), and one concrete leaf class
per numeric type (`Float`, `Double`, `Int8`/`16`/`32`/`64`,
`UInt8`/`16`/`32`/`64`) — each a thin wrapper whose constructor reads
exactly one value of its type from a `BufferedReader`. `printObjectData`
is a debug-dump helper (`std::cout`-based) covering all ten numeric types.
Only numeric (`ObjectType::kNumber`) types have concrete classes here
despite `ObjectType` also declaring `kString`/`kArray`/`kFunction`/`kBlob`
— string topics are handled as a special case directly in
`main_window_serial.cpp` (outside this module, reading a raw string rather
than constructing an `objects::` type), and `kArray`/`kFunction`/`kBlob`
have **no implementation anywhere** in this module — they're declared
enum values with no corresponding class or parsing path.

### `definitions.h`

Shared constants and macros for the whole module: `kBufferSize` (8 MiB),
the four framing byte constants, `NUM_BYTES_AVAILABLE_TO_READ` (ring-buffer
distance macro), the `TIOCINQ`/`FIONREAD` portability fallback chain (for
"how many bytes waiting to be read" across platforms that name the ioctl
differently), `NumberDataType`/`ObjectType` enums, `TopicId`
(`uint16_t`) + `kUnknownTopicId` (`0xFFFF`), and the three
`READ_ESCAPED_*` macros described above.

## Threading model

Two threads touch this module's state:
1. **`SerialInterface`'s own background thread**
   (`readSerialDataThreadFunction`) — only ever calls `readSerialData()`,
   which reads from the OS into the ring buffer (`head_idx_` is only
   written here).
2. **Whatever thread calls `extractRawDataFrames()`** — advances
   `tail_idx_`, reads from the ring buffer. Based on `main_window_serial.cpp`
   (see "Integration outside this module" below), this is called from
   `MainWindow`'s own periodic mechanism, not from `SerialInterface`'s
   background thread.

`head_idx_`/`tail_idx_` are plain `size_t`, **not atomics, and not
guarded by any mutex** — this is a single-producer/single-consumer ring
buffer relying on the natural memory-ordering properties of `size_t`
read/write on the target platforms rather than explicit synchronization.
This works in practice on common desktop platforms but is not
standards-guaranteed thread-safe C++ (a data race on `head_idx_`/
`tail_idx_` is technically undefined behavior even though it "usually
works"). Worth keeping in mind if this is ever ported to a platform/
compiler with more aggressive reordering, or if `-fsanitize=thread` is ever
run against it.

## Integration outside this module

- `src/main_application/main_window_serial.cpp` (not part of this module)
  owns the `SerialInterface` instance, calls `extractRawDataFrames()`
  (presumably from a periodic wx timer tick, matching the pattern used for
  the TCP receive queue — see the `communication/` docs), and does the
  actual `ObjectType`/`NumberDataType` dispatch: reads a `TopicId`
  (`readUInt16`), an `ObjectType` byte, a timestamp, then constructs the
  matching `objects::` subclass via `BufferedReader`. Routes by `TopicId`
  to subscribed `PlotPane`s (`pushStreamData`) or scrolling-text GUI
  elements (`pushNewText`) — this is the actual "device data becomes a
  moving line on screen" wiring, and it lives entirely outside this
  module.
- `src/main_application/plot_objects/stream_object_base/` and
  `stream_objects/{plot2d,scatter,stairs}/` (see the `plot_objects/` docs)
  are the rendering-side consumers of the `objects::BaseObject` values this
  module produces — `plot_objects/stream_objects/conversion_function.h`'s
  `getFloatValue()` is the one place in the `plot_objects` module that
  understands this module's `NumberDataType` enum.
- `src/main_application/project_state/`'s `SubscribedStreamSettings`/
  `GuiElementSettings::publish_to_serial`/`id` fields (see that module's
  docs) configure which `TopicId`s a given plot pane or GUI element
  subscribes to or publishes on.

## Known rough edges

- ~~**`serial_port.h`/`.cpp` unconditionally include macOS-only headers**~~
  **FIXED as of the `39a6dc9e` pull** (previously documented at `23e65b7a`):
  `<mach/clock.h>`/`<mach/mach.h>` in both `serial_port.h` and
  `serial_port.cpp` are now correctly wrapped in
  `#ifdef PLATFORM_APPLE_M ... #endif`. This was the most significant
  portability issue found in this module (and one of two confirmed
  instances of the same pattern across `main_application` — see
  `src/main_application/docs/ARCHITECTURE.md` for the other one, in
  `platform_paths.cpp`, also now fixed). No action needed here anymore;
  left in this document as a record of what was wrong and confirmation
  it's resolved, in case the same pattern recurs elsewhere.
- **`ObjectType::kArray`/`kFunction`/`kBlob` are declared but have no
  parsing path anywhere** in this module (only `kNumber` has concrete
  `objects::` classes; `kString` is special-cased entirely outside this
  module in `main_window_serial.cpp`). Sending a frame tagged with one of
  these three types would need new code before it could be handled.
- **No synchronization on the ring buffer's `head_idx_`/`tail_idx_`**
  shared between the reader thread and whichever thread calls
  `extractRawDataFrames()` — see "Threading model" above.
- **`readSerialData()`'s wraparound-leftover-read path** has a
  `std::cout`-logged error case ("Buffer data that was leftover did not fit
  into buffer_!") that is detected but not actually handled — the
  offending read still proceeds (`num_read_bytes = serial_port_.readBytes(...)`
  runs regardless of the logged condition), so this looks like a
  detect-only diagnostic rather than a real guard against buffer overrun.
- **`SerialInterface`'s constructor comment**: `// TODO: How to detect if
  head_idx_ catches up to tail_idx_?` — there is no explicit
  buffer-full/overrun detection; a sufficiently fast sender combined with a
  slow consumer could silently overwrite unread data in the ring buffer.
- Frame-boundary bytes (`0x7E`/`0x7F`) are matched **before** unescaping
  (see `RawDataFrame`/`BufferedReader` section above) — this is standard
  for byte-stuffing schemes but means the framing's correctness depends
  entirely on the sender-side encoder escaping payload bytes correctly;
  nothing in this module can verify that independently.
