#!/usr/bin/env python3
"""Generate deterministic SVG figures from committed machine-readable evidence."""

from __future__ import annotations

import argparse
import csv
import html
import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ASSET_DIRECTORY = ROOT / "assets"
RAW_DIRECTORY = ROOT / "results" / "raw"

NAVY = "#0F2742"
BLUE = "#2878B5"
TEAL = "#16A085"
CORAL = "#E76F51"
GOLD = "#F4A261"
INK = "#182230"
MUTED = "#5E6B78"
GRID = "#DCE4EA"
PANEL = "#FFFFFF"
BACKGROUND = "#F4F7FA"


def text(x: float, y: float, value: str, size: int = 16, *, anchor: str = "start",
         weight: int = 400, fill: str = INK) -> str:
    return (
        f'<text x="{x:.1f}" y="{y:.1f}" text-anchor="{anchor}" '
        f'font-family="Inter,Segoe UI,Arial,sans-serif" font-size="{size}" '
        f'font-weight="{weight}" fill="{fill}">{html.escape(value)}</text>'
    )


def base_svg(title: str, subtitle: str, body: str, footer: str) -> str:
    return (
        '<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="760" '
        'viewBox="0 0 1200 760" role="img">\n'
        f'<rect width="1200" height="760" fill="{BACKGROUND}"/>\n'
        f'{text(60, 62, title, 30, weight=700, fill=NAVY)}\n'
        f'{text(60, 92, subtitle, 15, fill=MUTED)}\n'
        f'{body}\n'
        f'<line x1="60" y1="710" x2="1140" y2="710" stroke="{GRID}"/>\n'
        f'{text(60, 737, footer, 13, weight=600, fill=MUTED)}\n'
        '</svg>\n'
    )


def panel(x: float, y: float, width: float, height: float) -> str:
    return (
        f'<rect x="{x:.1f}" y="{y:.1f}" width="{width:.1f}" height="{height:.1f}" '
        f'rx="12" fill="{PANEL}" stroke="{GRID}"/>\n'
    )


def load_values(filename: str) -> list[float]:
    with (RAW_DIRECTORY / filename).open(newline="", encoding="utf-8") as handle:
        return [float(row[0]) for row in csv.reader(handle)]


def load_summary() -> dict[tuple[str, str], dict[str, str]]:
    with (ROOT / "results" / "distance-summary.csv").open(newline="", encoding="utf-8") as handle:
        return {(row["geometry"], row["measure"]): row for row in csv.DictReader(handle)}


def load_benchmarks() -> list[dict[str, str]]:
    with (ROOT / "results" / "report-table-benchmarks.csv").open(
        newline="", encoding="utf-8"
    ) as handle:
        return list(csv.DictReader(handle))


def histogram(values: list[float], bins: int = 36) -> tuple[list[int], float, float]:
    lower = min(values)
    upper = max(values)
    width = (upper - lower) / bins
    counts = [0] * bins
    for value in values:
        index = min(int((value - lower) / width), bins - 1) if width else 0
        counts[index] += 1
    return counts, lower, upper


def histogram_panel(x: float, y: float, width: float, height: float, title_value: str,
                    values: list[float], color: str) -> str:
    counts, lower, upper = histogram(values)
    chart_x = x + 52
    chart_y = y + 52
    chart_width = width - 76
    chart_height = height - 100
    maximum = max(counts)
    elements = [panel(x, y, width, height), text(x + 24, y + 32, title_value, 17, weight=650)]
    for tick in range(4):
        tick_y = chart_y + chart_height * tick / 3
        elements.append(
            f'<line x1="{chart_x:.1f}" y1="{tick_y:.1f}" x2="{chart_x + chart_width:.1f}" '
            f'y2="{tick_y:.1f}" stroke="{GRID}" stroke-width="1"/>'
        )
    bar_width = chart_width / len(counts)
    for index, count in enumerate(counts):
        bar_height = chart_height * count / maximum
        elements.append(
            f'<rect x="{chart_x + index * bar_width:.2f}" '
            f'y="{chart_y + chart_height - bar_height:.2f}" width="{max(bar_width - 0.8, 0.5):.2f}" '
            f'height="{bar_height:.2f}" fill="{color}" opacity="0.88"/>'
        )
    elements.extend(
        [
            f'<line x1="{chart_x:.1f}" y1="{chart_y + chart_height:.1f}" '
            f'x2="{chart_x + chart_width:.1f}" y2="{chart_y + chart_height:.1f}" stroke="{INK}"/>',
            text(chart_x, chart_y + chart_height + 22, f"{lower:.6f}", 11, fill=MUTED),
            text(chart_x + chart_width, chart_y + chart_height + 22, f"{upper:.6f}", 11,
                 anchor="end", fill=MUTED),
            text(x + width - 20, y + 33, f"mean {sum(values) / len(values):.6f}", 12,
                 anchor="end", fill=MUTED),
        ]
    )
    return "\n".join(elements)


