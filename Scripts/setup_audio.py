"""
Integra el audio en el proyecto (correr con el editor cerrado):
    UnrealEditor-Cmd.exe Labrynn.uproject -run=pythonscript -script=Scripts/setup_audio.py

1. Importa SourceAudio/**/*.wav a /Game/Labrynn/Audio/<carpeta>.
2. Mezcla: Sound Classes (General > Efectos > UI, Voz, Música, Ambiente), Submixes y Sound Mix del menú.
3. Atenuaciones 3D con los mismos alcances que oye el Minotauro, y límites de concurrencia.
4. Llena /Game/Labrynn/Core/DA_Audio desde SourceAudio/manifest.json.

Si un asset ya existe y está commiteado, hay que bloquearlo antes (git lfs lock), porque si no queda en solo lectura.
"""
import json
import os

import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(ROOT, "SourceAudio")
AUDIO = "/Game/Labrynn/Audio"
MIX = AUDIO + "/Mix"
DA_PATH = "/Game/Labrynn/Core/DA_Audio"

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def log(msg):
    unreal.log(f"[LabAudio] {msg}")


def get_or_create(path, cls, factory):
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    folder, name = path.rsplit("/", 1)
    return tools.create_asset(name, folder, cls, factory)


# ------------------------------------------------------------------ 1. importar
def import_wavs():
    tasks = []
    for dirpath, _, files in os.walk(SRC):
        for f in sorted(files):
            if not f.lower().endswith(".wav"):
                continue
            rel = os.path.relpath(dirpath, SRC).replace("\\", "/")
            t = unreal.AssetImportTask()
            t.filename = os.path.join(dirpath, f)
            t.destination_path = f"{AUDIO}/{rel}"
            t.destination_name = os.path.splitext(f)[0]
            t.automated = True
            t.replace_existing = True
            t.save = False
            tasks.append(t)
    tools.import_asset_tasks(tasks)
    log(f"importados {len(tasks)} wav")


def wave(rel):
    a = eal.load_asset(f"{AUDIO}/{rel}")
    if not a:
        raise RuntimeError(f"falta {AUDIO}/{rel}")
    return a


# ------------------------------------------------------------------ 2. mezcla
def make_mix():
    classes = {}
    for name in ("Master", "SFX", "UI", "Voice", "Music", "Ambience"):
        classes[name] = get_or_create(f"{MIX}/SC_{name}", unreal.SoundClass, unreal.SoundClassFactory())
    tree = {"Master": ["SFX", "Voice", "Music", "Ambience"], "SFX": ["UI"]}
    for parent, kids in tree.items():
        classes[parent].set_editor_property("child_classes", [classes[k] for k in kids])
        for k in kids:
            classes[k].set_editor_property("parent_class", classes[parent])
    submixes = {}
    for name in ("SFX", "Voice", "Music", "Ambience"):
        submixes[name] = get_or_create(f"{MIX}/SM_{name}", unreal.SoundSubmix, unreal.SoundSubmixFactory())
    mix = get_or_create(f"{MIX}/MIX_Settings", unreal.SoundMix, unreal.SoundMixFactory())
    log("mezcla: 6 sound classes, 4 submixes, MIX_Settings")
    return classes, submixes, mix


def make_attenuations():
    # nombre: (radio a volumen pleno, distancia de caída, oclusión). Coinciden con el ruido para la IA:
    # Step 1400 = sprint; Medium 1800 = piedra; Loud 3200 = botella/trampa (3000); Beast = Minotauro.
    presets = {
        "Small": (100, 900, False),
        "Step": (150, 1400, False),
        "Medium": (200, 1800, True),
        "Loud": (300, 3200, True),
        "Beast": (400, 4500, True),
        "Amb": (200, 2200, False),
    }
    out = {}
    for name, (inner, falloff, occl) in presets.items():
        a = get_or_create(f"{MIX}/ATT_{name}", unreal.SoundAttenuation, unreal.SoundAttenuationFactory())
        s = a.get_editor_property("attenuation")
        s.set_editor_property("attenuate", True)
        s.set_editor_property("spatialize", True)
        s.set_editor_property("distance_algorithm", unreal.AttenuationDistanceModel.NATURAL_SOUND)
        s.set_editor_property("d_b_attenuation_at_max", -45.0)
        s.set_editor_property("attenuation_shape", unreal.AttenuationShape.SPHERE)
        s.set_editor_property("attenuation_shape_extents", unreal.Vector(inner, 0, 0))
        s.set_editor_property("falloff_distance", float(falloff))
        # A la distancia se pierden los agudos (como en una caverna).
        s.set_editor_property("attenuate_with_lpf", True)
        s.set_editor_property("lpf_radius_min", float(inner))
        s.set_editor_property("lpf_radius_max", float(inner + falloff))
        s.set_editor_property("lpf_frequency_at_min", 20000.0)
        s.set_editor_property("lpf_frequency_at_max", 2500.0)
        if occl:
            s.set_editor_property("enable_occlusion", True)
            s.set_editor_property("occlusion_trace_channel", unreal.CollisionChannel.ECC_VISIBILITY)
            s.set_editor_property("occlusion_low_pass_filter_frequency", 1500.0)
            s.set_editor_property("occlusion_volume_attenuation", 0.55)
            s.set_editor_property("occlusion_interpolation_time", 0.3)
        a.set_editor_property("attenuation", s)
        out[name] = a
    log(f"atenuaciones: {', '.join(out)}")
    return out


