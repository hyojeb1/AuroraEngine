# 
# https://github.com/Cosmic-Disaster/AliceBlenderBatchFBX
# 
# -*- coding: utf-8 -*-
# Build:
#   python -m PyInstaller --onefile --noconsole --icon Icon/Icon.ico fbx_scale_fixer_gui.py

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
    if getattr(sys, "frozen", False):
        return os.path.dirname(sys.executable)
    return os.path.dirname(os.path.abspath(__file__))


def app_icon_path() -> str:
    return os.path.join(app_base_dir(), "Icon", "Icon.ico")


def decode_best_effort(b: bytes) -> str:
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
    if bpy.ops.object.mode_set.poll():
        try:
            bpy.ops.object.mode_set(mode='OBJECT')
        except:
            pass

    selectable_types = {"MESH"} 
    if apply_armature_transform:
        selectable_types.add("ARMATURE")

    objs = [o for o in bpy.context.scene.objects if o.type in selectable_types]
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

            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj

            bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

            dim_after = obj.dimensions.copy()
            sc_after = obj.scale.copy()
            rot_after = obj.rotation_euler.copy()

            if obj.type == "MESH":
                def safe_ratio(a, b):
                    if abs(b) < 1e-12: return 1.0
                    return a / b

                rx = safe_ratio(dim_before.x, dim_after.x)
                ry = safe_ratio(dim_before.y, dim_after.y)
                rz = safe_ratio(dim_before.z, dim_after.z)

                if (abs(rx - 1.0) > 1e-4) or (abs(ry - 1.0) > 1e-4) or (abs(rz - 1.0) > 1e-4):
                    m = Matrix.Diagonal((rx, ry, rz, 1.0))
                    obj.data.transform(m)
                    obj.data.update()
                    dim_after2 = obj.dimensions.copy()
                    log(f"   [Bake] {obj.name}: Corrected Dimensions.")
                else:
                    log(f"   [Bake] {obj.name}: Dimensions OK.")
            else:
                log(f"   [Bake] {obj.name}: Applied.")

        except Exception as e:
            log(f"   [Bake][WARN] {obj.name} failed: {e}")

