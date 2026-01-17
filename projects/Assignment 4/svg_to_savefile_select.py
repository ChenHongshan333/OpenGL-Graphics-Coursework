#!/usr/bin/env python3
"""
svg_to_savefile_select.py
Convert an SVG (from Inkscape, etc.) into savefile.txt format
for CS3241 Assignment 4 (Bezier curves).

✔ Reads SVG paths
✔ Filters and sorts them by geometric size
✔ Samples each cubic Bezier path into control points
✔ Outputs coordinates grouped per subpath, separated by "-1 -1"

Usage:
  python svg_to_savefile_select.py input.svg savefile.txt \
      --keep-top 999999 --min-len 0 --min-area 0 --score-lambda 0
"""

import argparse
import math
import xml.etree.ElementTree as ET
from svgpathtools import parse_path

# ------------------------
# Command-line arguments
# ------------------------
parser = argparse.ArgumentParser(description="Convert SVG paths to CS3241 savefile.txt")
parser.add_argument("input_svg", help="input SVG filename")
parser.add_argument("output_txt", help="output text filename")
parser.add_argument("--keep-top", type=int, default=999999)
parser.add_argument("--min-len", type=float, default=0.0)
parser.add_argument("--min-area", type=float, default=0.0)
parser.add_argument("--score-lambda", type=float, default=0.0)
args = parser.parse_args()

# ------------------------
# Helper functions
# ------------------------
def path_length(path):
    """Approximate path length."""
    try:
        return sum(seg.length(error=1e-2) for seg in path)
    except Exception:
        return 0.0

def path_bbox(path):
    """Return (minx,miny,maxx,maxy)."""
    xs, ys = [], []
    for seg in path:
        for t in [i / 10.0 for i in range(11)]:
            p = seg.point(t)
            xs.append(p.real)
            ys.append(p.imag)
    return min(xs), min(ys), max(xs), max(ys)

# ------------------------
# Load and parse SVG
# ------------------------
print(f"[info] parsing {args.input_svg} ...")
tree = ET.parse(args.input_svg)
root = tree.getroot()

# extract paths
paths = []
for elem in root.iter():
    if elem.tag.endswith("path") and elem.get("d"):
        # 跳过带 fill 的（填充路径），只保留轮廓线
        style = elem.get("style", "")
        if "fill" in style and not "fill:none" in style:
            continue

        try:
            p = parse_path(elem.get("d"))
            paths.append(p)
        except Exception as e:
            print("[warn] failed to parse path:", e)

print(f"[info] path elements (after filtering): {len(paths)}")

# ------------------------
# Compute bbox + filtering
# ------------------------
scored = []
for p in paths:
    minx, miny, maxx, maxy = path_bbox(p)
    length = path_length(p)
    area = (maxx - minx) * (maxy - miny)
    score = length + args.score_lambda * area
    if length >= args.min_len and area >= args.min_area:
        scored.append((score, p, (minx, miny, maxx, maxy)))

scored.sort(key=lambda x: -x[0])
kept_subpaths = [p for _, p, _ in scored[: args.keep_top]]
print(f"[info] total subpaths={len(scored)}, kept={len(kept_subpaths)}")

# ------------------------
# Compute global bbox for scaling
# ------------------------
if not kept_subpaths:
    print("[warn] no subpaths kept, exiting.")
    exit(0)

all_x, all_y = [], []
for p in kept_subpaths:
    minx, miny, maxx, maxy = path_bbox(p)
    all_x += [minx, maxx]
    all_y += [miny, maxy]

minx, miny, maxx, maxy = min(all_x), min(all_y), max(all_x), max(all_y)
cx, cy = (minx + maxx) / 2, (miny + maxy) / 2
scale = 600.0 / max(maxx - minx, maxy - miny)
print(f"[info] bbox=({minx:.1f},{miny:.1f})-({maxx:.1f},{maxy:.1f}), scale={scale:.4f}")

# ------------------------
# Sample cubic Bezier control points
# ------------------------
def cubic_points_from_path(path):
    """Split a path into (P0,P1,P2,P3) control points."""
    curves = []
    for seg in path:
        if hasattr(seg, "start") and hasattr(seg, "control1") and hasattr(seg, "control2") and hasattr(seg, "end"):
            curves.append((seg.start, seg.control1, seg.control2, seg.end))
    return curves

kept_subpaths_ctrl = [cubic_points_from_path(p) for p in kept_subpaths]

# ------------------------
# Write savefile.txt (multi-segment)
# ------------------------
total_pts = 0
with open(args.output_txt, "w", encoding="utf-8") as f:
    for sp in kept_subpaths_ctrl:
        first = True
        for (P0, P1, P2, P3) in sp:
            seq = (P0, P1, P2, P3) if first else (P1, P2, P3)
            for P in seq:
                xi = int(round((P.real - cx) * scale + 300))
                yi = int(round((- P.imag + cy) * scale + 300))
                f.write(f"{xi} {yi}\n")
                total_pts += 1
            first = False
        f.write("-1 -1\n")

print(f"[done] wrote {args.output_txt} with {total_pts} points")
print(f"[info] kept_subpaths={len(kept_subpaths)}, scale={scale:.4f}, "
      f"bbox=({minx:.1f},{miny:.1f})-({maxx:.1f},{maxy:.1f})")