def topology_svg() -> str:
    elements = [panel(60, 125, 500, 545), panel(640, 125, 500, 545)]
    elements.extend(
        [
            text(90, 170, "Standard geometry", 22, weight=700, fill=NAVY),
            text(90, 198, "Bounded unit square", 14, fill=MUTED),
            text(670, 170, "Wraparound geometry", 22, weight=700, fill=NAVY),
            text(670, 198, "Periodic shortest path (2D torus)", 14, fill=MUTED),
            f'<rect x="125" y="245" width="370" height="330" fill="#F8FBFD" stroke="{NAVY}" stroke-width="3"/>',
            f'<rect x="705" y="245" width="370" height="330" fill="#F8FBFD" stroke="{TEAL}" stroke-width="3" stroke-dasharray="10 6"/>',
        ]
    )
    for offset in (0, 580):
        elements.extend(
            [
                f'<circle cx="{170 + offset}" cy="390" r="10" fill="{BLUE}"/>',
                f'<circle cx="{450 + offset}" cy="390" r="10" fill="{CORAL}"/>',
                text(170 + offset, 370, "A (0.1, 0.1)", 13, anchor="middle", fill=MUTED),
                text(450 + offset, 370, "B (0.9, 0.1)", 13, anchor="middle", fill=MUTED),
            ]
        )
    elements.extend(
        [
            f'<line x1="180" y1="390" x2="440" y2="390" stroke="{CORAL}" stroke-width="5"/>',
            text(310, 430, "direct Euclidean distance = 0.8", 15, anchor="middle", weight=600),
            f'<line x1="750" y1="390" x2="705" y2="390" stroke="{TEAL}" stroke-width="5"/>',
            f'<line x1="1075" y1="390" x2="1030" y2="390" stroke="{TEAL}" stroke-width="5"/>',
            text(890, 430, "boundary-crossing distance = 0.2", 15, anchor="middle", weight=600),
            text(310, 620, "Maximum possible distance: √2 ≈ 1.4142", 14,
                 anchor="middle", fill=MUTED),
            text(890, 620, "Maximum shortest-path distance: √0.5 ≈ 0.7071", 14,
                 anchor="middle", fill=MUTED),
            text(690, 396, "↔", 22, anchor="middle", weight=700, fill=TEAL),
            text(1090, 396, "↔", 22, anchor="middle", weight=700, fill=TEAL),
        ]
    )
    return base_svg(
        "Standard vs wraparound geometry",
        "The same point pair can have a different shortest-path distance when opposite edges are identified.",
        "\n".join(elements),
        "SCHEMATIC — unit-square coordinates; not benchmark data",
    )


def distributions_svg() -> str:
    datasets = (
        ("Standard — nearest", "standard_nearest.csv", BLUE),
        ("Wraparound — nearest", "wrap_nearest.csv", TEAL),
        ("Standard — furthest", "standard_furthest.csv", CORAL),
        ("Wraparound — furthest", "wrap_furthest.csv", GOLD),
    )
    positions = ((60, 125), (610, 125), (60, 405), (610, 405))
    body = "\n".join(
        histogram_panel(x, y, 530, 250, title_value, load_values(filename), color)
        for (title_value, filename, color), (x, y) in zip(datasets, positions)
    )
    return base_svg(
        "Distance distributions",
        "Four committed 100,000-observation result files; each panel uses its own x-axis range.",
        body,
        "DERIVED FROM COMMITTED RAW CSV — deterministic 36-bin histograms",
    )


