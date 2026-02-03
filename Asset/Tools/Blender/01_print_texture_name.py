import bpy

def print_material_textures():
    # 선택된 오브젝트 가져오기
    obj = bpy.context.active_object
    
    if not obj or obj.type != 'MESH':
        print("메쉬 오브젝트를 선택해주세요.")
        return

    print(f"--- Object: {obj.name} ---")

    for slot in obj.material_slots:
        mat = slot.material
        if mat and mat.use_nodes:
            print(f"Material: {mat.name}")
            
            # 노드 중에서 'TEX_IMAGE' 타입 탐색
            for node in mat.node_tree.nodes:
                if node.type == 'TEX_IMAGE':
                    image = node.image
                    if image:
                        print(f"  - Texture Node: {node.name}")
                        print(f"  - Image Name: {image.name}")
                        print(f"  - File Path: {bpy.path.abspath(image.filepath)}")
                    else:
                        print(f"  - Texture Node: {node.name} (No Image Loaded)")
        else:
            print(f"Material: {mat.name if mat else 'None'} (No Nodes used)")

# 실행
print_material_textures()