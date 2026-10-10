"""Eigentime32 case: parametric CadQuery model.

Run:  python3 case/eigentime32_case.py [outdir]
Writes one STL and one STEP per printed part, plus an assembly STEP.

Coordinates: origin at the board centre, z = 0 on the board's FRONT face
(display side), +z towards the viewer, +y = 12 o'clock, +x = 3 o'clock
(front view). KiCad positions convert as x = kx - 148.5, y = 105 - ky.
"""
import math
import os
import sys

import cadquery as cq

# ---------------------------------------------------------------- board stack
PCB_R = 20.0          # board radius
PCB_T = 0.8           # JLC04081H-3313
HOLDER_H = 4.06       # Keystone 3034 above the board's back face
FRAME_T = 1.2         # spacer under the panel edges (docs/decisions.md)
TAPE_T = 0.05         # transfer tape bonding the panel's back to the frame (3M 467MP)
PANEL_W, PANEL_H, PANEL_T = 26.6, 30.3, 0.75   # LS013B7DH03 outline
ACTIVE = 23.04        # active area, square
ACTIVE_CY = 1.85      # active-area centre above the board centre (Sharp drawing: 1.78 mm top margin)

Z_BOARD_BACK = -PCB_T
Z_FLOOR_TOP = Z_BOARD_BACK - HOLDER_H      # the holder rests on the floor
Z_PANEL_BOT = FRAME_T + TAPE_T
Z_PANEL_TOP = FRAME_T + PANEL_T

# ---------------------------------------------------------------- case
CLEAR = 0.35          # board edge to bore (panel corners reach r = 20.15)
R_IN = PCB_R + CLEAR
WALL = 1.6
R_OUT = R_IN + WALL
FLOOR_T = 0.9

# window: flat mineral watch crystal pressed into the bezel from the front
WINDOW = "round"      # "round" or "square"
APERTURE_D = 23.4     # round: visible circle; square: side of the visible square
CRYSTAL_D = 26.0      # round crystal diameter (square: CRYSTAL_D x CRYSTAL_D acrylic)
CRYSTAL_T = 1.0
LEDGE_T = 0.4         # bezel material under the crystal

Z_BEZEL_BOT = Z_PANEL_TOP - 0.25          # outside the panel pocket
Z_POCKET = Z_PANEL_TOP + 0.05             # pocket ceiling, 0.05 above the glass
Z_CRYSTAL_BOT = Z_POCKET + LEDGE_T
Z_TOP = Z_CRYSTAL_BOT + CRYSTAL_T         # bezel, crystal and shell rim are flush

LID_GAP = 0.25
LID_T = 1.4
Z_LID_BOT = Z_TOP + LID_GAP
Z_LID_TOP = Z_LID_BOT + LID_T
Z_BOTTOM = Z_FLOOR_TOP - FLOOR_T

# hinge at 9 o'clock, 1.0 mm steel pin (e.g. a cut paper clip)
PIN_D = 1.0
KNUCKLE_R = 1.25
HINGE_X = -(R_OUT + KNUCKLE_R + 0.05)
HINGE_Z = Z_LID_BOT
LID_KNUCKLE = 2.9     # half-length of the lid's centre knuckle
SHELL_KNUCKLE = (3.1, 6.6)

# magnet: 3 x 1 mm N52 disc in the lid, over the CT8132 (U9)
U9_XY = (165.1049 - 148.5, 105 - 99.3353)
MAGNET_D, MAGNET_T = 3.0, 1.0

# side buttons: EVQ-P7A actuators at r = 20.0, pressed through flexure tabs
BUTTONS = {  # name: (kicad x, kicad y)
    "SW2": (134.788, 93.494),
    "SW3": (162.796, 94.228),
}
ACTUATOR_Z = Z_BOARD_BACK - 0.65
TAB_HALF_W = 1.2
SLIT = 0.5
TAB_Z = (Z_FLOOR_TOP + 0.4, Z_BOARD_BACK - 0.2)   # anchored at the bottom, free at the top
NUB_GAP = 0.1                                      # nub face to actuator tip
RIB_TAN = (1.9, 2.6)                               # anti-rotation ribs beside each button body
RIB_R_IN = 18.8