def bar_panel(x: float, y: float, width: float, height: float, title_value: str,
              labels: list[str], values: list[float], colors: list[str], maximum: float,
              decimals: int = 6) -> str:
    elements = [panel(x, y, width, height), text(x + 24, y + 38, title_value, 19, weight=700)]
    chart_x = x + 90
    chart_y = y + 70
    chart_width = width - 130
    chart_height = height - 140
    for tick in range(5):
        tick_value = maximum * tick / 4
        tick_y = chart_y + chart_height - chart_height * tick / 4
        elements.append(
            f'<line x1="{chart_x:.1f}" y1="{tick_y:.1f}" x2="{chart_x + chart_width:.1f}" '
            f'y2="{tick_y:.1f}" stroke="{GRID}"/>'
        )
        elements.append(text(chart_x - 10, tick_y + 4, f"{tick_value:.{decimals}f}", 11,
                             anchor="end", fill=MUTED))
    slot = chart_width / len(values)
    for index, (label, value, color) in enumerate(zip(labels, values, colors)):
        bar_width = min(90.0, slot * 0.55)
        bar_height = chart_height * value / maximum
        bar_x = chart_x + slot * index + (slot - bar_width) / 2
        elements.extend(
            [
                f'<rect x="{bar_x:.1f}" y="{chart_y + chart_height - bar_height:.1f}" '
                f'width="{bar_width:.1f}" height="{bar_height:.1f}" rx="4" fill="{color}"/>',
                text(bar_x + bar_width / 2, chart_y + chart_height + 24, label, 13,
                     anchor="middle", weight=600),
                text(bar_x + bar_width / 2, chart_y + chart_height - bar_height - 10,
                     f"{value:.{decimals}f}", 13, anchor="middle", weight=650),
            ]
        )
    return "\n".join(elements)


def distance_summary_svg() -> str:
    summary = load_summary()
    standard_nearest = float(summary[("standard", "nearest")]["mean"])
    wrap_nearest = float(summary[("wraparound", "nearest")]["mean"])
    standard_furthest = float(summary[("standard", "furthest")]["mean"])
    wrap_furthest = float(summary[("wraparound", "furthest")]["mean"])
    reduction = (standard_furthest - wrap_furthest) / standard_furthest * 100.0
    body = "\n".join(
        [
            bar_panel(60, 135, 520, 500, "Average nearest-neighbour distance",
                      ["Standard", "Wraparound"], [standard_nearest, wrap_nearest],
                      [BLUE, TEAL], 0.0020, 6),
            bar_panel(620, 135, 520, 500, "Average furthest-neighbour distance",
                      ["Standard", "Wraparound"], [standard_furthest, wrap_furthest],
                      [CORAL, GOLD], 1.2, 3),
            f'<rect x="816" y="185" width="300" height="48" rx="24" fill="#FFF3E8" stroke="{GOLD}"/>',
            text(966, 216, f"{reduction:.2f}% lower under wraparound", 15,
                 anchor="middle", weight=700, fill="#9A4D13"),
        ]
    )
    return base_svg(
        "Geometry changes global distances, not local spacing",
        "Nearest means are almost unchanged; furthest means reflect the different topology.",
        body,
        "DERIVED FROM COMMITTED RAW CSV — means recalculated from 100,000 rows per file",
    )


def report_benchmark_svg() -> str:
    rows = load_benchmarks()
    groups = (
        ("Standard · naïve", "standard", "naive"),
        ("Wraparound · naïve", "wraparound", "naive"),
        ("Geometry not specified · optimized", "not-specified", "optimized"),
    )
    positions = (60, 415, 770)
    body_parts: list[str] = []
    schedules = ["static", "dynamic", "guided"]
    for x, (title_value, geometry, algorithm) in zip(positions, groups):
        values = [
            float(next(row["time_seconds"] for row in rows
                       if row["geometry"] == geometry and row["algorithm"] == algorithm
                       and row["schedule"] == schedule and row["threads"] == "8"))
            for schedule in schedules
        ]
        body_parts.append(
            bar_panel(x, 145, 325, 500, title_value, [name.title() for name in schedules], values,
                      [BLUE, TEAL, GOLD], 50.0, 2)
        )
    return base_svg(
        "Historical 8-thread timing tables",
        "These values are transcribed from Tables 1–3; they were not reproduced during this rebuild.",
        "\n".join(body_parts),
        "REPORT-TABLE RESULT — historical hardware metadata is incomplete",
    )


