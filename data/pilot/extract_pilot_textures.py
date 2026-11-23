#!/usr/bin/env python3
"""AI-assisted: recover the pilot textures lost in the Blender GLB -> OBJ export.

Usage (from this folder):
    python3 -I extract_pilot_textures.py low-poly_psx_pilot_flying_a_plane_animated.glb Untitled.mtl

The GLB material uses KHR_materials_pbrSpecularGlossiness, which Blender's
importer does not connect, so the exported MTL has no map_Kd. This script reads
the GLB directly (standard library only) and:
  - writes the embedded images: pilot_diffuse.<ext> (colour) and
    pilot_normal.<ext> (normal map, not used by the game's shader);
  - rewrites the MTL with map_Kd pointing to the colour image.
"""
import json
import struct
import sys

MIME_EXT = {"image/jpeg": "jpg", "image/png": "png"}


def read_glb(path):
    data = open(path, "rb").read()
    magic, version, _ = struct.unpack("<4sII", data[:12])
    if magic != b"glTF" or version != 2:
        sys.exit("not a glTF 2.0 binary file: " + path)
    gltf, binary, offset = None, None, 12
    while offset < len(data):
        length, kind = struct.unpack("<I4s", data[offset:offset + 8])
        chunk = data[offset + 8:offset + 8 + length]
        if kind == b"JSON":
            gltf = json.loads(chunk)
        elif kind == b"BIN\0":
            binary = chunk
        offset += 8 + length
    return gltf, binary


def image_bytes(gltf, binary, texture_index):
    image = gltf["images"][gltf["textures"][texture_index]["source"]]
    view = gltf["bufferViews"][image["bufferView"]]
    start = view.get("byteOffset", 0)
    return binary[start:start + view["byteLength"]], MIME_EXT[image["mimeType"]]


def main(glb_path, mtl_path):
    gltf, binary = read_glb(glb_path)
    material = gltf["materials"][0]
    specgloss = material.get("extensions", {}).get("KHR_materials_pbrSpecularGlossiness", {})
    diffuse = specgloss.get("diffuseTexture") or \
        material.get("pbrMetallicRoughness", {}).get("baseColorTexture")
    if diffuse is None:
        sys.exit("material has no colour texture")
    written = {}
    for role, ref in (("diffuse", diffuse), ("normal", material.get("normalTexture"))):
        if ref is None:
            continue
        data, ext = image_bytes(gltf, binary, ref["index"])
        name = "pilot_%s.%s" % (role, ext)
        open(name, "wb").write(data)
        written[role] = name
        print("wrote %s (%d bytes)" % (name, len(data)))
    # Rewrite the MTL: white Kd so the texture colour is not darkened, no
    # specular highlight (the GLB has specularFactor 0), and the texture map.
    lines = []
    for line in open(mtl_path):
        key = line.split()[0] if line.split() else ""
        if key == "Kd":
            line = "Kd 1.000000 1.000000 1.000000\n"
        elif key == "Ks":
            line = "Ks 0.000000 0.000000 0.000000\n"
        elif key == "Ni":
            line = "Ni 1.450000\n"
        elif key.startswith("map_"):
            continue
        lines.append(line)
    lines.append("map_Kd %s\n" % written["diffuse"])
    open(mtl_path, "w").writelines(lines)
    print("updated %s with map_Kd %s" % (mtl_path, written["diffuse"]))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
