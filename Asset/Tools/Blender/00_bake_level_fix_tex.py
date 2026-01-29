import bpy
import json
import os
import shutil
import math
from mathutils import Matrix, Vector, Quaternion

# ==============================================================================
# [설정] 프로젝트 경로 및 씬 설정
# ==============================================================================
SCENE_NAME = "HyojeTestScene"
PROJECT_ROOT = r"C:\dev\AuroraEngine\Asset"

PATH_SCENE = os.path.join(PROJECT_ROOT, "Scene")
PATH_MODELS = os.path.join(PROJECT_ROOT, "Model", SCENE_NAME)
PATH_TEXTURES = os.path.join(PROJECT_ROOT, "Texture", SCENE_NAME)
JSON_OUTPUT_PATH = os.path.join(PATH_SCENE, f"{SCENE_NAME}.json")

FBX_AXIS_FORWARD = '-Z'
FBX_AXIS_UP = 'Y'

# ==============================================================================
# [헬퍼] 텍스처 및 디렉토리 처리
# ==============================================================================
def ensure_directory(path):
    if not os.path.exists(path):
        os.makedirs(path)

def sanitize_filename(name):
    return "".join([c for c in name if c.isalnum() or c in (' ', '.', '_')]).rstrip()

def get_texture_destination_path(image):
    """이미지가 저장될 최종 목적지 절대 경로를 반환"""
    raw_name = os.path.basename(image.filepath) if image.filepath else f"{image.name}.png"
    if not raw_name: raw_name = f"{image.name}.png"
    return os.path.join(PATH_TEXTURES, raw_name)

def process_texture_and_relink(node, processed_images):
    """
    텍스처를 추출하고, 블렌더 이미지 경로를 추출된 경로로 '강제 교체'하여 
    FBX Export 시 상대 경로가 맞도록 함.
    """
    if not (node.type == 'TEX_IMAGE' and node.image):
        return

    img = node.image
    
    # 1. 저장될 경로 계산
    raw_name = os.path.basename(img.filepath) if img.filepath else f"{img.name}.png"
    if not raw_name: raw_name = f"{img.name}.png"
    
    dst_path = os.path.join(PATH_TEXTURES, raw_name)
    src_path = bpy.path.abspath(img.filepath)
    src_path = os.path.normpath(src_path)

    # 2. 이미 처리된 이미지라도, 경로는 다시 한 번 리매핑 해줘야 함 (FBX를 위해)
    #    (하지만 파일 쓰기는 중복 방지)
    if img.name not in processed_images:
        try:
            # [CASE 1] 디스크에 원본이 있으면 복사
            if os.path.exists(src_path) and os.path.isfile(src_path):
                if src_path != dst_path:
                    shutil.copy2(src_path, dst_path)
                    print(f"   [Copy] {raw_name}")
            
            # [CASE 2] 파일은 없는데 패킹되어 있으면 -> 바이너리 덤프 (작성하신 핵심 코드!)
            elif img.packed_file:
                with open(dst_path, 'wb') as f:
                    f.write(img.packed_file.data)
                print(f"   [Unpack] {raw_name} (from memory)")
            
            # [CASE 3] 데이터만 메모리에 있음 (Generated 등)
            elif img.has_data:
                # save_render는 컬러 매니지먼트 영향을 받을 수 있어 save()가 나을 수 있으나
                # 경로가 깨진 상태면 save_render가 더 안전할 때도 있음. 
                # 여기선 작성하신대로 save() 시도하되, 경로 먼저 바꾸고 저장
                old_fp = img.filepath
                img.filepath = dst_path
                img.file_format = 'PNG'
                img.save()
                img.filepath = old_fp # 일단 복구 (아래에서 다시 할당)
                print(f"   [Save] {raw_name} (generated)")
            
            processed_images.add(img.name)

        except Exception as e:
            print(f"   [Error] Texture {raw_name}: {e}")

    img.filepath = dst_path

# ==============================================================================
# [코어] 데이터 변환
# ==============================================================================
def convert_transform_aurora(obj):
    loc, rot, scl = obj.matrix_local.decompose()
    out_pos = [loc.x, loc.z, loc.y, 0.0]
    out_rot = [rot.x, -rot.z, -rot.y, rot.w]
    out_scl = [scl.x, scl.z, scl.y, 1.0]
    return out_pos, out_rot, out_scl

