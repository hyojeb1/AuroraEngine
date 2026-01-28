import bpy
import os
import shutil

# ==========================================
# 1. 경로 설정 (사용자 환경에 맞게 수정)
# ==========================================
PROJECT_ROOT = r"C:\dev\AuroraLocal\10_test_house_tex"
SCENE_NAME = "test_house_tex"  # 현재 작업 중인 씬 이름 (변수화 필요)

# 목표 디렉토리
MODEL_DIR = os.path.join(PROJECT_ROOT, "Model", SCENE_NAME)
TEXTURE_DIR = os.path.join(PROJECT_ROOT, "Texture", SCENE_NAME)

# 디렉토리가 없으면 생성
os.makedirs(MODEL_DIR, exist_ok=True)
os.makedirs(TEXTURE_DIR, exist_ok=True)

# ==========================================
# 2. 텍스처 처리 (복사 및 경로 리매핑)
# ==========================================
print(f"--- Processing Textures for {SCENE_NAME} ---")

for img in bpy.data.images:
    # 렌더 결과물이나 뷰어 노드 등 실제 파일이 없는 이미지는 건너뜀
    if img.source != 'FILE' or not img.filepath:
        continue
    
    # 이미지가 패킹(Packed)되어 있다면 언패킹하거나 별도 처리 필요
    # 여기서는 외부 파일이라고 가정
    
    # 1) 원본 절대 경로 획득
    src_path = bpy.path.abspath(img.filepath)
    src_path = os.path.normpath(src_path)
    
    if not os.path.exists(src_path):
        print(f"Warning: Missing file {src_path}")
        continue

    # 2) 파일명 추출
    file_name = os.path.basename(src_path)
    
    # 3) 목표 경로 설정 (Asset/Texture/{SceneName}/)
    dst_path = os.path.join(TEXTURE_DIR, file_name)
    
    # 4) 파일 복사 (이미 존재하지 않거나, 갱신이 필요할 때만)
    #    (shutil.copy는 메타데이터까지 복사하므로 안전)
    try:
        if src_path != dst_path: # 원본과 타겟이 다를 때만 복사
            shutil.copy2(src_path, dst_path)
            print(f"Copied: {file_name} -> {TEXTURE_DIR}")
    except Exception as e:
        print(f"Error copying {file_name}: {e}")

    # 5) [중요] 블렌더 내부 이미지 경로를 '새로운 위치'로 변경
    #    이 과정이 있어야 FBX Exporter가 상대 경로를 정확히 계산함
    img.filepath = dst_path

# ==========================================
# 3. FBX Export (RELATIVE 모드)
# ==========================================
fbx_filename = f"{SCENE_NAME}.fbx"
fbx_filepath = os.path.join(MODEL_DIR, fbx_filename)

bpy.ops.export_scene.fbx(
    filepath=fbx_filepath,
    
    # [핵심] 상대 경로 모드
    # 블렌더가 (FBX 저장 위치)와 (이미지 경로)를 비교해
    # "..\..\Texture\{SceneName}\Image.png" 를 만들어냄
    path_mode='RELATIVE', 
    embed_textures=False,   # 임베딩 끄기
    
    # 오로라 엔진용 세팅
    use_selection=True,
    axis_forward='-Z',
    axis_up='Y',
    object_types={'MESH'},
    mesh_smooth_type='FACE',
    apply_scale_options='FBX_SCALE_ALL',
    bake_space_transform=True
)

print(f"--- Export Success: {fbx_filepath} ---")