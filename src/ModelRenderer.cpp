#include "ModelRenderer.hpp"
#include <tiny_obj_loader.h>
#include <stb_image.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <cstdio>
#include <cmath>

namespace {
std::string Directory(const std::string& path) {
    const auto i=path.find_last_of("/\\");
    return i==std::string::npos ? "" : path.substr(0,i+1);
}
std::string Join(const std::string& base,const std::string& file) {
    if (file.empty() || file[0]=='/' || file.find(':')!=std::string::npos) return file;
    return base+file;
}
struct Vertex { glm::vec3 position,normal; glm::vec2 uv; };
// Manifest schema is deliberately fixed: no general-purpose JSON parser needed.
std::string StringValue(const std::string& line) {
    const auto start=line.find('"',line.find(':')+1);
    const auto end=line.find('"',start+1);
    if(start==std::string::npos||end==std::string::npos) throw std::runtime_error("Invalid manifest string");
    return line.substr(start+1,end-start-1);
}
}

GLuint ModelRenderer::LoadTexture(const std::string& path) {
    auto found=textures.find(path);
    if(found!=textures.end())return found->second;
    int w,h,n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* pixels=stbi_load(path.c_str(),&w,&h,&n,4);
    if(!pixels)throw std::runtime_error("Cannot load texture: "+path);
    GLuint texture;glGenTextures(1,&texture);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    glGenerateMipmap(GL_TEXTURE_2D);stbi_image_free(pixels);
    textures[path]=texture;return texture;
}

RenderModel ModelRenderer::Load(const std::string& path) {
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warning,error,base=Directory(path);
    if(!tinyobj::LoadObj(&attributes,&shapes,&materials,&warning,&error,path.c_str(),base.c_str(),true))
        throw std::runtime_error(path+": "+error);
    RenderModel result;
    result.min=glm::vec3(std::numeric_limits<float>::max());
    result.max=glm::vec3(std::numeric_limits<float>::lowest());
    std::map<int,std::vector<Vertex>> grouped;
    for(const auto& shape:shapes) {
        size_t offset=0;
        for(size_t face=0;face<shape.mesh.num_face_vertices.size();++face) {
            const int count=shape.mesh.num_face_vertices[face];
            if(count!=3)throw std::runtime_error("Non-triangulated mesh: "+path);
            int id=shape.mesh.material_ids[face];
            Vertex triangle[3];
            for(int corner=0;corner<3;++corner) {
                auto index=shape.mesh.indices[offset+corner];
                Vertex& v=triangle[corner];
                v.position=glm::vec3(attributes.vertices[3*index.vertex_index],attributes.vertices[3*index.vertex_index+1],attributes.vertices[3*index.vertex_index+2]);
                result.min=glm::min(result.min,v.position);result.max=glm::max(result.max,v.position);
                v.normal=glm::vec3(0);
                if(index.normal_index>=0)v.normal=glm::vec3(attributes.normals[3*index.normal_index],attributes.normals[3*index.normal_index+1],attributes.normals[3*index.normal_index+2]);
                v.uv=index.texcoord_index<0?glm::vec2(0):glm::vec2(attributes.texcoords[2*index.texcoord_index],attributes.texcoords[2*index.texcoord_index+1]);
            }
            glm::vec3 normal=glm::cross(triangle[1].position-triangle[0].position,triangle[2].position-triangle[0].position);
            normal=glm::dot(normal,normal)>1e-12f?glm::normalize(normal):glm::vec3(0,1,0);
            for(auto& v:triangle) {if(glm::dot(v.normal,v.normal)<1e-12f)v.normal=normal;grouped[id].push_back(v);}
            offset+=3;
        }
    }
    for(auto& group:grouped) {
        RenderBatch batch;
        if(group.first>=0 && size_t(group.first)<materials.size()) {
            const auto& m=materials[group.first];
            batch.material.diffuse=glm::vec3(m.diffuse[0],m.diffuse[1],m.diffuse[2]);
            batch.material.opacity=m.dissolve;
            batch.material.clamp=m.diffuse_texopt.clamp;
            if(!m.diffuse_texname.empty())batch.material.texture=LoadTexture(Join(base,m.diffuse_texname));
        }
        batch.count=GLsizei(group.second.size());
        glm::vec3 low(std::numeric_limits<float>::max()),high(std::numeric_limits<float>::lowest());
        for(const auto& vertex:group.second){low=glm::min(low,vertex.position);high=glm::max(high,vertex.position);}
        batch.center=(low+high)*.5f;
        glGenVertexArrays(1,&batch.vao);glBindVertexArray(batch.vao);
        glGenBuffers(1,&batch.vbo);glBindBuffer(GL_ARRAY_BUFFER,batch.vbo);
        glBufferData(GL_ARRAY_BUFFER,group.second.size()*sizeof(Vertex),group.second.data(),GL_STATIC_DRAW);
        for(GLuint attribute=0;attribute<3;++attribute) {
            const size_t offsets[]={offsetof(Vertex,position),offsetof(Vertex,normal),offsetof(Vertex,uv)};
            glVertexAttribPointer(attribute,attribute==2?2:3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsets[attribute]));
            glEnableVertexAttribArray(attribute);
        }
        result.batches.push_back(batch);
    }
    glBindVertexArray(0);
    if(result.batches.empty())throw std::runtime_error("Empty OBJ: "+path);
    return result;
}

