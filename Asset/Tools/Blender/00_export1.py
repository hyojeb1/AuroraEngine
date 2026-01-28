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
# 2. 텍스처 처리 (패킹된 파일 추출 및 경로 리매핑)
# ==========================================
print(f"==========================================")
print(f"--- Processing Textures for {SCENE_NAME} ---")
print(f"==========================================")

for img in bpy.data.images:
    # 1) 유효성 검사: 렌더 결과물이거나 데이터가 없는 경우 건너뜀
    if img.source != 'FILE':
        continue
    
    # 2) 파일명 추출 및 목표 경로 설정
    # 파일 경로가 비어있을 경우(가끔 발생) 이미지 이름을 사용
    raw_name = os.path.basename(img.filepath) if img.filepath else f"{img.name}.png"
    if not raw_name:
        raw_name = f"{img.name}.png"
        
    dst_path = os.path.join(TEXTURE_DIR, raw_name)
    
    # 3) 절대 경로 변환 (디스크 확인용)
    src_path = bpy.path.abspath(img.filepath)
    src_path = os.path.normpath(src_path)
    
    is_packed = img.packed_file is not None
    
    try:
        # CASE A: 이미지가 블렌더 안에 패킹되어 있는 경우
        if is_packed:
            print(f"[Packed] Extracting: {raw_name}")
            
            # 경로를 목표 위치로 변경하고 저장(Unpack 효과)
            img.filepath = dst_path
            img.save() 
            
        # CASE B: 패킹되지 않았고, 원본 파일이 디스크에 존재하는 경우
        elif os.path.exists(src_path):
            if src_path != dst_path:
                shutil.copy2(src_path, dst_path)
                print(f"[Copied] {raw_name}")
            img.filepath = dst_path # 경로 리매핑
            
        # CASE C: 패킹도 안 되어 있고, 원본 파일도 없는 경우 (진짜 유실됨)
        else:
            print(f"Warning: Missing file {src_path} (Not packed)")
            continue

    except Exception as e:
        print(f"Error processing {raw_name}: {e}")

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