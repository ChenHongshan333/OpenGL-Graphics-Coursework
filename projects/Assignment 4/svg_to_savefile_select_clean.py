#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
svg_to_savefile_clean.py
Convert SVG paths to savefile.txt compatible with CS3241 OpenGL viewer.
Features:
 - skips filled paths
 - removes nearly horizontal flat subpaths
 - Y-axis flipped for OpenGL
 - writes multiple segments separated by -1 -1
"""

import argparse
import math
import cmath
from xml.etree import ElementTree as ET
from svgpathtools import parse_path

def cubic_eval(cseg, t):
    """Evaluate svgpathtools segment (CubicBezier/Line) at t -> complex"""
    return cseg.point(t)

def sample_subpath(subpath, nsamp=20):
    pts = []
    for seg in subpath:
        for k in range(nsamp + 1):
            t = k / float(nsamp)
            try:
                pts.append(seg.point(t))
            except Exception:
                pass
    return pts

def bbox_of_points(pts):
    xs = [z.real for z in pts]
    ys = [z.imag for z in pts]
    return (min(xs), min(ys), max(xs), max(ys))

def is_flat_horizontal(pts, crest_w, crest_h):
    """判断是否为横向排线"""
    if len(pts) < 2:
        return False
    minx, miny, maxx, maxy = bbox_of_points(pts)
    w = maxx - minx
    h = maxy - miny
    if w <= 0 or h <= 0:
        return False
    # 判定条件：宽度足够大，高度极小，且高宽比小于阈值
    if w > crest_w * 0.25 and h < max(crest_h * 0.005, 6.0) and (h / w) < 0.03:
        return True
    return False

def main(svg_file, output_file):
    tree = ET.parse(svg_file)
    root = tree.getroot()

    # 解析所有 path
    paths = []
    for elem in root.iter():
        if elem.tag.endswith("path") and elem.get("d"):
            # 过滤带 fill 的路径
            style = (elem.get("style") or "").replace(" ", "").lower()
            fill_attr = elem.get("fill")
            has_fill = (("fill:" in style and "fill:none" not in style)
                        or (fill_attr is not None and fill_attr.lower() != "none"))
            if has_fill:
                continue
            try:
                p = parse_path(elem.get("d"))
                paths.append(p)
            except Exception as e:
                print("[warn] failed to parse path:", e)

    print(f"[info] path elements (after fill-filter): {len(paths)}")

    # 提取所有子路径
    subpaths = []
    for p in paths:
        subs = []
        current = []
        last_end = None
        for seg in p:
            if last_end is not None and abs(seg.start - last_end) > 1e-3:
                if current:
                    subs.append(current)
                    current = []
            current.append(seg)
            last_end = seg.end
        if current:
            subs.append(current)
        subpaths += subs

    print(f"[info] total subpaths={len(subpaths)}")

    # 计算整体范围
    minx = min((seg.start.real for p in paths for seg in p), default=0.0)
    maxx = max((seg.end.real for p in paths for seg in p), default=1.0)
    miny = min((seg.start.imag for p in paths for seg in p), default=0.0)
    maxy = max((seg.end.imag for p in paths for seg in p), default=1.0)
    crest_w = maxx - minx
    crest_h = maxy - miny
    cx = (minx + maxx) / 2
    cy = (miny + maxy) / 2
    scale = 600.0 / max(crest_w, crest_h)

    # 过滤扁平横线
    kept_subpaths = []
    for sp in subpaths:
        pts = sample_subpath(sp, nsamp=24)
        if not pts:
            continue
        if is_flat_horizontal(pts, crest_w, crest_h):
            continue
        kept_subpaths.append(sp)

    print(f"[info] kept_subpaths(after flat-filter): {len(kept_subpaths)}")

    # 写出 savefile.txt（多段格式）
    with open(output_file, "w") as f:
        first_seg = True
        total_pts = 0
        for sp in kept_subpaths:
            if not first_seg:
                f.write("-1 -1\n")
            first_seg = False

            first_curve = True
            for seg in sp:
                # 样本化一个段落为 CubicBezier 格式
                pts = [seg.start, seg.end]
                seq = pts if first_curve else pts[1:]
                for P in seq:
                    xi = int(round((P.real - cx) * scale + 300))
                    yi = int(round((-P.imag + cy) * scale + 300))  # 注意：Y取反
                    f.write(f"{xi} {yi}\n")
                    total_pts += 1
                first_curve = False
        print(f"[done] wrote {output_file} with {total_pts} points, {len(kept_subpaths)} segments.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert SVG to CS3241 savefile.txt (clean version)")
    parser.add_argument("svg_file", help="Input SVG file")
    parser.add_argument("output_txt", help="Output savefile.txt")
    args = parser.parse_args()
    main(args.svg_file, args.output_txt)
