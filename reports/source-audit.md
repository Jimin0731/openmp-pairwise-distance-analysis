# Original source audit

This audit records every C++ source file present on `main` at commit `16ac2603e83339a099a9c4431b998a8524299da9`. Files were inspected individually before the portfolio reorganization. Their contents are preserved under `archive/original-code/`; only the filenames containing the Korean suffix “복사본” (“copy”) were normalized to portable ASCII archive names.

All six historical programs model geometric pairwise distance in a unit square. None implements gravitational dynamics. Every program uses a time-derived `std::mt19937` seed, accepts either generated points or a machine-specific CSV path, computes nearest/furthest arrays, and writes result CSVs into the working directory.

| Original path on `main` | Archived path | Purpose and algorithms | OpenMP schedule | Geometry | Key distinctions | Canonical contribution |
|---|---|---|---|---|---|---|
| `main1.cpp` | `archive/original-code/main1.cpp` | Comprehensive interactive benchmark driver. Implements naïve all-pairs and symmetry-optimized `j > i` analysis, each in three scheduling variants. | Static, Dynamic, Guided for both algorithms | Standard and wraparound | Only source containing all 12 algorithm/schedule/geometry combinations. Optimized variants allocate per-thread minima/maxima and merge them after the triangular loop. Uses `100.0` as the minimum sentinel and a hard-coded local input path. | **Primary canonical basis.** Its complete experiment matrix, `Point{x,y}`, distance functions, reductions, per-thread arrays, and merge design were retained behind a tested library/CLI API. |
| `main_optimization 복사본.cpp` | `archive/original-code/main_optimization-copy.cpp` | Naïve all-pairs baseline plus symmetry-optimized Static, Dynamic, and Guided implementations. | Naïve Static; optimized Static/Dynamic/Guided | Standard and wraparound | Uses `numeric_limits<double>::max()` rather than `100.0`; contains all optimized schedules but not all naïve schedules. Runs every optimized configuration sequentially. | Informed sentinel handling and independently corroborated the optimized thread-local/merge structure. |
| `main_static 복사본.cpp` | `archive/original-code/main_static-copy.cpp` | Single naïve all-pairs experiment. | Static | Standard and wraparound | Writes fixed-six-decimal CSV output; hard-coded local path; time-derived seed. | Corroborated the original static naïve baseline. No unique active code retained. |
| `main_static.cpp` | `archive/original-code/main_static.cpp` | Single naïve all-pairs experiment. | Static | Standard and wraparound | Byte-for-byte identical to `main_static 복사본.cpp` in the source commit (same Git blob `caf1aeb4…`). | Preserved as a distinct uploaded path; no unique active code retained. |
| `main_dynamic 복사본.cpp` | `archive/original-code/main_dynamic-copy.cpp` | Single naïve all-pairs experiment. | Dynamic | Standard and wraparound | Differs from the static copy in its schedule, initializes maximum distance to `-1.0`, and contains output-label typos. | Corroborated the original Dynamic naïve variant. No unique active code retained. |
| `main_guided 복사본.cpp` | `archive/original-code/main_guided-copy.cpp` | Single naïve all-pairs experiment. | Guided | Standard and wraparound | Differs from the static copy in its schedule, initializes maximum distance to `-1.0`, and omits expected values from the two-point console check. | Corroborated the original Guided naïve variant. No unique active code retained. |

## Shared implementation characteristics

- Standard distance is ordinary Euclidean distance, `sqrt(dx² + dy²)`.
- Wraparound distance replaces each coordinate separation above `0.5` with `1 - separation` before applying Euclidean distance.
- The naïve rectangular loop evaluates approximately `N(N-1)` ordered pairs. Its outer iterations have approximately equal work and use an OpenMP sum reduction for aggregate nearest/furthest means.
- The optimized triangular loop evaluates approximately `N(N-1)/2` unordered pairs. Because one distance updates both endpoints, it uses `N × max_threads` thread-local minima and maxima, then merges all thread-local values per point.
- The optimized outer-loop work decreases with `i`. Contiguous static assignment can therefore load the early-index threads more heavily than later ones.

## Historical limitations addressed in the canonical implementation

- The active program accepts a user-provided `--input` path; the personal absolute path remains only in the archive.
- Generated datasets accept an explicit seed and default to a documented deterministic seed.
- CSV parsing now reports malformed/out-of-range input instead of silently skipping rows.
- Inputs with fewer than two points are rejected because nearest-neighbour distance is undefined.
- The canonical code uses infinities instead of a geometry-specific magic minimum sentinel.
- Historical sources could overwrite the committed evidence files when run from the repository root. The canonical CLI writes only when an explicit `--output-prefix` is supplied.
- Correctness tests compare every scheduling and algorithm variant for both geometries and multiple OpenMP thread counts.
