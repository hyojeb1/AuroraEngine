import bpy
import os
import shutil

# ==========================================
# 1. 환경 설정
# ==========================================
PROJECT_ROOT = r"C:\dev\AuroraLocal\10_test_house_tex"
SCENE_NAME = "test_house_tex"

# 목표 디렉토리 설정
MODEL_DIR = os.path.join(PROJECT_ROOT, "Model", SCENE_NAME)
TEXTURE_DIR = os.path.join(PROJECT_ROOT, "Texture", SCENE_NAME)

# 디렉토리 생성
os.makedirs(MODEL_DIR, exist_ok=True)
os.makedirs(TEXTURE_DIR, exist_ok=True)

# ==========================================
# 2. 함수 정의: 선택된 오브젝트에서 이미지 추출
# ==========================================
def get_images_from_selected_objects():
    """
    선택된 오브젝트의 머티리얼 노드를 순회하여
    실제로 사용 중인 이미지(bpy.types.Image)들을 set으로 반환합니다.
    (중복 제거를 위해 set 사용)
    """
    selected_objects = bpy.context.selected_objects
    collected_images = set()

    if not selected_objects:
        print(">>> 선택된 오브젝트가 없습니다. 스크립트를 종료합니다.")
        return set()

    print(f"--- 텍스처 수집 시작 (선택된 오브젝트: {len(selected_objects)}개) ---")

    for obj in selected_objects:
        # 머티리얼 슬롯 확인
        for slot in obj.material_slots:
            if slot.material and slot.material.use_nodes and slot.material.node_tree:
                # 노드 트리에서 이미지 텍스처 노드 검색
                for node in slot.material.node_tree.nodes:
                    if node.type == 'TEX_IMAGE':
                        img = node.image
                        # 이미지가 존재하고, 파일 소스인 경우만 수집
                        if img and img.source == 'FILE':
                            collected_images.add(img)
    
    return collected_images

# ==========================================
# 3. 메인 로직: 복사, 리매핑, Export
# ==========================================
def process_and_export():
    # 1) 이미지 수집
    target_images = get_images_from_selected_objects()
    
    if not target_images:
        print(">>> 처리할 텍스처가 없거나 선택된 오브젝트가 없습니다.")
        return

    print(f"--- 총 {len(target_images)}개의 고유 텍스처를 처리합니다. ---")

    # 2) 이미지 복사 및 경로 리매핑
    for img in target_images:
        # 현재 블렌더 상의 파일 경로를 절대 경로로 변환
        src_path = bpy.path.abspath(img.filepath)
        src_path = os.path.normpath(src_path)

        # 파일이 실제 디스크에 존재하는지 확인
        if not os.path.exists(src_path):
            print(f"[Warning] 파일 없음: {src_path}")
            continue

        # 파일명 추출
        file_name = os.path.basename(src_path)
        
        # 목표 경로 설정
        dst_path = os.path.join(TEXTURE_DIR, file_name)

        # 파일 복사 (자신에게 덮어쓰기 방지)
        if src_path != dst_path:
            try:
                shutil.copy2(src_path, dst_path)
                print(f"[Copy] {file_name} -> {TEXTURE_DIR}")
            except Exception as e:
                print(f"[Error] Copy failed for {file_name}: {e}")
        else:
            print(f"[Skip] 원본과 대상 경로가 동일: {file_name}")

        # [중요] 블렌더 내부 데이터의 경로를 복사된 새 경로로 변경
        # 이렇게 해야 FBX Export 시 상대 경로가 올바르게 계산됨
        img.filepath = dst_path

    # 3) FBX Export
    fbx_filename = f"{SCENE_NAME}.fbx"
    fbx_filepath = os.path.join(MODEL_DIR, fbx_filename)
    
    print(f"--- FBX Export 시작: {fbx_filepath} ---")

    try:
        bpy.ops.export_scene.fbx(
            filepath=fbx_filepath,
            path_mode='RELATIVE',      # 텍스처 경로를 상대 경로로 설정
            embed_textures=False,      # 텍스처 포함 안 함 (파일로 관리)
            use_selection=True,        # 선택된 오브젝트만 내보내기
            axis_forward='-Z',
            axis_up='Y',
            object_types={'MESH'},
            mesh_smooth_type='FACE',
            apply_scale_options='FBX_SCALE_ALL',
            bake_space_transform=True
        )
        print(">>> Export 완료 성공!")
    except Exception as e:
        print(f">>> Export 실패: {e}")

# ==========================================
# 실행
# ==========================================
process_and_export()