import bpy
import json
import os
import shutil
import math
from mathutils import Matrix, Vector, Quaternion

# ==============================================================================
# [설정] 프로젝트 경로 및 씬 설정
# ==============================================================================
SCENE_NAME = "HyojeTestScene"  # 베이킹될 씬(폴더) 이름
PROJECT_ROOT = r"C:\dev\AuroraEngine\Asset" # 엔진 에셋 루트

# 경로 설정
PATH_SCENE = os.path.join(PROJECT_ROOT, "Scene")
PATH_MODELS = os.path.join(PROJECT_ROOT, "Model", SCENE_NAME)
PATH_TEXTURES = os.path.join(PROJECT_ROOT, "Texture", SCENE_NAME)
JSON_OUTPUT_PATH = os.path.join(PATH_SCENE, f"{SCENE_NAME}.json")

# FBX 설정
FBX_AXIS_FORWARD = '-Z'
FBX_AXIS_UP = 'Y'

# ==============================================================================
# [헬퍼] 디렉토리 및 텍스처 처리 (00_export_relative.py 로직 통합)
# ==============================================================================
def ensure_directory(path):
    if not os.path.exists(path):
        os.makedirs(path)

def sanitize_filename(name):
    return "".join([c for c in name if c.isalnum() or c in (' ', '.', '_')]).rstrip()

def process_texture_node(node, processed_images):
    """텍스처 노드를 분석하여 이미지를 타겟 폴더로 복사/저장"""
    if not (node.type == 'TEX_IMAGE' and node.image):
        return

    img = node.image
    if img.name in processed_images:
        return

    # 파일명 결정
    raw_name = os.path.basename(img.filepath) if img.filepath else f"{img.name}.png"
    if not raw_name: raw_name = f"{img.name}.png"
    
    dst_path = os.path.join(PATH_TEXTURES, raw_name)
    
    # 원본 경로 추적
    src_path = bpy.path.abspath(img.filepath)
    src_path = os.path.normpath(src_path)

    try:
        # 1. 디스크 복사
        if os.path.exists(src_path) and os.path.isfile(src_path):
            if src_path != dst_path:
                shutil.copy2(src_path, dst_path)
        # 2. Packed 파일 추출
        elif img.packed_file:
            with open(dst_path, 'wb') as f:
                f.write(img.packed_file.data)
        # 3. 메모리 데이터 저장
        elif img.has_data:
            old_filepath = img.filepath
            img.filepath = dst_path
            img.file_format = 'PNG'
            img.save()
            img.filepath = old_filepath # 경로 복구
        
        print(f"   [Texture] {raw_name} Processed.")
        processed_images.add(img.name)
        
    except Exception as e:
        print(f"   [Error] Texture {raw_name}: {e}")

# ==============================================================================
# [코어] 데이터 변환 및 추출 로직
# ==============================================================================

def convert_transform_aurora(obj):
    """
    Blender Transform(Matrix Local)을 Aurora Engine 좌표계(Y-Up)로 변환
    부모-자식 관계가 있으므로 'matrix_local'을 사용합니다.
    """
    # 분해 (Location, Rotation, Scale)
    loc, rot, scl = obj.matrix_local.decompose()

    # 좌표계 변환: Blender(Right, Z-up) -> Aurora(Left, Y-up 가정)
    # 위치: (x, z, y)
    out_pos = [loc.x, loc.z, loc.y]

    # 회전: Quaternion (x, z, y, w) 
    # 주의: 엔진의 쿼터니언 연산 방식에 따라 w의 부호나 순서가 다를 수 있음.
    # 기존 코드 패턴(x, z, y, w)을 따름
    out_rot = [rot.x, rot.z, rot.y, rot.w]

    # 스케일: (x, z, y)
    out_scl = [scl.x, scl.z, scl.y]

    return out_pos, out_rot, out_scl

def export_mesh_fbx(obj):
    """단일 오브젝트 FBX 추출 (Relative Path)"""
    clean_name = sanitize_filename(obj.name)
    fbx_filename = f"{clean_name}.fbx"
    fbx_full_path = os.path.join(PATH_MODELS, fbx_filename)
    
    # 선택 상태 조작 (이 오브젝트만 Export)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    # FBX Export
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
        path_mode='RELATIVE', # 텍스처 경로 상대적으로
        bake_space_transform=True 
    )
    
    # 텍스처 처리
    processed_imgs = set()
    for slot in obj.material_slots:
        if slot.material and slot.material.use_nodes:
            for node in slot.material.node_tree.nodes:
                process_texture_node(node, processed_imgs)

    # 오로라 엔진에서 로드할 경로 (SceneName/FileName.fbx)
    return f"{SCENE_NAME}/{fbx_filename}"

