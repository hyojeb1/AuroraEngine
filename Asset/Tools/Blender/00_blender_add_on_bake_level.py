bl_info = {
    "name": "Aurora Engine Exporter",
    "author": "Hyoje",
    "version": (1, 0),
    "blender": (3, 0, 0),
    "location": "View3D > Sidebar > Aurora",
    "description": "Export scene to Aurora Engine JSON/FBX format",
    "category": "Import-Export",
}

import bpy
import json
import os
import shutil
import math
from mathutils import Matrix, Vector, Quaternion

# ==============================================================================
# [Core Logic] 엑스포터 클래스 (경로 상태 관리)
# ==============================================================================
class AuroraExporter:
    def __init__(self, context, project_root, scene_name):
        self.context = context
        self.project_root = project_root
        self.scene_name = scene_name
        
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

    def get_texture_destination_path(self, image):
        raw_name = os.path.basename(image.filepath) if image.filepath else f"{image.name}.png"
        if not raw_name: raw_name = f"{image.name}.png"
        return os.path.join(self.PATH_TEXTURES, raw_name)

    def process_texture_and_relink(self, node, processed_images):
        if not (node.type == 'TEX_IMAGE' and node.image):
            return

        img = node.image
        raw_name = os.path.basename(img.filepath) if img.filepath else f"{img.name}.png"
        if not raw_name: raw_name = f"{img.name}.png"
        
        dst_path = os.path.join(self.PATH_TEXTURES, raw_name)
        src_path = bpy.path.abspath(img.filepath)
        src_path = os.path.normpath(src_path)

        if img.name not in processed_images:
            try:
                # [CASE 1] 디스크 복사
                if os.path.exists(src_path) and os.path.isfile(src_path):
                    if src_path != dst_path:
                        shutil.copy2(src_path, dst_path)
                        print(f"   [Copy] {raw_name}")
                # [CASE 2] Packed -> Dump
                elif img.packed_file:
                    with open(dst_path, 'wb') as f:
                        f.write(img.packed_file.data)
                    print(f"   [Unpack] {raw_name}")
                # [CASE 3] Generated -> Save
                elif img.has_data:
                    old_fp = img.filepath
                    img.filepath = dst_path
                    img.file_format = 'PNG'
                    img.save()
                    img.filepath = old_fp
                    print(f"   [Save] {raw_name} (generated)")
                
                processed_images.add(img.name)

            except Exception as e:
                print(f"   [Error] Texture {raw_name}: {e}")

        # FBX Export를 위해 경로 변경
        img.filepath = dst_path

    def convert_transform_aurora(self, obj):
        loc, rot, scl = obj.matrix_local.decompose()
        out_pos = [loc.x, loc.z, loc.y, 0.0]
        out_rot = [rot.x, -rot.z, -rot.y, rot.w]
        out_scl = [scl.x, scl.z, scl.y, 1.0]
        return out_pos, out_rot, out_scl

    def export_mesh_fbx(self, obj):
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
                        img = node.image
                        if img not in images_to_relink:
                            images_to_relink[img] = img.filepath

        # 2. Relink & Export
        try:
            for img, old_path in images_to_relink.items():
                new_abs_path = self.get_texture_destination_path(img)
                img.filepath = new_abs_path
            
            original_active = self.context.view_layer.objects.active
            original_selected = self.context.selected_objects
            
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            self.context.view_layer.objects.active = obj

            bpy.ops.export_scene.fbx(
                filepath=fbx_full_path,
                use_selection=True,
                global_scale=1.0,
                apply_unit_scale=True,
                axis_forward=self.FBX_AXIS_FORWARD,
                axis_up=self.FBX_AXIS_UP,
                object_types={'MESH'},
                use_mesh_modifiers=True,
                embed_textures=False,
                path_mode='RELATIVE',
                bake_space_transform=True 
            )
            
            bpy.ops.object.select_all(action='DESELECT')
            for sel_obj in original_selected:
                sel_obj.select_set(True)
            self.context.view_layer.objects.active = original_active

        finally:
            for img, old_path in images_to_relink.items():
                img.filepath = old_path

        return f"{self.scene_name}/{fbx_filename}"

    def create_model_component(self, obj):
        model_path = self.export_mesh_fbx(obj)
        
        base_color = [1.0, 1.0, 1.0, 1.0]
        emission = [0.0, 0.0, 0.0, 0.0]
        
        mat_data = {
            "ambientOcclusionFactor": 1.0,
            "baseColorFactor": base_color,
            "emissionFactor": emission,
            "metallicFactor": 0.0,
            "normalScale": 1.0,
            "roughnessFactor": 1.0
        }

        component = {
            "blendState": int(obj.get("ae_blend", 0)),
            "materialFactorData": mat_data,
            "modelFileName": model_path,
            "psShaderName": obj.get("ae_ps", "PSModel.hlsl"),
            "rasterState": int(obj.get("ae_raster", 1)),
            "type": "ModelComponent",
            "vsShaderName": obj.get("ae_vs", "VSModel.hlsl"),
        }
        return component

    def parse_game_object(self, obj):
        pos, rot, scl = self.convert_transform_aurora(obj)
        
        game_object = {
            "childGameObjects": [],
            "components": [],
            "name": obj.name,
            "position": pos,
            "rotation": rot,
            "scale": scl,
            "type": "GameObjectBase",
        }

        if obj.type == 'MESH':
            print(f" -> Exporting Mesh: {obj.name}")
            comp = self.create_model_component(obj)
            game_object["components"].append(comp)

        for child in obj.children:
            child_data = self.parse_game_object(child)
            game_object["childGameObjects"].append(child_data)
            
        return game_object

    def execute(self):
        print(f"=== Aurora Map Baking Start: {self.scene_name} ===")
        
        self.ensure_directory(self.PATH_SCENE)
        self.ensure_directory(self.PATH_MODELS)
        self.ensure_directory(self.PATH_TEXTURES)

        selected_objects = self.context.selected_objects
        if not selected_objects:
            return False, "선택된 오브젝트가 없습니다."

        root_objects = []
        if self.context.active_object and self.context.active_object.mode != 'OBJECT':
            bpy.ops.object.mode_set(mode='OBJECT')

        for obj in selected_objects:
            if obj.parent is None or obj.parent not in selected_objects:
                root_objects.append(obj)

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
            scene_json["rootGameObjects"].append(root_data)

        with open(self.JSON_OUTPUT_PATH, 'w', encoding='utf-8') as f:
            json.dump(scene_json, f, indent=4)

        return True, f"Baking 완료! ({self.JSON_OUTPUT_PATH})"


