# -*- coding: utf-8 -*-
"""
Convert an SVG (paths) to CS3241 savefile.txt (multi-segment, '-1 -1' separators).
- Uses lightweight 'svg.path' (NOT svgpathtools) to parse path 'd'.
- Supports Line, CubicBezier, QuadraticBezier, Arc (arcs auto-expanded to cubics).
- Each <path> element becomes one output segment (3k+1 control points).
- Straight lines are emitted as degenerate cubics: P0,P0,P3,P3.
- Scales/centers to 600x600 (y-down), with margin.
"""

import sys, math, xml.etree.ElementTree as ET
from svg.path import parse_path, Line, CubicBezier, QuadraticBezier, Arc, Path as SvgPath

TARGET_W, TARGET_H = 600, 600
MARGIN = 20
MAX_POINTS = 1000         # hard limit from your C++ code
KEEP_TOP_PATHS = 40       # keep longest N paths to avoid overflow

def q_to_cubic(P0, Q1, P3):
    # QuadraticBezier -> CubicBezier controls
    # P1 = P0 + 2/3 (Q1 - P0)
    # P2 = P3 + 2/3 (Q1 - P3)
    P1 = P0 + (Q1 - P0) * (2.0/3.0)
    P2 = P3 + (Q1 - P3) * (2.0/3.0)
    return (P0, P1, P2, P3)

def line_to_cubic(P0, P3):
    # degenerate cubic (straight line)
    return (P0, P0, P3, P3)

def bbox_points(pts):
    xs = [p.real for p in pts]
    ys = [p.imag for p in pts]
    return min(xs), min(ys), max(xs), max(ys)

def collect_paths(svg_file):
    # Parse SVG & collect all <path d="...">
    ns_clean = lambda tag: tag.split('}')[-1]  # strip namespace if present
    root = ET.parse(svg_file).getroot()
    paths = []
    for elem in root.iter():
        if ns_clean(elem.tag) == 'path' and 'd' in elem.attrib:
            d = elem.attrib['d']
            try:
                p = parse_path(d)
                if len(p) > 0:
                    paths.append(p)
            except Exception:
                pass
    return paths

def cubicize_path(p: SvgPath):
    """Return list of cubic-bezier tuples (P0,P1,P2,P3) for this SVG Path.
       If the path has multiple subpaths, we keep them in order (we’ll
       split segments by discontinuities later)."""
    cubics = []
    cursor = None
    first_point = None

    for seg in p:
        if isinstance(seg, CubicBezier):
            P0, P1, P2, P3 = seg.start, seg.control1, seg.control2, seg.end
            if not cubics: first_point = P0
            cubics.append((P0, P1, P2, P3))
            cursor = P3

        elif isinstance(seg, Line):
            P0, P3 = seg.start, seg.end
            if not cubics: first_point = P0
            cubics.append(line_to_cubic(P0, P3))
            cursor = P3

        elif isinstance(seg, QuadraticBezier):
            P0, Q1, P3 = seg.start, seg.control, seg.end
            if not cubics: first_point = P0
            cubics.append(q_to_cubic(P0, Q1, P3))
            cursor = P3

        elif isinstance(seg, Arc):
            # Expand arc into multiple CubicBezier segments
            try:
                cubs = seg.as_cubic_curves()
            except Exception:
                cubs = []  # fallback
            for c in cubs:
                if not cubics: first_point = c.start
                cubics.append((c.start, c.control1, c.control2, c.end))
                cursor = c.end

        else:
            # Unknown type -> skip
            pass

    return cubics

def path_length_est(cubics):
    # Rough length = sum of straight distances between P0..P3
    L = 0.0
    for (P0,P1,P2,P3) in cubics:
        L += abs(P1-P0) + abs(P2-P1) + abs(P3-P2)
    return L

def fit_to_view(all_cubics):
    # Compute bbox over all control points, then scale+center to 600x600
    pts = []
    for cubs in all_cubics:
        for (P0,P1,P2,P3) in cubs:
            pts += [P0,P1,P2,P3]
    if not pts:
        return all_cubics

    xmin, ymin, xmax, ymax = bbox_points(pts)
    w = max(1e-6, xmax - xmin); h = max(1e-6, ymax - ymin)
    sx = (TARGET_W - 2*MARGIN) / w
    sy = (TARGET_H - 2*MARGIN) / h
    s  = min(sx, sy)

    tx = (TARGET_W - s*(xmin + xmax))/2.0
    ty = (TARGET_H - s*(ymin + ymax))/2.0
    def T(z: complex):
        # y-down (SVG) -> y-down (your ortho). No flip needed.
        return complex(z.real * s + tx, z.imag * s + ty)

    out = []
    for cubs in all_cubics:
        out.append([(T(P0), T(P1), T(P2), T(P3)) for (P0,P1,P2,P3) in cubs])
    return out

def cubics_to_segment_points(cubics):
    """Convert list of cubic tuples to a 3k+1 control point list:
       [P0, P1, P2, P3,  P1, P2, P3, ...] (sharing endpoints)."""
    if not cubics: return []
    pts = [cubics[0][0]]
    for (P0,P1,P2,P3) in cubics:
        pts.extend([P1,P2,P3])
    # Ensure 3k+1
    if ((len(pts)-1) % 3) != 0:
        k = (len(pts)-1)//3
        pts = pts[:1+3*k]
    return pts

def write_savefile(segments, out="savefile.txt"):
    total = 0
    with open(out, "w", encoding="utf-8") as f:
        first = True
        for seg in segments:
            if not seg: continue
            if not first:
                f.write("-1 -1\n")
            first = False
            for z in seg:
                x, y = int(round(z.real)), int(round(z.imag))
                f.write(f"{x} {y}\n")
                total += 1
    print(f"Wrote {out} with {total} points, {len(segments)} segments.")

def main(svg_file, out_file="savefile.txt"):
    paths = collect_paths(svg_file)
    if not paths:
        print("No <path> elements found. Export Plain SVG and ensure shapes are converted to paths.")
        sys.exit(2)

    # Convert each path to list of cubic-beziers
    all_cubics = [cubicize_path(p) for p in paths]
    # Rank by rough length; keep longest N to avoid overflows
    ranked = sorted(all_cubics, key=path_length_est, reverse=True)[:KEEP_TOP_PATHS]

    # Scale to fit canvas
    ranked = fit_to_view(ranked)

    # Convert to 3k+1 control lists; enforce MAX_POINTS
    segments = []
    used = 0
    for cubs in ranked:
        seg_pts = cubics_to_segment_points(cubs)
        if len(seg_pts) < 4: continue
        if used + len(seg_pts) + 2 > MAX_POINTS:   # +2 buffer for "-1 -1"
            break
        segments.append(seg_pts)
        used += len(seg_pts)

    # Flatten complex -> output
    write_savefile(segments, out_file)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python svg_to_savefile_light.py crest.svg [savefile.txt]")
        sys.exit(1)
    svg = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) >= 3 else "savefile.txt"
    main(svg, out)
