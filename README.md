# NF_Tx_Core_Tester

CLI tester for **Nf_Tx_Core** (the C++20 core behind the CDCsvParser
Android app). Its job is to prove core behavior *before* it is synced into
the app — the ASan pre-gate for core changes, not a product of its own.

One binary, two ways in:

- **argv CLI** — dumps the core's **daily wallet series** (`initWithData`
  -> `getDailyWalletSeries` — the same calls the Android app makes) for a
  transaction CSV. The ground truth for the balance side of the app's
  charts, without re-implementing trade semantics outside the core.
- **Interactive prompt** (no args) — the classic tester loop: mode choice,
  price source, per-symbol asset audit, save/load round-trip.

## Build

Built and run against the **canonical `Nf_Tx_Core` checkout** (sibling dir,
default `../Nf_Tx_Core`): `StaticPrices.h` is single-sourced from its
`Price/` dir (no local copy - it lives in the core repo since 2026-10-18),
and at runtime `libNF_Tx_Core.so` is dlopened from its `cmake-build-debug`
(see the path in `main.cpp`).

```
cmake -B cmake-build-debug && cmake --build cmake-build-debug -j
```

If your core checkout is renamed/elsewhere: `cmake -B cmake-build-debug
-DNF_TX_CORE_DIR=/path/to/core` (the runtime lib path in `main.cpp` or the
CLI's first argument is adjusted separately).

After any change in the core repo, rebuild its `cmake-build-debug` too —
the dlsym'd C API is declared by hand and must match the core headers
exactly (a stale 3-arg `initWithData` type vs the core's 4th parse-budget
arg bit us in 2026-10-18; nothing surfaced until a rebuild).

## Usage

```
NF_Tx_Core_Tester [path/to/libNF_Tx_Core.so] <csv-file> [mode] [out-file] [date] [--extended]
```

- `so-path` — optional; detected as the first argument that is an existing
  file with a `.so`/`.dll`/`.dylib` in its name. Without one, a candidate
  chain is tried in order — `../../Nf_Tx_Core/cmake-build-debug/…`,
  `../Nf_Tx_Core/cmake-build-debug/…` (the standard sibling checkout),
  `./libNF_Tx_Core.so` — and the first that exists wins
- `csv-file` — the transaction file (e.g. a BlockPit export)
- `mode` — `0=CDC 1=Card 2=Default 3=Custom 4=Kraken 5=BlockPit` (default 5)
- `out-file` — default `wallet_series.txt`
- `date` — optional `YYYY-MM-DD`: writes a per-symbol snapshot
  (`CUR;BALANCE;BONUS`, last series day at/before that date) instead of the
  full series
- `--extended` — also prints the core's wallet/transaction strings to the
  console (the series file stays clean either way)
- `--help` / `-h` — usage

Output line format (core shape): `CUR;YYYY-MM-DD;BALANCE;BONUS`, ascending
per symbol. The core's own log lines go to the console; the output file is
clean.

```
./cmake-build-debug/NF_Tx_Core_Tester /path/to/trans.csv 5
```

Interactive prompt flow (no args):

1. Loads the shared library `libNF_Tx_Core.so` from the core checkout.
2. Choose the mode: **1=CDC, 2=Card, 3=Custom** (own file, then crypto or
card), **4=Default, 5=Kraken, 6=BlockPit**.
3. Optionally enable extended output.
4. Pick the price source (1 = the core's static fallback table — offline).
5. The data file resolves automatically: a real export at the repo root
   (same file name) is preferred, otherwise the committed `fixtures/`
   (see below).
6. Initializes, sets the data, calculates balances, optionally saves/loads —
   wallets, cards and the daily series are displayed.

## CTest

`ctest --test-dir cmake-build-debug` runs the **core's committed fixtures**
(`${NF_TX_CORE_DIR}/Tests/fixtures/`) through the argv CLI in no-price
modes (`cdc_crypto`→0, `cdc_card`→1, `kraken_trades`→4,
`blockpit_export`→5 — offline; the CI network path is a 429 minefield, so
price modes stay out of ctest). Every test asserts the parse succeeded; the
three asset fixtures additionally assert a non-empty series (the #44-class
regression — parse OK, wrong/empty series — fails there), while
`cdc_card` asserts the parse only: a card file legitimately yields an EMPTY
series (card = spending, no held assets). Tests load `NF_TX_CORE_SO`
(default: the core's local debug build; the core's CI points it at its
release `build-shared` lib instead). The wallet **assertions** themselves live in the core's own
suite; ctest asserts the parse/series lines through the app's exact call
path.

The core repo's CI runs these automatically: after its plain Release
`build-shared` step, it fetches this repo (`@main`) and runs the same
`ctest` against `build-shared/libNF_Tx_Core.so` (see
`Nf_Tx_Core/.github/workflows/ci.yml`).

## Test data (`fixtures/`)

`fixtures/` holds a **reduced, anonymized subset** of the real exports
(same file names as the modes above), committed so CI / fresh checkouts
can run the tester end-to-end. The full exports stay local-only (see
`.gitignore`); the interactive modes prefer a real file at the repo root
and fall back to `fixtures/` when it is absent.

How the fixtures differ from the real exports:
- reduced: ~200 of ~2440 BlockPit lines / ~100 of ~247 CRO lines - a
deterministic time-strided sample that keeps every transaction
label/kind (including the one-line ones); one malformed real record
(a quoted comment with an embedded newline, which the line-based core
parser skips anyway) is dropped
- `kraken_trades.csv` (Kraken mode) is the synthetic fixture verbatim
from the core repo's `Tests/fixtures/` - an intentional malformed line
is in it there (the core suite asserts 2 parsed / 1 failed on it)
- anonymized: transaction ids blanked, personal integration labels
renamed to `Personal Depot n` / `Cold Storage n`, one raw wallet address
gone, all amounts scaled by a fixed factor (x0.87, same decimal
precision), all dates shifted back by one year
- line endings preserved (BlockPit CRLF, CRO/card LF)

## License

Shield: [![CC BY-NC-ND 4.0][cc-by-nc-nd-shield]][cc-by-nc-nd]

This work is licensed under a
[Creative Commons Attribution Non Commercial No Derivatives 4.0 International License][cc-by-nc-nd].

[![CC BY-NC-ND 4.0][cc-by-nc-nd-image]][cc-by-nc-nd]

[cc-by-nc-nd]: http://creativecommons.org/licenses/by-nc-nd/4.0/
[cc-by-nc-nd-image]: https://licensebuttons.net/l/by-nc-nd/4.0/88x31.png
[cc-by-nc-nd-shield]: https://img.shields.io/badge/License-CC%20BY--NC--ND%204.0-lightgrey.svg
