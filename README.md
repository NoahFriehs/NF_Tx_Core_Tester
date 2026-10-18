# NF_Tx_Core_Tester

This C++ program serves as a tester for the NF_TX_Core library, which manages financial transactions and balances. The tester demonstrates the functionality of the library through various operations.

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
-DNF_TX_CORE_DIR=/path/to/core` (the runtime lib path in `main.cpp` or
`NF_Tx_Core_WalletDump`'s first argument is adjusted separately).

## Usage

1. Load the shared library `libNF_Tx_Core.so`, from the NF_TX_Core project.
2. Choose the mode: CDC, Card, Custom, or Default.
3. Optionally enable extended output.
4. Input the necessary data file based on the chosen mode.
5. Perform operations such as initialization, setting data, calculating balances, saving, and loading.
6. Retrieve and display results.


## Wallet dump tool (`NF_Tx_Core_WalletDump`)

A small CLI tool that dumps the core's **daily wallet series**
(`initWithData` -> `getDailyWalletSeries` — the same calls the Android
app makes) for a transaction CSV. It is the ground truth for the balance
side of the app's charts, without re-implementing trade semantics outside
the core.

```
NF_Tx_Core_WalletDump [path/to/libNF_Tx_Core.so] <csv-file> [mode] [out-file] [date]
```

- `so-path` — optional; default `../../Nf_Tx_Core/cmake-build-debug/libNF_Tx_Core.so`
- `mode` — `0=CDC 1=Card 2=Default 3=Custom 4=Kraken 5=BlockPit` (default 5)
- `out-file` — default `wallet_series.txt`
- `date` — optional `YYYY-MM-DD`: writes a per-symbol snapshot
  (`CUR;BALANCE;BONUS`, last series day at/before that date) instead of the
  full series

Output line format (core shape): `CUR;YYYY-MM-DD;BALANCE;BONUS`, ascending
per symbol. The core's own log lines go to the console; the output file is
clean.

Build and run:

```
cmake -B cmake-build-debug && cmake --build cmake-build-debug -j
./cmake-build-debug/NF_Tx_Core_WalletDump /path/to/trans.csv 5
```

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


