#!/usr/bin/env python3
"""Turn the host preview's PBM frames into one PNG that mimics the round window.
Usage: python3 firmware/tools/pbm2png.py build/host out.png"""
import glob
import os
import sys

from PIL import Image, ImageDraw

SCALE = 3
R = 63.5  # visible window radius in pixels (bezel aperture 23.4 mm / 0.18 mm per px)


def main(src, out):
    files = sorted(glob.glob(os.path.join(src, "face_*.pbm")))
    tile = 128 * SCALE
    sheet = Image.new("RGB", (len(files) * (tile + 20) + 20, tile + 60), (236, 234, 228))
    d = ImageDraw.Draw(sheet)
    for i, f in enumerate(files):
        im = Image.open(f).convert("1").resize((tile, tile), Image.NEAREST)
        px = im.load()
        rgb = Image.new("RGB", (tile, tile), (170, 164, 152))
        p2 = rgb.load()
        c = tile / 2
        for y in range(tile):
            for x in range(tile):
                if (x - c + 0.5) ** 2 + (y - c + 0.5) ** 2 <= (R * SCALE) ** 2:
                    p2[x, y] = (34, 36, 40) if px[x, y] == 0 else (196, 200, 192)
        x0 = 20 + i * (tile + 20)
        sheet.paste(rgb, (x0, 20))
        d.text((x0, tile + 30), os.path.basename(f)[5:-4].replace("_", " "), fill=(60, 60, 60))
    sheet.save(out)
    print("wrote", out)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