# CR2032 door at 12 o'clock
SLOT_HALF_W = 10.6
SLOT_Z = (Z_FLOOR_TOP, Z_BOARD_BACK + 0.1)   # cell slides in flat against the board
DOOR_CLEAR = 0.1



def slot_end_y(x):
    """Mid-wall y of the slot's flat end face at |x|."""
    return (math.sqrt(R_IN ** 2 - x ** 2) + math.sqrt(R_OUT ** 2 - x ** 2)) / 2


# bow lug at 12 o'clock, above the cell door
LUG_HALF_W = 2.2
LUG_HOLE_D = 1.8

# bezel retention: a rib on the bezel snaps into a groove in the bore
GROOVE_Z = (Z_TOP - 0.85, Z_TOP - 0.5)
GROOVE_DEPTH = 0.2
RIB_INTERFERENCE = 0.15

# frame clamp: ears outside the panel at 3, 9 and 12 o'clock reach up to the
# bezel's underside, so the bezel presses frame, board and cell holder onto the
# floor and no load goes through the glass. Crush ribs on the ears set the preload.
EARS = [  # (x0, x1, y0, y1), clipped to the bore
    (PANEL_W / 2 + 0.3, R_IN, -1.2, 4.8),
    (-R_IN, -PANEL_W / 2 - 0.3, -1.2, 4.8),
    (-3.0, 3.0, PANEL_H / 2 + 0.3, R_IN),
]
EAR_LINK = 0.6        # ears join the frame under the panel edge by this much
CRUSH = 0.15          # crush-rib height above the bezel underside (net ~0.1 after the snap's play)
CRUSH_W = 0.6

# lid catch at 3 o'clock
CATCH_DEG = 10.0


def ring(r_out, r_in, z0, z1):
    return (cq.Workplane("XY").workplane(offset=z0).circle(r_out).circle(r_in)
            .extrude(z1 - z0))


def disc(r, z0, z1, x=0.0, y=0.0):
    return cq.Workplane("XY").workplane(offset=z0).center(x, y).circle(r).extrude(z1 - z0)


def box(x0, x1, y0, y1, z0, z1):
    return (cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0)
            .translate(((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2)))


def sector(r_out, r_in, deg0, deg1, z0, z1):
    """Annular sector between two angles (degrees, CCW from +x)."""
    a0, a1 = math.radians(deg0), math.radians(deg1)
    am = (a0 + a1) / 2
    pts_out = [(r_out * math.cos(a), r_out * math.sin(a)) for a in (a0, am, a1)]
    pts_in = [(r_in * math.cos(a), r_in * math.sin(a)) for a in (a1, am, a0)]
    return (cq.Workplane("XY").workplane(offset=z0)
            .moveTo(*pts_out[0]).threePointArc(pts_out[1], pts_out[2])
            .lineTo(*pts_in[0]).threePointArc(pts_in[1], pts_in[2]).close()
            .extrude(z1 - z0))


def radial_box(angle_deg, r0, r1, t0, t1, z0, z1):
    """Box spanning radius r0..r1 and tangential offset t0..t1 along a ray."""
    return box(r0, r1, t0, t1, z0, z1).rotate((0, 0, 0), (0, 0, 1), angle_deg)


def kicad_xy(kx, ky):
    return kx - 148.5, 105 - ky


def button_angle(name):
    x, y = kicad_xy(*BUTTONS[name])
    return math.degrees(math.atan2(y, x))


def y_cylinder(r, x, z, y0, y1):
    return (cq.Workplane("XZ").workplane(offset=-y1).center(x, z).circle(r)
            .extrude(y1 - y0))


