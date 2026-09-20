# `src/main_application/communication/` — architecture overview

> **Note (post-`39a6dc9e` pull):** `main_application` now depends on the
> external `third_party/LumosAlgo` submodule instead of
> `src/interfaces/cpp/duoplot/` for its wire-protocol types — every
> `duoplot::internal::`/`#include "duoplot/..."` reference below (e.g.
> `duoplot::internal::CommunicationHeader`, `kTcpPortNum`, `kMagicNumber`)
> now reads `lumos::internal::`/`#include "lumos/plotting/..."` in the
> actual source. The protocol itself is unchanged (verified: identical
> enum values, port numbers, and magic number) — see
> `src/main_application/docs/ARCHITECTURE.md`'s dedicated section for what
> this rename is and the two-copies risk it creates. No other behavioral
> changes were found in this module's two files.

This is the small, self-contained module that terminates the client→server
TCP connection on port 9755 (the "plot channel" — see
`src/interfaces/cpp/duoplot/docs/PROTOCOL.md` for the wire format this
module parses) and hands parsed messages off to `MainWindow`. It is four
files, ~300 lines total, and has exactly two classes.

**GUI-toolkit note (relevant to the planned wxWidgets → Qt migration): this
module contains no wxWidgets code whatsoever** — no `wx*` includes, no
`wxString`, no wx event types. It is written entirely against POSIX sockets
(`<sys/socket.h>`, `<netinet/in.h>`, etc.) and the client-interface library's
own types (`duoplot::internal::CommunicationHeader`, `UInt8ArrayView`, ...).
It should be reusable as-is under a Qt-based rewrite. The actual
wx-toolkit coupling in this data path lives **outside** this module — see
"Where wxWidgets enters the picture" below — and that's the part a Qt
migration needs to replace, not anything here.

## The two classes

### `DataReceiver` (`data_receiver.h/.cpp`)

Owns exactly one listening TCP socket, bound and put into `listen()` state
in the constructor (port = `duoplot::internal::kTcpPortNum`, i.e. 9755;
`SO_REUSEADDR` set so a restarted `main_application` can rebind immediately
after a previous instance exits). Has one public method:

```cpp
ReceivedData receiveAndGetDataFromTcp();
```

This is a **blocking** call meant to be run in a loop on a dedicated thread
(see below — that thread is owned by `MainWindow`, not by this class).
Behavior:
1. If not currently connected, blocks on `accept()` for the next client
   connection. **Only one client connection is serviced at a time** — this
   class calls `accept()` exactly once per connection lifetime; a second
   client attempting to connect while one is already active just sits in the
   OS-level backlog (`listen(fd, 5)`) until the first disconnects.
2. Reads the 8-byte length prefix. A `read()` returning 0 bytes, or a
   length prefix of 0, is treated as a clean disconnect: the connection is
   closed, `is_connected_` reset to `false`, and an **empty** `ReceivedData{}`
   (default-constructed, not exception-raising) is returned — callers must
   check for this rather than assuming every call yields a usable message.
3. Otherwise reads exactly that many more bytes in a loop (handling short
   reads), validates the 8-byte magic number at the expected offset
   (`+1` past the endianness byte — see the interfaces protocol doc), and
   throws `std::runtime_error` if it doesn't match `kMagicNumber`.
4. Calls `received_data.parseHeader()` (see below) and returns the fully
   parsed `ReceivedData` by value (move-constructed — see `ReceivedData`'s
   move semantics).

A mid-read disconnect (`read()` returns 0 partway through the payload) is
also treated as a clean disconnect and returns an empty `ReceivedData{}`,
the same as an early disconnect — not distinguished from case 2 by the
caller's return value.

### `ReceivedData` (`received_data.h/.cpp`)

An owning, move-only buffer plus a lazily-populated parsed view of it.
- Default constructor: an empty/invalid instance (used as the disconnect
  sentinel above, and as the moved-from state).
- `ReceivedData(size_t size_to_allocate)`: allocates `raw_data_` via
  `new uint8_t[...]`; this is what `DataReceiver` constructs before filling
  it with the socket read.
