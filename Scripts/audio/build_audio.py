"""
Genera todo el audio de Labrynn en SourceAudio/ (WAV 16-bit 48 kHz) y SourceAudio/manifest.json.

- Sintetizado 100 % acá (numpy/scipy): ambiente, fuego, viento, goteo, latido, Minotauro, música, stingers, UI.
- Procesado desde packs CC0 de Kenney (Impact Sounds, RPG Audio) en Tools/downloads/kenney/.
- Mono para todo lo 3D; estéreo solo para música, stingers y UI.

Uso (desde la raíz del repo):
    Tools\\.venv\\Scripts\\python.exe Scripts\\audio\\build_audio.py
Determinista (seed fija): regenerar da los mismos archivos.
"""
import json
import shutil
from fractions import Fraction
from pathlib import Path

import numpy as np
import soundfile as sf
from scipy import signal

SR = 48000
ROOT = Path(__file__).resolve().parents[2]
KEN = ROOT / "Tools" / "downloads" / "kenney"
OUT = ROOT / "SourceAudio"
rng = np.random.default_rng(20261113)

MANIFEST = {}   # evento -> datos para el editor
SOURCES = {}    # archivo -> originales de Kenney usados


# ------------------------------------------------------------------ utilidades DSP
def n_of(d):
    return int(round(d * SR))


def tt(d):
    return np.arange(n_of(d)) / SR


def noise(d):
    return rng.standard_normal(n_of(d))


def filt(x, kind, f, order=4):
    sos = signal.butter(order, f, btype=kind, fs=SR, output="sos")
    return signal.sosfilt(sos, x, axis=0)


def resonator(x, f, q):
    b, a = signal.iirpeak(f, q, fs=SR)
    return signal.lfilter(b, a, x)


def env_exp(n, tau, attack=0.002):
    t = np.arange(n) / SR
    e = np.exp(-t / tau)
    a = min(n, n_of(attack))
    if a > 1:
        e[:a] *= np.linspace(0, 1, a)
    r = min(n, n_of(0.015))  # cierre suave: evita el clic al final del segmento
    if r > 1:
        e[-r:] *= np.linspace(1, 0, r)
    return e


def env_ahr(n, a, h, r):
    """Ataque lineal, sostén, release (segundos)."""
    e = np.zeros(n)
    na, nh, nr = n_of(a), n_of(h), n_of(r)
    i = 0
    seg = min(na, n - i); e[i:i + seg] = np.linspace(0, 1, na)[:seg]; i += seg
    seg = min(nh, n - i); e[i:i + seg] = 1; i += seg
    seg = min(nr, n - i); e[i:i + seg] = np.linspace(1, 0, nr)[:seg]; i += seg
    return e


def sweep_noise(d, f0, f1, q=2.0, steps=24):
    """Ruido por un pasa-banda que barre de f0 a f1 (whoosh)."""
    x = noise(d)
    out = np.zeros_like(x)
    edges = np.linspace(0, len(x), steps + 1).astype(int)
    for i in range(steps):
        fc = f0 * (f1 / f0) ** (i / max(1, steps - 1))
        lo, hi = fc / (1 + 1 / q), min(fc * (1 + 1 / q), SR / 2 - 100)
        seg = filt(x[max(0, edges[i] - 2000):edges[i + 1]], "bandpass", [lo, hi], 2)
        seg = seg[-(edges[i + 1] - edges[i]):]
        out[edges[i]:edges[i + 1]] = seg
    return out


def sine_glide(d, f0, f1, curve=4.0):
    t = tt(d)
    f = f1 + (f0 - f1) * np.exp(-curve * t / d)
    return np.sin(2 * np.pi * np.cumsum(f) / SR)


def saw(freq, d, kmax=40):
    """Diente de sierra limitado en banda (aditivo). freq: escalar o array por muestra."""
    n = n_of(d)
    f = np.full(n, freq) if np.isscalar(freq) else freq[:n]
    ph = 2 * np.pi * np.cumsum(f) / SR
    out = np.zeros(n)
    kmax = int(min(kmax, (SR / 2) / max(1.0, float(np.max(f)))))
    for k in range(1, kmax + 1):
        out += np.sin(k * ph) / k
    return out * 0.6


def pitch(x, ratio):
    """Cambia la velocidad (ratio<1 = más grave y largo)."""
    fr = Fraction(ratio).limit_denominator(64)
    return signal.resample_poly(x, fr.denominator, fr.numerator, axis=0)


def make_ir(decay, damp=5000.0, predelay=0.012, stereo=False):
    n = n_of(decay * 1.2)
    t = np.arange(n) / SR
    ch = 2 if stereo else 1
    ir = rng.standard_normal((n, ch)) * np.exp(-6.9 * t / decay)[:, None]
    ir = filt(ir, "lowpass", damp, 2)
    ir = np.vstack([np.zeros((n_of(predelay), ch)), ir])
    ir /= np.sqrt(np.sum(ir ** 2, axis=0, keepdims=True))
    return ir if stereo else ir[:, 0]


def reverb(x, wet=0.25, decay=1.2, damp=5000.0, stereo_out=False):
    if stereo_out:
        x2 = x if x.ndim == 2 else np.stack([x, x], axis=1)
        ir = make_ir(decay, damp, stereo=True)
        w = np.stack([signal.fftconvolve(x2[:, c], ir[:, c]) for c in range(2)], axis=1)
        out = np.zeros_like(w)
        out[:len(x2)] += x2
        return out + wet * w
    ir = make_ir(decay, damp)
    w = signal.fftconvolve(x, ir)
    out = np.zeros_like(w)
    out[:len(x)] += x
    return out + wet * w