def export_fbx(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.export_scene.fbx(
        filepath=path,
        use_selection=False,
        use_active_collection=False,
        global_scale=1.0,
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_UNITS',
        axis_forward='-Z',
        axis_up='Y',
        use_space_transform=True,
        bake_space_transform=True,
        object_types={'EMPTY', 'CAMERA', 'LIGHT', 'ARMATURE', 'MESH', 'OTHER'},
        add_leaf_bones=False,
        primary_bone_axis='Y',
        secondary_bone_axis='X',
        use_mesh_modifiers=True,
        use_custom_props=False,
        path_mode='AUTO',
        embed_textures=False,
    )

def safe_overwrite(src_tmp, dst):
    if os.path.exists(dst):
        os.remove(dst)
    os.replace(src_tmp, dst)

def main():
    argv = sys.argv
    argv = argv[argv.index("--") + 1:] if "--" in argv else []
    
    if not argv:
        log("[ERROR] No input file provided.")
        return 2

    # argv[0] is the path to the text file containing the list of FBX files
    input_list_file = argv[0]
    apply_armature_transform = ("--apply_armature_scale" in argv)

    log("=== Blender FBX Batch Fix ===")
    log(f"Input List : {input_list_file}")
    log(f"Apply Armature : {apply_armature_transform}")
    
    files_to_process = []
    if os.path.isfile(input_list_file):
        with open(input_list_file, 'r', encoding='utf-8') as f:
            for line in f:
                path = line.strip()
                if path and os.path.exists(path):
                    files_to_process.append(path)
    else:
        log("[ERROR] Input list file not found.")
        return 2

    log(f"Total Files to Process: {len(files_to_process)}")

    errors = []

    for i, in_path in enumerate(files_to_process, 1):
        try:
            log(f"\n[{i}/{len(files_to_process)}] {in_path}")
            
            base_dir = os.path.dirname(in_path)
            base_name = os.path.splitext(os.path.basename(in_path))[0]
            tmp_out = os.path.join(base_dir, base_name + ".__fbxfix_tmp__.fbx")

            reset_scene()
            import_fbx(in_path)
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
            except: pass

    log("\n=== Done ===")
    if errors:
        log(f"Errors: {len(errors)}")
        return 1
    else:
        log("All files processed successfully.")
        return 0

if __name__ == "__main__":
    raise SystemExit(main())
'''

# -----------------------------
# Scrollable Checkbox List Helper
# -----------------------------
class ScrollableCheckFrame(ttk.Frame):
    def __init__(self, container, *args, **kwargs):
        super().__init__(container, *args, **kwargs)
        self.canvas = tk.Canvas(self, height=150, bg="#ffffff")
        self.scrollbar = ttk.Scrollbar(self, orient="vertical", command=self.canvas.yview)
        self.scrollable_frame = ttk.Frame(self.canvas)

        self.scrollable_frame.bind(
            "<Configure>",
            lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all"))
        )

        self.canvas.create_window((0, 0), window=self.scrollable_frame, anchor="nw")
        self.canvas.configure(yscrollcommand=self.scrollbar.set)

        self.canvas.pack(side="left", fill="both", expand=True)
        self.scrollbar.pack(side="right", fill="y")
        
        # Mousewheel scrolling
        self.canvas.bind_all("<MouseWheel>", self._on_mousewheel)
        
        self.check_vars = {} # path -> BooleanVar

    def _on_mousewheel(self, event):
        # Only scroll if needed
        if self.canvas.winfo_height() < self.scrollable_frame.winfo_height():
            self.canvas.yview_scroll(int(-1*(event.delta/120)), "units")

    def set_files(self, file_paths):
        # Clear old
        for widget in self.scrollable_frame.winfo_children():
            widget.destroy()
        self.check_vars.clear()

        for fpath in file_paths:
            var = tk.BooleanVar(value=True) # Default checked
            self.check_vars[fpath] = var
            
            # Display simpler name, but store full path
            display_name = os.path.basename(fpath)
            parent_dir = os.path.basename(os.path.dirname(fpath))
            full_display = f"[{parent_dir}] {display_name}"

            chk = ttk.Checkbutton(self.scrollable_frame, text=full_display, variable=var)
            chk.pack(anchor="w", padx=5, pady=2)

    def get_checked_files(self):
        return [path for path, var in self.check_vars.items() if var.get()]

    def set_all(self, value: bool):
        for var in self.check_vars.values():
            var.set(value)


# -----------------------------
# Main Application
# -----------------------------
class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("FBX Scale Fixer (Blender Batch)")
        self.geometry("820x650") # Increased height for list
        
        ico = app_icon_path()
        if os.path.exists(ico):
            try:
                self.iconbitmap(ico)
            except: pass

        self.blender_path_var = tk.StringVar()
        self.target_dir_var = tk.StringVar()
        self.running = False
        self.proc = None

        # 1. Blender Path
        row1 = ttk.Frame(self)
        row1.pack(fill="x", padx=12, pady=(12, 6))
        ttk.Label(row1, text="Blender Path").pack(side="left")
        ttk.Entry(row1, textvariable=self.blender_path_var).pack(side="left", fill="x", expand=True, padx=8)
        ttk.Button(row1, text="Browse...", command=self.pick_blender).pack(side="left")

        # 2. Target Folder
        row2 = ttk.Frame(self)
        row2.pack(fill="x", padx=12, pady=6)
        ttk.Label(row2, text="Target Folder").pack(side="left")
        ttk.Entry(row2, textvariable=self.target_dir_var).pack(side="left", fill="x", expand=True, padx=8)
        ttk.Button(row2, text="Browse...", command=self.pick_target).pack(side="left")

        # 3. File Selection Controls
        row_ctrl = ttk.Frame(self)
        row_ctrl.pack(fill="x", padx=12, pady=(10, 2))
        ttk.Label(row_ctrl, text="Files found:").pack(side="left")
        ttk.Button(row_ctrl, text="Uncheck All", command=lambda: self.file_list.set_all(False)).pack(side="right")
        ttk.Button(row_ctrl, text="Check All", command=lambda: self.file_list.set_all(True)).pack(side="right", padx=5)

        # 4. File List Area
        self.file_list = ScrollableCheckFrame(self, borderwidth=1, relief="sunken")
        self.file_list.pack(fill="both", expand=True, padx=12, pady=2)

        # 5. Options & Run
        row_opt = ttk.Frame(self)
        row_opt.pack(fill="x", padx=12, pady=(10, 6))

        self.armature_scale_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(
            row_opt, text="(주의) Apply Armature Transform too", variable=self.armature_scale_var
        ).pack(side="left")

        self.run_btn = ttk.Button(row_opt, text="RUN (Selected Files)", command=self.run)
        self.run_btn.pack(side="right")

        # 6. Progress & Log
        self.progress = ttk.Progressbar(self, mode="indeterminate")
        self.progress.pack(fill="x", padx=12, pady=(0, 8))

        ttk.Label(self, text="Log").pack(anchor="w", padx=12)
        self.text = tk.Text(self, height=10) # Log slightly smaller
        self.text.pack(fill="x", padx=12, pady=(0, 12))
        self.text.configure(state="disabled")

        self.autofill_blender_path()

    def append_log(self, s: str):
        self.text.configure(state="normal")
        self.text.insert("end", s + "\n")
        self.text.see("end")
        self.text.configure(state="disabled")

    # ... (Blender Discovery Logic unchanged) ...
    def _parse_version_score(self, blender_exe_path: str) -> int:
        p = blender_exe_path.lower()
        score = 0
        if "lts" in p: score += 1000
        m = re.search(r'blender[\s_-]?(\d+)\.(\d+)', p)
        if m: score += int(m.group(1))*10000 + int(m.group(2))*100
        if "\\program files" in p: score += 50
        return score

    def _find_blender_candidates_fast(self):
        candidates = set()
        try:
            which = shutil.which("blender")
            if which and which.lower().endswith("blender.exe"): candidates.add(os.path.abspath(which))
        except: pass
        
        pf_roots = [os.environ.get("ProgramFiles", r"C:\Program Files"), os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")]
        for pf in pf_roots:
            if not pf: continue
            base = os.path.join(pf, "Blender Foundation")
            if os.path.isdir(base):
                for entry in os.listdir(base):
                    exe = os.path.join(base, entry, "blender.exe")
                    if os.path.exists(exe): candidates.add(os.path.abspath(exe))
        return sorted(candidates)

    def autofill_blender_path(self):
        if self.blender_path_var.get(): return
        cands = self._find_blender_candidates_fast()
        if cands:
            best = max(cands, key=self._parse_version_score)
            self.blender_path_var.set(best)
            self.append_log(f"[Auto] Found Blender: {best}")
        else:
            self.append_log("[Auto] Blender not found automatically.")

    # -----------------------------
    # UI Handlers
    # -----------------------------
    def pick_blender(self):
        path = filedialog.askopenfilename(title="Select blender.exe", filetypes=[("Blender", "blender.exe")])
        if path: self.blender_path_var.set(path)

    def pick_target(self):
        path = filedialog.askdirectory(title="Select Target Folder")
        if path:
            self.target_dir_var.set(path)
            self.scan_files(path)

    def scan_files(self, root_dir):
        # Find all fbx files
        found = []
        for dirpath, _, filenames in os.walk(root_dir):
            for fn in filenames:
                if fn.lower().endswith(".fbx"):
                    found.append(os.path.join(dirpath, fn))
        
        self.file_list.set_files(found)
        self.append_log(f"Scanned: found {len(found)} FBX files.")

    def run(self):
        if self.running: return

        blender = self.blender_path_var.get().strip('" ')
        target = self.target_dir_var.get().strip('" ')
        
        if not os.path.isfile(blender):
            messagebox.showerror("Error", "Blender path is invalid.")
            return

        # Get checked files
        selected_files = self.file_list.get_checked_files()
        if not selected_files:
            messagebox.showwarning("Warning", "No files selected.")
            return

        if not messagebox.askyesno("Confirm", f"Process {len(selected_files)} files?\n(Originals will be overwritten)"):
            return

        # Write selected files to temp file
        tmp_dir = tempfile.mkdtemp(prefix="fbxfix_")
        list_file = os.path.join(tmp_dir, "file_list.txt")
        script_path = os.path.join(tmp_dir, "blender_batch.py")

        with open(list_file, "w", encoding="utf-8") as f:
            for p in selected_files:
                f.write(p + "\n")

        with open(script_path, "w", encoding="utf-8") as f:
            f.write(BLENDER_BATCH_SCRIPT)

        args = [blender, "-b", "-P", script_path, "--", list_file]
        if self.armature_scale_var.get():
            args.append("--apply_armature_scale")

        self.append_log(f"Starting batch for {len(selected_files)} files...")
        self.progress.start(10)
        self.run_btn.configure(state="disabled")
        self.running = True

        def worker():
            try:
                env = os.environ.copy()
                env["PYTHONIOENCODING"] = "utf-8"
                self.proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
                
                for raw in iter(self.proc.stdout.readline, b""):
                    if not raw: break
                    line = decode_best_effort(raw).rstrip("\r\n")
                    self.safe_log(line)
                
                self.proc.wait()
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
            self.append_log("=== Finished ===")
        self.after(0, done_ui)

if __name__ == "__main__":
    app = App()
    app.mainloop()