def x_cylinder(r, y, z, x0, x1):
    return (cq.Workplane("YZ").workplane(offset=x0).center(y, z).circle(r)
            .extrude(x1 - x0))


# ---------------------------------------------------------------- parts
def shell():
    s = disc(R_OUT, Z_BOTTOM, Z_TOP)
    s = s.faces("<Z").edges().fillet(1.0)
    s = s.cut(disc(R_IN, Z_FLOOR_TOP, Z_TOP + 1))
    # bezel groove
    s = s.cut(ring(R_IN + GROOVE_DEPTH, R_IN - 0.01, *GROOVE_Z))

    # cell door slot at 12 o'clock (through the wall only, floor kept)
    s = s.cut(box(-SLOT_HALF_W, SLOT_HALF_W, 0, R_OUT + 1, *SLOT_Z)
              .intersect(ring(R_OUT + 1, R_IN - 0.5, *SLOT_Z)))
    # dimples for the door's click bumps
    for sx in (-1, 1):
        s = s.cut(cq.Workplane("XY").workplane(offset=SLOT_Z[0])
                  .center(sx * SLOT_HALF_W, slot_end_y(SLOT_HALF_W)).circle(0.35)
                  .extrude(SLOT_Z[1] - SLOT_Z[0]))

    # bow lug
    lug = box(-LUG_HALF_W, LUG_HALF_W, R_OUT - 1.0, R_OUT + 3.4, SLOT_Z[1] + 0.5, Z_TOP)
    lug = lug.edges("|X").fillet(0.8)
    lug = lug.cut(x_cylinder(LUG_HOLE_D / 2, R_OUT + 1.9, (SLOT_Z[1] + 0.5 + Z_TOP) / 2,
                             -LUG_HALF_W - 1, LUG_HALF_W + 1))
    lug = lug.cut(disc(R_IN, Z_FLOOR_TOP, Z_TOP + 1))
    s = s.union(lug)

    # buttons: flexure tab, inner nub, outer press pad, anti-rotation ribs
    for name in BUTTONS:
        a = button_angle(name)
        z0, z1 = TAB_Z
        for side in (-1, 1):
            t0 = side * TAB_HALF_W
            t1 = side * (TAB_HALF_W + SLIT)
            s = s.cut(radial_box(a, R_IN - 0.5, R_OUT + 0.5, min(t0, t1), max(t0, t1), z0, z1 + SLIT))
        s = s.cut(radial_box(a, R_IN - 0.5, R_OUT + 0.5, -TAB_HALF_W - SLIT, TAB_HALF_W + SLIT,
                             z1, z1 + SLIT))
        nub_face = PCB_R + NUB_GAP
        s = s.union(radial_box(a, nub_face, R_IN + 0.2, -0.8, 0.8,
                               ACTUATOR_Z - 0.4, min(ACTUATOR_Z + 0.4, z1)))
        s = s.union(radial_box(a, R_OUT - 0.2, R_OUT + 0.45, -1.0, 1.0,
                               ACTUATOR_Z - 0.7, min(ACTUATOR_Z + 0.4, z1)))
        for side in (-1, 1):
            t = sorted((side * RIB_TAN[0], side * RIB_TAN[1]))
            s = s.union(radial_box(a, RIB_R_IN, R_IN + 0.2, t[0], t[1],
                                   Z_FLOOR_TOP - 0.1, Z_BOARD_BACK - 0.2))

    # hinge knuckles at 9 o'clock
    for sgn in (-1, 1):
        y0, y1 = sorted((sgn * SHELL_KNUCKLE[0], sgn * SHELL_KNUCKLE[1]))
        k = y_cylinder(KNUCKLE_R, HINGE_X, HINGE_Z, y0, y1)
        k = k.union(box(HINGE_X, -R_OUT + 0.8, y0, y1, HINGE_Z - KNUCKLE_R, Z_TOP))
        k = k.cut(disc(R_IN, Z_FLOOR_TOP, Z_TOP + 1))
        k = k.cut(y_cylinder(PIN_D / 2 - 0.03, HINGE_X, HINGE_Z, y0 - 1, y1 + 1))  # press fit
        s = s.union(k)

    # catch groove at 3 o'clock
    s = s.cut(sector(R_OUT + 0.1, R_OUT - 0.3, -CATCH_DEG - 2, CATCH_DEG + 2,
                     Z_TOP - 1.0, Z_TOP - 0.5))
    return s