def mix(parts, d=None):
    """parts: lista de (señal, offset_s, ganancia). Mono o estéreo (todas iguales)."""
    end = max(n_of(o) + len(s) for s, o, g in parts)
    if d:
        end = max(end, n_of(d))
    stereo = parts[0][0].ndim == 2
    out = np.zeros((end, 2)) if stereo else np.zeros(end)
    for s, o, g in parts:
        i = n_of(o)
        out[i:i + len(s)] += g * s
    return out


def wrap(y, n):
    """Pliega la cola sobre el principio: loop perfecto (convolución circular)."""
    if len(y) < n:
        y = np.concatenate([y, np.zeros((n - len(y),) + y.shape[1:])])
    out = y[:n].copy()
    tail = y[n:]
    while len(tail):
        k = min(n, len(tail))
        out[:k] += tail[:k]
        tail = tail[k:]
    return out


def circ_noise(n, shape):
    """Ruido coloreado circular (para loops): shape(f) -> ganancia."""
    X = np.fft.rfft(rng.standard_normal(n))
    f = np.fft.rfftfreq(n, 1 / SR)
    return np.fft.irfft(X * shape(f), n)


def fade(x, fin=0.002, fout=0.02):
    x = x.copy()
    a, b = n_of(fin), n_of(fout)
    if a:
        x[:a] *= np.linspace(0, 1, a)[:, None] if x.ndim == 2 else np.linspace(0, 1, a)
    if b:
        x[-b:] *= np.linspace(1, 0, b)[:, None] if x.ndim == 2 else np.linspace(1, 0, b)
    return x


def trim(x, thresh_db=-60.0):
    """Corta silencio final."""
    mono = np.abs(x if x.ndim == 1 else x.max(axis=1))
    th = 10 ** (thresh_db / 20) * max(1e-9, mono.max())
    idx = np.where(mono > th)[0]
    return x[: idx[-1] + n_of(0.05)] if len(idx) else x


def norm_peak(x, db=-1.0):
    return x * (10 ** (db / 20) / max(1e-9, np.max(np.abs(x))))


def norm_rms(x, db=-20.0, ceiling=-1.0):
    x = x * (10 ** (db / 20) / max(1e-9, np.sqrt(np.mean(x ** 2))))
    pk = np.max(np.abs(x))
    if pk > 10 ** (ceiling / 20):
        x *= 10 ** (ceiling / 20) / pk
    return x


def K(name):
    """Carga un ogg de Kenney (mono, 48 kHz)."""
    for sub in ("impact", "rpg"):
        p = KEN / sub / f"{name}.ogg"
        if p.exists():
            x, sr = sf.read(p, always_2d=True)
            x = x.mean(axis=1)
            if sr != SR:
                x = signal.resample_poly(x, SR, sr)
            K.used.add(f"{sub}/{name}.ogg")
            return x
    raise FileNotFoundError(f"Falta {name}.ogg en {KEN} (bajalo: ver CREDITS.md)")


K.used = set()


def save(rel, x, loop=False, stereo=False):
    path = OUT / f"{rel}.wav"
    path.parent.mkdir(parents=True, exist_ok=True)
    if stereo and x.ndim == 1:
        x = np.stack([x, x], axis=1)
    if not stereo and x.ndim == 2:
        x = x.mean(axis=1)
    if loop:
        x = x - np.mean(x, axis=0)  # quita DC sin romper el empalme
    else:
        x = fade(trim(filt(x, "highpass", 20, 2)))
    pk = np.max(np.abs(x))
    if pk > 0.95:
        x = x * (0.95 / pk)  # el pasa-altos puede subir picos
    sf.write(path, x, SR, subtype="PCM_16")
    SOURCES[rel] = sorted(K.used)
    K.used = set()
    return rel


def event(name, files, cat, att, conc, vol=(1, 1), pitch_rng=(1, 1), loop=False):
    MANIFEST[name] = dict(files=files, category=cat, attenuation=att, concurrency=conc,
                          volume=list(vol), pitch=list(pitch_rng), loop=loop)


