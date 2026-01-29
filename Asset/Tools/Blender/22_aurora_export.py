# bof 22_aurora_export.py
import bpy
import json
import os
import shutil
import math
from mathutils import Matrix, Vector, Quaternion

# ----- [설정] -----
SCENE_NAME = "TaehyeonTestScene"  
PROJECT_ROOT = r"C:\dev\AuroraEngine\Asset"

PATH_MODELS = os.path.join(PROJECT_ROOT, "Model", SCENE_NAME)
PATH_SCENE = os.path.join(PROJECT_ROOT, "Scene")
PATH_TEXTURES = os.path.join(PROJECT_ROOT, "Texture", SCENE_NAME)
JSON_OUTPUT_PATH = os.path.join(PATH_SCENE, f"{SCENE_NAME}.json")

FBX_AXIS_FORWARD = '-Z'
FBX_AXIS_UP = 'Y'

# ----- [헬퍼 함수] -----

def ensure_directory(path):
    if not os.path.exists(path):
        os.makedirs(path)  

def sanitize_filename(name):
    """파일 이름으로 쓸 수 없는 문자 제거"""
    return "".join([c for c in name if c.isalnum() or c in (' ', '.', '_')]).rstrip()

def copy_textures(obj, target_dir):
    """
    재질 텍스처 처리 로직 개선:
    1. Packed(내장)된 이미지는 강제로 타겟 폴더에 저장 (경로 무시)
    2. Unpacked(외부) 이미지는 경로 추적 후 복사
    """
    copied_count = 0
    # 중복 저장 방지를 위한 캐시
    processed_images = set()

    for slot in obj.material_slots:
        if slot.material and slot.material.use_nodes:
            for node in slot.material.node_tree.nodes:
                if node.type == 'TEX_IMAGE' and node.image:
                    img = node.image
                    
                    # 이미 처리한 이미지는 패스
                    if img.name in processed_images:
                        continue
                    
                    try:
                        # 1. 파일명 추출 (경로가 깨져도 이름은 살아있음)
                        # 경로에서 파일명을 떼어내거나, 블렌더 내부 이름을 사용
                        file_name = os.path.basename(img.filepath)
                        if not file_name: 
                            file_name = f"{img.name}.png" # 파일명이 없으면 내부 이름 사용

                        dst_path = os.path.join(target_dir, file_name)
                        
                        # 2. Packed Image 처리 (가장 중요!)
                        if img.packed_file:
                            # 원본 경로(D드라이브)를 잠시 현재 타겟 경로로 바꿈
                            original_filepath = img.filepath
                            
                            try:
                                img.filepath = dst_path
                                img.save() # 블렌더 내부 데이터를 디스크로 씀
                                print(f"   💾 Unpacked & Saved: {file_name}")
                                copied_count += 1
                            finally:
                                # 원본 경로 복구 (블렌더 내부 상태 보존)
                                img.filepath = original_filepath
                        
                        # 3. 외부 파일 처리 (Source Path가 존재할 때)
                        else:
                            src_path = bpy.path.abspath(img.filepath)
                            if os.path.exists(src_path):
                                shutil.copy2(src_path, dst_path)
                                print(f"   📂 Copied: {file_name}")
                                copied_count += 1
                            else:
                                print(f"   ❌ Missing File: {src_path} (Packed 되지도 않음)")

                        processed_images.add(img.name)

                    except Exception as e:
                        print(f"   ⚠️ Texture Save Failed [{img.name}]: {e}")
                        
    return copied_count

# ----- [메인 로직] -----