- Move-only (copy constructor/assignment explicitly `= delete`d); moves
  null out the source's pointers so the destructor's `delete[]` is safe on
  both sides.
- `parseHeader()`: wraps `raw_data_` in a `UInt8ArrayView` and constructs a
  `duoplot::internal::CommunicationHeader` from it (the same deserializing
  constructor documented in the interfaces protocol doc), extracts the
  `Function` enum, and computes `payload_data_` as a **non-owning** pointer
  into `raw_data_` starting at `hdr_.numBytes() + kHeaderDataStartOffset`
  (17 — the fixed envelope prefix size). Throws `std::runtime_error` if that
  offset would exceed the buffer size (a truncated/malformed message).
  If there's no payload beyond the header, `payload_data_` is left `nullptr`.
- Accessors: `getFunction()`, `getCommunicationHeader()`,
  `payloadData()` (may be `nullptr`), `rawData()` (the full buffer,
  including the header), `size()` — note `size()` returns the **payload**
  byte count (`num_data_bytes_`), not the total buffer size; use `rawData()`
  together with knowledge of `hdr.numBytes()` if you need the whole frame.

## Where this fits in the pipeline

`DataReceiver`/`ReceivedData` only get you as far as "one fully-parsed
message, still sitting on a background thread." Turning that into rendered
pixels is entirely outside this module:

- `MainWindow` (`src/main_application/main_window.h/.cpp`,
  `main_window_receive.cpp`) owns the `DataReceiver` instance
  (`data_receiver_`) and spawns a plain `std::thread` at startup
  (`tcp_receive_thread_ = new std::thread(&MainWindow::tcpReceiveThreadFunction, this)`)
  whose body just loops calling `data_receiver_.receiveAndGetDataFromTcp()`
  and forwarding each result to `manageReceivedData()`.
- `manageReceivedData()`/`addActionToQueue()` (in `main_window_receive.cpp`)
  do the routing/dispatch (by `Function`, by target element name) described
  in `src/interfaces/cpp/duoplot/docs/PROTOCOL.md` and push results into a
  `std::mutex`-guarded (`receive_mtx_`) per-element `queued_data_` map — this
  is the hand-off point from the background receive thread to the GUI
  thread.

### Where wxWidgets enters the picture

Nothing in `communication/` touches wx. The wx dependency is entirely in
`MainWindow`'s consumption side: a `wxTimer receive_timer_`, bound to
`MainWindow::OnReceiveTimer` (a `wxEVT_TIMER` handler), fires periodically
on the wx GUI thread and drains `queued_data_` into each `PlotPane`. **This
timer-plus-mutex-guarded-queue pattern is the actual cross-thread
marshalling mechanism that a Qt port needs to replace** (most naturally with
a queued `QMetaObject::invokeMethod`/signal-slot connection or a `QTimer`
doing the equivalent poll) — `DataReceiver`/`ReceivedData` themselves need no
changes for that migration.

## Known rough edges

- `DataReceiver`'s destructor is empty — the listening socket
  (`tcp_sockfd_`) is never explicitly `close()`d, only the per-connection
  socket (`tcp_connfd_`) is closed on disconnect. In practice this relies on
  process exit to release the listening socket; it is not cleaned up if a
  `DataReceiver` were ever destroyed and recreated within a running process.
- `ReceivedData::parseHeader()` can throw (`std::runtime_error`) on a
  malformed offset, but its only caller,
  `DataReceiver::receiveAndGetDataFromTcp()`, does not catch it — since that
  function runs on `MainWindow`'s dedicated `std::thread` with no
  surrounding `try`/`catch`, an uncaught exception there calls
  `std::terminate()` rather than failing gracefully. A malformed or
  truncated message from a misbehaving client can currently crash the whole
  application, not just drop the connection.
- Single-client-at-a-time design (see `DataReceiver` above): there is no
  provision for multiple simultaneous plotting clients sharing one
  `main_application` instance — a second client's connection attempt just
  waits in the kernel backlog until the first disconnects.