# ------------------------------------------------------------------ jugador
def build_player():
    files = []
    for i in range(5):
        x = filt(K(f"footstep_concrete_{i:03d}"), "highpass", 70)
        files.append(save(f"Player/Footstep_{i+1:02d}", norm_peak(reverb(x, 0.12, 0.5, 4000))))
    event("Footstep", files, "SFX", "Step", "Footsteps", (0.75, 0.95), (0.9, 1.08))

    files = []
    for i, c in enumerate(["cloth1", "cloth2"]):
        huff = filt(noise(0.25), "bandpass", [450, 1400], 2) * env_ahr(n_of(0.25), 0.03, 0.05, 0.17) * 0.25
        files.append(save(f"Player/Jump_{i+1:02d}", norm_peak(mix([(K(c), 0, 1), (huff, 0.02, 1)]), -3)))
    event("Jump", files, "SFX", "Step", "Footsteps", (0.7, 0.85), (0.95, 1.05))

    files = []
    for i in range(3):
        x = mix([(K(f"impactSoft_heavy_{i:03d}"), 0, 1), (K(f"footstep_concrete_{i+2:03d}"), 0.01, 0.6)])
        files.append(save(f"Player/Land_{i+1:02d}", norm_peak(reverb(x, 0.15, 0.6))))
    event("Land", files, "SFX", "Step", "Footsteps", (0.8, 1.0), (0.92, 1.05))

    files = []
    for i in range(3):
        x = mix([(K(f"impactSoft_heavy_{i+2:03d}"), 0, 1), (pitch(K(f"impactPunch_heavy_{i:03d}"), 0.85), 0.02, 0.7),
                 (K(f"cloth{i+2}"), 0.05, 0.5)])
        files.append(save(f"Player/BodyFall_{i+1:02d}", norm_peak(reverb(x, 0.2, 0.9))))
    event("BodyFall", files, "SFX", "Medium", "Impacts", (0.9, 1.0), (0.92, 1.04))

    files = []
    for i in range(3):
        grunt = filt(noise(0.22), "bandpass", [300, 1100], 2) * env_ahr(n_of(0.22), 0.01, 0.04, 0.17)
        grunt = resonator(grunt, 520, 4) + 0.5 * resonator(grunt, 1100, 5)
        x = mix([(pitch(K(f"impactPunch_heavy_{i+1:03d}"), 1.05), 0, 1), (grunt, 0.01, 0.35)])
        files.append(save(f"Player/Hurt_{i+1:02d}", norm_peak(x, -2)))
    event("Hurt", files, "SFX", "Medium", "Impacts", (0.85, 1.0), (0.95, 1.08))

    # Revivir: arpegio suave ascendente (sintetizado) + tela.
    notes = [392.0, 493.9, 587.3, 784.0]
    parts = [(K("cloth4"), 0, 0.6)]
    for j, f in enumerate(notes):
        tone = (np.sin(2 * np.pi * f * tt(1.4)) + 0.3 * np.sin(2 * np.pi * 2 * f * tt(1.4))) * env_exp(n_of(1.4), 0.45, 0.01)
        parts.append((tone, 0.08 + 0.11 * j, 0.25))
    event("Revive", [save("Player/Revive_01", norm_peak(reverb(mix(parts), 0.35, 1.6), -2))], "SFX", "Medium", "Default", (0.9, 0.9))

    files = []
    for i, c in enumerate(["beltHandle1", "cloth2"]):
        huff = filt(noise(0.35), "bandpass", [350, 1200], 2) * env_ahr(n_of(0.35), 0.04, 0.08, 0.23)
        huff = resonator(huff, 600, 3)
        files.append(save(f"Player/Effort_{i+1:02d}", norm_peak(mix([(K(c), 0, 0.8), (huff, 0.0, 0.5)]), -2)))
    event("Effort", files, "SFX", "Medium", "Default", (0.9, 1.0), (0.95, 1.05))

    # Respiración agitada (loop de 2 respiraciones).
    T = 2.4
    parts = []
    for k in range(2):
        o = k * 1.2
        inh = filt(noise(0.42), "bandpass", [900, 2800], 2) * env_ahr(n_of(0.42), 0.25, 0.05, 0.12)
        exh = filt(noise(0.6), "bandpass", [450, 1600], 2) * env_ahr(n_of(0.6), 0.04, 0.16, 0.4)
        exh = exh + 0.6 * resonator(exh, 700, 4)
        parts += [(inh, o, 0.55), (exh, o + 0.45, 1.0)]
    br = wrap(mix(parts), n_of(T))
    event("Breath", [save("Player/Breath_loop", norm_rms(br, -24), loop=True)], "SFX", "Small", "Default", (1, 1), loop=True)

    event("Pickup", [save("Player/Pickup_01", norm_peak(K("handleSmallLeather"), -3)),
                     save("Player/Pickup_02", norm_peak(K("handleSmallLeather2"), -3))], "SFX", "Small", "Default", (0.8, 0.9), (0.95, 1.08))
    event("Drop", [save("Player/Drop_01", norm_peak(K("dropLeather"), -3))], "SFX", "Small", "Default", (0.8, 0.9), (0.95, 1.05))

    files = []
    for i in range(3):
        d = 0.3 + 0.08 * i
        w = sweep_noise(d, 500 + 150 * i, 2600 - 300 * i, 2.5) * np.hanning(n_of(d))
        files.append(save(f"Player/Throw_{i+1:02d}", norm_peak(w, -4)))
    event("Throw", files, "SFX", "Small", "Default", (0.8, 1.0), (0.92, 1.1))

    huff = resonator(filt(noise(0.4), "bandpass", [300, 1000], 2) * env_ahr(n_of(0.4), 0.05, 0.1, 0.25), 550, 3)
    event("CarryLift", [save("Player/CarryLift_01", norm_peak(mix([(K("cloth1"), 0, 1), (K("cloth3"), 0.12, 0.8), (huff, 0.05, 0.6)]), -2))],
          "SFX", "Medium", "Default", (0.9, 1.0), (0.95, 1.05))

    event("HideEnter", [save("Player/HideEnter_01", norm_peak(mix([(K("creak1"), 0, 0.8), (K("impactWood_medium_000"), 0.25, 0.6)]), -2)),
                        save("Player/HideEnter_02", norm_peak(mix([(K("creak2"), 0, 0.8), (K("impactWood_medium_001"), 0.2, 0.6)]), -2))],
          "SFX", "Small", "Default", (0.8, 0.9), (0.95, 1.05))
    event("HideExit", [save("Player/HideExit_01", norm_peak(mix([(K("impactWood_medium_002"), 0, 0.6), (K("creak3"), 0.05, 0.8)]), -2))],
          "SFX", "Small", "Default", (0.8, 0.9), (0.95, 1.05))


