# -*- coding: utf-8 -*-
# Build:
#   python -m PyInstaller --onefile --noconsole --icon Icon/Icon.ico fbx_scale_fixer_gui.py
# 실행 폴더 구조(윈도우 창 아이콘용):
#   fbx_scale_fixer_gui.exe
#   Icon/Icon.ico

import os
import sys
import re
import shutil
import tempfile
import subprocess
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

# Windows registry (optional)
try:
    import winreg
except ImportError:
    winreg = None


# -----------------------------
# App icon helpers
# -----------------------------
def app_base_dir() -> str:
    """EXE로 빌드된 경우 exe가 있는 폴더, 스크립트 실행이면 .py가 있는 폴더"""
    if getattr(sys, "frozen", False):
        return os.path.dirname(sys.executable)
    return os.path.dirname(os.path.abspath(__file__))


def app_icon_path() -> str:
    """현재 폴더(=exe 옆 또는 py 옆)의 Icon/Icon.ico"""
    return os.path.join(app_base_dir(), "Icon", "Icon.ico")


def decode_best_effort(b: bytes) -> str:
    """윈도우/블렌더 출력 인코딩이 섞일 때 안전하게 문자열로 변환"""
    if b is None:
        return ""
    for enc in ("utf-8", "cp949", "euc-kr", "latin-1"):
        try:
            return b.decode(enc)
        except UnicodeDecodeError:
            continue
    return b.decode("utf-8", errors="replace")


