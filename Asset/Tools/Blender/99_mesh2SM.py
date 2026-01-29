import bpy

# 설정: 이름을 변경할 대상 컬렉션 (없으면 씬 전체)
TARGET_COLLECTION_NAME = "AE_Export" 

def auto_rename_by_material():
    # 컬렉션 가져오기
    if TARGET_COLLECTION_NAME in bpy.data.collections:
        target_objects = bpy.data.collections[TARGET_COLLECTION_NAME].all_objects
    else:
        target_objects = bpy.context.scene.objects

    # 이름 중복 방지를 위한 카운터
    name_counter = {}

    print("--- 이름 자동 정리 시작 ---")

    for obj in target_objects:
        # Mesh 타입이 아니면 패스 (카메라, 라이트 등)
        if obj.type != 'MESH':
            continue
            
        # 이미 네이밍 규칙(COL 등)을 지키고 있는 것은 건너뜀
        if obj.name.startswith("COL_") or obj.name.startswith("SM_"):
            continue

        # 매테리얼 슬롯 확인
        base_name = "SM_Object" # 기본값
        if obj.data.materials and obj.data.materials[0]:
            # 첫 번째 매테리얼 이름을 가져옴 (예: "Brick_Wall")
            mat_name = obj.data.materials[0].name
            # 이름 안전하게 변경 (공백 -> 언더바)
            base_name = "SM_" + mat_name.replace(".", "_").replace(" ", "_")
        
        # 카운팅 및 새 이름 생성
        count = name_counter.get(base_name, 0) + 1
        name_counter[base_name] = count
        
        new_name = f"{base_name}_{count:02d}"
        
        # 이름 변경 적용
        old_name = obj.name
        obj.name = new_name
        print(f"Renamed: {old_name} -> {new_name}")

    print("--- 정리 완료 ---")

auto_rename_by_material()