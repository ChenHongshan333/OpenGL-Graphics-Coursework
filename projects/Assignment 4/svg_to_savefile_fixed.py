# svg_to_savefile_light.py
# Convert an SVG file (with <path d="...">) into savefile.txt for CS3241 Bézier project
# Supports Potrace SVG output with transform="translate(...) scale(...)"
# Works in Python 3.8+ and Ubuntu/WSL

import math
import re
import xml.etree.ElementTree as ET
from svg.path import parse_path, Line, QuadraticBezier, CubicBezier, Arc

# ---------- Helper: parse transform="translate(...) scale(...)" ----------
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
                ty += float(b)
                tx += a
        elif kind == 'scale':
            if b is None:
                sx *= a
                sy *= a
            else:
                sy *= float(b)
                sx *= a
    return sx, sy, tx, ty

def apply_affine(z: complex, sx, sy, tx, ty):
    return complex(z.real * sx + tx, z.imag * sy + ty)


# ---------- Collect all <path> ----------
def collect_paths(svg_file):
    ns_clean = lambda tag: tag.split('}')[-1]
    root = ET.parse(svg_file).getroot()

    total_elems = 0
    total_paths = 0
    found_d = 0
    parsed_ok = 0
    paths = []

    for elem in root.iter():
        total_elems += 1
        if ns_clean(elem.tag) == 'path':
            total_paths += 1
            d = elem.attrib.get('d')
            if not d:
                continue
            found_d += 1
            try:
                p = parse_path(d)
                if len(p) > 0:
                    paths.append(elem)  # keep elem for later (need transform)
                    parsed_ok += 1
            except Exception as e:
                print("[error] parse_path() failed:", repr(e))
                print("   d[:120] =", d[:120].replace('\n',' '))
                continue

    print(f"[info] XML elements={total_elems}, path_nodes={total_paths}, with_d={found_d}, parsed_OK={parsed_ok}")
    return paths


# ---------- Convert a path element into cubic Bezier segments ----------
def cubicize_path(elem):
    d = elem.attrib.get('d', '')
    p = parse_path(d)
    sx, sy, tx, ty = parse_transform_attr(elem.attrib.get('transform', ''))

    cubics = []
    for seg in p:
        if isinstance(seg, CubicBezier):
            P0, P1, P2, P3 = seg.start, seg.control1, seg.control2, seg.end
        elif isinstance(seg, Line):
            P0, P3 = seg.start, seg.end
            P1 = P2 = (P0 + P3) / 2
        elif isinstance(seg, QuadraticBezier):
            P0, Q1, P3 = seg.start, seg.control, seg.end
            P1 = P0 + (Q1 - P0) * (2.0 / 3.0)
            P2 = P3 + (Q1 - P3) * (2.0 / 3.0)
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
        cubics = [
            (apply_affine(P0, sx, sy, tx, ty),
             apply_affine(P1, sx, sy, tx, ty),
             apply_affine(P2, sx, sy, tx, ty),
             apply_affine(P3, sx, sy, tx, ty))
            for (P0, P1, P2, P3) in cubics
        ]

    return cubics


# ---------- Convert to control points (int) ----------
def to_int_point(z: complex, scale=1.0):
    return int(round(z.real * scale)), int(round(z.imag * scale))


# ---------- Main ----------
def main(svg_file, out_file):
    paths = collect_paths(svg_file)
    all_cubics = [cubicize_path(e) for e in paths]
    pts = []
    for cubics in all_cubics:
        for (P0, P1, P2, P3) in cubics:
            pts.extend([P0, P1, P2, P3])

    if not pts:
        print("[warn] No points found; check if your SVG has valid <path d=\"...\"> entries.")
        open(out_file, 'w').write("0\n")
        print(f"Wrote {out_file} with 0 points, 0 segments.")
        return

    # find bounding box
    xs = [p.real for p in pts]
    ys = [p.imag for p in pts]
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    width, height = maxx - minx, maxy - miny

    # normalize to 0..600 window
    scale = 600.0 / max(width, height)
    print(f"[info] bounding box=({minx},{miny})-({maxx},{maxy}), scale={scale:.4f}")

    out_lines = []
    nPt = 0
    for cubics in all_cubics:
        for (P0, P1, P2, P3) in cubics:
            for P in (P0, P1, P2, P3):
                x, y = to_int_point((P.real - minx) * scale, scale=1.0), to_int_point((P.imag - miny) * scale, scale=1.0)
            # Actually store properly:
    nPt = 0
    all_points = []
    for cubics in all_cubics:
        for (P0, P1, P2, P3) in cubics:
            for P in (P0, P1, P2, P3):
                xi = int(round((P.real - minx) * scale))
                yi = int(round((P.imag - miny) * scale))
                all_points.append((xi, yi))
                nPt += 1

    with open(out_file, 'w') as f:
        f.write(f"{nPt}\n")
        for (x, y) in all_points:
            f.write(f"{x} {y}\n")

    print(f"[done] Wrote {out_file} with {nPt} points, {len(all_cubics)} segments.")


# ---------- Entry ----------
if __name__ == "__main__":
    import sys
    if len(sys.argv) < 3:
        print("Usage: python svg_to_savefile_light.py input.svg output.txt")
    else:
        svg, out = sys.argv[1], sys.argv[2]
        main(svg, out)
