import sys, math
from pathlib import Path
from svgpathtools import svg2paths2, Path as SvgPath, CubicBezier, Line, Move, Close

# --- Config ---
TARGET_W = 600
TARGET_H = 600
MAX_POINTS = 1000             # your program limit
KEEP_TOP_PATHS = 40           # keep top-N longest paths (tune as needed)
MARGIN = 20                   # padding around drawing box

def line_to_cubic(p0, p1):
    # degenerate cubic: P0, P0, P1, P1
    return (p0, p0, p1, p1)

def complex_to_xy(z):
    return (z.real, z.imag)

def bbox_of_paths(paths):
    xs, ys = [], []
    for sp in paths:
        for seg in sp:
            if isinstance(seg, (CubicBezier, Line)):
                pts = [seg.start, getattr(seg, 'control1', seg.start), getattr(seg, 'control2', seg.end), seg.end]
                for p in pts:
                    x, y = p.real, p.imag
                    xs.append(x); ys.append(y)
            elif isinstance(seg, Move):
                x, y = seg.end.real, seg.end.imag
                xs.append(x); ys.append(y)
            # Close contributes no new coords
    if not xs:
        return (0,0,1,1)
    return (min(xs), min(ys), max(xs), max(ys))

def scale_and_center(paths, view_w=TARGET_W, view_h=TARGET_H, margin=MARGIN):
    # compute original bbox
    xmin, ymin, xmax, ymax = bbox_of_paths(paths)
    src_w = max(1e-6, xmax - xmin)
    src_h = max(1e-6, ymax - ymin)

    # scale to fit
    sx = (view_w - 2*margin) / src_w
    sy = (view_h - 2*margin) / src_h
    s = min(sx, sy)

    # center
    tx = (view_w - s*(xmin + xmax))/2.0
    ty = (view_h - s*(ymin + ymax))/2.0

    def transform(z):
        x = z.real * s + tx
        y = z.imag * s + ty  # SVG y down; your ortho has y down too -> no flip
        return complex(x, y)

    new_paths = []
    for sp in paths:
        segs = []
        for seg in sp:
            if isinstance(seg, CubicBezier):
                segs.append(CubicBezier(transform(seg.start),
                                        transform(seg.control1),
                                        transform(seg.control2),
                                        transform(seg.end)))
            elif isinstance(seg, Line):
                segs.append(Line(transform(seg.start), transform(seg.end)))
            elif isinstance(seg, Move):
                segs.append(Move(transform(seg.end)))
            elif isinstance(seg, Close):
                segs.append(Close(transform(seg.start)))
            else:
                # Should not happen if you flattened in Inkscape
                pass
        new_paths.append(SvgPath(*segs))
    return new_paths

def path_length(sp: SvgPath):
    # Approx length: sum of segment lengths (CubicBezier has .length())
    length = 0.0
    for seg in sp:
        try:
            length += seg.length(error=1e-3)
        except Exception:
            # fallback
            length += abs(seg.end - seg.start)
    return length

def svg_to_segments(svg_file: str):
    paths, attributes, svg_attr = svg2paths2(svg_file)
    # Collect only paths made of M/L/C/Z
    cleaned = []
    for sp in paths:
        ok = True
        for seg in sp:
            if not isinstance(seg, (CubicBezier, Line, Move, Close)):
                ok = False; break
        if ok and len(sp) > 0:
            cleaned.append(sp)
    if not cleaned:
        raise SystemExit("No usable paths found. In Inkscape: Object to Path + Flatten Beziers first.")

    # Keep longest a few to avoid overshooting MAX_POINTS
    cleaned.sort(key=path_length, reverse=True)
    cleaned = cleaned[:KEEP_TOP_PATHS]

    # Normalize bbox to 600x600
    norma = scale_and_center(cleaned)

    # Convert each path to control point sequence P0,(P1,P2,P3)xN (3k+1 points)
    segments = []
    total_points = 0
    for sp in norma:
        pts = []
        cursor = None
        for seg in sp:
            if isinstance(seg, Move):
                cursor = seg.end
                if not pts:
                    pts.append(cursor)
                continue
            if isinstance(seg, Line):
                P0 = cursor if cursor is not None else seg.start
                P3 = seg.end
                c = line_to_cubic(P0, P3)
            elif isinstance(seg, CubicBezier):
                P0 = seg.start
                c = (P0, seg.control1, seg.control2, seg.end)
            elif isinstance(seg, Close):
                # make a closing line to the first point
                if pts:
                    P0 = complex(pts[0].real, pts[0].imag)
                    P3 = P0
                    c = line_to_cubic(cursor, P3)
                else:
                    continue
            else:
                continue

            # Append cubic controls to list:
            # If this is the first cubic, ensure we start with P0 once.
            if not pts:
                pts.append(c[0])
            # Then append P1,P2,P3
            pts.extend([c[1], c[2], c[3]])
            cursor = c[3]

        # enforce 3k+1
        if len(pts) < 4 or ((len(pts)-1) % 3) != 0:
            # Drop tail to nearest (3k+1)
            k = (len(pts)-1) // 3
            pts = pts[:1 + 3*k]
            if len(pts) < 4:
                continue

        if total_points + len(pts) + 2 > MAX_POINTS:  # +2 for a potential "-1 -1"
            break

        segments.append(pts)
        total_points += len(pts)

    return segments

def write_savefile(segments, out_file="savefile.txt"):
    with open(out_file, "w", encoding="utf-8") as f:
        first = True
        for seg in segments:
            if not first:
                f.write("-1 -1\n")
            first = False
            for z in seg:
                x, y = int(round(z.real)), int(round(z.imag))
                f.write(f"{x} {y}\n")
    print(f"Wrote {out_file} with {sum(len(s) for s in segments)} points, {len(segments)} segments.")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python svg_to_savefile.py crest.svg [output.txt]")
        sys.exit(1)
    svg_path = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) >= 3 else "savefile.txt"
    segs = svg_to_segments(svg_path)
    if not segs:
        print("No segments extracted.")
        sys.exit(2)
    write_savefile(segs, out)
