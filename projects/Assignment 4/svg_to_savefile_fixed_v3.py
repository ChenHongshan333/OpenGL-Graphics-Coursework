# svg_to_savefile_fixed_v3.py
# Convert SVG <path d="..."> to CS3241 savefile.txt control points.
# Fixes: 1) per-subpath grouping (no straight lines across MoveTos)
#        2) transform="translate/scale" handling
#        3) 4+3+3... chaining; each (sub)path starts fresh
#        4) center+fit to 600x600

import re
import xml.etree.ElementTree as ET
from svg.path import parse_path, Line, QuadraticBezier, CubicBezier, Arc

# ---------- transform helpers ----------
_TRANSFORM_RE = re.compile(r'(translate|scale)\s*\(\s*([-\d\.eE]+)(?:\s*,\s*([-\d\.eE]+))?\s*\)')

def parse_transform_attr(tstr: str):
    sx = sy = 1.0
    tx = ty = 0.0
    for m in _TRANSFORM_RE.finditer(tstr or ''):
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

def aff(z: complex, sx, sy, tx, ty) -> complex:
    return complex(z.real * sx + tx, z.imag * sy + ty)

# ---------- parse SVG paths ----------
def collect_path_elems(svg_file):
    root = ET.parse(svg_file).getroot()
    ns_clean = lambda t: t.split('}')[-1]
    paths = []
    total = with_d = ok = 0
    for el in root.iter():
        if ns_clean(el.tag) == 'path':
            total += 1
            d = el.attrib.get('d')
            if not d:
                continue
            with_d += 1
            try:
                p = parse_path(d)       # just to validate
                if len(p) > 0:
                    paths.append(el)
                    ok += 1
            except Exception as e:
                print("[warn] skip malformed <path>:", e)
    print(f"[info] path_elems={len(paths)}, parsed_OK={ok}")
    return paths

# ---------- convert one <path> into list of subpaths, each a list of cubics ----------
def cubicize_elem_into_subpaths(elem):
    """Return: List[List[(P0,P1,P2,P3)]]  (each inner list is one subpath)"""
    d = elem.attrib.get('d', '')
    path = parse_path(d)
    sx, sy, tx, ty = parse_transform_attr(elem.attrib.get('transform', ''))

    subpaths = []
    current = []
    last_end = None
    EPS = 1e-9

    def push_seg(P0, P1, P2, P3):
        nonlocal current
        # start a new subpath if disconnected
        nonlocal last_end
        if last_end is None or abs(P0 - last_end) > EPS:
            if current:
                subpaths.append(current)
            current = []
        current.append((P0, P1, P2, P3))
        last_end = P3

    for seg in path:
        if isinstance(seg, CubicBezier):
            P0, P1, P2, P3 = seg.start, seg.control1, seg.control2, seg.end
            push_seg(P0, P1, P2, P3)
        elif isinstance(seg, QuadraticBezier):
            P0, Q1, P3 = seg.start, seg.control, seg.end
            P1 = P0 + (Q1 - P0) * (2.0/3.0)
            P2 = P3 + (Q1 - P3) * (2.0/3.0)
            push_seg(P0, P1, P2, P3)
        elif isinstance(seg, Line):
            P0, P3 = seg.start, seg.end
            # simple line -> degenerate cubic (midpoint handles)
            mid = (P0 + P3) / 2.0
            push_seg(P0, mid, mid, P3)
        elif isinstance(seg, Arc):
            try:
                for cc in seg.as_cubic_curves():
                    P0, P1, P2, P3 = cc.start, cc.control1, cc.control2, cc.end
                    push_seg(P0, P1, P2, P3)
            except Exception:
                pass
        else:
            # unknown seg; break subpath on safety
            last_end = None

    if current:
        subpaths.append(current)

    # apply transform
    if sx != 1.0 or sy != 1.0 or tx != 0.0 or ty != 0.0:
        subpaths = [
            [
                (aff(P0,sx,sy,tx,ty), aff(P1,sx,sy,tx,ty),
                 aff(P2,sx,sy,tx,ty), aff(P3,sx,sy,tx,ty))
                for (P0,P1,P2,P3) in sp
            ] for sp in subpaths
        ]
    return subpaths

# ---------- main export ----------
def main(svg_file, out_file):
    elems = collect_path_elems(svg_file)
    # grouped as: per <path> -> per subpath -> list of cubics
    grouped = [cubicize_elem_into_subpaths(e) for e in elems]

    # gather all points to compute bbox
    pts = []
    for subpaths in grouped:
        for sp in subpaths:
            for (P0,P1,P2,P3) in sp:
                pts.extend([P0,P1,P2,P3])

    if not pts:
        print("[error] No cubic data found.")
        with open(out_file, 'w') as f:
            f.write("0\n")
        return

    xs = [p.real for p in pts]
    ys = [p.imag for p in pts]
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    width  = maxx - minx
    height = maxy - miny
    scale = 600.0 / max(width, height)
    cx = (minx + maxx) / 2.0
    cy = (miny + maxy) / 2.0

    # write points with 4+3+3... per subpath; subpaths and paths are independent
    out_pts = []
    for subpaths in grouped:
        for sp in subpaths:
            first = True
            for (P0,P1,P2,P3) in sp:
                seq = (P0,P1,P2,P3) if first else (P1,P2,P3)
                for P in seq:
                    xi = int(round((P.real - cx) * scale + 300))
                    yi = int(round((P.imag - cy) * scale + 300))
                    out_pts.append((xi, yi))
                first = False

    with open(out_file, 'w') as f:
        f.write(f"{len(out_pts)}\n")
        for (x,y) in out_pts:
            f.write(f"{x} {y}\n")

    print(f"[done] Wrote {out_file} with {len(out_pts)} points.")
    print(f"[info] bbox=({minx:.2f},{miny:.2f})-({maxx:.2f},{maxy:.2f}), scale={scale:.4f}, paths={len(elems)}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 3:
        print("Usage: python svg_to_savefile_fixed_v3.py input.svg output.txt")
    else:
        main(sys.argv[1], sys.argv[2])
