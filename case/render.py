"""Preview renders of the case with VTK.
Run headless: xvfb-run -a python3 case/render.py [outdir]   (PNGs in case/renders by default)
"""
import os
import sys

import vtk

sys.path.insert(0, os.path.dirname(__file__))
import eigentime32_case as C  # noqa: E402
import fitcheck as F  # noqa: E402

COL = {"shell": (0.80, 0.77, 0.72), "bezel": (0.70, 0.67, 0.62), "lid": (0.84, 0.81, 0.76),
       "cell_door": (0.62, 0.59, 0.54), "frame": (0.89, 0.64, 0.23), "pcb": (0.18, 0.42, 0.27),
       "LCD": (0.20, 0.23, 0.26), "crystal": (0.65, 0.82, 0.95), "parts": (0.30, 0.32, 0.34),
       "magnet": (0.55, 0.57, 0.60), "cell": (0.85, 0.85, 0.87)}


def actor(wp, color, opacity=1.0):
    app = vtk.vtkAppendPolyData()
    for s in wp.solids().vals():
        app.AddInputData(s.toVtkPolyData(0.02, 0.1, normals=True))
    app.Update()
    m = vtk.vtkPolyDataMapper()
    m.SetInputData(app.GetOutput())
    a = vtk.vtkActor()
    a.SetMapper(m)
    p = a.GetProperty()
    p.SetColor(*color)
    p.SetOpacity(opacity)
    p.SetSpecular(0.25)
    p.SetSpecularPower(30)
    p.SetDiffuse(0.85)
    p.SetAmbient(0.15)
    return a


def shot(items, path, cam_pos, view_up=(0, 0, 1), size=(1100, 900), title=None, zoom=1.0):
    r = vtk.vtkRenderer()
    r.SetBackground(1, 1, 1)
    r.SetBackground2(0.86, 0.88, 0.92)
    r.GradientBackgroundOn()
    r.SetUseDepthPeeling(1)
    for wp, color, op in items:
        r.AddActor(actor(wp, color, op))
    if title:
        t = vtk.vtkTextActor()
        t.SetInput(title)
        t.GetTextProperty().SetFontSize(24)
        t.GetTextProperty().SetColor(0.15, 0.15, 0.15)
        t.SetPosition(20, size[1] - 45)
        r.AddActor2D(t)
    cam = r.GetActiveCamera()
    cam.SetFocalPoint(0, 0, 0)
    cam.SetPosition(*cam_pos)
    cam.SetViewUp(*view_up)
    r.ResetCamera()
    cam.Zoom(zoom)
    light = vtk.vtkLight()
    light.SetLightTypeToCameraLight()
    light.SetPosition(0.4, 0.8, 1)
    r.AddLight(light)
    rw = vtk.vtkRenderWindow()
    rw.SetOffScreenRendering(1)
    rw.SetMultiSamples(8)
    rw.SetAlphaBitPlanes(1)
    rw.AddRenderer(r)
    rw.SetSize(*size)
    rw.Render()
    w = vtk.vtkWindowToImageFilter()
    w.SetInput(rw)
    w.Update()
    pw = vtk.vtkPNGWriter()
    pw.SetFileName(path)
    pw.SetInputConnection(w.GetOutputPort())
    pw.Write()
    print("wrote", path)


def main(out):
    os.makedirs(out, exist_ok=True)
    P = C.build()
    B = F.board_assembly()
    parts = None
    for k, s in B.items():
        if k in ("pcb", "LCD", "crystal", "magnet", "FPC bend"):
            continue
        parts = s if parts is None else parts.union(s)
    hinge = ((C.HINGE_X, 0, C.HINGE_Z), (C.HINGE_X, 1, C.HINGE_Z))
    lid_open = P["lid"].rotate(*hinge, -110)
    mag_open = B["magnet"].rotate(*hinge, -110)
    T = f"{2 * C.R_OUT:.1f} mm across, {C.Z_LID_TOP - C.Z_BOTTOM:.1f} mm thick"

    shot([(P["shell"], COL["shell"], 1), (P["bezel"], COL["bezel"], 1), (B["LCD"], COL["LCD"], 1),
          (B["crystal"], COL["crystal"], 0.35), (lid_open, COL["lid"], 1),
          (mag_open, COL["magnet"], 1)],
         os.path.join(out, "case_front_open.png"), (60, -90, 80),
         title="Lid open: round window over the display. " + T)

    shot([(P["shell"], COL["shell"], 1), (P["bezel"], COL["bezel"], 1), (P["lid"], COL["lid"], 1)],
         os.path.join(out, "case_closed.png"), (40, -70, 90), title="Closed: hinge at 9, catch at 3, bow lug at 12")

    shot([(P["shell"], COL["shell"], 1), (P["cell_door"], COL["cell_door"], 1),
          (P["lid"], COL["lid"], 1)],
         os.path.join(out, "case_back.png"), (-30, 80, -90), view_up=(0, 0, -1),
         title="Back: cell door under the bow lug, button pads at 10 and 2")

    dz = {"cell_door": -14, "shell": -6, "frame": 6, "bezel": 13, "lid": 22}
    items = [(P[k].translate((0, 0, d)), COL[k], 1) for k, d in dz.items()]
    items += [(B["pcb"].translate((0, 0, 1)), COL["pcb"], 1),
              (parts.translate((0, 0, 1)), COL["parts"], 1),
              (B["LCD"].translate((0, 0, 9)), COL["LCD"], 1),
              (B["crystal"].translate((0, 0, 17)), COL["crystal"], 0.5),
              (B["magnet"].translate((0, 0, 20)), COL["magnet"], 1)]
    shot(items, os.path.join(out, "case_exploded.png"), (70, -100, 45), size=(1000, 1100),
         title="Exploded: door, shell, board, frame, LCD, bezel, crystal, magnet, lid")

    cut = C.box(-30, 30, -30, 0, -20, 20)
    sect = [(P[k].cut(cut), COL[k], 1) for k in ("shell", "bezel", "frame", "lid", "cell_door")]
    sect += [(B["pcb"].cut(cut), COL["pcb"], 1), (parts.cut(cut), COL["parts"], 1),
             (B["LCD"].cut(cut), COL["LCD"], 1), (B["crystal"].cut(cut), COL["crystal"], 0.6)]
    shot(sect, os.path.join(out, "case_section.png"), (8, -120, 18), size=(1300, 650), zoom=1.5,
         title="Section at 9-3 o'clock: cell holder, board, frame, LCD, crystal, lid")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "renders"))