# -----------------------------
# Blender batch script
#   요구사항:
#     1) Dimensions(보이는 크기) 유지
#     2) Scale=1.0으로 정규화
#     3) Rotation도 0,0,0으로(=현재 모습 유지한 채 Ctrl-A Rotation bake)
#     4) Export 시 Apply Unit + Apply Scalings: FBX Units Scale 적용
#     5) 그 외 옵션 유지
# -----------------------------
BLENDER_BATCH_SCRIPT = r'''
import bpy
import os
import sys
import traceback
from mathutils import Matrix

def log(msg):
    print(msg, flush=True)

def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    if bpy.ops.object.mode_set.poll():
        try:
            bpy.ops.object.mode_set(mode='OBJECT')
        except:
            pass

    # 단위는 기본(미터)로 고정(혼란 방지)
    try:
        us = bpy.context.scene.unit_settings
        us.system = 'METRIC'
        us.scale_length = 1.0
    except:
        pass

def import_fbx(path):
    bpy.ops.import_scene.fbx(
        filepath=path,
        use_image_search=True
    )

def _hierarchy_depth(obj):
    d = 0
    p = obj.parent
    while p is not None:
        d += 1
        p = p.parent
    return d

def normalize_transforms_keep_dimensions(apply_armature_transform=False):
    """
    목표:
      - 현재 보이는 Dimensions(크기)는 그대로
      - Scale = 1,1,1
      - Rotation = 0,0,0 (현재 모습 유지한 채 Rotation을 메시 데이터에 bake)
        => Ctrl-A: Rotation + Scale 효과

    중요:
      - 회전/스케일을 오브젝트 트랜스폼이 아닌 "데이터"로 굽는다.
      - Armature는 기본 False(리깅 깨질 수 있어서). 필요하면 옵션으로 켬.
    """

    if bpy.ops.object.mode_set.poll():
        try:
            bpy.ops.object.mode_set(mode='OBJECT')
        except:
            pass

    selectable_types = {"MESH"}  # 핵심은 Mesh
    if apply_armature_transform:
        selectable_types.add("ARMATURE")

    objs = [o for o in bpy.context.scene.objects if o.type in selectable_types]
    # 부모->자식 순서가 안전(부모 적용 시 자식 보정이 더 자연스러움)
    objs.sort(key=_hierarchy_depth)

    def is_non_identity_rot_scale(obj, eps=1e-6):
        s = obj.scale
        r = obj.rotation_euler
        non_scale = (abs(s.x - 1.0) > eps) or (abs(s.y - 1.0) > eps) or (abs(s.z - 1.0) > eps)
        non_rot = (abs(r.x) > eps) or (abs(r.y) > eps) or (abs(r.z) > eps)
        return non_scale or non_rot

    for obj in objs:
        try:
            if not is_non_identity_rot_scale(obj):
                continue

            dim_before = obj.dimensions.copy()
            sc_before = obj.scale.copy()
            rot_before = obj.rotation_euler.copy()

            # operator 컨텍스트
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj

            # Ctrl-A: Rotation + Scale (Location은 유지)
            bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

            dim_after = obj.dimensions.copy()
            sc_after = obj.scale.copy()
            rot_after = obj.rotation_euler.copy()

            # 대부분 여기서 끝. 그래도 dimensions가 눈에 띄게 흔들리면 MESH만 보정.
            if obj.type == "MESH":
                def safe_ratio(a, b):
                    if abs(b) < 1e-12:
                        return 1.0
                    return a / b

                rx = safe_ratio(dim_before.x, dim_after.x)
                ry = safe_ratio(dim_before.y, dim_after.y)
                rz = safe_ratio(dim_before.z, dim_after.z)

                if (abs(rx - 1.0) > 1e-4) or (abs(ry - 1.0) > 1e-4) or (abs(rz - 1.0) > 1e-4):
                    m = Matrix.Diagonal((rx, ry, rz, 1.0))
                    obj.data.transform(m)
                    obj.data.update()

                    dim_after2 = obj.dimensions.copy()
                    log(f"   [Bake] {obj.name}: rot {tuple(rot_before)} -> {tuple(rot_after)} | scale {tuple(sc_before)} -> {tuple(sc_after)} | dim {tuple(dim_before)} -> {tuple(dim_after2)}")
                else:
                    log(f"   [Bake] {obj.name}: rot {tuple(rot_before)} -> {tuple(rot_after)} | scale {tuple(sc_before)} -> {tuple(sc_after)} | dim 유지")
            else:
                log(f"   [Bake] {obj.name}: rot {tuple(rot_before)} -> {tuple(rot_after)} | scale {tuple(sc_before)} -> {tuple(sc_after)}")

        except Exception as e:
            log(f"   [Bake][WARN] {obj.name} failed: {e}")

def export_fbx(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)

    bpy.ops.export_scene.fbx(
        filepath=path,
        use_selection=False,
        use_active_collection=False,

        # 스케일 관련(요청사항)
        global_scale=1.0,
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_UNITS',  # Apply Scalings = FBX Units Scale

        # 축/트랜스폼 (유지)
        axis_forward='-Z',
        axis_up='Y',
        use_space_transform=True,
        bake_space_transform=True,  # Apply Transform

        # 오브젝트 타입
        object_types={'EMPTY', 'CAMERA', 'LIGHT', 'ARMATURE', 'MESH', 'OTHER'},

        # Leaf Bones off
        add_leaf_bones=False,

        # Bone axis
        primary_bone_axis='Y',
        secondary_bone_axis='X',

        use_mesh_modifiers=True,
        use_custom_props=False,
        path_mode='AUTO',
        embed_textures=False,
    )

def iter_fbx_files(root):
    for dirpath, _, filenames in os.walk(root):
        for fn in filenames:
            if fn.lower().endswith(".fbx"):
                yield os.path.join(dirpath, fn)

def safe_overwrite(src_tmp, dst):
    os.replace(src_tmp, dst)

def main():
    argv = sys.argv
    argv = argv[argv.index("--") + 1:] if "--" in argv else []
    if not argv:
        log("[ERROR] No target directory provided.")
        return 2

    target_root = os.path.abspath(argv[0])

    # 옵션: Armature도 transform bake 할지 여부 (기본 False)
    apply_armature_transform = ("--apply_armature_scale" in argv)

    log("=== Blender FBX Batch Fix ===")
    log(f"Target : {target_root}")
    log(f"Apply Armature Transform : {apply_armature_transform}")
    log("Goal : keep Dimensions, set Scale=1.0 & Rotation=0, export with FBX Units Scale")
    log("=============================")

    if not os.path.isdir(target_root):
        log("[ERROR] Target is not a directory.")
        return 2

    files = list(iter_fbx_files(target_root))
    log(f"Found {len(files)} FBX files.")

    errors = []

    for i, in_path in enumerate(files, 1):
        try:
            log(f"\n[{i}/{len(files)}] {in_path}")

            base_dir = os.path.dirname(in_path)
            base_name = os.path.splitext(os.path.basename(in_path))[0]
            tmp_out = os.path.join(base_dir, base_name + ".__fbxfix_tmp__.fbx")

            reset_scene()
            import_fbx(in_path)

            # 핵심: Rotation/Scale bake 해서 rot=0, scale=1, dim 유지
            normalize_transforms_keep_dimensions(apply_armature_transform=apply_armature_transform)

            export_fbx(tmp_out)
            safe_overwrite(tmp_out, in_path)
            log("   OK (overwritten)")

        except Exception as e:
            log("   FAILED: " + str(e))
            traceback.print_exc()
            errors.append((in_path, str(e)))

            try:
                if os.path.exists(tmp_out):
                    os.remove(tmp_out)
            except:
                pass

    log("\n=== Done ===")
    if errors:
        log(f"Errors: {len(errors)}")
        for p, msg in errors:
            log(" - " + p + " : " + msg)
        return 1
    else:
        log("All files processed successfully.")
        return 0

if __name__ == "__main__":
    raise SystemExit(main())
'''


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("FBX Scale Fixer (Blender Batch)")
        self.geometry("820x520")
        self.resizable(True, True)

        ico = app_icon_path()
        if os.path.exists(ico):
            try:
                self.iconbitmap(ico)
            except:
                pass

        self.blender_path_var = tk.StringVar()
        self.target_dir_var = tk.StringVar()
        self.running = False
        self.proc = None

        row1 = ttk.Frame(self)
        row1.pack(fill="x", padx=12, pady=(12, 6))
        ttk.Label(row1, text="Blender Path").pack(side="left")
        ttk.Entry(row1, textvariable=self.blender_path_var).pack(side="left", fill="x", expand=True, padx=8)
        ttk.Button(row1, text="Browse...", command=self.pick_blender).pack(side="left")

        row2 = ttk.Frame(self)
        row2.pack(fill="x", padx=12, pady=6)
        ttk.Label(row2, text="Target Folder").pack(side="left")
        ttk.Entry(row2, textvariable=self.target_dir_var).pack(side="left", fill="x", expand=True, padx=8)
        ttk.Button(row2, text="Browse...", command=self.pick_target).pack(side="left")

        row_opt = ttk.Frame(self)
        row_opt.pack(fill="x", padx=12, pady=(0, 6))

        # 기존 옵션명은 그대로 두되, 실제로는 Armature에 Rotation/Scale bake까지 포함(리깅 주의)
        self.armature_scale_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(
            row_opt, text="(주의) Apply Armature Transform too", variable=self.armature_scale_var
        ).pack(side="left")

        self.run_btn = ttk.Button(row_opt, text="RUN (Overwrite Originals)", command=self.run)
        self.run_btn.pack(side="right")

        self.progress = ttk.Progressbar(self, mode="indeterminate")
        self.progress.pack(fill="x", padx=12, pady=(0, 8))

        ttk.Label(self, text="Log").pack(anchor="w", padx=12)
        self.text = tk.Text(self, height=18)
        self.text.pack(fill="both", expand=True, padx=12, pady=(0, 12))
        self.text.configure(state="disabled")

        self.autofill_blender_path()

    def append_log(self, s: str):
        self.text.configure(state="normal")
        self.text.insert("end", s + "\n")
        self.text.see("end")
        self.text.configure(state="disabled")

    # -----------------------------
    # Blender auto-discovery
    # -----------------------------
    def _parse_version_score(self, blender_exe_path: str) -> int:
        p = blender_exe_path.lower()
        score = 0
        if "lts" in p:
            score += 1000

        m = re.search(r'blender[\s_-]?(\d+)\.(\d+)(?:\.(\d+))?', p)
        if not m:
            m = re.search(r'\\(\d+)\.(\d+)(?:\.(\d+))?\\blender\.exe$', p)
        if not m:
            m = re.search(r'(\d+)\.(\d+)(?:\.(\d+))?', p)

        if m:
            major = int(m.group(1))
            minor = int(m.group(2))
            patch = int(m.group(3)) if m.group(3) else 0
            score += major * 1_000_000 + minor * 10_000 + patch * 100

        if "\\program files" in p:
            score += 50
        if "\\program files (x86)" in p:
            score += 10
        return score

    def _find_blender_candidates_fast(self):
        candidates = set()

        try:
            which = shutil.which("blender")
            if which and which.lower().endswith("blender.exe") and os.path.exists(which):
                candidates.add(os.path.abspath(which))
        except:
            pass

        if winreg:
            reg_paths = [
                (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\blender.exe"),
                (winreg.HKEY_CURRENT_USER, r"SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\blender.exe"),
            ]
            for root, subkey in reg_paths:
                try:
                    with winreg.OpenKey(root, subkey) as k:
                        val, _ = winreg.QueryValueEx(k, "")
                        if val and os.path.exists(val):
                            candidates.add(os.path.abspath(val))
                except:
                    pass

        pf_roots = [
            os.environ.get("ProgramFiles", r"C:\Program Files"),
            os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
        ]
        for pf in pf_roots:
            if not pf or not os.path.isdir(pf):
                continue
            base = os.path.join(pf, "Blender Foundation")
            if os.path.isdir(base):
                try:
                    for entry in os.listdir(base):
                        d = os.path.join(base, entry)
                        exe = os.path.join(d, "blender.exe")
                        if os.path.exists(exe):
                            candidates.add(os.path.abspath(exe))
                except:
                    pass

        return sorted(candidates)

    def _find_blender_candidates_deep_c(self, max_results=50):
        candidates = set()

        priority_dirs = [
            r"C:\Program Files\Blender Foundation",
            r"C:\Program Files (x86)\Blender Foundation",
            r"C:\Program Files\Steam",
            r"C:\Program Files (x86)\Steam",
            os.path.join(os.environ.get("LOCALAPPDATA", ""), "Programs"),
        ]

        for base in priority_dirs:
            if base and os.path.isdir(base):
                for root, _, files in os.walk(base):
                    for f in files:
                        if f.lower() == "blender.exe":
                            candidates.add(os.path.abspath(os.path.join(root, f)))
                            if len(candidates) >= max_results:
                                return sorted(candidates)

        for root, _, files in os.walk(r"C:\\"):
            low = root.lower()
            if any(x in low for x in [
                r"\windows",
                r"\programdata",
                r"\$recycle.bin",
                r"\system volume information",
            ]):
                continue
            for f in files:
                if f.lower() == "blender.exe":
                    candidates.add(os.path.abspath(os.path.join(root, f)))
                    if len(candidates) >= max_results:
                        return sorted(candidates)

        return sorted(candidates)

    def autofill_blender_path(self):
        if self.blender_path_var.get():
            return

        candidates = self._find_blender_candidates_fast()
        if not candidates:
            self.append_log("[Auto] Blender not found in common locations. Deep scanning C:\\ (may take a bit)...")
            candidates = self._find_blender_candidates_deep_c()

        if not candidates:
            self.append_log("[Auto] Blender not found. Please select blender.exe manually.")
            return

        best = max(candidates, key=self._parse_version_score)
        self.blender_path_var.set(best)
        self.append_log(f"[Auto] Found Blender: {best}")

    # -----------------------------
    # UI handlers
    # -----------------------------
    def pick_blender(self):
        path = filedialog.askopenfilename(
            title="Select blender.exe",
            filetypes=[("Blender Executable", "blender.exe"), ("All files", "*.*")]
        )
        if path:
            self.blender_path_var.set(path)

    def pick_target(self):
        path = filedialog.askdirectory(title="Select Target Folder")
        if path:
            self.target_dir_var.set(path)

    def validate(self):
        blender = self.blender_path_var.get().strip('" ')
        target = self.target_dir_var.get().strip('" ')
        if not blender or not os.path.isfile(blender):
            messagebox.showerror("Error", "Valid blender.exe path is required.")
            return None
        if not target or not os.path.isdir(target):
            messagebox.showerror("Error", "Valid target folder is required.")
            return None
        return blender, target

    def run(self):
        if self.running:
            messagebox.showinfo("Running", "Already running.")
            return

        vt = self.validate()
        if not vt:
            return
        blender, target = vt

        if not messagebox.askyesno(
            "Confirm Overwrite",
            "This will OVERWRITE all .fbx files under the target folder.\n\nProceed?"
        ):
            return

        tmp_dir = tempfile.mkdtemp(prefix="fbxfix_")
        script_path = os.path.join(tmp_dir, "blender_batch_fbx_fix.py")
        with open(script_path, "w", encoding="utf-8") as f:
            f.write(BLENDER_BATCH_SCRIPT)

        args = [blender, "-b", "-P", script_path, "--", target]
        if self.armature_scale_var.get():
            # Blender 스크립트 안에서는 이 플래그를 "armature transform bake"로 사용
            args.append("--apply_armature_scale")

        self.append_log("===================================")
        self.append_log("RUNNING Blender batch...")
        self.append_log("Command: " + " ".join(f'"{a}"' if " " in a else a for a in args))
        self.append_log("===================================")

        self.progress.start(10)
        self.run_btn.configure(state="disabled")
        self.running = True

        def worker():
            try:
                env = os.environ.copy()
                env["PYTHONIOENCODING"] = "utf-8"

                self.proc = subprocess.Popen(
                    args,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    env=env,
                )

                assert self.proc.stdout is not None
                for raw in iter(self.proc.stdout.readline, b""):
                    if not raw:
                        break
                    line = decode_best_effort(raw).rstrip("\r\n")
                    self.safe_log(line)

                rc = self.proc.wait()
                self.safe_log(f"\n[Exit Code] {rc}")

            except Exception as e:
                self.safe_log(f"[ERROR] {e}")
            finally:
                self.safe_done()

        threading.Thread(target=worker, daemon=True).start()

    def safe_log(self, msg):
        self.after(0, lambda: self.append_log(msg))

    def safe_done(self):
        def done_ui():
            self.progress.stop()
            self.run_btn.configure(state="normal")
            self.running = False
            self.proc = None
            self.append_log("=== Finished ===")
        self.after(0, done_ui)


if __name__ == "__main__":
    app = App()
    app.mainloop()
