# `src/main_application/misc/` — architecture overview

> **Note (post-`39a6dc9e` pull):** `rgb_triplet.h`'s
> `#include "duoplot/enumerations.h"`/`"duoplot/logging.h"` now read
> `#include "lumos/plotting/enumerations.h"`/`"lumos/logging.h"` — part of
> the repo-wide swap from `src/interfaces/cpp/duoplot/` to the external
> `third_party/LumosAlgo` submodule, see
> `src/main_application/docs/ARCHITECTURE.md`. No other change in this
> module.

A small grab-bag utility module: number-to-string formatting for axis tick
labels, and a generic RGB color triplet type. Three files, ~310 lines total,
two unrelated concerns that happen to share a directory.

**GUI-toolkit note (relevant to the planned wxWidgets → Qt migration): no
wxWidgets usage anywhere in this module** — no `wx*` includes or types. Both
`misc.h`'s functions and `RGBTriplet<T>` are pure C++/STL and are consumed
widely across the codebase (`RGBTripletf` alone is used in ~43 files) purely
as plain data/algorithms — nothing here needs to change for a Qt port.

## `misc.h` / `number_formatting.cpp` — tick-label number formatting

Despite the generic `misc.h` name, every declared function
(`getIntExponent`, `getBase`, `formatNumber`, `formatNumberInternal`,
`toStringWithNumDecimalPlaces`, `zeroLStrip`, `zeroRStrip`,
`stripNumberFromLastZeros`) exists to answer one question: **how do axis
grid-tick numbers get rendered as compact, non-redundant strings?**
`formatNumber(num, n)` is the public entry point — consumed directly by
`drawGridNumbers()` in `src/main_application/axes/grid_numbers.cpp` (see
that module's docs) to render each tick value.

Pipeline: `formatNumber` → `formatNumberInternal` (choose fixed vs.
scientific notation and initial decimal precision) → `stripNumberFromLastZeros`
(trim redundant trailing/leading zeros and clean up the exponent) →
`zeroLStrip`/`zeroRStrip` (the low-level trimming primitives).

- **`getIntExponent(d)`**: `floor(log10(|d|))`, or `0` for `d == 0`. Has a
  `// TODO: Change to std::int32_t` — currently returns a plain `int`.
- **`getBase(num)`**: mantissa such that `num == getBase(num) * 10^getIntExponent(num)`.
- **`formatNumberInternal(num, n)`**: chooses the output form by exponent
  magnitude — scientific notation (`mantissa e[+]exponent`, precision `n`)
  if `exponent > 4` or `exponent < -2`; otherwise fixed-point with precision
  `5 - exponent` (i.e. always ~5 significant digits regardless of
  magnitude, in the non-scientific range). Special-cases exact `0.0` →
  `"0.0"`.
- **`zeroLStrip`/`zeroRStrip`**: strip leading/trailing `'0'` characters,
  each always leaving at least one character behind (won't reduce a string
  to empty).
- **`stripNumberFromLastZeros(input_num)`**: the heavyweight cleanup pass —
  parses the string into sign / integer part / fractional part / exponent
  by locating `.` and `e`, zero-strips each piece independently, then
  reassembles. Throws `std::runtime_error` on an empty string, a
  single `"."`, or an exponent that parses to an empty string after
  stripping the sign. Guarantees a fractional part is always present in the
  output (defaults to `.0` if there wasn't one) and normalizes a zero
  exponent (`e+0`/`e-0`/`e0`) away entirely.

This is straightforward string-manipulation code, but the parsing logic
(especially `stripNumberFromLastZeros`) has several interacting edge cases
(sign handling, presence/absence of `.` and `e` in all four combinations,
zero exponent collapsing) — read it in full rather than assuming behavior
from the function name alone if you need to modify tick-label formatting.
`getBaseAsString`/`getIntExponentAsString` exist only as a commented-out
block at the top of the file — dead, not currently reachable.

## `rgb_triplet.h` — generic color type

`RGBTriplet<T>` is a header-only template: three public fields (`red`,
`green`, `blue`), a default constructor, a three-value constructor, and a
constructor that unpacks a 24-bit hex color code (`0xRRGGBB`) into
normalized `[0,1]` floats — **note this hex-unpacking constructor always
produces `float` math internally regardless of `T`**, so instantiating
`RGBTriplet<uint8_t>` from a hex code would silently truncate/misbehave;
in practice only `RGBTripletf` (`= RGBTriplet<float>`) is used anywhere in
the codebase, so this has not been an issue, but it's a latent trap for a
future `RGBTriplet<uint8_t>` use. Also provides `operator==`/`operator!=`
and a generic `operator<<` for logging/debugging (formats as
`"{ r, g, b }"` via `std::to_string`, which does **not** control decimal
precision — printed colors will show full `float` precision noise).

`RGBTripletf` is the type actually threaded throughout the rendering code
(`AxesSettings`, `PlotPaneSettings`, `LegendProperties`, shader color
uniforms, etc.) — this module doesn't consume its own type anywhere; it's
purely a shared leaf dependency for everything under `axes/` and the shader
layer.

## Known rough edges

- `getIntExponent`'s `int` return type is flagged by its own `// TODO`
  comment as should-be `std::int32_t` — purely cosmetic/consistency, not a
  behavioral issue at current usage scale.
- Dead code: `getBaseAsString`/`getIntExponentAsString`, fully commented out
  in `number_formatting.cpp`.
- `RGBTriplet`'s hex-code constructor hardcodes `float` division regardless
  of the template parameter `T` — safe today only because `RGBTripletf` is
  the sole instantiation in use.
