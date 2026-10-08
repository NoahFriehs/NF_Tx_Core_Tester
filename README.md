# NF_Tx_Core_Tester

This C++ program serves as a tester for the NF_TX_Core library, which manages financial transactions and balances. The tester demonstrates the functionality of the library through various operations.

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

## License

Shield: [![CC BY-NC-ND 4.0][cc-by-nc-nd-shield]][cc-by-nc-nd]

This work is licensed under a
[Creative Commons Attribution Non Commercial No Derivatives 4.0 International License][cc-by-nc-nd].

[![CC BY-NC-ND 4.0][cc-by-nc-nd-image]][cc-by-nc-nd]

[cc-by-nc-nd]: http://creativecommons.org/licenses/by-nc-nd/4.0/
[cc-by-nc-nd-image]: https://licensebuttons.net/l/by-nc-nd/4.0/88x31.png
[cc-by-nc-nd-shield]: https://img.shields.io/badge/License-CC%20BY--NC--ND%204.0-lightgrey.svg