std::vector<MapChunk> ModelRenderer::LoadMap(const std::string& manifest) {
    std::ifstream input(manifest);
    if(!input)throw std::runtime_error("Cannot open map manifest: "+manifest);
    std::vector<MapChunk> result;
    std::string line;
    bool chunks=false;
    while(std::getline(input,line)) {
        if(line.find("\"chunks\"")!=std::string::npos)chunks=true;
        if(line.find("\"textures\"")!=std::string::npos)break;
        if(chunks&&line.find("\"file\"")!=std::string::npos) {
            MapChunk chunk;chunk.file=StringValue(line);
            if(chunk.file.size()<4||chunk.file.substr(chunk.file.size()-4)!=".obj")throw std::runtime_error("Invalid map chunk");
            chunk.model=Load(Join(Directory(manifest),chunk.file));
            chunk.min=chunk.model.min;chunk.max=chunk.model.max;
            result.push_back(chunk);
            std::printf("Map chunk %zu: %s\n",result.size(),chunk.file.c_str());std::fflush(stdout);
        }
    }
    if(result.empty())throw std::runtime_error("No map chunks in manifest");
    return result;
}

void ModelRenderer::SetProgram(GLuint program) {
    modelUniform=glGetUniformLocation(program,"model");
    colorUniform=glGetUniformLocation(program,"material_diffuse");
    opacityUniform=glGetUniformLocation(program,"material_opacity");
    hasTextureUniform=glGetUniformLocation(program,"has_diffuse_texture");
    textureUniform=glGetUniformLocation(program,"diffuse_texture");
    if(!samplers[0]) {
        glGenSamplers(2,samplers);
        for(int i=0;i<2;++i) {
            glSamplerParameteri(samplers[i],GL_TEXTURE_WRAP_S,i?GL_CLAMP_TO_EDGE:GL_REPEAT);
            glSamplerParameteri(samplers[i],GL_TEXTURE_WRAP_T,i?GL_CLAMP_TO_EDGE:GL_REPEAT);
            glSamplerParameteri(samplers[i],GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
            glSamplerParameteri(samplers[i],GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        }
    }
}

void ModelRenderer::Draw(const RenderModel& model,const glm::mat4& transform,const glm::vec3& camera,bool transparent) {
    glUniformMatrix4fv(modelUniform,1,GL_FALSE,glm::value_ptr(transform));
    glUniform1i(textureUniform,0);glActiveTexture(GL_TEXTURE0);
    std::vector<const RenderBatch*> batches;
    for(const auto& batch:model.batches)if((batch.material.opacity<.999f)==transparent)batches.push_back(&batch);
    // Bounds-center sorting is approximate for intersecting glass surfaces.
    if(transparent)std::sort(batches.begin(),batches.end(),[&](const RenderBatch* a,const RenderBatch* b){
        glm::vec3 da=glm::vec3(transform*glm::vec4(a->center,1))-camera;
        glm::vec3 db=glm::vec3(transform*glm::vec4(b->center,1))-camera;
        return glm::dot(da,da)>glm::dot(db,db);
    });
    for(const auto* batch:batches) {
        const auto& material=batch->material;
        glUniform3fv(colorUniform,1,glm::value_ptr(material.diffuse));
        glUniform1f(opacityUniform,material.opacity);
        glUniform1i(hasTextureUniform,material.texture!=0);
        glBindTexture(GL_TEXTURE_2D,material.texture);glBindSampler(0,samplers[material.clamp?1:0]);
        glBindVertexArray(batch->vao);glDrawArrays(GL_TRIANGLES,0,batch->count);
    }
    glBindVertexArray(0);
}

bool ModelRenderer::Visible(const MapChunk& chunk,const glm::mat4& matrix) const {
    glm::vec4 corners[8];
    for(int i=0;i<8;++i)corners[i]=matrix*glm::vec4(i&1?chunk.max.x:chunk.min.x,i&2?chunk.max.y:chunk.min.y,i&4?chunk.max.z:chunk.min.z,1);
    for(int axis=0;axis<3;++axis)for(int sign:{-1,1}) {
        bool outside=true;
        for(const auto& c:corners)if(sign*c[axis]<=c.w){outside=false;break;}
        if(outside)return false;
    }
    return true;
}

void ModelRenderer::Release(RenderModel& model) {
    for(auto& batch:model.batches){glDeleteBuffers(1,&batch.vbo);glDeleteVertexArrays(1,&batch.vao);}
    model.batches.clear();
}
ModelRenderer::~ModelRenderer() {
    for(const auto& item:textures)glDeleteTextures(1,&item.second);
    if(samplers[0])glDeleteSamplers(2,samplers);
}
