import bpy
import os

def print_selected_textures():
    # 선택된 오브젝트가 있는지 확인
    selected_objects = bpy.context.selected_objects
    if not selected_objects:
        print(">>> 선택된 오브젝트가 없습니다.")
        return

    print("-" * 60)
    print(f"텍스처 경로 추출 시작 (총 {len(selected_objects)}개 오브젝트)")
    print("-" * 60)

    for obj in selected_objects:
        print(f"\n[오브젝트]: {obj.name}")
        
        # 오브젝트에 머티리얼 슬롯이 없는 경우
        if not obj.material_slots:
            print("  -> 머티리얼 없음")
            continue

        for slot in obj.material_slots:
            if slot.material:
                mat = slot.material
                print(f"  [머티리얼]: {mat.name}")
                
                # 머티리얼이 노드를 사용하는지 확인
                if mat.use_nodes and mat.node_tree:
                    texture_found = False
                    
                    # 노드 트리를 순회하며 이미지 텍스처 찾기
                    for node in mat.node_tree.nodes:
                        if node.type == 'TEX_IMAGE':
                            img = node.image
                            if img:
                                texture_found = True
                                # 블렌더 내부 경로(//)를 절대 경로로 변환하여 보기 좋게 출력
                                abs_path = bpy.path.abspath(img.filepath)
                                norm_path = os.path.normpath(abs_path)
                                
                                print(f"    - 텍스처 이름: {img.name}")
                                print(f"    - 파일 경로  : {norm_path}")
                    
                    if not texture_found:
                        print("    -> 이미지 텍스처 노드 없음")
                else:
                    print("    -> 노드를 사용하지 않는 머티리얼")

    print("\n" + "-" * 60)
    print("출력 완료")
    print("-" * 60)

# 함수 실행
print_selected_textures()