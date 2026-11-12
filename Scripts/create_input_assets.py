"""
Crea (o actualiza) las Input Actions propias de Labrynn y su Input Mapping Context.
  /Game/Labrynn/Core/Input/IA_Sprint    (bool)  -> Left Shift
  /Game/Labrynn/Core/Input/IA_Interact  (bool)  -> E
  /Game/Labrynn/Core/Input/IA_Lantern   (bool)  -> F
  /Game/Labrynn/Core/Input/IA_Talk      (bool)  -> V   (push-to-talk)
  /Game/Labrynn/Core/Input/IA_Crouch    (bool)  -> C   (mantener para agacharse)
  /Game/Labrynn/Core/Input/IMC_Labrynn            (lo agrega ALabPlayerController con prioridad 1)
Idempotente. Uso igual que setup_project.py (UnrealEditor-Cmd -run=pythonscript -script=...).
"""
import unreal

FOLDER = "/Game/Labrynn/Core/Input"
ACTIONS = [
    ("IA_Sprint", "LeftShift"),
    ("IA_Interact", "E"),
    ("IA_Lantern", "F"),
    ("IA_Talk", "V"),
    ("IA_Crouch", "C"),
    ("IA_Throw", "LeftMouseButton"),
]


def log(msg):
    unreal.log(f"[LabInput] {msg}")


def get_or_create(name, cls, factory):
    path = f"{FOLDER}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER, cls, factory)
    log(f"creado {path}")
    return asset


def main():
    actions = {}
    for name, _ in ACTIONS:
        ia = get_or_create(name, unreal.InputAction, unreal.InputAction_Factory())
        ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
        unreal.EditorAssetLibrary.save_loaded_asset(ia, only_if_is_dirty=False)
        actions[name] = ia

    imc = get_or_create("IMC_Labrynn", unreal.InputMappingContext, unreal.InputMappingContext_Factory())
    imc.unmap_all()
    for name, key_name in ACTIONS:
        key = unreal.Key()
        key.set_editor_property("key_name", key_name)
        imc.map_key(actions[name], key)
        log(f"  {name} -> {key_name}")
    unreal.EditorAssetLibrary.save_loaded_asset(imc, only_if_is_dirty=False)
    log("Listo.")


main()
