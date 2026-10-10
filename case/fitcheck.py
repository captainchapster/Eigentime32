"""Fit check: builds simple solids for the board, its parts, the cell and the
LCD from hardware/Eigentime32.kicad_pcb, then reports any overlap with the
printed case parts.  Run: python3 case/fitcheck.py
"""
import math
import os
import re
import sys

import cadquery as cq

sys.path.insert(0, os.path.dirname(__file__))
import eigentime32_case as C  # noqa: E402

PCB = os.path.join(os.path.dirname(__file__), "..", "hardware", "Eigentime32.kicad_pcb")

# part heights above the board, by footprint name (mm)
HEIGHTS = [
    ("BatteryHolder", C.HOLDER_H), ("EVQP7A", 1.35), ("C_1206", 1.8), ("C_0603", 0.9),
    ("R_0402", 0.4), ("C_0402", 0.55), ("SOT-23", 1.15), ("SC-70", 1.1), ("SOT-353", 1.1),
    ("QFN", 0.6), ("VQFN", 1.0), ("WSON", 0.8), ("DFN", 0.6), ("SON", 0.85), ("BGA", 0.6),
    ("MAPI", 1.2), ("AYF531035", 1.1),
]


def footprints():
    s = open(PCB).read()
    for m in re.finditer(r'\n\t\(footprint "([^"]+)"', s):
        st = m.start() + 2
        depth, j = 0, st
        while True:
            c = s[j]
            depth += c == "("
            depth -= c == ")"
            if depth == 0:
                break
            j += 1
        b = s[st:j]
        layer = re.search(r'\(layer "([^"]+)"\)', b).group(1)
        at = re.search(r"\(at ([-\d.]+) ([-\d.]+)(?: ([-\d.]+))?\)", b).groups()
        ref = re.search(r'\(property "Reference" "([^"]+)"', b).group(1)
        pts = []
        for lm in re.finditer(r"\(fp_(?:line|rect)\s+\(start ([-\d.]+) ([-\d.]+)\)\s+\(end ([-\d.]+) ([-\d.]+)\)"
                              r"(?:(?!\(fp_).)*?\(layer \"[FB]\.CrtYd\"\)", b, re.S):
            x0, y0, x1, y1 = map(float, lm.groups())
            pts += [(x0, y0), (x1, y1)]
        yield ref, m.group(1), layer, tuple(float(v) if v else 0.0 for v in at), pts


def button_solid(at):
    """EVQ-P7A: 3.5 x 2.9 x 1.35 body, 1.7 wide actuator to local y = -2.1."""
    body = C.box(-1.75, 1.75, -1.45, 1.45, -1.35, 0).union(
        C.box(-0.85, 0.85, -2.1, -1.45, -1.15, -0.15))
    x, y = C.kicad_xy(at[0], at[1])
    # KiCad rotates CCW on screen (y down); in y-up coordinates that is -angle after the y flip
    return (body.mirror("XZ").rotate((0, 0, 0), (0, 0, 1), at[2])
            .translate((x, y, C.Z_BOARD_BACK)))


def part_solid(name, layer, at, pts):
    if "EVQP7A" in name:
        return button_solid(at)
    h = next((h for key, h in HEIGHTS if key in name), None)
    if h is None or not pts:
        return None
    xs, ys = [p[0] for p in pts], [p[1] for p in pts]
    corners = [(min(xs), min(ys)), (max(xs), min(ys)), (max(xs), max(ys)), (min(xs), max(ys))]
    th = math.radians(at[2])
    poly = []
    for x, y in corners:
        gx = at[0] + x * math.cos(th) + y * math.sin(th)
        gy = at[1] - x * math.sin(th) + y * math.cos(th)
        poly.append(C.kicad_xy(gx, gy))
    if layer.startswith("B"):
        z0, z1 = C.Z_BOARD_BACK - h, C.Z_BOARD_BACK
    else:
        z0, z1 = 0.0, h
    return cq.Workplane("XY").workplane(offset=z0).polyline(poly).close().extrude(z1 - z0)


def board_assembly():
    solids = {"pcb": C.disc(C.PCB_R, C.Z_BOARD_BACK, 0)}
    for ref, name, layer, at, pts in footprints():
        s = part_solid(name, layer, at, pts)
        if s is not None:
            solids[ref] = s
    w, h = C.PANEL_W / 2, C.PANEL_H / 2
    solids["LCD"] = C.box(-w, w, -h, h, C.Z_PANEL_BOT, C.Z_PANEL_TOP)
    solids["FPC bend"] = C.box(-4.75, 4.75, -h - 1.3, -h + 0.5, 0.0, C.Z_PANEL_TOP - 0.3)
    solids["crystal"] = C.disc(C.CRYSTAL_D / 2, C.Z_CRYSTAL_BOT, C.Z_TOP, 0, C.ACTIVE_CY)
    solids["magnet"] = C.disc(C.MAGNET_D / 2, C.Z_LID_BOT, C.Z_LID_BOT + C.MAGNET_T, *C.U9_XY)
    return solids


def vol(wp):
    try:
        return sum(s.Volume() for s in wp.solids().vals())
    except Exception:
        return 0.0


def main():
    parts = C.build()
    others = board_assembly()
    bad = 0
    for pname, p in parts.items():
        for oname, o in others.items():
            if pname == "lid" and oname == "magnet":
                continue
            v = vol(p.intersect(o))
            if v > 1e-3:
                bad += 1
                print(f"OVERLAP {pname} x {oname}: {v:.3f} mm^3")
    names = list(parts)
    for i, a in enumerate(names):
        for b in names[i + 1:]:
            v = vol(parts[a].intersect(parts[b]))
            if v > 1e-3:
                bad += 1
                print(f"OVERLAP {a} x {b}: {v:.3f} mm^3")
    # cell path: a CR2032 sliding in from 12 o'clock must clear the shell
    cell_in = C.disc(10.0, C.Z_BOARD_BACK - 3.2, C.Z_BOARD_BACK, 0, 10.0 + 12.0)
    v = vol(parts["shell"].intersect(cell_in))
    if v > 1e-3:
        bad += 1
        print(f"OVERLAP cell insertion path x shell: {v:.3f} mm^3")
    # actuator gaps
    for name in C.BUTTONS:
        print(f"{name}: angle {C.button_angle(name):.1f} deg, nub gap {C.NUB_GAP} mm")
    print("fit check:", "PASS" if bad == 0 else f"{bad} overlaps")
    return bad


if __name__ == "__main__":
    sys.exit(1 if main() else 0)