def thread_scaling_svg() -> str:
    rows = load_benchmarks()
    configs = (
        ("Standard naïve static", "standard", "naive", "static"),
        ("Wraparound naïve static", "wraparound", "naive", "static"),
        ("Optimized dynamic\n(geometry not specified)", "not-specified", "optimized", "dynamic"),
    )
    elements = [panel(60, 135, 1080, 520)]
    chart_x, chart_y, chart_width, chart_height = 140.0, 195.0, 930.0, 360.0
    for tick in range(5):
        value = 80.0 * tick / 4
        tick_y = chart_y + chart_height - chart_height * tick / 4
        elements.append(
            f'<line x1="{chart_x}" y1="{tick_y:.1f}" x2="{chart_x + chart_width}" '
            f'y2="{tick_y:.1f}" stroke="{GRID}"/>'
        )
        elements.append(text(chart_x - 14, tick_y + 4, f"{value:.0f}s", 12,
                             anchor="end", fill=MUTED))
    group_width = chart_width / len(configs)
    for group_index, (label, geometry, algorithm, schedule) in enumerate(configs):
        values = {
            int(row["threads"]): float(row["time_seconds"])
            for row in rows
            if row["geometry"] == geometry and row["algorithm"] == algorithm
            and row["schedule"] == schedule and row["threads"] in {"4", "8"}
        }
        center = chart_x + group_width * (group_index + 0.5)
        for bar_index, (threads, color) in enumerate(((4, BLUE), (8, TEAL))):
            value = values[threads]
            bar_width = 72.0
            bar_x = center + (bar_index - 1) * 85 + 12
            bar_height = chart_height * value / 80.0
            elements.extend(
                [
                    f'<rect x="{bar_x:.1f}" y="{chart_y + chart_height - bar_height:.1f}" '
                    f'width="{bar_width}" height="{bar_height:.1f}" rx="4" fill="{color}"/>',
                    text(bar_x + bar_width / 2, chart_y + chart_height - bar_height - 9,
                         f"{value:.2f}s", 12, anchor="middle", weight=650),
                    text(bar_x + bar_width / 2, chart_y + chart_height + 22,
                         f"{threads} threads", 12, anchor="middle", fill=MUTED),
                ]
            )
        label_lines = label.split("\n")
        for line_index, line in enumerate(label_lines):
            elements.append(text(center, 610 + line_index * 17, line, 13,
                                 anchor="middle", weight=600))
    return base_svg(
        "Historical 4-thread vs 8-thread results",
        "Only the two measured thread counts are shown; no intermediate scaling curve is implied.",
        "\n".join(elements),
        "REPORT-TABLE RESULT — Table 4 transcription; not reproduced timings",
    )


def generated_assets() -> dict[Path, str]:
    return {
        ASSET_DIRECTORY / "topology-comparison.svg": topology_svg(),
        ASSET_DIRECTORY / "distance-distributions.svg": distributions_svg(),
        ASSET_DIRECTORY / "distance-summary.svg": distance_summary_svg(),
        ASSET_DIRECTORY / "report-benchmark-comparison.svg": report_benchmark_svg(),
        ASSET_DIRECTORY / "thread-scaling.svg": thread_scaling_svg(),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify committed SVG bytes")
    arguments = parser.parse_args()
    assets = generated_assets()
    failures: list[str] = []
    for path, content in assets.items():
        encoded = content.encode("utf-8")
        if arguments.check:
            if not path.exists() or path.read_bytes() != encoded:
                failures.append(str(path.relative_to(ROOT)))
        else:
            path.write_bytes(encoded)
            print(f"generated {path.relative_to(ROOT)}")
    if failures:
        print("ERROR: non-reproducible assets: " + ", ".join(failures))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
