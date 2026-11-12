"""
Setup del proyecto Labrynn (se puede correr varias veces sin romper nada).

Uso (sin abrir el editor):
  "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
     C:\\Dev\\Labrynn\\Labrynn.uproject -run=pythonscript -script="C:\\Dev\\Labrynn\\Scripts\\setup_project.py" -unattended -nop4 -nosplash

Hace:
  1. Crea el mapa vacío /Game/Labrynn/Maps/L_Labrynn si no existe (el laberinto se genera en runtime).
  2. Verifica que los Blueprints de la plantilla hereden de las clases C++ de Labrynn (CoreRedirects)
     y muestra qué meshes/animaciones tienen asignados.
"""
import unreal

MAP_DIR = "/Game/Labrynn/Maps"
MAP_NAME = "L_Labrynn"
MAP_PATH = f"{MAP_DIR}/{MAP_NAME}"


def log(msg):
    unreal.log(f"[LabSetup] {msg}")


def ensure_map():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        log(f"El mapa ya existe: {MAP_PATH}")
        return
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    world = tools.create_asset(MAP_NAME, MAP_DIR, unreal.World, unreal.WorldFactory())
    if world is None:
        unreal.log_error(f"[LabSetup] No se pudo crear {MAP_PATH}")
        return
    unreal.EditorAssetLibrary.save_loaded_asset(world, only_if_is_dirty=False)
    log(f"Mapa creado: {MAP_PATH}")


def describe_bp(path):
    cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
    if cls is None:
        unreal.log_error(f"[LabSetup] No se pudo cargar {path}")
        return
    cdo = unreal.get_default_object(cls)
    log(f"{path} -> CDO clase {cdo.get_class().get_name()}")
    # Cadena de padres
    parents = []
    c = cls
    for _ in range(6):
        try:
            c = unreal.SystemLibrary.get_super_class(c) if hasattr(unreal.SystemLibrary, "get_super_class") else None
        except Exception:
            c = None
        if c is None:
            break
        parents.append(c.get_name())
    if parents:
        log(f"   padres: {' -> '.join(parents)}")
    return cdo


def main():
    ensure_map()

    ch = describe_bp("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter")
    if ch is not None:
        log(f"   es ALabCharacter: {isinstance(ch, unreal.LabCharacter) if hasattr(unreal, 'LabCharacter') else 'clase no expuesta'}")
        for prop in ("mesh", "first_person_mesh"):
            try:
                comp = ch.get_editor_property(prop)
                sk = comp.get_editor_property("skeletal_mesh_asset")
                anim = comp.get_editor_property("anim_class")
                log(f"   {prop}: mesh={sk.get_path_name() if sk else None} anim={anim.get_name() if anim else None}")
            except Exception as e:
                log(f"   {prop}: no accesible ({e})")
        for prop in ("jump_action", "move_action", "look_action", "mouse_look_action"):
            try:
                v = ch.get_editor_property(prop)
                log(f"   {prop}: {v.get_path_name() if v else None}")
            except Exception as e:
                log(f"   {prop}: no accesible ({e})")

    pc = describe_bp("/Game/FirstPerson/Blueprints/BP_FirstPersonPlayerController")
    if pc is not None:
        try:
            ctxs = pc.get_editor_property("default_mapping_contexts")
            log(f"   default_mapping_contexts: {[c.get_name() for c in ctxs if c]}")
        except Exception as e:
            log(f"   default_mapping_contexts: no accesible ({e})")

    log("Listo.")


main()
