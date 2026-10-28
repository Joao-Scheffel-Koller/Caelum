"""Deterministic spatial OBJ splitter, written with AI assistance (Codex).

Faces belong to the XZ cell containing their centroid. Oversized cells are
quartered recursively. Coordinates, winding, materials, normals and UVs are
preserved; faces crossing a cell boundary remain whole. Extremely dense
coincident geometry raises an explicit error after 24 subdivision levels.
Uses only Python's standard library; streams face records through scratch files.
"""
import argparse
from array import array
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path, help='New output directory (must not exist)')
    parser.add_argument('--cell-size', type=float, default=2000)
    parser.add_argument('--max-bytes', type=int, default=48_000_000)
    args = parser.parse_args()
    if args.cell_size <= 0 or args.max_bytes < 1024:
        parser.error('cell-size must be positive and max-bytes >= 1024')
    if args.output.exists():
        parser.error('output already exists; choose a new directory')
    args.output.mkdir(parents=True)
    vertices, texcoords, normals = array('d'), [], array('d')
    materials, material_ids, libraries = [], {}, []
    buckets, handles, counts = {}, {}, Counter()
    current_material = -1
    input_digest = hashlib.sha256()
    face_digest = hashlib.sha256()

    def resolve(token):
        fields = token.split('/')
        limits = (len(vertices)//3, len(texcoords), len(normals)//3)
        result = []
        for i in range(3):
            if i >= len(fields) or not fields[i]: result.append(''); continue
            index = int(fields[i])
            index = index if index > 0 else limits[i]+index+1
            if index <= 0 or index > limits[i]:
                raise ValueError(f'Invalid or forward OBJ index: {token}')
            result.append(str(index))
        return '/'.join(result).rstrip('/')

    def center(tokens):
        ids = [int(t.split('/')[0])-1 for t in tokens]
        return (sum(vertices[3*i] for i in ids)/len(ids),
                sum(vertices[3*i+2] for i in ids)/len(ids))

    with tempfile.TemporaryDirectory(prefix='.caelum_obj_',dir=args.output.parent) as scratch_name:
        scratch = Path(scratch_name)
        with args.source.open('rb') as source:
            for raw in source:
                input_digest.update(raw)
                line = raw.decode('utf-8-sig').strip()
                if not line or line.startswith('#'): continue
                command, _, content = line.partition(' ')
                values = content.split()
                counts[command] += 1
                if command == 'v':
                    if len(values) != 3: raise ValueError('Expected XYZ vertices without colors/weights')
                    vertices.extend(map(float, values))
                elif command == 'vt': texcoords.append(tuple(map(float, values)))
                elif command == 'vn': normals.extend(map(float, values))
                elif command == 'mtllib': libraries.append(content.strip())
                elif command == 'usemtl':
                    if content not in material_ids:
                        material_ids[content] = len(materials); materials.append(content)
                    current_material = material_ids[content]
                elif command == 'f':
                    if len(values) < 3: raise ValueError('Face has fewer than 3 vertices')
                    tokens = [resolve(v) for v in values]
                    x,z = center(tokens)
                    cell = (math.floor(x/args.cell_size), math.floor(z/args.cell_size))
                    record = str(current_material)+' '+' '.join(tokens)+'\n'
                    face_digest.update(record.encode())
                    if cell not in buckets:
                        buckets[cell] = scratch/f'cell_{cell[0]}_{cell[1]}.faces'
                        handles[cell] = buckets[cell].open('w',encoding='ascii',newline='\n')
                    handles[cell].write(record)
                elif command not in ('o','g','s'):
                    raise ValueError(f'Unsupported OBJ element: {command}')
        for handle in handles.values(): handle.close()
        print(f'Read {counts["f"]:,} faces; {len(buckets)} initial cells', flush=True)

        # Bundle every referenced texture, keeping options such as -clamp on.
        texture_dir = args.output/'textures'; texture_dir.mkdir()
        copied = {}
        texture_manifest = []
        for library in sorted(set(libraries)):
            original_mtl = args.source.parent/library
            output_lines = []
            for line in original_mtl.read_text(encoding='utf-8-sig').splitlines():
                if re.match(r'\s*(?:map_\S+|bump|disp|decal)\s',line):
                    match = re.match(r'^(\s*\S+\s+)((?:-\S+\s+\S+\s+)*)(.*)$',line)
                    if not match: raise ValueError(f'Unrecognized material image reference: {line}')
                    prefix,options,reference = match.groups()
                    reference = reference.strip().strip('"')
                    path = Path(reference)
                    if not path.is_absolute(): path = original_mtl.parent/path
                    if not path.is_file(): raise FileNotFoundError(path)
                    digest = hashlib.sha256(path.read_bytes()).hexdigest()
                    # Content-addressed prefixes avoid filename collisions and machine paths.
                    name = digest[:12]+'_'+path.name
                    if name not in copied:
                        shutil.copy2(path,texture_dir/name);copied[name]=digest
                        texture_manifest.append({'file':'textures/'+name,'sha256':digest})
                    line = prefix+options+'textures/'+name
                output_lines.append(line)
            (args.output/Path(library).name).write_text('\n'.join(output_lines).rstrip()+'\n',encoding='utf-8',newline='\n')

        outputs=[]
        def write_cell(path,name,bounds,depth=0):
            target=args.output/(name+'.obj')
            maps=[{}, {}, {}]; totals=[0,0,0]; used_material=None; face_count=0
            low=[math.inf]*3;high=[-math.inf]*3
            with target.open('w',encoding='utf-8',newline='\n') as out:
                out.write('# Deterministic Caelum map chunk. Shared coordinates, Y up.\n')
                for lib in sorted(set(libraries)):out.write('mtllib '+Path(lib).name+'\n')
                out.write('o '+name+'\ns off\n')
                for line in path.open(encoding='ascii'):
                    parts=line.split(); mat_id=int(parts[0]); remapped=[]
                    if mat_id!=used_material:
                        if mat_id>=0:out.write('usemtl '+materials[mat_id]+'\n')
                        elif used_material is not None:raise ValueError('Cannot unset an OBJ material')
                        used_material=mat_id
                    for token in parts[1:]:
                        idx=token.split('/');new=[]
                        for kind in range(3):
                            if kind>=len(idx) or not idx[kind]:new.append('');continue
                            old=int(idx[kind]);mapping=maps[kind]
                            if old not in mapping:
                                totals[kind]+=1;mapping[old]=totals[kind]
                                start=3*(old-1)
                                coords=(vertices[start:start+3] if kind==0 else texcoords[old-1] if kind==1 else normals[start:start+3])
                                out.write(('v','vt','vn')[kind]+' '+' '.join(map(repr,coords))+'\n')
                                if kind==0:
                                    for axis,value in enumerate(coords):low[axis]=min(low[axis],value);high[axis]=max(high[axis],value)
                            new.append(str(mapping[old]))
                        remapped.append('/'.join(new).rstrip('/'))
                    out.write('f '+' '.join(remapped)+'\n');face_count+=1
            size=target.stat().st_size
            if size>args.max_bytes:
                target.unlink() # Only this invocation's newly generated temporary result.
                xmin,zmin,xmax,zmax=bounds;xm=(xmin+xmax)/2;zm=(zmin+zmax)/2
                child_bounds=[(xmin,zmin,xm,zm),(xm,zmin,xmax,zm),(xmin,zm,xm,zmax),(xm,zm,xmax,zmax)]
                child_paths=[scratch/(name+f'_{i}.faces') for i in range(4)]
                children=[p.open('w',encoding='ascii',newline='\n') for p in child_paths]
                child_counts=[0]*4
                with path.open(encoding='ascii') as src:
                    for record in src:
                        x,z=center(record.split()[1:]);i=int(x>=xm)+2*int(z>=zm)
                        children[i].write(record);child_counts[i]+=1
                for child in children:child.close()
                if depth>=24:raise ValueError('Coincident geometry exceeds size limit; reduce source complexity')
                for i,count in enumerate(child_counts):
                    if count:write_cell(child_paths[i],name+f'_{i}',child_bounds[i],depth+1)
            else:
                outputs.append({'file':target.name,'bytes':size,'faces':face_count,'bounds':[low,high], 'sha256':hashlib.sha256(target.read_bytes()).hexdigest()})
                print(f'{target.name}: {size/1e6:.1f} MB, {face_count:,} faces',flush=True)

        for (x,z),path in sorted(buckets.items()):
            write_cell(path,f'rio_x{x:+04d}_z{z:+04d}',(x*args.cell_size,z*args.cell_size,(x+1)*args.cell_size,(z+1)*args.cell_size))

        # Independent readback verifies exact face content, multiplicity and indices.
        verified=Counter()
        for item in outputs:
            v=[None];t=[None];n=[None];mat_id=-1
            with (args.output/item['file']).open(encoding='utf-8') as f:
                for line in f:
                    fields=line.split()
                    if not fields:continue
                    cmd=fields[0]
                    if cmd=='v':v.append(tuple(map(float,fields[1:])))
                    elif cmd=='vt':t.append(tuple(map(float,fields[1:])))
                    elif cmd=='vn':n.append(tuple(map(float,fields[1:])))
                    elif cmd=='usemtl':mat_id=material_ids[line.strip()[7:]]
                    elif cmd=='f':
                        face=[]
                        for token in fields[1:]:
                            indices=token.split('/');attrs=[]
                            for k,table in enumerate((v,t,n)):
                                attrs.append(table[int(indices[k])] if k<len(indices) and indices[k] else None)
                            face.append(attrs)
                        digest=hashlib.sha256(repr((mat_id,face)).encode()).hexdigest()
                        verified[digest]+=1
        expected=Counter()
        for path in buckets.values():
            for line in path.open(encoding='ascii'):
                fields=line.split();face=[]
                for token in fields[1:]:
                    indices=token.split('/');attrs=[]
                    for kind in range(3):
                        if kind>=len(indices) or not indices[kind]:attrs.append(None);continue
                        i=int(indices[kind])-1
                        attrs.append(tuple(vertices[3*i:3*i+3]) if kind==0 else texcoords[i] if kind==1 else tuple(normals[3*i:3*i+3]))
                    face.append(attrs)
                expected[hashlib.sha256(repr((int(fields[0]),face)).encode()).hexdigest()]+=1
        assert expected==verified,'Face geometry/material/UV/normal mismatch'
        assert sum(o['faces'] for o in outputs)==counts['f']
        manifest={'source_sha256':input_digest.hexdigest(),'source_faces':counts['f'],
                  'cell_size_m':args.cell_size,'max_bytes':args.max_bytes,
                  'rule':'XZ centroid, floor grid, recursive NW/NE/SW/SE quadrants (-Z is north); original face order within cells',
                  'coordinates':'Original OSM2World coordinates; Y up; origin lat -22.84895 lon -43.2142',
                  'validation':'Exact face multiset (ordered vertices, UVs, normals, material) verified after readback',
                  'chunks':outputs,'textures':sorted(texture_manifest,key=lambda t:t['file'])}
        (args.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
        print(f'VERIFIED: {len(outputs)} chunks, {counts["f"]:,} faces preserved.',flush=True)


if __name__=='__main__':main()