# ------------------------------------------------------------------ UI
def build_ui():
    d = 0.5
    a = np.sin(2 * np.pi * 880 * tt(d)) * env_exp(n_of(d), 0.09, 0.003)
    b = np.sin(2 * np.pi * 1318.5 * tt(d)) * env_exp(n_of(d), 0.12, 0.003)
    ping = reverb(mix([(a, 0, 0.6), (b, 0.09, 0.6)]), 0.25, 0.8, stereo_out=True)
    event("Ping", [save("UI/Ping_01", norm_peak(ping, -4), stereo=True)], "UI", "None", "UI", (0.9, 0.9))

    tick = filt(noise(0.004), "highpass", 3000) * 0.5
    click = np.sin(2 * np.pi * 1600 * tt(0.08)) * env_exp(n_of(0.08), 0.012, 0.001)
    event("UIClick", [save("UI/Click_01", norm_peak(mix([(click, 0, 1), (tick, 0, 1)]), -6), stereo=True)], "UI", "None", "UI", (1, 1), (0.97, 1.03))


# ------------------------------------------------------------------ faroles e items
def build_lantern():
    whoosh = sweep_noise(0.55, 250, 1800, 2) * env_ahr(n_of(0.55), 0.08, 0.1, 0.37)
    thump = sine_glide(0.3, 90, 50) * env_exp(n_of(0.3), 0.08)
    crack = filt(noise(0.4), "bandpass", [1500, 6000], 2) * (rng.random(n_of(0.4)) > 0.985) * 3
    ign = reverb(mix([(whoosh, 0, 1), (thump, 0.02, 0.6), (crack, 0.15, 0.4)]), 0.15, 0.6)
    event("LanternLight", [save("Lantern/Light_01", norm_peak(ign, -2))], "SFX", "Small", "Default", (0.85, 0.95), (0.95, 1.05))

    puff = filt(noise(0.25), "lowpass", 900) * env_exp(n_of(0.25), 0.06, 0.005)
    hiss = filt(noise(0.6), "highpass", 3000) * env_exp(n_of(0.6), 0.18, 0.01) * 0.3
    event("LanternOut", [save("Lantern/Out_01", norm_peak(mix([(puff, 0, 1), (hiss, 0.02, 1)]), -3))], "SFX", "Small", "Default", (0.85, 0.95), (0.95, 1.05))

    # Crepitar de la llama (loop de 6 s).
    T = 6.0
    n = n_of(T)
    bed = circ_noise(n, lambda f: np.where((f > 80) & (f < 1500), 1 / np.maximum(f, 80), 0))
    flut = circ_noise(n, lambda f: np.where((f > 60) & (f < 300), 1.0, 0)) * (1 + 0.5 * np.sin(2 * np.pi * 7 * np.arange(n) / SR))
    y = np.zeros(n + n_of(0.05))
    times = np.cumsum(rng.exponential(1 / 9, 200))
    for tc in times[times < T]:
        d = rng.uniform(0.003, 0.015)
        fc = rng.uniform(1500, 6000)
        c = filt(noise(d + 0.01), "bandpass", [fc * 0.7, min(fc * 1.4, 20000)], 2) * env_exp(n_of(d + 0.01), d / 2, 0.0005)
        i = n_of(tc)
        y[i:i + len(c)] += c * rng.lognormal(0, 0.6)
    cr = wrap(y, n)
    lvl = np.std(cr)
    fire = cr + bed / np.std(bed) * lvl * 0.25 + flut / np.std(flut) * lvl * 0.2  # el crepitar manda
    fire = np.tanh(fire / (np.std(fire) * 3.0))  # compresión suave: chasquidos sin picos enormes
    event("LanternFire", [save("Lantern/Fire_loop", norm_rms(fire, -26), loop=True)], "SFX", "Small", "Default", (1, 1), loop=True)

    event("LanternTake", [save("Lantern/Take_01", norm_peak(mix([(pitch(K("metalLatch"), 1.1), 0, 0.8), (K("handleSmallLeather"), 0.05, 0.6)]), -3))],
          "SFX", "Small", "Default", (0.85, 0.95), (0.95, 1.05))

    slosh = filt(noise(0.45), "lowpass", 700) * (0.5 + 0.5 * np.sin(2 * np.pi * 6 * tt(0.45))) * env_ahr(n_of(0.45), 0.05, 0.15, 0.25)
    event("OilPickup", [save("Items/OilPickup_01", norm_peak(mix([(K("impactGlass_medium_000"), 0, 0.5), (slosh, 0.05, 0.8), (K("handleSmallLeather2"), 0.1, 0.5)]), -3))],
          "SFX", "Small", "Default", (0.85, 0.95), (0.95, 1.05))
    event("MedkitPickup", [save("Items/MedkitPickup_01", norm_peak(mix([(K("beltHandle1"), 0, 0.8), (K("cloth4"), 0.08, 0.6)]), -3))],
          "SFX", "Small", "Default", (0.85, 0.95), (0.95, 1.05))