# ==============================================================================
# [코어] FBX Export 및 Component 생성
# ==============================================================================
def export_mesh_fbx(obj):
    clean_name = sanitize_filename(obj.name)
    fbx_filename = f"{clean_name}.fbx"
    fbx_full_path = os.path.join(PATH_MODELS, fbx_filename)
    
    # 1. 텍스처 파일 물리적 복사 수행
    processed_imgs = set()
    images_to_relink = {} # {image_object: original_filepath}

    for slot in obj.material_slots:
        if slot.material and slot.material.use_nodes:
            for node in slot.material.node_tree.nodes:
                if node.type == 'TEX_IMAGE' and node.image:
                    # 파일 복사
                    process_texture_and_relink(node, processed_imgs)
                    
                    # 경로 교체를 위해 리스트업 (중복 방지)
                    img = node.image
                    if img not in images_to_relink:
                        images_to_relink[img] = img.filepath

    # 2. 텍스처 경로 잠시 변경 (Relink)
    # FBX Exporter가 올바른 Relative Path('Texture/...')를 계산하도록 유도
    try:
        for img, old_path in images_to_relink.items():
            new_abs_path = get_texture_destination_path(img)
            img.filepath = new_abs_path
            
        # 3. FBX Export
        original_active = bpy.context.view_layer.objects.active
        original_selected = bpy.context.selected_objects
        
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj

        bpy.ops.export_scene.fbx(
            filepath=fbx_full_path,
            use_selection=True,
            global_scale=1.0,
            apply_unit_scale=True,
            axis_forward=FBX_AXIS_FORWARD,
            axis_up=FBX_AXIS_UP,
            object_types={'MESH'},
            use_mesh_modifiers=True,
            embed_textures=False,
            path_mode='RELATIVE', # 여기서 변경된 filepath를 기준으로 상대 경로 계산됨
            bake_space_transform=True 
        )
        
        # 선택 상태 복구
        bpy.ops.object.select_all(action='DESELECT')
        for sel_obj in original_selected:
            sel_obj.select_set(True)
        bpy.context.view_layer.objects.active = original_active

    finally:
        # 4. 텍스처 경로 원상 복구 (매우 중요: 블렌더 파일 꼬임 방지)
        for img, old_path in images_to_relink.items():
            img.filepath = old_path

    return f"{SCENE_NAME}/{fbx_filename}"

def create_model_component(obj):
    model_path = export_mesh_fbx(obj)
    
    # PBR 기본값
    base_color = [1.0, 1.0, 1.0, 1.0]
    metallic = 0.0
    roughness = 1.0
    emission = [0.0, 0.0, 0.0, 0.0]
    normal_scale = 1.0

    mat_data = {
        "ambientOcclusionFactor": 1.0,
        "baseColorFactor": base_color,
        "emissionFactor": emission,
        "metallicFactor": metallic,
        "normalScale": normal_scale,
        "roughnessFactor": roughness
    }

    component = {
        "blendState": int(obj.get("ae_blend", 0)),
        "materialFactorData": mat_data,
        "modelFileName": model_path,
        "psShaderName": obj.get("ae_ps", "PSModel.hlsl"),
        "rasterState": int(obj.get("ae_raster", 1)),
        "type": "ModelComponent",
        "vsShaderName": obj.get("ae_vs", "VSModel.hlsl"),
    }
    return component

# ==============================================================================
# [재귀] GameObject 파싱
# ==============================================================================
def parse_game_object(obj):
    pos, rot, scl = convert_transform_aurora(obj)
    
    game_object = {
        "childGameObjects": [],
        "components": [],
        "name": obj.name,
        "position": pos,
        "rotation": rot,
        "scale": scl,
        "type": "GameObjectBase",
    }

    if obj.type == 'MESH':
        print(f" -> Exporting Mesh: {obj.name}")
        comp = create_model_component(obj)
        game_object["components"].append(comp)

    for child in obj.children:
        child_data = parse_game_object(child)
        game_object["childGameObjects"].append(child_data)
        
    return game_object

def execute_aurora_bake():
    print(f"=== Aurora Map Baking Start: {SCENE_NAME} ===")
    
    ensure_directory(PATH_SCENE)
    ensure_directory(PATH_MODELS)
    ensure_directory(PATH_TEXTURES)

    selected_objects = bpy.context.selected_objects
    if not selected_objects:
        print("❌ 선택된 오브젝트가 없습니다.")
        return

    root_objects = []
    if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')

    for obj in selected_objects:
        if obj.parent is None or obj.parent not in selected_objects:
            root_objects.append(obj)

    print(f"🔍 Found {len(root_objects)} Root Objects.")

    scene_json = {
        "environmentMapFileName": "Skybox.dds",
        "lightColor": [1.0, 1.0, 1.0, 1.0],
        "lightDirection": [-0.5, -1.0, -0.5, 1.0],
        "navPolyIndices": [],
        "navVertices": [],
        "rootGameObjects": []
    }

    for root_obj in root_objects:
        print(f"Processing Root: {root_obj.name}...")
        root_data = parse_game_object(root_obj)
        scene_json["rootGameObjects"].append(root_data)

    with open(JSON_OUTPUT_PATH, 'w', encoding='utf-8') as f:
        json.dump(scene_json, f, indent=4)

    print(f"=== Baking Completed! ===")
    print(f"📄 JSON: {JSON_OUTPUT_PATH}")

if __name__ == "__main__":
    execute_aurora_bake()