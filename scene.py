bl_addon_info = {
"name": "SCENE Export",
"author": "Matej Drame",
"version": (0, 1),
"blender": (2, 5, 6),
"api": 32412,
"location": "View3D > File > Export",
"description": "Export to .scene format",
"warning": "",
"wiki_url": "http://wiki.blender.org/index.php/Extensions:2.5/Py/Scripts/Import-Export/Raw_Mesh_IO",
"tracker_url": "https://projects.blender.org/tracker/index.php?func=detail&aid=21733&group_id=153&atid=469",
"category": "Import-Export"}

import bpy
from bpy.props import *
from struct import pack
from io import BytesIO

class SceneExporter(bpy.types.Operator):
  '''Exports to SCENE'''
  bl_idname = 'export.scene'
  bl_label = 'Export SCENE'
  bl_options = {'REGISTER', 'UNDO'}

  filepath = StringProperty(name="File Path",
    description="Filepath used for exporting the SCENE file.",
    maxlen= 1024,
    default= "", subtype='FILE_PATH')
  check_existing = BoolProperty(name="Check Existing",
    description="Check and warn on overwriting existing files.",
    default=True, options={'HIDDEN'})

  def execute(self, context):
    """Exports all selected objects into scene. Lines containing names of objects in the scene.
    """
    objects = [object for object in bpy.context.visible_objects if object.type == 'MESH']
    with open(self.filepath, 'w') as f:
      for object in objects:
        f.write(object.name + '\n')
    return {'FINISHED'}

  def invoke(self, context, event):
    wm = context.window_manager
    wm.fileselect_add(self)
    return {'RUNNING_MODAL'}

def menu_export(self, context):
  import os
  default_path = os.path.splitext(bpy.data.filepath)[0] + ".scene"
  self.layout.operator(SceneExporter.bl_idname, text="SCENE (.scene)").filepath = default_path

def register():
  bpy.types.INFO_MT_file_export.append(menu_export)

def unregister():
  bpy.types.INFO_MT_file_export.remove(menu_export)

if __name__ == "__main__":
  register()