# ------------------------------------------------------------------ lanzables y trampas
def build_world_sfx():
    files = []
    for i in range(5):
        files.append(save(f"Throw/StoneImpact_{i+1:02d}", norm_peak(reverb(filt(K(f"impactMining_{i:03d}"), "highpass", 90), 0.2, 0.9))))
    event("StoneImpact", files, "SFX", "Medium", "Impacts", (0.85, 1.0), (0.9, 1.1))

    files = []
    for i in range(3):
        tink = np.zeros(n_of(0.7))
        for _ in range(28):
            f = rng.uniform(3000, 8000)
            d = rng.uniform(0.03, 0.12)
            s = np.sin(2 * np.pi * f * tt(d)) * env_exp(n_of(d), d / 4)
            o = n_of(rng.uniform(0, 0.5) ** 1.5)
            tink[o:o + len(s)] += s * rng.uniform(0.1, 0.4)
        x = mix([(K(f"impactGlass_heavy_{i:03d}"), 0, 1), (tink, 0.01, 0.5)])
        files.append(save(f"Throw/BottleShatter_{i+1:02d}", norm_peak(reverb(x, 0.25, 1.1))))
    event("BottleShatter", files, "SFX", "Loud", "Impacts", (0.95, 1.0), (0.92, 1.06))

    event("TrapWarn", [save("Traps/Warn_01", norm_peak(mix([(K("metalClick"), 0, 1), (pitch(K("impactMetal_light_000"), 1.2), 0.03, 0.5)]), -2)),
                       save("Traps/Warn_02", norm_peak(mix([(K("metalClick"), 0, 1), (pitch(K("impactMetal_light_001"), 1.25), 0.02, 0.5)]), -2))],
          "SFX", "Medium", "Impacts", (0.9, 1.0), (0.97, 1.03))

    files = []
    for i in range(2):
        sh = sweep_noise(0.2, 1500, 6000, 3) * np.hanning(n_of(0.2))
        x = mix([(K(f"impactMetal_heavy_{i:03d}"), 0, 1), (K("drawKnife1"), 0.0, 0.8), (sh, 0, 0.3)])
        files.append(save(f"Traps/SpikeFire_{i+1:02d}", norm_peak(reverb(x, 0.25, 1.0))))
    event("SpikeFire", files, "SFX", "Loud", "Impacts", (0.95, 1.0), (0.95, 1.05))

    rumble = filt(noise(1.8), "lowpass", 150) * env_ahr(n_of(1.8), 0.05, 0.4, 1.35) * 2
    debris = np.zeros(n_of(1.8))
    for k in range(70):
        o = n_of(1.6 * (k / 70) ** 1.6)
        d = rng.uniform(0.01, 0.05)
        c = filt(noise(d), "bandpass", [300, 3000], 2) * env_exp(n_of(d), d / 3)
        debris[o:o + len(c)] += c * rng.uniform(0.2, 1.0) * (1 - k / 80)
    pit = mix([(pitch(K("impactWood_heavy_000"), 0.8), 0, 1), (rumble, 0, 0.8), (debris, 0.05, 0.6)])
    event("PitFire", [save("Traps/PitFire_01", norm_peak(reverb(pit, 0.3, 1.4)))], "SFX", "Loud", "Impacts", (0.95, 1.0), (0.95, 1.05))

    d = 2.6
    rush = filt(noise(d), "bandpass", [300, 4000], 2) * env_ahr(n_of(d), 0.5, 1.0, 1.1)
    whistle = sweep_noise(d, 900, 1400, 18) * env_ahr(n_of(d), 0.7, 0.8, 1.1)
    event("GasFire", [save("Traps/GasFire_01", norm_peak(reverb(mix([(rush, 0, 1), (whistle, 0, 0.6)]), 0.2, 1.0)))],
          "SFX", "Loud", "Impacts", (0.95, 1.0), (0.95, 1.05))

    event("TrapReset", [save("Traps/Reset_01", norm_peak(mix([(pitch(K("metalLatch"), 0.9), 0, 1), (K("impactMetal_light_002"), 0.08, 0.3)]), -3))],
          "SFX", "Medium", "Impacts", (0.7, 0.8), (0.97, 1.03))


# ------------------------------------------------------------------ Corazón, reliquia, stingers
def bell(f0, d, partials=(0.5, 1, 1.19, 1.56, 2, 2.51, 2.66, 3.01)):
    out = np.zeros(n_of(d))
    for k, p in enumerate(partials):
        out += np.sin(2 * np.pi * f0 * p * tt(d)) * env_exp(n_of(d), d / (1.5 + k * 0.7), 0.002) / (1 + k * 0.4)
    return out


