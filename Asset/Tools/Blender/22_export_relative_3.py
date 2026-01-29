import bpy
import os
import shutil

# ==========================================
# 1. 경로 및 환경 설정
# ==========================================
PROJECT_ROOT = r"C:\dev\AuroraLocal\10_test_house_tex"
SCENE_NAME = "test_house_tex"

# 목표 디렉토리
MODEL_DIR = os.path.join(PROJECT_ROOT, "Model", SCENE_NAME)
TEXTURE_DIR = os.path.join(PROJECT_ROOT, "Texture", SCENE_NAME)

os.makedirs(MODEL_DIR, exist_ok=True)
os.makedirs(TEXTURE_DIR, exist_ok=True)

# ==========================================
# 2. 선택된 오브젝트에서 텍스처 수집 함수
# ==========================================
def get_images_from_selected():
    images = set()
    selected_objs = bpy.context.selected_objects
    if not selected_objs:
        return images
    
    print(f"--- 텍스처 수집 중 (대상 오브젝트: {len(selected_objs)}개) ---")
    
    for obj in selected_objs:
        for slot in obj.material_slots:
            if slot.material and slot.material.use_nodes and slot.material.node_tree:
                for node in slot.material.node_tree.nodes:
                    if node.type == 'TEX_IMAGE' and node.image:
                        images.add(node.image)
    return images

# ==========================================
# 3. 텍스처 처리 (우선순위: 복사 > 패킹데이터 쓰기 > 강제 저장)
# ==========================================
print(f"==========================================")
print(f"--- Processing Textures for {SCENE_NAME} ---")
print(f"==========================================")

target_images = get_images_from_selected()

if not target_images:
    print(">>> 처리할 텍스처가 없습니다 (선택된 오브젝트 확인 필요).")
else:
    for img in target_images:
        # 파일명 결정 (경로가 없으면 블렌더 내부 이름 사용)
        raw_name = os.path.basename(img.filepath) if img.filepath else f"{img.name}.png"
        if not raw_name: 
            raw_name = f"{img.name}.png"
            
        # 목표 경로
        dst_path = os.path.join(TEXTURE_DIR, raw_name)
        
        # 원본 경로 (디스크 확인용)
        src_path = bpy.path.abspath(img.filepath)
        src_path = os.path.normpath(src_path)
        
        # 처리 상태 플래그
        processed = False

        print(f"Target: {raw_name}")

        try:
            # ---------------------------------------------------------
            # PRIORITY 1: 원본 파일이 디스크에 존재하면 복사 (가장 안전)
            # ---------------------------------------------------------
            if os.path.exists(src_path) and os.path.isfile(src_path):
                if src_path != dst_path:
                    shutil.copy2(src_path, dst_path)
                    print(f"  [1] Copied from Disk: {src_path}")
                else:
                    print(f"  [1] Skipped (Same Path)")
                processed = True

            # ---------------------------------------------------------
            # PRIORITY 2: 파일은 없는데 패킹되어 있다면 -> 바이너리 강제 쓰기
            # (블렌더의 save() 함수를 쓰지 않고 데이터 직접 추출)
            # ---------------------------------------------------------
            elif img.packed_file:
                print(f"  [2] Extracting Packed Data (Binary Write)...")
                # 패킹된 데이터(bytes)를 직접 파일로 씀 (원본 경로 에러 무시 가능)
                with open(dst_path, 'wb') as f:
                    f.write(img.packed_file.data)
                print(f"  -> Success: Extracted to {dst_path}")
                processed = True

            # ---------------------------------------------------------
            # PRIORITY 3: 파일도 없고 패킹도 안됨 -> 메모리 데이터 강제 저장
            # (Generated Texture 등)
            # ---------------------------------------------------------
            elif img.has_data:
                print(f"  [3] Force Saving Memory Data...")
                
                # 기존 경로 백업
                old_filepath = img.filepath
                
                # 강제 저장 설정
                img.filepath = dst_path
                img.file_format = 'PNG'
                img.save()
                
                # 경로 복구 (나중을 위해) 혹은 유지
                # 여기서는 리매핑을 위해 유지
                print(f"  -> Success: Saved from memory")
                processed = True
            
            else:
                print(f"  [X] Failed: Image is missing (No file, No pack, No data).")

            # ---------------------------------------------------------
            # FINAL: 블렌더 내부 경로 리매핑 (FBX Relative Path용)
            # ---------------------------------------------------------
            if processed:
                img.filepath = dst_path

        except Exception as e:
            print(f"  [!] Error processing {raw_name}: {e}")

# ==========================================
# 4. FBX Export
# ==========================================
fbx_filename = f"{SCENE_NAME}.fbx"
fbx_filepath = os.path.join(MODEL_DIR, fbx_filename)

print(f"--- Exporting FBX... ---")
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