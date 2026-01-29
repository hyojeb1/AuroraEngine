#bof 00_export_relative.py
import bpy
import os
import shutil

# ==========================================
# 1. 경로 및 환경 설정
# ==========================================
PROJECT_ROOT = r"C:\dev\AuroraLocal\00_blenderExport"
SCENE_NAME = "4q_test_scene_1blend"

# 목표 디렉토리
MODEL_DIR = os.path.join(PROJECT_ROOT, "Model", SCENE_NAME)
TEXTURE_DIR = os.path.join(PROJECT_ROOT, "Texture", SCENE_NAME)

os.makedirs(MODEL_DIR, exist_ok=True)
os.makedirs(TEXTURE_DIR, exist_ok=True)

# ==========================================
# 2. 텍스처 수집 및 처리 (모든 선택된 오브젝트 대상)
# ==========================================
def process_textures(objects):
    images = set()
    
    # 1) 텍스처 목록 수집
    print(f"--- 텍스처 수집 중... ---")
    for obj in objects:
        for slot in obj.material_slots:
            if slot.material and slot.material.use_nodes and slot.material.node_tree:
                for node in slot.material.node_tree.nodes:
                    if node.type == 'TEX_IMAGE' and node.image:
                        images.add(node.image)

    if not images:
        print(">>> 처리할 텍스처가 없습니다.")
        return

    # 2) 텍스처 복사 및 경로 리매핑
    print(f"--- 텍스처 처리 시작 (총 {len(images)}개) ---")
    
    for img in images:
        raw_name = os.path.basename(img.filepath) if img.filepath else f"{img.name}.png"
        if not raw_name: raw_name = f"{img.name}.png"
        
        dst_path = os.path.join(TEXTURE_DIR, raw_name)
        src_path = bpy.path.abspath(img.filepath)
        src_path = os.path.normpath(src_path)
        
        processed = False
        
        try:
            # 우선순위 1: 디스크 복사
            if os.path.exists(src_path) and os.path.isfile(src_path):
                if src_path != dst_path:
                    shutil.copy2(src_path, dst_path)
                processed = True
            # 우선순위 2: 패킹 데이터 추출
            elif img.packed_file:
                with open(dst_path, 'wb') as f:
                    f.write(img.packed_file.data)
                processed = True
            # 우선순위 3: 메모리 데이터 저장
            elif img.has_data:
                old_filepath = img.filepath
                img.filepath = dst_path
                img.file_format = 'PNG'
                img.save()
                processed = True
            
            # 경로 리매핑 (FBX가 텍스처를 잘 찾도록 블렌더 내부 경로 변경)
            if processed:
                img.filepath = dst_path
                print(f" [OK] {raw_name}")
                
        except Exception as e:
            print(f" [Error] {raw_name}: {e}")

# ==========================================
# 3. 개별 FBX 내보내기 (Batch Export)
# ==========================================
def batch_export_fbx():
    # 현재 선택된 오브젝트들을 리스트로 저장 (나중에 순회하기 위함)
    target_objects = bpy.context.selected_objects[:]
    
    if not target_objects:
        print(">>> 선택된 오브젝트가 없습니다. Export를 중단합니다.")
        return

    # 1단계: 텍스처 먼저 일괄 처리
    process_textures(target_objects)

    print(f"\n==========================================")
    print(f"--- 개별 FBX Export 시작 ({len(target_objects)}개) ---")
    print(f"==========================================")

    # 선택 해제 (초기화)
    bpy.ops.object.select_all(action='DESELECT')

    for obj in target_objects:
        # 2단계: 해당 오브젝트만 선택
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj # 활성 오브젝트 설정

        # 파일명 설정 (특수문자 제거 등 필요시 추가 로직 가능)
        fbx_name = f"{obj.name}.fbx"
        fbx_filepath = os.path.join(MODEL_DIR, fbx_name)

        print(f"Exporting: {fbx_name}...")

        # 3단계: FBX 내보내기 (use_selection=True 필수)
        bpy.ops.export_scene.fbx(
            filepath=fbx_filepath,
            path_mode='RELATIVE',      # 텍스처 경로를 상대 경로로
            embed_textures=False,      # 텍스처 포함 안 함 (폴더에 따로 있으므로)
            use_selection=True,        # 중요: 현재 선택된 것만 내보냄
            axis_forward='-Z',
            axis_up='Y',
            object_types={'MESH'},
            mesh_smooth_type='FACE',
            apply_scale_options='FBX_SCALE_ALL',
            bake_space_transform=True
        )

        # 다음 녀석을 위해 선택 해제
        obj.select_set(False)

    # (옵션) 원래 선택 상태 복구
    for obj in target_objects:
        obj.select_set(True)
    
    print("-" * 60)
    print("모든 작업 완료!")

# 실행
batch_export_fbx()
#eof 00_export_relative.py