# ==============================================================================
# [UI & Operator] 블렌더 인터페이스
# ==============================================================================

class AURORA_OT_BakeScene(bpy.types.Operator):
    """선택한 오브젝트를 Aurora Engine 포맷으로 내보냅니다."""
    bl_idname = "aurora.bake_scene"
    bl_label = "Bake Scene"
    
    def execute(self, context):
        props = context.scene.aurora_props
        
        if not props.project_root:
            self.report({'ERROR'}, "Project Root 경로를 설정해주세요.")
            return {'CANCELLED'}
            
        if not props.scene_name:
            self.report({'ERROR'}, "Scene Name을 설정해주세요.")
            return {'CANCELLED'}

        # 실행
        exporter = AuroraExporter(context, props.project_root, props.scene_name)
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
        box.prop(props, "project_root", text="Project Root")
        box.prop(props, "scene_name", text="Scene Name")
        
        layout.separator()
        
        row = layout.row()
        row.scale_y = 1.5
        row.operator("aurora.bake_scene", icon='EXPORT')
        
        layout.label(text="Output Info:", icon='INFO')
        layout.label(text=f"Scene: .../Scene/{props.scene_name}.json")
        layout.label(text=f"Model: .../Model/{props.scene_name}/")

# 프로퍼티 그룹 (설정값 저장용)
class AuroraProperties(bpy.types.PropertyGroup):
    project_root: bpy.props.StringProperty(
        name="Project Root",
        description="오로라 엔진 프로젝트 루트 폴더 (Asset 폴더 상위)",
        default=r"C:\dev\AuroraEngine\Asset",
        subtype='DIR_PATH' # 폴더 아이콘 활성화
    )
    scene_name: bpy.props.StringProperty(
        name="Scene Name",
        description="생성될 씬 이름 (JSON 파일명 및 폴더명)",
        default="HyojeTestScene"
    )

classes = (
    AuroraProperties,
    AURORA_OT_BakeScene,
    AURORA_PT_Panel,
)

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