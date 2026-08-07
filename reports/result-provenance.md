# Result provenance

This project keeps scientific results, historical performance claims, and new validation evidence separate.

| Evidence class | Location | Status and permitted use |
|---|---|---|
| 1. Original source files | `archive/original-code/` | Unmodified file contents from remote `main` commit `16ac2603…`; filenames with the Korean “copy” suffix were normalized during archival. Use to audit implementation history, not as the supported build. |
| 2. Original coursework PDF | `reports/original-report.pdf` | Original bytes preserved unchanged (SHA-256 `23a8dd32dce1ad9ec580ade06696a94022746b304aa30a7759c1dd72bad71f71`). It contains the submitted narrative, tables, and figures. |
| 3. Raw distance CSVs | `results/raw/*.csv` | Four committed primary evidence files, each containing 100,000 numeric observations. The rebuild never regenerates or overwrites them. |
| 4. Formal report-table benchmarks | `results/report-table-benchmarks.csv` | Manual transcription of Tables 1–4 with `evidence_type=report-table-result`. These are historical table values, not raw timing logs or reproduced benchmarks. |
| 5. Contradictory report prose | `reports/benchmark-audit.md` | Conflicts are quoted by value and kept unresolved because the repository contains no evidence that identifies the intended revision. |
| 6. Historical plotting-script values | `archive/historical-scripts/plot_performance.py` and `reports/benchmark-audit.md` | A separate hard-coded timing revision. Preserved for audit only; excluded from canonical CSVs and active figures. |
| 7. New correctness and smoke results | `tests/`, CTest, and CI logs | Deterministic small-data validation of distance functions, parsing, seeded generation, schedule/algorithm equality, and 1/2/4-thread equivalence. These runs establish correctness, not historical performance. |
| 8. New local benchmarks | None committed | The rebuild intentionally records no new benchmark series. Ad hoc CLI times are labeled local runs and must not replace coursework results. |

## Derived data chain

`scripts/analyze_distances.py` reads the four raw CSVs, validates each row, computes streaming descriptive statistics, records source SHA-256 hashes, and writes `results/distance-summary.csv` with `evidence_type=derived-from-committed-raw-csv`. Population standard deviation is used (`ddof=0`).

`scripts/generate_assets.py` reads the raw CSVs, the derived distance summary, and the formal report-table CSV. It writes deterministic SVGs and embeds an explicit provenance footer in every figure. `--check` mode regenerates bytes in memory and fails if committed outputs differ.

## What is not claimed

- The historical 100,000-point timings were not reproduced.
- No raw timing logs were recovered.
- Report prose/table conflicts were not resolved by inference.
- Hardware-counter explanations were not experimentally verified.
- The point set behind the four raw distance arrays is not committed, so the arrays can be analyzed but cannot be regenerated exactly from coordinates.
