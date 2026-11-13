"""
Brillo medio por franjas de distancia en capturas de la prueba de visibilidad (-LabEyeTest).

Para cada fila de una franja central de la imagen calcula a qué distancia está el piso
(altura de cámara, inclinación y FOV salen del log [AutoShot]) y promedia la luma (sRGB 0-255)
en 0-3 m, 3-8 m y 8-15 m. Compara los casos y verifica la regla: con farol >= sin farol.

Uso: python Scripts/eye_bands.py <carpeta con eye_<caso>.log> <carpeta de capturas>
"""
import math
import re
import sys
from pathlib import Path

import numpy as np
from PIL import Image

BANDS = [(0, 3), (3, 8), (8, 15)]
CASES = [("none", "Sin farol (60 s a oscuras)"), ("hand", "Farol en la mano"), ("floor", "Farol en el piso a 3 m")]


def parse_log(p):
    s = p.read_text(encoding="utf-8", errors="ignore")
    cam = re.search(r"\[AutoShot\] cam=V\(X=([-\d.]+), Y=([-\d.]+), Z=([-\d.]+)\) rot=R\(P=([-\d.]+)(?:, Y=([-\d.]+))?(?:, R=([-\d.]+))?\) fov=([\d.]+)", s)
    corr = re.search(r"\[EyeTest\].*\(([\d.]+) m\)", s)
    eye = re.findall(r"Ojos: adaptación=([\d.]+) .*exposición=([\d.]+)", s)
    if not cam:
        raise SystemExit(f"sin [AutoShot] cam en {p}")
    z, pitch, fov = float(cam.group(3)), float(cam.group(4)), float(cam.group(7))
    return z, pitch, fov, float(corr.group(1)) if corr else 99.0, eye[-1] if eye else None


def bands(img_path, cam_z, pitch, fov, corridor_m):
    im = np.asarray(Image.open(img_path).convert("RGB")).astype(np.float32)
    h, w, _ = im.shape
    luma = 0.2126 * im[..., 0] + 0.7152 * im[..., 1] + 0.0722 * im[..., 2]
    f = (w / 2) / math.tan(math.radians(fov) / 2)
    strip = luma[:, int(w * 0.42):int(w * 0.58)]
    out = []
    for lo, hi in BANDS:
        vals = []
        for y in range(h):
            elev = pitch + math.degrees(math.atan((h / 2 - y) / f))
            if elev >= -0.5:
                continue
            d = (cam_z / 100.0) / math.tan(math.radians(-elev))
            if lo <= d < min(hi, corridor_m - 0.3):
                vals.append(strip[y].mean())
        out.append((float(np.mean(vals)) if vals else float("nan"), len(vals)))
    return out


def main():
    logs, shots = Path(sys.argv[1]), Path(sys.argv[2])
    res = {}
    for key, label in CASES:
        z, pitch, fov, corr, eye = parse_log(logs / f"eye_{key}.log")
        res[key] = bands(shots / f"Eye_{key}.png", z, pitch, fov, corr)
        print(f"{label:30s} cam z={z:.0f} pitch={pitch:.1f} fov={fov:.0f} pasillo={corr:.1f} m ojos(adapt,EV)={eye}")
    print()
    print("| Franja | " + " | ".join(l for _, l in CASES) + " | Regla (con farol >= sin farol) |")
    print("|---|" + "---|" * len(CASES) + "---|")
    ok_all = True
    for i, (lo, hi) in enumerate(BANDS):
        base = res["none"][i][0]
        cells = [f"{res[k][i][0]:.1f}" for k, _ in CASES]
        ok = all(res[k][i][0] >= base - 0.5 for k in ("hand", "floor"))  # 0,5 de margen por ruido de cuantización
        ok_all &= ok
        print(f"| {lo}–{hi} m | " + " | ".join(cells) + f" | {'✅' if ok else '❌'} |")
    print(f"\nREGLA: {'CUMPLE' if ok_all else 'NO CUMPLE'}")


if __name__ == "__main__":
    main()
