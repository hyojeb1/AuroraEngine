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

def process_texture_node(node, processed_images):
    if not (node.type == 'TEX_IMAGE' and node.image):
        return

    img = node.image
    if img.name in processed_images:
        return

    raw_name = os.path.basename(img.filepath) if img.filepath else f"{img.name}.png"
    if not raw_name: raw_name = f"{img.name}.png"
    
    dst_path = os.path.join(PATH_TEXTURES, raw_name)
    src_path = bpy.path.abspath(img.filepath)
    src_path = os.path.normpath(src_path)

    try:
        if os.path.exists(src_path) and os.path.isfile(src_path):
            if src_path != dst_path:
                shutil.copy2(src_path, dst_path)
        elif img.packed_file:
            with open(dst_path, 'wb') as f:
                f.write(img.packed_file.data)
        elif img.has_data:
            img.save_render(dst_path) # save -> save_render가 더 안전할 때가 있음
        
        print(f"   [Texture] {raw_name} Processed.")
        processed_images.add(img.name)
        
    except Exception as e:
        print(f"   [Error] Texture {raw_name}: {e}")

# ==============================================================================
# [코어] 데이터 변환 (Vec4 패딩 적용)
# ==============================================================================
def convert_transform_aurora(obj):
    """
    Blender(Z-up, RH) -> Aurora(Y-up, LH)
    이슈 해결:
    1. FBX Export 세팅과 맞추기 위해 Z축(엔진 Y축) 기준 180도 회전 보정
    2. 오른손 -> 왼손 좌표계 변환 시 쿼터니언 회전축 반전 (-x, -z, -y)
    """
    # 1. 원본 로컬 행렬 복사
    mat = obj.matrix_local.copy()
    
    # 2. [전방 보정] Blender 기준 Z축으로 180도 회전
    # FBX가 -Z Forward로 나갈 때 발생하는 180도 뒤집힘 현상을 매트릭스 단계에서 선반영
    rot_fix = Matrix.Rotation(math.radians(180), 4, 'Z')
    mat = mat @ rot_fix 
    
    # 3. 분해 (Location, Rotation, Scale)
    loc, rot, scl = mat.decompose()

    # 4. 좌표계 변환 (Swizzle & Negate for LH)
    
    # Position: (x, z, y, 0.0) -> 보통 위치는 축만 바꿉니다.
    out_pos = [loc.x, loc.z, loc.y, 0.0]

    # Rotation: Blender(w, x, y, z) -> DX11(x, y, z, w)
    # *** 중요: 오른손 -> 왼손 변환 시 회전축(x, y, z)을 반전(-)시켜야 회전 방향이 유지됨 ***
    # Blender Z-up -> Engine Y-up 이므로 (x, z, y) 순서로 배치하되 마이너스 적용
    out_rot = [-rot.x, -rot.z, -rot.y, rot.w]

    # Scale: (x, z, y, 1.0)
    out_scl = [scl.x, scl.z, scl.y, 1.0]

    return out_pos, out_rot, out_scl

# ==============================================================================
# [코어] FBX Export 및 Component 생성
# ==============================================================================
def export_mesh_fbx(obj):
    clean_name = sanitize_filename(obj.name)
    fbx_filename = f"{clean_name}.fbx"
    fbx_full_path = os.path.join(PATH_MODELS, fbx_filename)
    
    # 선택 상태 저장 및 변경
    # (주의: 재귀 함수 내부에서 실행되므로, 부모의 선택 상태가 풀릴 수 있음 -> 메인 루프에서 복구 필요 X, 여기서 복구)
    original_active = bpy.context.view_layer.objects.active
    original_selected = bpy.context.selected_objects
    
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    try:
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
            path_mode='RELATIVE',
            bake_space_transform=True 
        )
    finally:
        # 선택 상태 복구 (중요: 재귀 루프 꼬임 방지)
        bpy.ops.object.select_all(action='DESELECT')
        for sel_obj in original_selected:
            sel_obj.select_set(True)
        bpy.context.view_layer.objects.active = original_active

    # 텍스처 추출
    processed_imgs = set()
    for slot in obj.material_slots:
        if slot.material and slot.material.use_nodes:
            for node in slot.material.node_tree.nodes:
                process_texture_node(node, processed_imgs)

    return f"{SCENE_NAME}/{fbx_filename}"

def create_model_component(obj):
    model_path = export_mesh_fbx(obj)
    
    # 기본값 설정
    base_color = [1.0, 1.0, 1.0, 1.0]
    metallic = 0.0
    roughness = 1.0
    emission = [0.0, 0.0, 0.0, 0.0]
    normal_scale = 1.0

    # JSON 구조 생성 (키 정렬에 주의)
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
    
    # 딕셔너리 생성 순서가 JSON 출력 순서가 됨 (Python 3.7+)
    game_object = {
        "childGameObjects": [],
        "components": [],
        "name": obj.name,
        "position": pos,
        "rotation": rot,
        "scale": scl,
        "type": "GameObjectBase",
    }

    # Mesh면 ModelComponent 추가
    if obj.type == 'MESH':
        print(f" -> Exporting Mesh: {obj.name}")
        comp = create_model_component(obj)
        game_object["components"].append(comp)

    # 자식 순회
    for child in obj.children:
        # 베이킹할 때는 숨겨진 오브젝트도 포함할지 결정해야 함
        # 여기서는 모든 자식을 가져오되, 필요시 if child.visible_get(): 등 추가
        child_data = parse_game_object(child)
        game_object["childGameObjects"].append(child_data)
        
    return game_object

# ==============================================================================
# [메인] 실행
# ==============================================================================
def execute_aurora_bake():
    print(f"=== Aurora Map Baking Start: {SCENE_NAME} ===")
    
    ensure_directory(PATH_SCENE)
    ensure_directory(PATH_MODELS)
    ensure_directory(PATH_TEXTURES)

    # 1. 루트 오브젝트 식별
    selected_objects = bpy.context.selected_objects
    if not selected_objects:
        print("❌ 선택된 오브젝트가 없습니다.")
        return

    root_objects = []
    
    # Object Mode 강제 전환
    if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')

    for obj in selected_objects:
        if obj.parent is None or obj.parent not in selected_objects:
            root_objects.append(obj)

    print(f"🔍 Found {len(root_objects)} Root Objects.")

    # 2. JSON 전체 구조
    scene_json = {
        "environmentMapFileName": "Skybox.dds",
        "lightColor": [1.0, 1.0, 1.0, 1.0],
        "lightDirection": [-0.5, -1.0, -0.5, 1.0],
        "navPolyIndices": [],
        "navVertices": [],
        "rootGameObjects": []
    }

    # 3. 데이터 채우기
    for root_obj in root_objects:
        print(f"Processing Root: {root_obj.name}...")
        root_data = parse_game_object(root_obj)
        scene_json["rootGameObjects"].append(root_data)

    # 4. JSON 저장
    with open(JSON_OUTPUT_PATH, 'w', encoding='utf-8') as f:
        json.dump(scene_json, f, indent=4)

    print(f"=== Baking Completed! ===")
    print(f"📄 JSON: {JSON_OUTPUT_PATH}")

if __name__ == "__main__":
    execute_aurora_bake()