def build_center():
    whoosh = sweep_noise(1.3, 100, 3000, 1.5) * env_ahr(n_of(1.3), 0.9, 0.1, 0.3)
    boom = sine_glide(1.5, 70, 35) * env_exp(n_of(1.5), 0.5)
    chord = sum(np.sin(2 * np.pi * f * tt(3.5)) for f in (220, 329.6, 440, 554.4, 659.3)) * env_ahr(n_of(3.5), 0.3, 0.6, 2.6)
    chord *= 0.8 + 0.2 * np.sin(2 * np.pi * 7 * tt(3.5))
    use = reverb(mix([(whoosh, 0, 0.8), (boom, 1.0, 1.0), (chord, 1.0, 0.25)]), 0.4, 3.0)
    event("HeartUse", [save("Center/HeartUse_01", norm_peak(use))], "SFX", "Loud", "Default", (1, 1))

    T = 2.2
    parts = []
    for o in (0.0, 1.1):
        lub = sine_glide(0.35, 85, 42, 6) * env_exp(n_of(0.35), 0.11, 0.004)
        dub = sine_glide(0.35, 70, 45, 6) * env_exp(n_of(0.35), 0.09, 0.004)
        parts += [(lub, o, 1.0), (dub, o + 0.3, 0.75)]
    hb = mix(parts, T)
    hb = hb + filt(hb, "lowpass", 200) * 0.5
    hb = wrap(reverb(hb, 0.12, 0.8, 1500), n_of(T))
    event("Heartbeat", [save("Center/Heartbeat_loop", norm_rms(hb, -18), loop=True)], "SFX", "Medium", "Default", (1, 1), loop=True)

    rel = reverb(mix([(bell(330, 3.0), 0, 1), (bell(495, 2.5), 0.12, 0.5)]), 0.45, 2.5)
    event("RelicTake", [save("Center/RelicTake_01", norm_peak(rel, -2))], "SFX", "Loud", "Default", (0.9, 0.9))

    # Stingers (2D, estéreo).
    boom = sine_glide(3.0, 60, 30, 3) * env_exp(n_of(3.0), 0.9)
    rumble = filt(noise(4.5), "lowpass", 120) * env_ahr(n_of(4.5), 0.3, 1.5, 2.7) * 3
    gate = reverb(pitch(K("doorOpen_1"), 0.6), 0.4, 2.0)
    fall = np.linspace(1, 0.94, n_of(4.5))
    tones = (saw(146.8 * fall, 4.5, 20) + saw(155.6 * fall, 4.5, 20)) * env_ahr(n_of(4.5), 0.6, 1.4, 2.5)
    tones = filt(tones, "lowpass", 800)
    x = mix([(boom, 0, 1.0), (rumble, 0, 0.7), (gate, 0.4, 0.8), (tones, 0.3, 0.25)])
    event("StingerExit", [save("Stingers/ExitOpen_01", norm_peak(reverb(x, 0.35, 2.5, stereo_out=True), -2), stereo=True)], "UI", "None", "UI", (0.9, 0.9))

    d = 5.0
    chord = sum(saw(f, d, 24) for f in (146.8, 185.0, 220.0, 293.7, 370.0)) * env_ahr(n_of(d), 0.6, 1.6, 2.8)
    chord = filt(chord, "lowpass", 2200)
    win = reverb(mix([(chord, 0, 0.5), (bell(587.3, 3.0), 0.0, 0.6)]), 0.45, 3.0, stereo_out=True)
    event("StingerVictory", [save("Stingers/Victory_01", norm_peak(win, -2), stereo=True)], "UI", "None", "UI", (0.9, 0.9))

    fall = np.linspace(1, 0.92, n_of(d))
    low = sum(saw(f * fall, d, 16) for f in (73.4, 87.3, 110.0, 116.5)) * env_ahr(n_of(d), 0.3, 2.0, 2.7)
    low = filt(low, "lowpass", 500)
    beats = mix([(sine_glide(0.4, 80, 40, 6) * env_exp(n_of(0.4), 0.12), o, g) for o, g in ((0.2, 1), (0.5, 0.7), (1.6, 0.8), (1.95, 0.5))], d)
    lose = reverb(mix([(low, 0, 0.6), (beats, 0, 0.8)]), 0.4, 3.0, stereo_out=True)
    event("StingerDefeat", [save("Stingers/Defeat_01", norm_peak(lose, -2), stereo=True)], "UI", "None", "UI", (0.9, 0.9))


# ------------------------------------------------------------------ Minotauro
def roar(d, f0, f1, f2, noise_amt=0.4, growl=32.0):
    t = tt(d)
    contour = np.interp(t, [0, d * 0.35, d], [f0, f1, f2]) * (1 + 0.04 * np.sin(2 * np.pi * 5 * t))
    src = saw(contour, d, 60)
    src *= 1 + 0.5 * np.sin(2 * np.pi * growl * t + rng.uniform(0, 6))
    src += filt(noise(d), "bandpass", [200, 1600], 2) * noise_amt * 0.6
    vt = resonator(src, 420, 3) + 0.8 * resonator(src, 760, 4) + 0.4 * resonator(src, 1900, 6)
    vt = np.tanh(2.5 * vt / np.max(np.abs(vt)))
    vt = filt(vt, "lowpass", 2600)  # más grande, menos soplido
    return vt * env_ahr(n_of(d), 0.22, d - 0.9, 0.68)


def build_minotaur():
    files = []
    for i in range(4):
        sub = sine_glide(0.5, 58, 30, 4) * env_exp(n_of(0.5), 0.16, 0.003)
        x = mix([(pitch(K(f"impactPunch_heavy_{i:03d}"), 0.5), 0, 1.0), (sub, 0, 1.0),
                 (pitch(K(f"impactWood_heavy_{i % 5:03d}"), 0.6), 0.01, 0.35)])
        files.append(save(f"Minotaur/Step_{i+1:02d}", norm_peak(reverb(filt(x, "lowpass", 3500), 0.3, 1.6, 3000))))
    event("MinoStep", files, "SFX", "Beast", "Minotaur", (0.9, 1.0), (0.9, 1.05))

    files = []
    for i in range(3):
        parts = []
        for o, d in ((0.0, 0.38), (0.48, 0.24)):
            s = filt(noise(d), "bandpass", [220, 1200], 2)
            s = resonator(s, 300 + 40 * i, 4) + 0.7 * resonator(s, 720, 5)
            s *= (1 + 0.6 * np.sin(2 * np.pi * (28 + 3 * i) * tt(d))) * env_ahr(n_of(d), 0.02, d * 0.3, d * 0.68)
            parts.append((s, o, 1.0 if o == 0 else 0.7))
        files.append(save(f"Minotaur/Snort_{i+1:02d}", norm_peak(reverb(pitch(mix(parts), 0.8), 0.25, 1.4, 3000))))
    event("MinoSnort", files, "SFX", "Beast", "Minotaur", (0.85, 1.0), (0.92, 1.05))

    files = [save(f"Minotaur/Roar_{i+1:02d}", norm_peak(reverb(roar(2.8, f0, f0 * 1.5, f0 * 0.85), 0.35, 2.2, 3500)))
             for i, f0 in enumerate((60.0, 52.0))]
    event("MinoRoar", files, "SFX", "Beast", "Minotaur", (1, 1), (0.95, 1.03))

    ff = mix([(roar(1.5, 110, 175, 85, 0.8, 40), 0, 1), (filt(noise(1.2), "highpass", 2500) * env_exp(n_of(1.2), 0.4), 0.1, 0.25)])
    event("MinoFireFear", [save("Minotaur/FireFear_01", norm_peak(reverb(ff, 0.3, 1.8, 3500)))], "SFX", "Beast", "Minotaur", (1, 1), (0.95, 1.05))

    files = []
    for i in range(2):
        w = sweep_noise(0.3, 300, 1500, 2) * np.hanning(n_of(0.3))
        x = mix([(w, 0, 0.5), (pitch(K(f"impactPunch_heavy_{4 - i:03d}"), 0.75), 0.18, 1.0), (roar(0.9, 85, 110, 70, 0.5), 0.0, 0.6)])
        files.append(save(f"Minotaur/Attack_{i+1:02d}", norm_peak(reverb(x, 0.25, 1.4, 3500))))
    event("MinoAttack", files, "SFX", "Beast", "Minotaur", (1, 1), (0.95, 1.05))


