"""AI-assisted bounded background for gaps in the OSM2World ground mesh.
FONTE: bounds/origin of the original OSM extract; OpenStreetMap contributors, ODbL.
The textured foundation stays below existing roads, land and water; the renderer
adds positive depth bias so it cannot replace existing visible surfaces.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import xml.etree.ElementTree as ET


def project(lat,lon,lat0=-22.84895,lon0=-43.2142):
    # FONTE: OSM2World MetricMapProjection/MercatorProjection convention.
    scale=40075016.686*math.cos(math.radians(lat0))
    def mercator(latitude):
        sine=math.sin(math.radians(latitude))
        return math.log((1+sine)/(1-sine))/(4*math.pi)
    return ((lon-lon0)*scale/360,-scale*(mercator(lat)-mercator(lat0)))


def build(source,output):
    bounds=None
    with source.open('rb') as stream:
        for _,element in ET.iterparse(stream,events=('end',)):
            if element.tag=='bounds':bounds=dict(element.attrib);break
    if bounds is None:raise ValueError('The original OSM bounds are required')
    lat0=(float(bounds['minlat'])+float(bounds['maxlat']))/2
    lon0=(float(bounds['minlon'])+float(bounds['maxlon']))/2
    minx,minz=project(float(bounds['maxlat']),float(bounds['minlon']),lat0,lon0)
    maxx,maxz=project(float(bounds['minlat']),float(bounds['maxlon']),lat0,lon0)
    points=[(minx,minz),(minx,maxz),(maxx,maxz),(maxx,minz)]
    lines=['# Bounded background from the original OSM extract; not new land classification.',
        '# OpenStreetMap contributors, ODbL: https://www.openstreetmap.org/copyright',
        '# AI-assisted offline preparation; existing water/streets cover this low surface.',
        'mtllib rio.obj.mtl','o base_terrain','usemtl GRASS_0']
    for x,z in points:lines.append(f'v {x:.8f} -0.02 {z:.8f}')
    for x,z in points:lines.append(f'vt {x:.8f} {-z:.8f}')
    lines+=['vn 0 1 0','f 1/1/1 2/2/1 3/3/1','f 1/1/1 3/3/1 4/4/1']
    output.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    digest=hashlib.sha256()
    with source.open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):digest.update(block)
    metadata={'source':'OpenStreetMap rio.osm','license':'ODbL','source_sha256':digest.hexdigest(),
        'bounds':bounds,'origin':[lat0,lon0],'height':-.02,'material':'GRASS_0',
        'purpose':'Fallback only; existing geometry/depth defines the land/water appearance',
        'obj_sha256':hashlib.sha256(output.read_bytes()).hexdigest()}
    output.with_suffix('.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('Bounded base terrain: 4 vertices, 2 triangles,',output.stat().st_size,'bytes')
    return metadata


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('osm',type=Path);parser.add_argument('output',type=Path)
    args=parser.parse_args();build(args.osm,args.output)
