bl_info = {
    "name": "Aurora Engine Exporter",
    "author": "Hyoje",
    "version": (1, 1),
    "blender": (3, 0, 0),
    "location": "View3D > Sidebar > Aurora",
    "description": "Export scene to Aurora Engine JSON/FBX format with Auto-Apply Transform",
    "category": "Import-Export",
}

import bpy
import json
import os
import shutil
from mathutils import Matrix, Vector, Quaternion

class AuroraExporter:
    def __init__(self, context, project_root, scene_name, apply_transform=True):
        self.context = context
        self.project_root = project_root
        self.scene_name = scene_name
        self.apply_transform = apply_transform
        
        # 경로 설정
        self.PATH_SCENE = os.path.join(self.project_root, "Scene")
        self.PATH_MODELS = os.path.join(self.project_root, "Model", self.scene_name)
        self.PATH_TEXTURES = os.path.join(self.project_root, "Texture", self.scene_name)
        self.JSON_OUTPUT_PATH = os.path.join(self.PATH_SCENE, f"{self.scene_name}.json")

        self.FBX_AXIS_FORWARD = '-Z'
        self.FBX_AXIS_UP = 'Y'

    def ensure_directory(self, path):
        if not os.path.exists(path):
            os.makedirs(path)

    def sanitize_filename(self, name):
        return "".join([c for c in name if c.isalnum() or c in (' ', '.', '_')]).rstrip()

    def is_valid_mesh(self, obj):
        """엔진 로드 에러 방지: 데이터가 없는 메쉬인지 확인"""
        if obj.type != 'MESH':
            return False
        if not obj.data or len(obj.data.vertices) == 0:
            print(f"   [Skip] {obj.name} has no vertex data.")
            return False
        return True

    def apply_object_transforms(self, obj):
        """오브젝트의 트랜스폼을 메쉬 데이터에 적용 (Freeze Transforms)"""
        prev_active = self.context.view_layer.objects.active
        self.context.view_layer.objects.active = obj
        obj.select_set(True)
        
        # 위치, 회전, 스케일 모두 적용
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        
        self.context.view_layer.objects.active = prev_active

    def process_texture_and_relink(self, node, processed_images):
        if not (node.type == 'TEX_IMAGE' and node.image):
            return

        img = node.image
        raw_name = os.path.basename(img.filepath) if img.filepath else f"{img.name}.png"
        
        dst_path = os.path.join(self.PATH_TEXTURES, raw_name)
        src_path = os.path.normpath(bpy.path.abspath(img.filepath))

        if img.name not in processed_images:
            try:
                if os.path.exists(src_path) and os.path.isfile(src_path):
                    if src_path != dst_path:
                        shutil.copy2(src_path, dst_path)
                elif img.packed_file:
                    with open(dst_path, 'wb') as f:
                        f.write(img.packed_file.data)
                elif img.has_data:
                    old_fp = img.filepath
                    img.filepath = dst_path
                    img.file_format = 'PNG'
                    img.save()
                    img.filepath = old_fp
                
                processed_images.add(img.name)
            except Exception as e:
                print(f"   [Error] Texture {raw_name}: {e}")

        img.filepath = dst_path

    def convert_transform_aurora(self, obj):
        """JSON에 기록될 트랜스폼 (Apply Transform 시 loc=0, rot=Id, scl=1이 됨)"""
        loc, rot, scl = obj.matrix_local.decompose()
        # 엔진 좌표계에 맞춤 (Blender Z-up -> Engine Y-up)
        out_pos = [loc.x, loc.z, loc.y, 0.0]
        out_rot = [rot.x, -rot.z, -rot.y, rot.w]
        out_scl = [scl.x, scl.z, scl.y, 1.0]
        return out_pos, out_rot, out_scl

    def export_mesh_fbx(self, obj):
        if not self.is_valid_mesh(obj):
            return None

        clean_name = self.sanitize_filename(obj.name)
        fbx_filename = f"{clean_name}.fbx"
        fbx_full_path = os.path.join(self.PATH_MODELS, fbx_filename)
        
        processed_imgs = set()
        images_to_relink = {} 

        # 1. 텍스처 처리
        for slot in obj.material_slots:
            if slot.material and slot.material.use_nodes:
                for node in slot.material.node_tree.nodes:
                    if node.type == 'TEX_IMAGE' and node.image:
                        self.process_texture_and_relink(node, processed_imgs)
                        if node.image not in images_to_relink:
                            images_to_relink[node.image] = node.image.filepath

        # 2. Export 준비 (선택 관리)
        original_selected = self.context.selected_objects[:]
        original_active = self.context.view_layer.objects.active

        try:
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            self.context.view_layer.objects.active = obj

            # [핵심] 트랜스폼 적용 옵션이 켜져있으면 실행
            if self.apply_transform:
                self.apply_object_transforms(obj)

            bpy.ops.export_scene.fbx(
                filepath=fbx_full_path,
                use_selection=True,
                global_scale=1.0,
                apply_unit_scale=True,
                axis_forward=self.FBX_AXIS_FORWARD,
                axis_up=self.FBX_AXIS_UP,
                object_types={'MESH'},
                use_mesh_modifiers=True,
                mesh_smooth_type='OFF', # Assimp aiProcess_GenSmoothNormals 사용하므로 OFF 권장
                embed_textures=False,
                path_mode='RELATIVE',
                bake_space_transform=True 
            )
        finally:
            # 복구
            bpy.ops.object.select_all(action='DESELECT')
            for sel_obj in original_selected:
                try: sel_obj.select_set(True)
                except: pass
            self.context.view_layer.objects.active = original_active

        return f"{self.scene_name}/{fbx_filename}"

    def parse_game_object(self, obj):
        pos, rot, scl = self.convert_transform_aurora(obj)
        
        game_object = {
            "name": obj.name,
            "type": "GameObjectBase",
            "position": pos,
            "rotation": rot,
            "scale": scl,
            "components": [],
            "childGameObjects": []
        }

        # 메쉬 정보 처리
        if obj.type == 'MESH':
            model_path = self.export_mesh_fbx(obj)
            if model_path: # 유효한 데이터가 있을 때만 컴포넌트 추가
                comp = {
                    "type": "ModelComponent",
                    "modelFileName": model_path,
                    "vsShaderName": obj.get("ae_vs", "VSModel.hlsl"),
                    "psShaderName": obj.get("ae_ps", "PSModel.hlsl"),
                    "blendState": int(obj.get("ae_blend", 0)),
                    "rasterState": int(obj.get("ae_raster", 1)),
                    "materialFactorData": {
                        "ambientOcclusionFactor": 1.0,
                        "baseColorFactor": [1.0, 1.0, 1.0, 1.0],
                        "emissionFactor": [0.0, 0.0, 0.0, 0.0],
                        "metallicFactor": 0.0,
                        "normalScale": 1.0,
                        "roughnessFactor": 1.0
                    }
                }
                game_object["components"].append(comp)
            else:
                # 메쉬 데이터가 없으면 엔진 로드 에러 방지를 위해 컴포넌트를 생략하거나
                # 필요하다면 여기서 return None 하여 객체 자체를 씬에서 뺄 수도 있습니다.
                pass

        for child in obj.children:
            child_data = self.parse_game_object(child)
            if child_data:
                game_object["childGameObjects"].append(child_data)
            
        return game_object

    def execute(self):
        self.ensure_directory(self.PATH_SCENE)
        self.ensure_directory(self.PATH_MODELS)
        self.ensure_directory(self.PATH_TEXTURES)

        selected_objects = [obj for obj in self.context.selected_objects]
        if not selected_objects:
            return False, "선택된 오브젝트가 없습니다."

        if self.context.active_object and self.context.active_object.mode != 'OBJECT':
            bpy.ops.object.mode_set(mode='OBJECT')

        root_objects = [obj for obj in selected_objects if obj.parent is None or obj.parent not in selected_objects]

        scene_json = {
            "environmentMapFileName": "Skybox.dds",
            "lightColor": [1.0, 1.0, 1.0, 1.0],
            "lightDirection": [-0.5, -1.0, -0.5, 1.0],
            "navPolyIndices": [],
            "navVertices": [],
            "rootGameObjects": []
        }

        for root_obj in root_objects:
            root_data = self.parse_game_object(root_obj)
            if root_data:
                scene_json["rootGameObjects"].append(root_data)

        with open(self.JSON_OUTPUT_PATH, 'w', encoding='utf-8') as f:
            json.dump(scene_json, f, indent=4)

        return True, f"Baking 완료: {len(scene_json['rootGameObjects'])} 개의 루트 오브젝트"

