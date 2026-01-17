# svg_to_savefile_fixed_v2.py
# Converts SVG <path> data into CS3241 Bezier "savefile.txt" format.
# Supports multiple <path> elements, grouped and smoothed.
# Output is normalized to 600x600 canvas, centered.

import re
import math
import xml.etree.ElementTree as ET
from svg.path import parse_path, Line, QuadraticBezier, CubicBezier, Arc

# --- Transform Parser ---
_transform_re = re.compile(r'(translate|scale)\s*\(\s*([-\d\.eE]+)(?:\s*,\s*([-\d\.eE]+))?\s*\)')

def parse_transform_attr(tstr):
    sx = sy = 1.0
    tx = ty = 0.0
    for m in _transform_re.finditer(tstr or ''):
        kind = m.group(1)
        a = float(m.group(2))
        b = m.group(3)
        if kind == 'translate':
            if b is None:
                tx += a
            else:
                tx += a
                ty += float(b)
        elif kind == 'scale':
            if b is None:
                sx *= a
                sy *= a
            else:
                sx *= a
                sy *= float(b)
    return sx, sy, tx, ty

def apply_affine(z: complex, sx, sy, tx, ty):
    return complex(z.real * sx + tx, z.imag * sy + ty)


# --- Collect all path elements ---
def collect_paths(svg_file):
    root = ET.parse(svg_file).getroot()
    ns_clean = lambda tag: tag.split('}')[-1]
    paths = []
    total = with_d = ok = 0
    for elem in root.iter():
        if ns_clean(elem.tag) == 'path':
            total += 1
            d = elem.attrib.get('d')
            if not d: continue
            with_d += 1
            try:
                p = parse_path(d)
                if len(p) > 0:
                    paths.append(elem)
                    ok += 1
            except Exception as e:
                print("[warn] skip malformed path:", e)
    print(f"[info] paths found={len(paths)}, parsed_OK={ok}")
    return paths


# --- Convert any segment to CubicBezier form ---
def cubicize_path(elem):
    d = elem.attrib.get('d', '')
    p = parse_path(d)
    sx, sy, tx, ty = parse_transform_attr(elem.attrib.get('transform', ''))
    cubics = []
    for seg in p:
        if isinstance(seg, CubicBezier):
            P0, P1, P2, P3 = seg.start, seg.control1, seg.control2, seg.end
        elif isinstance(seg, QuadraticBezier):
            P0, Q1, P3 = seg.start, seg.control, seg.end
            P1 = P0 + (Q1 - P0) * (2/3)
            P2 = P3 + (Q1 - P3) * (2/3)
        elif isinstance(seg, Line):
            P0, P3 = seg.start, seg.end
            P1 = P2 = (P0 + P3) / 2
        elif isinstance(seg, Arc):
            try:
                for cc in seg.as_cubic_curves():
                    cubics.append((cc.start, cc.control1, cc.control2, cc.end))
            except Exception:
                continue
            continue
        else:
            continue
        cubics.append((P0, P1, P2, P3))
    # Apply transform
    if sx != 1.0 or sy != 1.0 or tx != 0.0 or ty != 0.0:
        cubics = [(apply_affine(P0, sx, sy, tx, ty),
                   apply_affine(P1, sx, sy, tx, ty),
                   apply_affine(P2, sx, sy, tx, ty),
                   apply_affine(P3, sx, sy, tx, ty))
                  for (P0, P1, P2, P3) in cubics]
    return cubics


# --- Main conversion ---
def main(svg_file, out_file):
    paths = collect_paths(svg_file)
    grouped = [cubicize_path(e) for e in paths]
    pts_all = [p for group in grouped for seg in group for p in seg]
    if not pts_all:
        print("[error] No cubic data found.")
        return

    # Bounding box + scale
    xs = [p.real for p in pts_all]
    ys = [p.imag for p in pts_all]
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    width, height = maxx - minx, maxy - miny
    scale = 600.0 / max(width, height)
    cx = (minx + maxx) / 2
    cy = (miny + maxy) / 2

    # --- Write points ---
    all_points = []
    for cubics in grouped:
        first = True
        for (P0, P1, P2, P3) in cubics:
            points = (P0, P1, P2, P3) if first else (P1, P2, P3)
            for P in points:
                xi = int(round((P.real - cx) * scale + 300))
                yi = int(round((P.imag - cy) * scale + 300))
                all_points.append((xi, yi))
            first = False

    # Output
    with open(out_file, 'w') as f:
        f.write(f"{len(all_points)}\n")
        for (x, y) in all_points:
            f.write(f"{x} {y}\n")

    print(f"[done] Wrote {out_file} with {len(all_points)} points, {len(grouped)} path groups.")
    print(f"[info] bounding box=({minx:.1f},{miny:.1f})-({maxx:.1f},{maxy:.1f}), scale={scale:.4f}")


if __name__ == "__main__":
    import sys
    if len(sys.argv) < 3:
        print("Usage: python svg_to_savefile_fixed_v2.py input.svg output.txt")
    else:
        main(sys.argv[1], sys.argv[2])
