# bof aurora_export.py
# Aurora Engine Blender Exporter
# Exports per-object FBX and a scene JSON compatible with AuroraEngine.
# Run in Blender Text Editor.

import bpy
import json
import os
import shutil
from mathutils import Matrix, Vector
from bpy_extras.io_utils import axis_conversion

# ----- Config -----
EXPORT_COLLECTION = "AE_Export"

SCENE_NAME = "HyojeTestScene"
# You can use Blender-relative paths with "//".
FBX_DIR = rf"C:\dev\AuroraEngine\Asset\Model\{SCENE_NAME}"
# Model root used to build modelFileName relative paths for Aurora.
MODEL_ROOT = r"C:\dev\AuroraEngine\Asset\Model"
SCENE_JSON_PATH = rf"C:\dev\AuroraEngine\Asset\Scene\{SCENE_NAME}.json"
TEXTURE_DIR = rf"C:\dev\AuroraEngine\Asset\Texture\{SCENE_NAME}"

# FBX axis settings. These should match how you want Aurora to see the model.
FBX_AXIS_FORWARD = "-Z"
FBX_AXIS_UP = "Y"

APPLY_UNIT_SCALE = True
GLOBAL_SCALE = 1.0

# Collider naming convention or custom props
COLLIDER_PREFIXES = {
    "box": "COL_BOX_",
    "obb": "COL_OBB_",
    "frustum": "COL_FRUSTUM_",
}

# Scene defaults (override with scene custom props if needed)
DEFAULT_ENV_MAP = "Skybox.dds"
DEFAULT_LIGHT_COLOR = [1.0, 1.0, 1.0, 1.0]
DEFAULT_LIGHT_DIR = [0.0, -1.0, 0.0, 1.0]

# ----- Helpers -----

def abspath(path):
    return bpy.path.abspath(path)


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def sanitize_name(name):
    safe = []
    for ch in name:
        if ch.isalnum() or ch in "-_":
            safe.append(ch)
        elif ch in " .":
            safe.append("_")
        else:
            safe.append("_")
    result = "".join(safe).strip("_")
    return result if result else "Object"


def get_scene_prop(scene, key, default):
    return scene[key] if key in scene else default


def is_collider(obj):
    if obj.get("ae_collider", False):
        return True
    upper = obj.name.upper()
    for prefix in COLLIDER_PREFIXES.values():
        if upper.startswith(prefix):
            return True
    return False


def collider_type(obj):
    if "ae_collider_type" in obj:
        return str(obj["ae_collider_type"]).lower()
    upper = obj.name.upper()
    for ctype, prefix in COLLIDER_PREFIXES.items():
        if upper.startswith(prefix):
            return ctype
    return None


def axis_convert_matrix():
    return axis_conversion(
        # Blender default forward is Y, up is Z.
        from_forward="Y",
        from_up="Z",
        to_forward=FBX_AXIS_FORWARD,
        to_up=FBX_AXIS_UP,
    ).to_4x4()


def lh_flip_matrix():
    return Matrix(((1.0, 0.0, 0.0, 0.0),
                   (0.0, 1.0, 0.0, 0.0),
                   (0.0, 0.0, -1.0, 0.0),
                   (0.0, 0.0, 0.0, 1.0)))


def convert_basis():
    # Match FBX axis conversion, then match Assimp ConvertToLeftHanded (flip Z).
    return lh_flip_matrix() @ axis_convert_matrix()


def convert_matrix(mat, basis, basis_inv):
    # Change of basis: M_engine = B * M_blender * B^-1
    return basis @ mat @ basis_inv


def decompose_to_engine(mat_engine):
    loc, rot, scale = mat_engine.decompose()
    return loc, rot, scale


def vec4_position(vec3):
    return [vec3.x, vec3.y, vec3.z, 0.0]


def vec4_scale(vec3):
    return [vec3.x, vec3.y, vec3.z, 1.0]


def quat_xyzw(quat):
    return [quat.x, quat.y, quat.z, quat.w]


def mesh_local_bounds(obj):
    if obj.type != "MESH" or obj.data is None:
        return Vector((0.0, 0.0, 0.0)), Vector((0.5, 0.5, 0.5))
    coords = [Vector(v) for v in obj.data.bound_box]
    min_v = Vector((min(v.x for v in coords), min(v.y for v in coords), min(v.z for v in coords)))
    max_v = Vector((max(v.x for v in coords), max(v.y for v in coords), max(v.z for v in coords)))
    center = (min_v + max_v) * 0.5
    extents = (max_v - min_v) * 0.5
    return center, extents