def create_model_component(obj):
    """Mesh 오브젝트용 ModelComponent 데이터 생성"""
    
    # 1. FBX 내보내기 및 경로 획득
    model_path = export_mesh_fbx(obj)
    
    # 2. 머테리얼 속성 추출 (기본값 설정)
    mat_data = {
        "ambientOcclusionFactor": 1.0,
        "baseColorFactor": [1.0, 1.0, 1.0, 1.0],
        "emissionFactor": [0.0, 0.0, 0.0, 0.0], 
        "metallicFactor": 0.0,
        "normalScale": 1.0,
        "roughnessFactor": 1.0
    }

    # 3. 컴포넌트 JSON 구성
    component = {
        "blendState": 0,
        "materialFactorData": mat_data,
        "modelFileName": model_path,
        "psShaderName": "PSModel.hlsl",
        "rasterState": 1,
        "type": "ModelComponent",
        "vsShaderName": "VSModel.hlsl",
    }
    return component

def parse_game_object(obj):
    """
    재귀적으로 GameObject와 자식들을 JSON 구조로 변환
    """
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

    # 컴포넌트 추가 로직
    if obj.type == 'MESH':
        print(f" -> Exporting Mesh: {obj.name}")
        comp = create_model_component(obj)
        game_object["components"].append(comp)

    # 자식 오브젝트 순회 (재귀)
    for child in obj.children:
        # 선택된 오브젝트(혹은 그 자손)만 처리하고 싶다면 체크 추가 가능
        # 여기서는 계층구조상의 모든 자식을 가져갑니다.
        child_data = parse_game_object(child)
        game_object["childGameObjects"].append(child_data)
        
    return game_object

# ==============================================================================
# [메인] 실행 함수
# ==============================================================================
def execute_aurora_bake():
    print(f"=== Aurora Map Baking Start: {SCENE_NAME} ===")
    
    # 1. 폴더 준비
    ensure_directory(PATH_SCENE)
    ensure_directory(PATH_MODELS)
    ensure_directory(PATH_TEXTURES)

    # 2. 루트 오브젝트 식별
    # 블렌더 씬에는 진짜 'Root'가 없으므로, 
    # "선택된 오브젝트 중 부모가 선택되지 않은 오브젝트"를 루트로 간주합니다.
    selected_objects = bpy.context.selected_objects
    if not selected_objects:
        print("❌ 선택된 오브젝트가 없습니다.")
        return

    root_objects = []
    
    # 작업 전 모드 변경 (Object Mode)
    if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')

    # 루트 선별 알고리즘
    for obj in selected_objects:
        # 부모가 없거나, 부모가 선택된 리스트에 없으면 루트로 취급
        if obj.parent is None or obj.parent not in selected_objects:
            root_objects.append(obj)

    print(f"🔍 Found {len(root_objects)} Root Objects.")

    # 3. JSON 데이터 구성
    scene_json = {
        "environmentMapFileName": "Skybox.dds",
        "lightColor": [1.0, 1.0, 1.0, 1.0],
        "lightDirection": [-0.5, -1.0, -0.5, 1.0],
        "navPolyIndices": [],
        "navVertices": [],
        "rootGameObjects": []
    }

    # 4. 재귀적 파싱 및 데이터 채우기
    for root_obj in root_objects:
        print(f"Processing Root: {root_obj.name}...")
        
        # 현재 선택 상태를 저장해두고, FBX export시 변경되었다가 복구해야 함
        # 하지만 재귀 함수 내부에서 selection을 건드리면 꼬일 수 있음.
        # 따라서, FBX export 함수 내에서 selection을 처리하고, 
        # 루프 마지막에 다시 원래 selection을 복구하는 전략은 복잡함.
        # -> parse_game_object가 데이터를 만들면서 파일도 씀.
        
        root_data = parse_game_object(root_obj)
        scene_json["rootGameObjects"].append(root_data)

    # 5. JSON 저장
    with open(JSON_OUTPUT_PATH, 'w', encoding='utf-8') as f:
        json.dump(scene_json, f, indent=4)

    # 6. 선택 상태 복구 (편의성)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in selected_objects:
        obj.select_set(True)

    print(f"=== Baking Completed! ===")
    print(f"📄 JSON: {JSON_OUTPUT_PATH}")
    print(f"📦 Models: {PATH_MODELS}")

if __name__ == "__main__":
    execute_aurora_bake()