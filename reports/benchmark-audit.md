# Historical benchmark audit

No raw timing logs, profiler captures, compiler command lines, CPU model, or complete runtime environment were committed with the coursework. The values below are therefore historical claims with distinct provenance, not reproduced measurements. The portfolio does not reconcile conflicts by choosing whichever number best supports a narrative.

## A. Formal report-table results

These are transcribed into `results/report-table-benchmarks.csv` with `evidence_type=report-table-result`.

### Tables 1–3: 8 threads

| Report table | Geometry | Algorithm | Static (s) | Dynamic (s) | Guided (s) |
|---|---|---:|---:|---:|---:|
| Table 1 | Standard | Naïve | 17.26 | 18.82 | 17.95 |
| Table 2 | Wraparound | Naïve | 43.68 | 18.82 | 17.95 |
| Table 3 | Not stated in the table | Optimized | 18.58 | 14.01 | 15.12 |

The formal Table 3 values make Dynamic the fastest listed optimized 8-thread configuration. The table does not identify whether those optimized timings are standard, wraparound, or another aggregation, so the machine-readable geometry is `not-specified`.

### Table 4: 4 vs 8 threads

| Configuration | 4 threads (s) | 8 threads (s) | Printed speedup | Recalculated 4T/8T |
|---|---:|---:|---:|---:|
| Standard naïve Static | 21.17 | 17.26 | 1.23× | 1.2265× |
| Wraparound naïve Static | 65.85 | 43.68 | 1.51× | 1.5076× |
| Optimized Dynamic (geometry not stated) | 27.42 | 14.01 | 1.96× | 1.9572× |

The printed speedups agree with the ratios after rounding.

## B. Conflicting report prose

1. **Wraparound naïve timing.** Table 2 lists Static `43.68 s`, Dynamic `18.82 s`, and Guided `17.95 s`. The adjacent prose says Guided was approximately `3.4 s` (about `8%`) faster than Static. The listed difference is `25.73 s`, or about `58.9%` of the Static time. The prose and table cannot both describe the same measurements; neither is silently corrected.
2. **Table 2 “Improvement for Naive” column.** The printed values are `2.53x`, `2.56x`, and `2.24x`. Only `43.68 / 17.26 ≈ 2.53` is consistent with the paired standard/wraparound table times. The Dynamic and Guided table times are identical across Tables 1 and 2, so their corresponding time ratios would be `1.00x`, not `2.56x` and `2.24x`.
3. **Optimized timing.** Table 3 lists `18.58 s`, `14.01 s`, and `15.12 s`, while the adjacent prose calls the Static result `21.96 s` and the Dynamic result `16.65 s`. No evidence identifies which revision is intended.
4. **Optimized percentage.** Table 3 prints Guided improvement as `12.48%`; using its own `17.26 s` naïve reference and `15.12 s` Guided time gives approximately `12.40%`. This small discrepancy is retained rather than normalized.
5. **Dynamic vs Guided.** Table 3 makes Dynamic (`14.01 s`) faster than Guided (`15.12 s`). The conclusion nevertheless describes Guided as empirically superior for decreasing-workload loops. The evidence supports scheduling sensitivity, but not an unconditional claim that Guided won this reported experiment.
6. **Unprofiled explanations.** The prose attributes behavior to branch misprediction, memory bandwidth saturation, and cache contention. No performance-counter or profiler evidence is committed, so these remain the report author’s interpretations.

## C. Historical plotting-script values

`archive/historical-scripts/plot_performance.py` embeds a third experiment revision:

| Geometry | Algorithm | Static (s) | Dynamic (s) | Guided (s) |
|---|---|---:|---:|---:|
| Standard | Naïve | 16.56 | 19.24 | 21.1837 |
| Standard | Optimized | 23.24 | 18.13 | 18.70 |
| Wraparound | Naïve | 42.99 | 43.08 | 43.04 |
| Wraparound | Optimized | 42.01 | 29.39 | 30.52 |

The script supplies no source log or hardware metadata. Its numbers are preserved as **historical plotting-script values** and are not mixed into the report-table CSV or active figures. The same script hard-codes rounded distance means (`0.00158`, `1.06909`, and `0.70571`); active figures instead read recalculated summaries.

## D. New portfolio-rebuild measurements

No new performance benchmark is committed. Local and CI runs execute small correctness/smoke workloads only. The CLI labels its elapsed field `elapsed_seconds_local_run` so ad hoc execution time is not confused with historical evidence. Any future benchmark should record point count, seed/input hash, CPU, memory, OS, compiler, OpenMP runtime, compiler flags, thread affinity, thread count, repetitions, and summary method under a separate `new-local-benchmark` evidence type.

## Evidence-aligned interpretation

- Static scheduling performed well for the uniform naïve loop in the reported standard-geometry experiment.
- Dynamic was the fastest optimized schedule in the formal 8-thread table.
- The formal table reports a `1.96×` 4-to-8-thread speedup for optimized Dynamic.
- Halving distance evaluations changes a rectangular loop into a triangular one. Load balance, thread-local storage, merge work, and memory traffic can therefore prevent a proportional wall-time reduction.
- None of these historical times should be treated as hardware-independent performance guarantees.
