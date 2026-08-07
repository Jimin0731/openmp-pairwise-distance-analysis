#!/usr/bin/env python3
"""Validate committed distance CSVs and derive deterministic summary statistics."""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import math
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RAW_DIRECTORY = ROOT / "results" / "raw"
OUTPUT_PATH = ROOT / "results" / "distance-summary.csv"
EVIDENCE_TYPE = "derived-from-committed-raw-csv"

SOURCES = (
    ("standard", "nearest", "standard_nearest.csv"),
    ("wraparound", "nearest", "wrap_nearest.csv"),
    ("standard", "furthest", "standard_furthest.csv"),
    ("wraparound", "furthest", "wrap_furthest.csv"),
)


@dataclass(frozen=True)
class Statistics:
    row_count: int
    valid_count: int
    missing_count: int
    invalid_count: int
    mean: float
    minimum: float
    maximum: float
    population_stddev: float
    sha256: str


def analyze_file(path: Path) -> Statistics:
    row_count = 0
    missing_count = 0
    invalid_count = 0
    valid_count = 0
    mean = 0.0
    sum_squared_differences = 0.0
    minimum = math.inf
    maximum = -math.inf

    with path.open("r", newline="", encoding="utf-8") as handle:
        for row in csv.reader(handle):
            row_count += 1
            if len(row) != 1 or not row[0].strip():
                missing_count += 1
                continue
            try:
                value = float(row[0])
            except ValueError:
                invalid_count += 1
                continue
            if not math.isfinite(value) or value < 0.0:
                invalid_count += 1
                continue

            valid_count += 1
            difference = value - mean
            mean += difference / valid_count
            sum_squared_differences += difference * (value - mean)
            minimum = min(minimum, value)
            maximum = max(maximum, value)

    if valid_count == 0:
        raise ValueError(f"{path} contains no valid observations")

    return Statistics(
        row_count=row_count,
        valid_count=valid_count,
        missing_count=missing_count,
        invalid_count=invalid_count,
        mean=mean,
        minimum=minimum,
        maximum=maximum,
        population_stddev=math.sqrt(sum_squared_differences / valid_count),
        sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
    )


def summary_bytes() -> tuple[bytes, dict[tuple[str, str], Statistics]]:
    statistics = {
        (geometry, measure): analyze_file(RAW_DIRECTORY / filename)
        for geometry, measure, filename in SOURCES
    }
    buffer = io.StringIO(newline="")
    fieldnames = (
        "geometry",
        "measure",
        "source_file",
        "row_count",
        "valid_count",
        "missing_count",
        "invalid_count",
        "mean",
        "minimum",
        "maximum",
        "population_stddev",
        "reduction_vs_standard_percent",
        "evidence_type",
        "source_sha256",
    )
    writer = csv.DictWriter(buffer, fieldnames=fieldnames, lineterminator="\n")
    writer.writeheader()
    for geometry, measure, filename in SOURCES:
        values = statistics[(geometry, measure)]
        reduction = ""
        if geometry == "wraparound":
            standard_mean = statistics[("standard", measure)].mean
            reduction = f"{((standard_mean - values.mean) / standard_mean * 100.0):.12f}"
        writer.writerow(
            {
                "geometry": geometry,
                "measure": measure,
                "source_file": f"results/raw/{filename}",
                "row_count": values.row_count,
                "valid_count": values.valid_count,
                "missing_count": values.missing_count,
                "invalid_count": values.invalid_count,
                "mean": f"{values.mean:.12f}",
                "minimum": f"{values.minimum:.12f}",
                "maximum": f"{values.maximum:.12f}",
                "population_stddev": f"{values.population_stddev:.12f}",
                "reduction_vs_standard_percent": reduction,
                "evidence_type": EVIDENCE_TYPE,
                "source_sha256": values.sha256,
            }
        )
    return buffer.getvalue().encode("utf-8"), statistics


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="fail if the committed summary differs from regenerated output",
    )
    arguments = parser.parse_args()

    generated, statistics = summary_bytes()
    invalid_sources = [
        f"{geometry}/{measure}"
        for (geometry, measure), values in statistics.items()
        if values.missing_count or values.invalid_count
    ]
    if arguments.check:
        if not OUTPUT_PATH.exists() or OUTPUT_PATH.read_bytes() != generated:
            print(f"ERROR: {OUTPUT_PATH.relative_to(ROOT)} is not reproducible")
            return 1
    else:
        OUTPUT_PATH.write_bytes(generated)

    for geometry, measure, _ in SOURCES:
        values = statistics[(geometry, measure)]
        print(
            f"{geometry:10} {measure:8} rows={values.row_count} "
            f"valid={values.valid_count} missing={values.missing_count} "
            f"invalid={values.invalid_count} mean={values.mean:.12f}"
        )
    if invalid_sources:
        print("ERROR: missing or invalid values found in " + ", ".join(invalid_sources))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