def bezel():
    b = ring(R_IN - 0.05, 0, Z_BEZEL_BOT, Z_TOP)
    b = b.union(ring(R_IN - 0.05 + RIB_INTERFERENCE + 0.05, R_IN - 0.3,
                     GROOVE_Z[0] + 0.05, GROOVE_Z[1] - 0.05))
    # pocket that locates the panel
    b = b.cut(box(-PANEL_W / 2 - 0.1, PANEL_W / 2 + 0.1, -PANEL_H / 2 - 0.1, PANEL_H / 2 + 0.1,
                  Z_BEZEL_BOT - 1, Z_POCKET))
    # relief over the folded FPC at 6 o'clock
    b = b.cut(box(-5.5, 5.5, -R_IN, -PANEL_H / 2 + 1.5, Z_BEZEL_BOT - 1, Z_POCKET + 0.15))
    if WINDOW == "round":
        b = b.cut(disc(CRYSTAL_D / 2 + 0.05, Z_CRYSTAL_BOT, Z_TOP + 1, 0, ACTIVE_CY))
        b = b.cut(disc(APERTURE_D / 2, Z_BEZEL_BOT - 1, Z_TOP + 1, 0, ACTIVE_CY))
    else:
        h = CRYSTAL_D / 2 + 0.05
        b = b.cut(box(-h, h, ACTIVE_CY - h, ACTIVE_CY + h, Z_CRYSTAL_BOT, Z_TOP + 1))
        a = APERTURE_D / 2
        b = b.cut(box(-a, a, ACTIVE_CY - a, ACTIVE_CY + a, Z_BEZEL_BOT - 1, Z_TOP + 1)
                  .edges("|Z").fillet(1.0))
    return b


def frame(crush=True):
    w, h = PANEL_W / 2, PANEL_H / 2
    f = box(-w, w, -h, h, 0, FRAME_T).intersect(disc(R_IN - 0.1, -1, FRAME_T + 1))
    bore = disc(R_IN - 0.1, -1, Z_BEZEL_BOT + 1)
    for x0, x1, y0, y1 in EARS:
        f = f.union(box(x0, x1, y0, y1, 0, Z_BEZEL_BOT).intersect(bore))
        # link the ear to the frame under the panel edge, below the panel
        if y0 > 0:
            f = f.union(box(x0, x1, h - EAR_LINK, y0, 0, FRAME_T))
        else:
            lx = (w - EAR_LINK, x0) if x0 > 0 else (x1, -w + EAR_LINK)
            f = f.union(box(lx[0], lx[1], y0, y1, 0, FRAME_T))
        if crush:  # two ribs across each ear
            if y0 > 0:
                for t in (x0 + 1.2, x1 - 1.2):
                    f = f.union(box(t - CRUSH_W / 2, t + CRUSH_W / 2, y0 + 0.5, y1 - 0.8,
                                    Z_BEZEL_BOT, Z_BEZEL_BOT + CRUSH))
            else:
                for t in (y0 + 1.2, y1 - 1.2):
                    xa, xb = (x0 + 0.5, x1 - 0.9) if x0 > 0 else (x0 + 0.9, x1 - 0.5)
                    f = f.union(box(xa, xb, t - CRUSH_W / 2, t + CRUSH_W / 2,
                                    Z_BEZEL_BOT, Z_BEZEL_BOT + CRUSH))
    f = f.cut(box(-w + 1.0, w - 1.0, -h + 1.0, h - 1.0, -1, FRAME_T + 1))
    f = f.union(box(11.2, 12.4, -4.4, -0.8, 0, FRAME_T))           # jog round C21 keeps it one piece
    f = f.cut(box(12.0, w + 1, -3.6, -1.6, -1, FRAME_T + 1))       # C21
    f = f.cut(box(-5.5, 5.5, -h - 1, -h + 3.0, -1, FRAME_T + 1))   # FPC fold
    return f