def export_fbx_for_object(obj, fbx_path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    bpy.ops.export_scene.fbx(
        filepath=fbx_path,
        use_selection=True,
        object_types={"MESH"},
        use_mesh_modifiers=True,
        use_mesh_modifiers_render=True,
        apply_unit_scale=APPLY_UNIT_SCALE,
        global_scale=GLOBAL_SCALE,
        axis_forward=FBX_AXIS_FORWARD,
        axis_up=FBX_AXIS_UP,
        bake_space_transform=False,
        add_leaf_bones=False,
        use_tspace=True,
        mesh_smooth_type="FACE",
        path_mode='COPY',
        embed_textures=False
    )

def move_exported_textures(source_dir, target_dir):
    # 이동시킬 이미지 확장자 목록
    image_extensions = {".png", ".jpg", ".jpeg", ".tga", ".dds", ".bmp", ".tif", ".tiff"}
    
    # 소스 디렉토리가 없으면 패스
    if not os.path.exists(source_dir):
        return

    files = os.listdir(source_dir)
    moved_count = 0

    for filename in files:
        name, ext = os.path.splitext(filename)
        if ext.lower() in image_extensions:
            src_path = os.path.join(source_dir, filename)
            dst_path = os.path.join(target_dir, filename)
            
            try:
                # 목적지에 파일이 이미 있으면 삭제 (덮어쓰기 위해)
                if os.path.exists(dst_path):
                    os.remove(dst_path)
                
                # 파일 이동
                shutil.move(src_path, dst_path)
                moved_count += 1
            except Exception as e:
                print(f"Failed to move texture {filename}: {e}")

    if moved_count > 0:
        print(f"Moved {moved_count} textures to {target_dir}")

def build_model_component(obj, model_file):
    return {
        "type": "ModelComponent",
        "vsShaderName": obj.get("ae_vs", "VSModel.hlsl"),
        "psShaderName": obj.get("ae_ps", "PSModel.hlsl"),
        "modelFileName": model_file,
        "materialFactorData": {
            "baseColorFactor": [1.0, 1.0, 1.0, 1.0],
            "ambientOcclusionFactor": 1.0,
            "roughnessFactor": 1.0,
            "metallicFactor": 1.0,
            "normalScale": 1.0,
            "emissionFactor": [1.0, 1.0, 1.0, 1.0],
        },
        "blendState": int(obj.get("ae_blend", 0)),
        "rasterState": int(obj.get("ae_raster", 1)),
    }


def add_collider_entry(collider_json, ctype, center, extents, orientation=None, frustum_props=None):
    if ctype == "box":
        collider_json["boundingBoxes"].append({
            "center": [center.x, center.y, center.z],
            "extents": [extents.x, extents.y, extents.z],
        })
    elif ctype == "obb":
        collider_json["boundingOrientedBoxes"].append({
            "center": [center.x, center.y, center.z],
            "extents": [extents.x, extents.y, extents.z],
            "orientation": quat_xyzw(orientation),
        })
    elif ctype == "frustum":
        data = {
            "origin": [center.x, center.y, center.z],
            "orientation": quat_xyzw(orientation),
            "rightSlope": frustum_props["rightSlope"],
            "leftSlope": frustum_props["leftSlope"],
            "topSlope": frustum_props["topSlope"],
            "bottomSlope": frustum_props["bottomSlope"],
            "near": frustum_props["near"],
            "far": frustum_props["far"],
        }
        collider_json["boundingFrustums"].append(data)


def build_collider_component(owner, colliders, basis, basis_inv):
    if not colliders:
        return None

    collider_json = {
        "type": "ColliderComponent",
        "boundingBoxes": [],
        "boundingOrientedBoxes": [],
        "boundingFrustums": [],
    }

    owner_world_inv = owner.matrix_world.inverted()

    for col in colliders:
        ctype = collider_type(col)
        if not ctype:
            continue

        rel_matrix = owner_world_inv @ col.matrix_world
        rel_engine = convert_matrix(rel_matrix, basis, basis_inv)

        base_center, base_extents = mesh_local_bounds(col)
        if col.type == "EMPTY":
            base_center = Vector((0.0, 0.0, 0.0))
            base_extents = Vector((1.0, 1.0, 1.0))

        loc, rot, scale = decompose_to_engine(rel_engine)
        abs_scale = Vector((abs(scale.x), abs(scale.y), abs(scale.z)))
        extents = Vector((base_extents.x * abs_scale.x,
                          base_extents.y * abs_scale.y,
                          base_extents.z * abs_scale.z))

        # Center respects mesh local center offset if any.
        base_center_4 = Vector((base_center.x, base_center.y, base_center.z, 1.0))
        center = (rel_engine @ base_center_4).to_3d()

        if ctype == "frustum":
            frustum_props = {
                "rightSlope": float(col.get("ae_right_slope", 1.0)),
                "leftSlope": float(col.get("ae_left_slope", -1.0)),
                "topSlope": float(col.get("ae_top_slope", 1.0)),
                "bottomSlope": float(col.get("ae_bottom_slope", -1.0)),
                "near": float(col.get("ae_near", 0.1)),
                "far": float(col.get("ae_far", 1000.0)),
            }
            add_collider_entry(collider_json, ctype, loc, extents, rot, frustum_props)
        elif ctype == "box":
            add_collider_entry(collider_json, ctype, center, extents)
        else:
            add_collider_entry(collider_json, ctype, center, extents, rot)

    if not (collider_json["boundingBoxes"] or collider_json["boundingOrientedBoxes"] or collider_json["boundingFrustums"]):
        return None

    return collider_json


def collect_colliders(objects):
    owner_map = {}
    export_objects = set(o for o in objects if not is_collider(o))

    for obj in objects:
        if not is_collider(obj):
            continue

        owner = obj.parent
        while owner and (owner not in export_objects):
            owner = owner.parent
        if not owner:
            continue

        owner_map.setdefault(owner, []).append(obj)

    return owner_map


def build_game_object(obj, export_set, owner_colliders, basis, basis_inv, model_map):
    mat_local = obj.matrix_local.copy()
    mat_engine = convert_matrix(mat_local, basis, basis_inv)
    loc, rot, scale = decompose_to_engine(mat_engine)

    go = {
        "type": obj.get("ae_type", "GameObjectBase"),
        "name": obj.name,
        "position": vec4_position(loc),
        "rotation": quat_xyzw(rot),
        "scale": vec4_scale(scale),
        "components": [],
        "childGameObjects": [],
    }

    if obj.type == "MESH":
        model_file = model_map.get(obj)
        if model_file:
            go["components"].append(build_model_component(obj, model_file))

    collider_component = build_collider_component(obj, owner_colliders.get(obj, []), basis, basis_inv)
    if collider_component:
        go["components"].append(collider_component)

    for child in obj.children:
        if child in export_set and not is_collider(child):
            go["childGameObjects"].append(build_game_object(child, export_set, owner_colliders, basis, basis_inv, model_map))

    return go


def main():
    collection = bpy.data.collections.get(EXPORT_COLLECTION)
    if not collection:
        raise RuntimeError("Collection not found: %s" % EXPORT_COLLECTION)

    all_objects = [o for o in collection.all_objects if not o.hide_get()]
    export_objects = set(o for o in all_objects if not is_collider(o))

    owner_colliders = collect_colliders(all_objects)

    # Resolve output paths
    fbx_dir = abspath(FBX_DIR)
    scene_path = abspath(SCENE_JSON_PATH)
    texture_dir = abspath(TEXTURE_DIR)

    ensure_dir(fbx_dir)
    ensure_dir(os.path.dirname(scene_path))
    ensure_dir(texture_dir)

    # Build unique model filenames
    used_names = {}
    model_map = {}
    for obj in export_objects:
        if obj.type != "MESH":
            continue
        if obj.data is None or len(obj.data.polygons) == 0:
            print("Skip empty mesh:", obj.name)
            continue
        base = sanitize_name(obj.name)
        count = used_names.get(base, 0)
        used_names[base] = count + 1
        if count > 0:
            base = "%s_%d" % (base, count + 1)
        model_map[obj] = base + ".fbx"

    # Export FBX per mesh object
    model_root = os.path.normpath(abspath(MODEL_ROOT))
    for obj, model_file in model_map.items():
        fbx_path = os.path.join(fbx_dir, model_file)
        export_fbx_for_object(obj, fbx_path)
        try:
            rel = os.path.relpath(fbx_path, model_root)
        except ValueError:
            rel = os.path.basename(fbx_path)
        rel = rel.replace("\\", "/")
        model_map[obj] = rel

    move_exported_textures(fbx_dir, texture_dir)

    # Build scene JSON
    scene = bpy.context.scene
    scene_json = {
        "environmentMapFileName": get_scene_prop(scene, "ae_environment", DEFAULT_ENV_MAP),
        "lightColor": get_scene_prop(scene, "ae_light_color", DEFAULT_LIGHT_COLOR),
        "lightDirection": get_scene_prop(scene, "ae_light_dir", DEFAULT_LIGHT_DIR),
        "rootGameObjects": [],
    }

    basis = convert_basis()
    basis_inv = basis.inverted()

    roots = [o for o in export_objects if (o.parent is None or o.parent not in export_objects)]
    for root in roots:
        scene_json["rootGameObjects"].append(build_game_object(root, export_objects, owner_colliders, basis, basis_inv, model_map))

    with open(scene_path, "w", encoding="utf-8") as f:
        json.dump(scene_json, f, indent=4)

    print("Aurora export done")
    print("FBX dir:", fbx_dir)
    print("Scene JSON:", scene_path)


if __name__ == "__main__":
    main()

# eof aurora_export.py
