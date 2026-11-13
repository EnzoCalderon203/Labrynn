"""Chequeo numérico de SourceAudio/: formato, duración, pico, RMS, clipping, DC y empalme de loops."""
from pathlib import Path

import numpy as np
import soundfile as sf

ROOT = Path(__file__).resolve().parents[2] / "SourceAudio"
bad = 0
for p in sorted(ROOT.rglob("*.wav")):
    info = sf.info(p)
    x, sr = sf.read(p, always_2d=True)
    m = x.mean(axis=1)
    peak = 20 * np.log10(max(1e-9, np.abs(x).max()))
    rms = 20 * np.log10(max(1e-9, np.sqrt(np.mean(m ** 2))))
    clip = int(np.sum(np.abs(x) >= 0.999))
    dc = float(np.mean(m))
    issues = []
    if info.samplerate != 48000 or info.subtype != "PCM_16":
        issues.append(f"formato {info.samplerate} {info.subtype}")
    if clip > 10:
        issues.append(f"clipping {clip}")
    if abs(dc) > 0.01:
        issues.append(f"DC {dc:.3f}")
    if rms < -50:
        issues.append("casi silencio")
    if "loop" in p.stem:
        # salto en el empalme comparado con el paso típico entre muestras
        step = np.median(np.abs(np.diff(m))) + 1e-9
        seam = abs(m[0] - m[-1]) / step
        if seam > 25:
            issues.append(f"empalme {seam:.0f}x")
    bad += bool(issues)
    rel = p.relative_to(ROOT).as_posix()
    print(f"{rel:34s} {info.channels}ch {info.duration:6.2f}s pico {peak:6.1f} rms {rms:6.1f} {'  <-- ' + ', '.join(issues) if issues else ''}")
print(f"\nproblemas: {bad}")
