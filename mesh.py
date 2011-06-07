bl_addon_info = {
"name": "MESH Export",
"author": "Matej Drame",
"version": (0, 3),
"blender": (2, 5, 7),
"api": 35266,
"location": "File > Export",
"description": "Export to .mesh format",
"warning": "",
"wiki_url": "",
"tracker_url": "",
"category": "Import-Export"}

import bpy
from bpy.props import *
from struct import pack
from io import BytesIO
import os

def export_mesh(obj, scene_path, mesh_name):
  """
  // Format:
  // format version (1 byte)
  // nVertices (4 bytes)
  // nIndices (4 bytes)
  // vertexSize (1 byte)
  // indexSize (1 byte)
  // vertexBuffer (nVertices * vertexSize)
  // indexBuffer (nIndices * indexSize)
  """
  #bpy.ops.object.mode_set(mode='EDIT')
  #bpy.ops.mesh.select_all(action='SELECT')
  #bpy.ops.mesh.quads_convert_to_tris() # TODO: copy mesh before
  #bpy.ops.object.mode_set(mode='OBJECT')
  # TODO: triangulate if necessary

  mesh = obj.data
  vertices = []
  flat = False
  if 'flat' in obj.game.properties and obj.game.properties['flat'].value:
    flat = True
  n_uvs = len(mesh.uv_textures)
  for face_index, face in enumerate(mesh.faces):
    vertex = [mesh.vertices[face.vertices[0]].co, mesh.vertices[face.vertices[0]].normal]
    if flat:
      vertex[1] = face.normal
    for u in range(n_uvs):
      uv_data = mesh.uv_textures[u].data
      coords = uv_data[face_index]
      vertex.append(coords.uv1)
    vertices.append(vertex)

    vertex = [mesh.vertices[face.vertices[1]].co, mesh.vertices[face.vertices[1]].normal]
    if flat:
      vertex[1] = face.normal
    for u in range(n_uvs):
      uv_data = mesh.uv_textures[u].data
      coords = uv_data[face_index]
      vertex.append(coords.uv2)
    vertices.append(vertex)

    vertex = [mesh.vertices[face.vertices[2]].co, mesh.vertices[face.vertices[2]].normal]
    if flat:
      vertex[1] = face.normal
    for u in range(n_uvs):
      uv_data = mesh.uv_textures[u].data
      coords = uv_data[face_index]
      vertex.append(coords.uv3)
    vertices.append(vertex)

  filepath = os.path.join(os.path.dirname(scene_path), mesh_name)
  with open(filepath, 'wb') as f:
    f.write(pack('<I', 2))
    f.write(pack('<I', len(vertices)))
    f.write(pack('<I', len(vertices)))
    vertex_size = sum(len(component)*4 for component in vertices[0])
    f.write(pack('<I', vertex_size))
    f.write(pack('<I', 4))

    for v in vertices:
      f.write(pack('<fff', *v[0]))
      f.write(pack('<fff', *v[1]))
      for uv in v[2:]:
        f.write(pack('<ff', *uv))

    for index in range(len(vertices)):
      f.write(pack('<I', index))

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
    obj = bpy.context.selected_objects[0]
    assert obj.type == 'MESH'
    print('Exporting mesh ' + obj.name)
    export_mesh(obj, self.filepath, obj.name.split('.')[0] + '.mesh')
    print('Done')
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
  bpy.utils.register_module(__name__)
  bpy.types.INFO_MT_file_export.append(menu_export)

def unregister():
  bpy.utils.unregister_module(__name__)
  bpy.types.INFO_MT_file_export.remove(menu_export)

if __name__ == "__main__":
  register()