# ------------------------------------------------------------------ ambiente
def build_ambience():
    files = []
    for i in range(5):
        f0 = rng.uniform(900, 1600)
        # "plink": tono que sube rápido (la gota golpea el agua) y se apaga.
        drop = np.sin(2 * np.pi * np.cumsum(f0 * (1 + 1.2 * (1 - np.exp(-tt(0.25) / 0.012)))) / SR) * env_exp(n_of(0.25), 0.045, 0.001)
        files.append(save(f"Ambience/Drip_{i+1:02d}", norm_peak(reverb(drop, 0.6, 2.6, 6000), -3)))
    event("AmbDrip", files, "Ambience", "Amb", "Ambient", (0.5, 0.8), (0.9, 1.1))

    files = []
    for i in range(3):
        d = 5.0
        body = filt(noise(d), "bandpass", [200, 1800], 2) * env_ahr(n_of(d), 1.8, 0.8, 2.4)
        wh = sweep_noise(d, rng.uniform(550, 700), rng.uniform(800, 1000), 20) * env_ahr(n_of(d), 2.0, 0.5, 2.5)
        files.append(save(f"Ambience/Wind_{i+1:02d}", norm_peak(reverb(mix([(body, 0, 1), (wh, 0, 0.5)]), 0.3, 2.0), -3)))
    event("AmbWind", files, "Ambience", "Amb", "Ambient", (0.5, 0.8), (0.9, 1.1))

    files = []
    for i in range(2):
        d = 5.0
        r = filt(noise(d), "lowpass", 90) * env_ahr(n_of(d), 1.5, 1.0, 2.5) * 3 + filt(noise(d), "lowpass", 250) * env_ahr(n_of(d), 1.0, 1.5, 2.5)
        peb = np.zeros(n_of(d))
        for _ in range(12):
            dd = rng.uniform(0.01, 0.04)
            c = filt(noise(dd), "bandpass", [600, 3000], 2) * env_exp(n_of(dd), dd / 3)
            o = n_of(rng.uniform(1.0, 4.0))
            peb[o:o + len(c)] += c * rng.uniform(0.1, 0.4)
        files.append(save(f"Ambience/Rumble_{i+1:02d}", norm_peak(reverb(r + peb, 0.4, 2.5, 2000), -3)))
    event("AmbRumble", files, "Ambience", "Amb", "Ambient", (0.5, 0.7), (0.9, 1.05))

    files = [save(f"Ambience/Creak_{i+1:02d}", norm_peak(filt(reverb(pitch(K(f"creak{i+1}"), 0.7), 0.5, 2.0), "lowpass", 3000), -3))
             for i in range(3)]
    event("AmbCreak", files, "Ambience", "Amb", "Ambient", (0.4, 0.6), (0.85, 1.0))

    # Cama de ambiente (loop de 40 s, mono 2D): drones con ciclos enteros + aire de cueva.
    T = 40.0
    n = n_of(T)
    t = np.arange(n) / SR
    hz = lambda f: round(f * T) / T  # frecuencia con ciclos enteros en el loop
    drone = (np.sin(2 * np.pi * hz(36.71) * t) * 0.8 + np.sin(2 * np.pi * hz(73.42) * t) * 0.5
             + np.sin(2 * np.pi * hz(73.42 + 0.1) * t) * 0.4 + np.sin(2 * np.pi * hz(110.0) * t) * 0.25)
    drone *= 0.75 + 0.25 * np.sin(2 * np.pi * (1 / T) * 4 * t)
    air = circ_noise(n, lambda f: np.where(f > 10, 1 / np.maximum(f, 10), 0) * (f < 400))
    hiss = circ_noise(n, lambda f: ((f > 400) & (f < 1200)).astype(float)) * (0.5 + 0.5 * np.sin(2 * np.pi * (3 / T) * t))
    bed = drone / np.std(drone) * 0.5 + air / np.std(air) * 0.6 + hiss / np.std(hiss) * 0.08
    MANIFEST["_AmbientBed"] = dict(files=[save("Ambience/Bed_loop", norm_rms(bed, -26), loop=True)], category="Ambience", loop=True)


# ------------------------------------------------------------------ música adaptativa (3 capas, 90 bpm, Re menor, 48 s)
BPM, T_MUS = 90, 48.0
BEAT = 60 / BPM
NOTE = lambda m: 440.0 * 2 ** ((m - 69) / 12)


def pan(x, p):
    """p en [-1, 1]."""
    return np.stack([x * np.sqrt((1 - p) / 2), x * np.sqrt((1 + p) / 2)], axis=1)


def finish_music(y, db):
    y = wrap(reverb(y, 0.35, 2.8, 4000, stereo_out=True), n_of(T_MUS))
    return norm_rms(y, db)


