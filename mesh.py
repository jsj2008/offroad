bl_addon_info = {
"name": "MESH Export",
"author": "Matej Drame",
"version": (0, 2),
"blender": (2, 5, 6),
"api": 32412,
"location": "View3D > File > Export",
"description": "Export to .mesh format",
"warning": "",
"wiki_url": "http://wiki.blender.org/index.php/Extensions:2.5/Py/Scripts/Import-Export/Raw_Mesh_IO",
"tracker_url": "https://projects.blender.org/tracker/index.php?func=detail&aid=21733&group_id=153&atid=469",
"category": "Import-Export"}

import bpy
from bpy.props import *
from struct import pack
from io import BytesIO

class MeshExporter(bpy.types.Operator):
  '''Exports to MESH'''
  bl_idname = 'export_mesh.mesh'
  bl_label = 'Export MESH'
  bl_options = {'REGISTER', 'UNDO'}

  filepath = StringProperty(name="File Path",
    description="Filepath used for exporting the MESH file.",
    maxlen= 1024,
    default= "", subtype='FILE_PATH')
  check_existing = BoolProperty(name="Check Existing",
    description="Check and warn on overwriting existing files.",
    default=True, options={'HIDDEN'})
  triangulate = BoolProperty(name="Triangulate",
    description="Triangulate quads.", default=True)
  duplicate = BoolProperty(name="Duplicate",
    description="Duplicate vertices across face.", default=False)
  includeCoords = BoolProperty(name="Coords",
    description="Include texture coordinates.", default=True)

  def execute(self, context):
    """Exports all selected objects into mesh format.

    // Format:
    // format version (1 byte)
    // nVertices (4 bytes)
    // nIndices (4 bytes)
    // vertexSize (1 byte)
    // indexSize (1 byte)
    // vertexBuffer (nVertices * vertexSize)
    // indexBuffer (nIndices * indexSize)
    """
    bio = BytesIO()
    obj = bpy.context.selected_objects[0]
    assert obj.type == 'MESH'
    mesh = obj.data
    vertices = [[vertex.co, vertex.normal] for vertex in mesh.vertices]
    indices = []
    if self.includeCoords:
      uv_data = mesh.uv_textures[0].data
      uvs = []
    for index, face in enumerate(mesh.faces):
      if len(face.vertices) == 4:
        indices.extend(face.vertices[:3])
        indices.extend(face.vertices[2:4])
        indices.append(face.vertices[0])
      else:
        indices.extend(face.vertices)
      if self.includeCoords:
        for i in range(len(face.vertices)):
          if len(vertices[face.vertices[i]]) == 2:
            vertices[face.vertices[i]].append(uv_data[index].uv_raw[i*2 : i*2+2])

    with open(self.filepath, 'wb') as f:
      f.write(pack('<I', 1))              # version
      f.write(pack('<I', len(vertices)))  # nVertices
      f.write(pack('<I', len(indices)))   # nIndices
      f.write(pack('<I', 32 if self.includeCoords else 24))             # vertexSize
      f.write(pack('<I', 4))              # indexSize

      for v in vertices:
        f.write(pack('<fff', *v[0]))
        f.write(pack('<fff', *v[1]))
        if self.includeCoords:
          f.write(pack('<ff', *v[2]))

      for index in indices:
        f.write(pack('<I', index))
    return {'FINISHED'}

  def invoke(self, context, event):
    wm = context.window_manager
    wm.fileselect_add(self)
    return {'RUNNING_MODAL'}

def menu_export(self, context):
  import os
  default_path = os.path.splitext(bpy.data.filepath)[0] + ".mesh"
  self.layout.operator(MeshExporter.bl_idname, text="MESH (.mesh)").filepath = default_path

def register():
  bpy.types.INFO_MT_file_export.append(menu_export)

def unregister():
  bpy.types.INFO_MT_file_export.remove(menu_export)

if __name__ == "__main__":
  register()