# ==============================================================================
# UI 영역
# ==============================================================================

class AURORA_OT_BakeScene(bpy.types.Operator):
    bl_idname = "aurora.bake_scene"
    bl_label = "Bake Scene"
    
    def execute(self, context):
        props = context.scene.aurora_props
        exporter = AuroraExporter(
            context, 
            props.project_root, 
            props.scene_name,
            apply_transform=props.apply_transform
        )
        success, msg = exporter.execute()
        if success:
            self.report({'INFO'}, msg)
            return {'FINISHED'}
        else:
            self.report({'WARNING'}, msg)
            return {'CANCELLED'}

class AURORA_PT_Panel(bpy.types.Panel):
    bl_label = "Aurora Exporter"
    bl_idname = "AURORA_PT_Panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Aurora'

    def draw(self, context):
        layout = self.layout
        props = context.scene.aurora_props
        
        box = layout.box()
        box.prop(props, "project_root")
        box.prop(props, "scene_name")
        box.prop(props, "apply_transform", text="Apply Transforms (Recommended)")
        
        layout.separator()
        layout.operator("aurora.bake_scene", icon='EXPORT')

class AuroraProperties(bpy.types.PropertyGroup):
    project_root: bpy.props.StringProperty(
        name="Project Root",
        subtype='DIR_PATH',
        default=r"C:\dev\AuroraEngine\Asset"
    )
    scene_name: bpy.props.StringProperty(
        name="Scene Name",
        default="TaehyeonTestScene"
    )
    apply_transform: bpy.props.BoolProperty(
        name="Apply Transform",
        description="내보내기 전 모든 트랜스폼을 데이터에 직접 적용합니다.",
        default=True
    )

classes = (AuroraProperties, AURORA_OT_BakeScene, AURORA_PT_Panel)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    bpy.types.Scene.aurora_props = bpy.props.PointerProperty(type=AuroraProperties)

def unregister():
    for cls in classes:
        bpy.utils.unregister_class(cls)
    del bpy.types.Scene.aurora_props

if __name__ == "__main__":
    register()