def export_aurora():
    print(f"--- Aurora Export Start: {SCENE_NAME} ---")

    # 1. 폴더 생성
    ensure_directory(PATH_MODELS)
    ensure_directory(PATH_SCENE)
    ensure_directory(PATH_TEXTURES)

    # 2. 작업 대상 선정 (선택된 오브젝트만 or 컬렉션)
    # 안전하게 작업하기 위해 Undo Stack 저장
    bpy.ops.ed.undo_push(message="Aurora Export")

    selected_objects = bpy.context.selected_objects
    if not selected_objects:
        print("No objects selected for export.")
        return
    
    # 3. 데이터 정규화 (핵심: Fixer GUI의 로직 이식)
    # Scale/Rot이 엉망일 수 있으므로, 적용
    # 위치(Location)는 씬 배치를 위해 유지해야 합니다. (효제: 아닐지도...)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    
    scene_json_data = {
        "environmentMapFileName": "Skybox.dds",
        "lightColor": [
            1.0,
            1.0,
            1.0,
            1.0
        ],
        "lightDirection": [
            -0.5,
            -1.0,
            -0.5,
            1.0
        ],
        "navPolyIndices": [],
        "navVertices": [],
        "name": SCENE_NAME,
        "rootGameObjects": []
    }

    # 4. 개별 오브젝트 처리
    for obj in selected_objects:
        if obj.type != 'MESH':
            continue  # 메시 오브젝트만 처리

        clean_name = sanitize_filename(obj.name)
        fbx_filename = f"{clean_name}.fbx"
        fbx_path = os.path.join(PATH_MODELS, fbx_filename)
        
        # 4-1. FBX Export
        # 선택된 것 하나만 내보내기 위해 선택 상태 조정
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        
        bpy.ops.export_scene.fbx(   
            filepath=fbx_path,
            use_selection=True,
            global_scale=1.0,
            apply_unit_scale=True,
            apply_scale_options='FBX_SCALE_UNITS',
            axis_forward=FBX_AXIS_FORWARD,
            axis_up=FBX_AXIS_UP,
            use_space_transform=True,
            bake_space_transform=True,
            object_types={'MESH', 'ARMATURE'},
            use_mesh_modifiers=True,
            embed_textures=False, # 텍스처는 별도 폴더로 관리
            path_mode='STRIP'
        )
        
        # 4-2. 텍스처 복사
        copy_textures(obj, PATH_TEXTURES)

        # 4-3. JSON 데이터 구성
        # Apply Transform을 했으므로,
        # Rotation은 (0,0,0), Scale은 (1,1,1)에 가깝습니다.
        # 하지만 Location은 살아있습니다.
        # 효제: 죽여야 할까요?

        # 좌표계 변환 (Blender Z-up -> Engine Y-up 가정)
        # 단순 스왑 방식 (pos.x, pos.z, pos.y)
        pos = obj.location
        rot = obj.rotation_euler.to_quaternion()
        scl = obj.scale

        game_object = {
            "childGameObjects": [],
            "components": [
                {
                    "type": "ModelComponent",
                    "vsShaderName": obj.get("ae_vs", "VSModel.hlsl"),
                    "psShaderName": obj.get("ae_ps", "PSModel.hlsl"),
                    # "modelFileName": fbx_filename,                # 이게 싱글 베이킹
                    "modelFileName": f"{SCENE_NAME}/{fbx_filename}", # 이게 맵 베이킹
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
            ],
            "name": obj.name,
            "position": [pos.x, pos.z, pos.y],
            "rotation": [rot.x, rot.z, rot.y, rot.w], 
            "scale": [scl.x, scl.z, scl.y] ,
            "type": "GameObjectBase"
        }

        # 콜라이더 로직이 필요하다면 여기에 추가 (Bounding Box 계산 등)
        # Apply Scale 되었으므로 obj.dimensions가 정확한 월드 크기입니다.

        scene_json_data["rootGameObjects"].append(game_object)

    # 5. JSON 저장
    with open(JSON_OUTPUT_PATH, 'w', encoding='utf-8') as f:
        json.dump(scene_json_data, f, indent=4)

    print(f"Export Completed: {len(selected_objects)} objects.")
    print(f"JSON Saved: {JSON_OUTPUT_PATH}")

    # 6. 원상 복구 (Undo)
    # 아티스트가 계속 작업을 해야 하므로, Apply 된 상태를 되돌립니다.
    bpy.ops.ed.undo()
    
# 실행
if __name__ == "__main__":
    export_aurora()

# eof aurora_export.py