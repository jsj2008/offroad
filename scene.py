bl_addon_info = {
"name": "SCENE Export",
"author": "Matej Drame",
"version": (0, 2),
"blender": (2, 5, 7),
"api": 35266,
"location": "File > Export",
"description": "Export to .scene format",
"warning": "",
"wiki_url": "",
"tracker_url": "",
"category": "Import-Export"}

import bpy
from bpy.props import *
from struct import pack
from io import BytesIO
from io_utils import ExportHelper
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

class SceneExporter(bpy.types.Operator, ExportHelper):
  '''Exports to SCENE'''
  bl_idname = 'export.scene'
  bl_label = 'Export SCENE'
  bl_options = {'REGISTER', 'UNDO'}

  filename_ext = '.scene'

  filepath = StringProperty(name="File Path",
    description="Filepath used for exporting the SCENE file.",
    maxlen= 1024,
    default= "", subtype='FILE_PATH')
  check_existing = BoolProperty(name="Check Existing",
    description="Check and warn on overwriting existing files.",
    default=True, options={'HIDDEN'})

  def execute(self, context):
    """Exports all visible objects (meshes) into .scene. Also exports meshes into separate files.
      Object with the same base name (before dot) are consider to be instances of the same mesh -
      those meshes are exported only once.
      Object must have materials applied (one material max, but unlimited uvs/textures).
    """
    objects = [object for object in bpy.context.visible_objects if object.type == 'MESH']
    with open(self.filepath, 'w') as f:
      f.write('<scene>\n') # TODO: does Python 3 have no built-in simple XML writers?!!
      exported_meshes = []
      for obj in objects:
        x,y,z = obj.location
        rot = obj.rotation_quaternion
        if obj.active_material == None:
          print(obj.name + ' does not have a material assigned!')
          continue
        slots = obj.active_material.texture_slots
        n_slots = len(list(filter(lambda s: s != None, slots)))
        shader = 'color.shader'
        if 'shader' in obj.game.properties:
          shader = obj.game.properties['shader'].value
        mesh = obj.name.split('.')[0] + '.mesh'

        f.write('<object ')
        f.write('name="%s" shader="%s" mesh="%s"' % (obj.name, shader, mesh))
        for i in range(n_slots):
          f.write(' texture%d="%s"' % (i, os.path.basename(slots[i].texture.image.filepath)))
        f.write('>\n')
        f.write('  <position x="%f" y="%f" z="%f" />\n' % (x,y,z))
        f.write('  <rotation x="%f" y="%f" z="%f" w="%f" />\n' % (rot.x, rot.y, rot.z, rot.w))
        f.write('</object>\n')
        # export appropriate mesh
        if mesh not in exported_meshes:
          print('Exporting ' + mesh)
          export_mesh(obj, self.filepath, mesh)
          exported_meshes.append(mesh)
      f.write('</scene>\n')
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
  bpy.utils.register_module(__name__)
  bpy.types.INFO_MT_file_export.append(menu_export)

def unregister():
  bpy.utils.unregister_module(__name__)
  bpy.types.INFO_MT_file_export.remove(menu_export)

if __name__ == "__main__":
  register()