def make_concurrency():
    # nombre: (máximo de voces, regla). Pasos: 4 jugadores x 2 cerca, el resto se descarta por distancia.
    presets = {
        "Footsteps": (8, unreal.MaxConcurrentResolutionRule.STOP_FARTHEST_THEN_OLDEST),
        "Impacts": (6, unreal.MaxConcurrentResolutionRule.STOP_FARTHEST_THEN_OLDEST),
        "Minotaur": (4, unreal.MaxConcurrentResolutionRule.STOP_OLDEST),
        "Ambient": (3, unreal.MaxConcurrentResolutionRule.STOP_OLDEST),
        "UI": (3, unreal.MaxConcurrentResolutionRule.STOP_OLDEST),
        "Default": (12, unreal.MaxConcurrentResolutionRule.STOP_FARTHEST_THEN_OLDEST),
    }
    out = {}
    for name, (count, rule) in presets.items():
        c = get_or_create(f"{MIX}/CON_{name}", unreal.SoundConcurrency, unreal.SoundConcurrencyFactory())
        s = c.get_editor_property("concurrency")
        s.set_editor_property("max_count", count)
        s.set_editor_property("resolution_rule", rule)
        s.set_editor_property("limit_to_owner", False)
        c.set_editor_property("concurrency", s)
        out[name] = c
    log(f"concurrencia: {', '.join(f'{k}={v[0]}' for k, v in presets.items())}")
    return out


# ------------------------------------------------------------------ 3/4. waves y DA_Audio
def enum_member(enum_cls, name):
    key = name.replace("_", "").lower()
    for m in dir(enum_cls):
        if m.isupper() and m.replace("_", "").lower() == key:
            return getattr(enum_cls, m)
    raise KeyError(f"{enum_cls.__name__}.{name}")


def setup_wave(w, cls, submix, loop, keep_playing):
    w.set_editor_property("sound_class_object", cls)
    if submix:
        w.set_editor_property("sound_submix_object", submix)
    w.set_editor_property("looping", loop)
    # Los loops (música en capas, fuego, latido) siguen corriendo en silencio: no pierden la sincronía.
    w.set_editor_property("virtualization_mode",
                          unreal.VirtualizationMode.PLAY_WHEN_SILENT if keep_playing else unreal.VirtualizationMode.RESTART)


def main():
    import_wavs()
    classes, submixes, mix = make_mix()
    atts = make_attenuations()
    concs = make_concurrency()

    with open(os.path.join(SRC, "manifest.json"), encoding="utf-8") as fh:
        manifest = json.load(fh)

    cat_class = {"SFX": "SFX", "UI": "UI", "Ambience": "Ambience", "Music": "Music"}
    cat_submix = {"SFX": "SFX", "UI": "SFX", "Ambience": "Ambience", "Music": "Music"}

    da = eal.load_asset(DA_PATH) if eal.does_asset_exist(DA_PATH) else \
        tools.create_asset("DA_Audio", "/Game/Labrynn/Core", unreal.LabAudioSettings, unreal.DataAssetFactory())
    events = unreal.Map(unreal.LabSfx, unreal.LabSoundEvent)
    n_waves = 0
    for name, ev in manifest.items():
        cat = ev["category"]
        waves = [wave(f) for f in ev["files"]]
        for w in waves:
            setup_wave(w, classes[cat_class[cat]], submixes[cat_submix[cat]], ev.get("loop", False), ev.get("loop", False))
            n_waves += 1
        if name == "_Music":
            da.set_editor_property("music_calm", waves[0])
            da.set_editor_property("music_tension", waves[1])
            da.set_editor_property("music_chase", waves[2])
            continue
        if name == "_AmbientBed":
            da.set_editor_property("ambient_bed", waves[0])
            continue
        e = unreal.LabSoundEvent()
        e.set_editor_property("variants", waves)
        e.set_editor_property("volume_min", float(ev["volume"][0]))
        e.set_editor_property("volume_max", float(ev["volume"][1]))
        e.set_editor_property("pitch_min", float(ev["pitch"][0]))
        e.set_editor_property("pitch_max", float(ev["pitch"][1]))
        e.set_editor_property("attenuation", atts.get(ev["attenuation"]))  # "None" → 2D
        e.set_editor_property("concurrency", concs.get(ev["concurrency"], concs["Default"]))
        events[enum_member(unreal.LabSfx, name)] = e

    da.set_editor_property("events", events)
    da.set_editor_property("settings_mix", mix)
    cmap = unreal.Map(unreal.LabAudioCategory, unreal.SoundClass)
    for cat, sc in (("MASTER", "Master"), ("SFX", "SFX"), ("VOICE", "Voice"), ("MUSIC", "Music"), ("AMBIENCE", "Ambience")):
        cmap[getattr(unreal.LabAudioCategory, cat)] = classes[sc]
    da.set_editor_property("classes", cmap)

    missing = [m for m in dir(unreal.LabSfx) if m.isupper() and getattr(unreal.LabSfx, m) not in events]
    log(f"DA_Audio: {len(events)} eventos, {n_waves} waves configuradas. Sin sonido: {missing or 'ninguno'}")

    ok = eal.save_directory(AUDIO, only_if_is_dirty=False, recursive=True)
    ok = eal.save_asset(DA_PATH, only_if_is_dirty=False) and ok
    log("guardado OK" if ok else "ERROR al guardar (¿assets en solo lectura? git lfs lock)")


main()
