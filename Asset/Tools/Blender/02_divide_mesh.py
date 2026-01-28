import bpy

def split_mesh_by_material():
    # 1. 대상 오브젝트 확인
    target_obj = bpy.context.active_object
    if not target_obj or target_obj.type != 'MESH':
        print("메시 오브젝트를 선택해주세요.")
        return
    
    original_name = target_obj.name
    collection = target_obj.users_collection[0] # 현재 컬렉션
    
    print(f"'{original_name}' 분리 작업을 시작합니다...")

    # 2. 블렌더 내장 기능으로 재질별 분리 (Edit Mode 진입 필요)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    # 재질 기준으로 메시 분리 (P key -> By Material)
    bpy.ops.mesh.separate(type='MATERIAL')
    bpy.ops.object.mode_set(mode='OBJECT')

    # 3. 분리된 오브젝트들 찾기 (선택된 상태로 유지됨)
    selected_objects = bpy.context.selected_objects
    
    # 4. 부모가 될 Empty 생성 (Engine Recommendation 2번 방식)
    parent_empty = bpy.data.objects.new(f"{original_name}_Root", None)
    collection.objects.link(parent_empty)
    
    # Empty 위치를 원래 오브젝트 위치로 (피벗 기준)
    parent_empty.location = target_obj.location
    parent_empty.rotation_euler = target_obj.rotation_euler
    parent_empty.scale = target_obj.scale

    # 5. 분리된 자식들을 Empty에 넣고 정리
    for child in selected_objects:
        # 부모 설정
        child.parent = parent_empty
        # 위치값 유지 (World Transform 유지 + Parent Inverse 적용)
        child.matrix_parent_inverse = parent_empty.matrix_world.inverted()
        
        # 이름 정리 (재질 이름 따라가기)
        if child.material_slots:
            mat_name = child.material_slots[0].material.name
            child.name = f"{original_name}_{mat_name}"
            
            # (옵션) 불필요한 빈 슬롯 제거
            bpy.context.view_layer.objects.active = child
            bpy.ops.object.material_slot_remove_unused()

    print(f"분리 완료! '{parent_empty.name}' 아래에 자식들이 생성되었습니다.")

# 실행
split_mesh_by_material()