def build_music():
    n = n_of(T_MUS)
    # Progresión (9 acordes x 8 tiempos = 72 tiempos = 48 s). Termina en La → vuelve a Rem.
    chords = [(38, 45, 53), (38, 45, 53), (34, 41, 50), (31, 38, 46), (38, 45, 53), (38, 45, 53), (34, 41, 50), (33, 40, 49), (33, 40, 49)]

    # --- Calma: colchón + campanas sueltas (pentatónica de Re menor).
    calm = np.zeros((n + n_of(4), 2))
    for i, ch in enumerate(chords):
        o, d = i * 8 * BEAT, 8 * BEAT + 1.5
        pad = sum(saw(NOTE(m) * (1 + det), d, 12) for m in ch for det in (-0.003, 0.003))
        pad = filt(pad, "lowpass", 650) * env_ahr(n_of(d), 1.5, d - 3.0, 1.5)
        k = n_of(o)
        calm[k:k + len(pad)] += pan(pad, 0) * 0.25
    penta = [62, 65, 67, 69, 72, 74, 77]
    b = 2
    while b < 72:
        f = NOTE(rng.choice(penta) + 12)
        s = bell(f / 2, 3.0, (1, 2, 2.76, 5.4)) * 0.18
        k = n_of(b * BEAT)
        calm[k:k + len(s)] += pan(s, rng.uniform(-0.6, 0.6))
        b += int(rng.integers(3, 7))
    calm = finish_music(calm, -22)

    # --- Tensión: pulso grave + trémolo disonante + tic metálico.
    ten = np.zeros((n + n_of(4), 2))
    for bt in range(72):
        k = n_of(bt * BEAT)
        g = 1.0 if bt % 2 == 0 else 0.55
        th = sine_glide(0.5, 70, 38, 5) * env_exp(n_of(0.5), 0.14, 0.003) * g
        ten[k:k + len(th)] += pan(th, 0) * 0.7
        tk = filt(noise(0.03), "highpass", 5000) * env_exp(n_of(0.03), 0.006)
        k2 = n_of((bt + 0.5) * BEAT)
        ten[k2:k2 + len(tk)] += pan(tk, 0.5 if bt % 2 else -0.5) * 0.12
    for i in range(9):
        o, d = i * 8 * BEAT, 8 * BEAT + 1.0
        cl = sum(saw(NOTE(m), d, 16) for m in (50, 51, 57))
        cl = filt(cl, "lowpass", 1100) * env_ahr(n_of(d), d * 0.6, d * 0.2, d * 0.2)
        cl *= 0.75 + 0.25 * np.sin(2 * np.pi * 6.0 * tt(d))
        k = n_of(o)
        ten[k:k + len(cl)] += pan(cl, -0.3 if i % 2 else 0.3) * 0.12
    ten = finish_music(ten, -22)

    # --- Persecución: tambores tipo taiko + bajo ostinato + golpes de metal grave.
    ch = np.zeros((n + n_of(4), 2))
    pat = [0, 3, 6, 8, 10, 11, 14]
    for bar in range(18):
        for st in range(16):
            k = n_of((bar * 4 + st / 4) * BEAT)
            if st in pat:
                g = 1.0 if st in (0, 8) else 0.65
                tom = sine_glide(0.45, 95 if st != 14 else 120, 52, 5) * env_exp(n_of(0.45), 0.13, 0.002)
                tom[:n_of(0.06)] += filt(noise(0.06), "lowpass", 900) * env_exp(n_of(0.06), 0.015) * 0.6
                ch[k:k + len(tom)] += pan(tom, rng.uniform(-0.2, 0.2)) * g
        bass = [38, 38, 41, 38, 43, 38, 41, 40]
        for e, m in enumerate(bass):
            k = n_of((bar * 4 + e / 2) * BEAT)
            pl = filt(saw(NOTE(m), BEAT / 2, 20), "lowpass", 500) * env_exp(n_of(BEAT / 2), 0.12, 0.003)
            ch[k:k + len(pl)] += pan(pl, 0) * 0.35
        if bar % 2 == 0:
            st = sum(saw(NOTE(m), 1.2, 24) for m in (38, 45, 46))
            st = filt(st, "lowpass", 1400) * env_exp(n_of(1.2), 0.35, 0.01)
            k = n_of(bar * 4 * BEAT)
            ch[k:k + len(st)] += pan(st, 0) * 0.12
    ch = finish_music(ch, -19)

    MANIFEST["_Music"] = dict(files=[save("Music/Calm_loop", calm, loop=True, stereo=True),
                                     save("Music/Tension_loop", ten, loop=True, stereo=True),
                                     save("Music/Chase_loop", ch, loop=True, stereo=True)],
                              category="Music", loop=True)


def main():
    if OUT.exists():
        shutil.rmtree(OUT)
    OUT.mkdir(parents=True)
    for step in (build_player, build_ui, build_lantern, build_world_sfx, build_center, build_minotaur, build_ambience, build_music):
        step()
        print(f"[audio] {step.__name__}: ok")
    (OUT / "manifest.json").write_text(json.dumps(MANIFEST, indent=1, ensure_ascii=False), encoding="utf-8")
    (OUT / "sources.json").write_text(json.dumps(SOURCES, indent=1, ensure_ascii=False), encoding="utf-8")
    wavs = list(OUT.rglob("*.wav"))
    total = sum(p.stat().st_size for p in wavs)
    print(f"[audio] {len(wavs)} wav, {total / 1e6:.1f} MB, {len([e for e in MANIFEST if not e.startswith('_')])} eventos")


if __name__ == "__main__":
    main()
