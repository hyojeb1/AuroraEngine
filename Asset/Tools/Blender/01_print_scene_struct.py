import bpy

def print_collection_hierarchy(collection, depth=0):
    """
    콜렉션과 그 내부의 오브젝트, 하위 콜렉션을 재귀적으로 출력합니다.
    """
    # 들여쓰기 설정 (깊이에 따라 공백 추가)
    indent = "    " * depth
    
    # 현재 콜렉션 이름 출력 (아이콘으로 구분)
    print(f"{indent}📁 [Collection] {collection.name}")
    
    # 해당 콜렉션에 속한 오브젝트들 출력
    for obj in collection.objects:
        print(f"{indent}    🔹 {obj.name} ({obj.type})")
        
    # 하위 콜렉션이 있다면 재귀적으로 함수 호출
    for child in collection.children:
        print_collection_hierarchy(child, depth + 1)

# --- 실행 부분 ---
print("\n" + "="*40)
print(" 🏗️ Scene Collection Hierarchy Structure")
print("="*40)

# 현재 씬의 마스터 콜렉션(Root)부터 시작
root_collection = bpy.context.scene.collection
print_collection_hierarchy(root_collection)

print("="*40 + "\n")