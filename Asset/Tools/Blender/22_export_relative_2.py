import bpy
import os
import shutil

# ==========================================
# 1. 경로 설정
# ==========================================
PROJECT_ROOT = r"C:\dev\AuroraLocal\10_test_house_tex"
SCENE_NAME = "test_house_tex"

# 목표 디렉토리
MODEL_DIR = os.path.join(PROJECT_ROOT, "Model", SCENE_NAME)
TEXTURE_DIR = os.path.join(PROJECT_ROOT, "Texture", SCENE_NAME)

os.makedirs(MODEL_DIR, exist_ok=True)
os.makedirs(TEXTURE_DIR, exist_ok=True)

# ==========================================
# 2. 텍스처 처리 (패킹, 복사, 그리고 '강제 저장')
# ==========================================
print(f"==========================================")
print(f"--- Processing Textures for {SCENE_NAME} ---")
print(f"==========================================")

# 선택된 오브젝트의 이미지만 처리하려면 이전 답변의 로직을 사용하세요.
# 여기서는 작성해주신 대로 '모든 이미지'를 대상으로 합니다.
for img in bpy.data.images:
    
    # 렌더 결과물(Viewer Node 등)은 보통 저장 대상이 아니므로 제외 (필요시 주석 해제)
    if img.type == 'RENDER_RESULT':
        continue
    
    # 2) 파일명 추출 및 목표 경로 설정
    # 파일 경로가 없으면 이미지 이름으로 PNG 파일명 생성
    raw_name = os.path.basename(img.filepath) if img.filepath else f"{img.name}.png"
    if not raw_name:
        raw_name = f"{img.name}.png"
        
    dst_path = os.path.join(TEXTURE_DIR, raw_name)
    
    # 3) 원본 경로 확인
    src_path = bpy.path.abspath(img.filepath)
    src_path = os.path.normpath(src_path)
    
    is_packed = img.packed_file is not None
    
    try:
        # ---------------------------------------------------------
        # CASE A: 이미지가 블렌더 안에 패킹(Packed)되어 있는 경우
        # ---------------------------------------------------------
        if is_packed:
            print(f"[Packed] Unpacking: {raw_name}")
            img.filepath = dst_path
            img.save() 
            
        # ---------------------------------------------------------
        # CASE B: 원본 파일이 디스크에 실제로 존재하는 경우 (단순 복사)
        # ---------------------------------------------------------
        elif os.path.exists(src_path) and os.path.isfile(src_path):
            if src_path != dst_path:
                shutil.copy2(src_path, dst_path)
                print(f"[Copied] {raw_name}")
            img.filepath = dst_path # 경로 리매핑
            
        # ---------------------------------------------------------
        # CASE C & D: 파일이 없거나(Missing), 생성된(Generated) 이미지인 경우
        # >>> "억지로 빼내기 (Force Save)"
        # ---------------------------------------------------------
        else:
            print(f"[Force Save] Attempting to save memory data: {raw_name}")
            
            # 경로가 유실되었어도 블렌더 메모리에 픽셀 데이터(img.has_data)가 있다면 저장 가능
            if img.has_data:
                # 1. 경로를 타겟 경로로 강제 설정
                img.filepath = dst_path
                
                # 2. 포맷이 불분명할 수 있으므로 PNG로 강제 설정 (안전장치)
                img.file_format = 'PNG' 
                
                # 3. 블렌더 메모리의 데이터를 디스크로 덤프
                img.save()
                print(f"  -> Success: Saved from memory to {dst_path}")
            else:
                print(f"  -> Failed: No file and No memory data for {img.name}")
                continue

    except Exception as e:
        print(f"Error processing {raw_name}: {e}")

# ==========================================
# 3. FBX Export
# ==========================================
fbx_filename = f"{SCENE_NAME}.fbx"
fbx_filepath = os.path.join(MODEL_DIR, fbx_filename)

bpy.ops.export_scene.fbx(
    filepath=fbx_filepath,
    path_mode='RELATIVE', 
    embed_textures=False,
    use_selection=True,
    axis_forward='-Z',
    axis_up='Y',
    object_types={'MESH'},
    mesh_smooth_type='FACE',
    apply_scale_options='FBX_SCALE_ALL',
    bake_space_transform=True
)

print(f"--- Export Success: {fbx_filepath} ---")