"""
Materiales provisorios (reemplazables por el kit medieval):
  /Game/Labrynn/Core/Materials/M_Glow : unlit-ish, color base oscuro + emisivo = VectorParameter "EmissiveColor".
El C++ crea instancias dinámicas y fija el color (musgo verde tenue, grieta de luna celeste) desde DA_Visuals.
Idempotente.
"""
import unreal

FOLDER = "/Game/Labrynn/Core/Materials"
NAME = "M_Glow"


def log(m):
    unreal.log(f"[LabMat] {m}")


def main():
    path = f"{FOLDER}/{NAME}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        log(f"ya existe {path}")
        return
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mel = unreal.MaterialEditingLibrary

    base = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, -100)
    base.set_editor_property("constant", unreal.LinearColor(0.02, 0.025, 0.02, 1.0))
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    emis = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 100)
    emis.set_editor_property("parameter_name", "EmissiveColor")
    emis.set_editor_property("default_value", unreal.LinearColor(0.05, 0.3, 0.08, 1.0))
    mel.connect_material_property(emis, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 250)
    rough.set_editor_property("r", 0.9)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    log(f"creado {path}")


main()
