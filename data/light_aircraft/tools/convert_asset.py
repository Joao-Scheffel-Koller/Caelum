"""AI-assisted data conversion for MagicGames' CC BY 4.0 airplane.

Run with Blender --background --factory-startup --python this_file.py.
No code from the downloaded FBX is executed. Its source license is retained.
"""
import bpy
import json
import math
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "source" / "Airplane.fbx"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(SOURCE))
objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
points = [o.matrix_world @ v.co for o in objects for v in o.data.vertices]
low = Vector(tuple(min(p[k] for p in points) for k in range(3)))
high = Vector(tuple(max(p[k] for p in points) for k in range(3)))
longitudinal_center = (low.y + high.y) / 2
lateral_center = (low.x + high.x) / 2
length_scale = 9.0 / (high.y - low.y)
span_scale = 11.0 / (high.x - low.x)
height_scale = 3.0 / (high.z - low.z)

def convert(p):
    # Original: nose -Y, up +Z. Caelum: nose +X, up +Y, right +Z.
    return Vector((-(p.y-longitudinal_center)*length_scale,
                   (p.z-low.z)*height_scale,
                   -(p.x-lateral_center)*span_scale))

triangles = []
materials = {}
for obj in objects:
    obj.data.calc_loop_triangles()
    for tri in obj.data.loop_triangles:
        material = obj.data.materials[tri.material_index] if obj.data.materials else None
        name = material.name if material else 'Body'
        color = list(material.diffuse_color[:3]) if material else [.8,.8,.8]
        # The original exterior has opaque blue windows. This derivative opens
        # visibility through them, then adds a small untextured cabin below.
        opacity = .18 if name == 'LightBlue' else 1.0
        if obj.name in {'wing','wing.001','wing.002'}:
            # A static opaque cartoon propeller otherwise blocks the pilot's
            # forward view. Translucency suggests motion without claiming a
            # propeller animation; the hub retains the original opaque finish.
            name = 'Propeller_'+name
            opacity = .24
        materials[name] = (color, opacity)
        triangles.append((obj.name, name, [convert(obj.matrix_world @ obj.data.vertices[i].co) for i in tri.vertices]))

def box(name, material, center, size):
    c, s = Vector(center), Vector(size)*.5
    vertices = [c+Vector((s.x if i&1 else -s.x,s.y if i&2 else -s.y,s.z if i&4 else -s.z)) for i in range(8)]
    for face in [(0,4,6,2),(1,3,7,5),(0,1,5,4),(2,6,7,3),(0,2,3,1),(4,5,7,6)]:
        for indices in [(face[0],face[1],face[2]),(face[0],face[2],face[3])]:
            triangles.append((name,material,[vertices[i] for i in indices]))

materials['CabinPanel'] = ([.035,.045,.055],1.0)
materials['CabinSeat'] = ([.055,.065,.075],1.0)
materials['CabinFloor'] = ([.08,.09,.10],1.0)
# These simple additions are new geometry, not part of the original asset.
box('CabinDashboard','CabinPanel',(3.04,1.88,0),(.16,.28,1.05))
box('CabinFloor','CabinFloor',(2.25,1.38,0),(1.65,.08,.90))
for side in [-1,1]:
    box('SeatBase','CabinSeat',(2.18,1.62,side*.30),(.50,.15,.42))
    box('SeatBack','CabinSeat',(1.94,1.88,side*.30),(.12,.60,.42))

lines = ['# FONTE: Low Poly Airplane by MagicGames, CC BY 4.0.',
         '# https://magic-games.itch.io/low-poly-airplane',
         '# Derivative: Caelum orientation/scale, transparent windows/propeller, simple cabin.',
         'mtllib aircraft.mtl', 'vt 0 0']
last_object = last_material = None
vertex_index = normal_index = 0
for name, material, vertices in triangles:
    if name != last_object:
        lines.append('o '+name.replace(' ','_')); last_object=name
    if material != last_material:
        lines.append('usemtl '+material); last_material=material
    normal = (vertices[1]-vertices[0]).cross(vertices[2]-vertices[0])
    if normal.length < 1e-10: continue
    normal.normalize()
    for p in vertices: lines.append('v %.8f %.8f %.8f'%tuple(p))
    normal_index += 1
    lines.append('vn %.8f %.8f %.8f'%tuple(normal))
    lines.append('f '+' '.join('%d/1/%d'%(vertex_index+i+1,normal_index) for i in range(3)))
    vertex_index += 3
(ROOT/'aircraft.obj').write_text('\n'.join(lines)+'\n',encoding='utf-8')
mtl = ['# FONTE: MagicGames, CC BY 4.0; modified windows/cabin for Caelum.']
for name, (color, opacity) in materials.items():
    mtl.extend(['newmtl '+name,'Kd %.6f %.6f %.6f'%tuple(color),'d %.3f'%opacity,
                'Ks 0.04 0.04 0.04','Ns 32','illum 2',''])
(ROOT/'aircraft.mtl').write_text('\n'.join(mtl),encoding='utf-8')
all_points = [p for _,_,vs in triangles for p in vs]
metadata = {
    'title':'Low Poly Airplane', 'author':'MagicGames',
    'source':'https://magic-games.itch.io/low-poly-airplane',
    'license':'CC BY 4.0', 'license_url':'https://creativecommons.org/licenses/by/4.0/',
    'coordinates':'meters, nose +X, up +Y, right +Z; pivot midpoint XZ/minY',
    'bounds_min':[min(p[k] for p in all_points) for k in range(3)],
    'bounds_max':[max(p[k] for p in all_points) for k in range(3)],
    'triangles':normal_index,
    'cockpit_eye':[2.44,2.40,-.26],
    'changes':['Normalized axis orientation and approximate meter scale (9m length,11m span,3m height)',
               'Blue window material made transparent',
               'Propeller blades translucent for clear cockpit visibility; no animation',
               'Added simple dashboard, floor and two seats; not a detailed Cessna cockpit'],
    'source_bounds_min':list(low),'source_bounds_max':list(high),
    'original_archive_sha256':'49e90bd99accf7647ddffb8b60f2a61646323c831719fc1f5e5419a9e26a1945',
    'original_fbx_sha256':'8d55c2e30326e3e1168fc5b07bcf631e9c44a90bfced6d0343ddb66b18bd9863',
    'gear':[]
}
for obj_name, side in [('Circle',-1),('Circle',1),('Circle.001',0)]:
    obj=bpy.data.objects[obj_name]
    pts=[convert(obj.matrix_world @ v.co) for v in obj.data.vertices
         if side==0 or (obj.matrix_world@v.co).x*side>0]
    lo=Vector(tuple(min(p[k] for p in pts) for k in range(3)))
    hi=Vector(tuple(max(p[k] for p in pts) for k in range(3)))
    center=(lo+hi)*.5
    metadata['gear'].append({'name':'tail' if side==0 else 'main', 'center':list(center),
                             'radius':(hi.y-lo.y)*.5,
                             'x_fraction':(center.x-metadata['bounds_min'][0])/9.0,
                             'z_fraction':center.z/11.0})
(ROOT/'metadata.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
print(json.dumps(metadata,indent=2))