def lid():
    l = disc(R_OUT, Z_LID_BOT, Z_LID_TOP)
    l = l.faces(">Z").edges().chamfer(0.5)
    # centre knuckle and its bridge
    k = y_cylinder(KNUCKLE_R, HINGE_X, HINGE_Z, -LID_KNUCKLE, LID_KNUCKLE)
    k = k.union(box(HINGE_X, -R_OUT + 1.0, -LID_KNUCKLE, LID_KNUCKLE, HINGE_Z, Z_LID_TOP))
    l = l.union(k)
    l = l.cut(y_cylinder(PIN_D / 2 + 0.1, HINGE_X, HINGE_Z, -LID_KNUCKLE - 1, LID_KNUCKLE + 1))
    # catch lip with an inward bump, and a thumb tab
    lip = sector(R_OUT + 1.0, R_OUT + 0.2, -CATCH_DEG, CATCH_DEG, Z_TOP - 1.1, Z_LID_TOP)
    lip = lip.union(sector(R_OUT + 0.25, R_OUT - 0.15, -CATCH_DEG + 3, CATCH_DEG - 3,
                           Z_TOP - 0.95, Z_TOP - 0.6))
    l = l.union(lip)
    tab = box(R_OUT - 1, R_OUT + 2.2, -3.0, 3.0, Z_LID_BOT, Z_LID_TOP).edges("|Z").fillet(1.2)
    l = l.union(tab)
    # magnet pocket on the underside, over U9
    l = l.cut(disc(MAGNET_D / 2 + 0.05, Z_LID_BOT - 0.1, Z_LID_BOT + MAGNET_T + 0.05, *U9_XY))
    return l


def door():
    z0, z1 = SLOT_Z[0] + DOOR_CLEAR, SLOT_Z[1] - DOOR_CLEAR
    d = (box(-SLOT_HALF_W + DOOR_CLEAR, SLOT_HALF_W - DOOR_CLEAR, 0, R_OUT + 1, z0, z1)
         .intersect(ring(R_OUT, R_IN, z0, z1)))
    for sx in (-1, 1):  # click bumps
        d = d.union(cq.Workplane("XY").workplane(offset=z0 + 0.6)
                    .center(sx * (SLOT_HALF_W - DOOR_CLEAR), slot_end_y(SLOT_HALF_W)).circle(0.3)
                    .extrude(z1 - z0 - 1.2))
    d = d.cut(box(-4, 4, R_OUT - 0.5, R_OUT + 1, (z0 + z1) / 2 - 0.35, (z0 + z1) / 2 + 0.35))  # nail slot
    return d


PARTS = {
    "shell": shell,
    "bezel": bezel,
    "frame": frame,
    "lid": lid,
    "cell_door": door,
}


def build():
    return {name: fn() for name, fn in PARTS.items()}


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "out")
    os.makedirs(out, exist_ok=True)
    parts = build()
    asm = cq.Assembly()
    for name, wp in parts.items():
        cq.exporters.export(wp, os.path.join(out, f"{name}.stl"), tolerance=0.02, angularTolerance=0.1)
        cq.exporters.export(wp, os.path.join(out, f"{name}.step"))
        asm.add(wp, name=name)
    asm.save(os.path.join(out, "eigentime32_case.step"))
    print(f"thickness {Z_LID_TOP - Z_BOTTOM:.2f} mm (case {Z_TOP - Z_BOTTOM:.2f} + lid), "
          f"diameter {2 * R_OUT:.2f} mm")
