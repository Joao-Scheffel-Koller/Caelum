#include "render/ModelRenderer.hpp"
#include "collision/CollisionWorld.hpp"
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
#include <cstring>
#include <tuple>
#include <memory>
#include <utility>

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
float MaxAnisotropy() {
    GLint count=0;glGetIntegerv(GL_NUM_EXTENSIONS,&count);
    for(GLint i=0;i<count;++i) {
        const char* extension=reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS,GLuint(i)));
        if(std::strcmp(extension,"GL_EXT_texture_filter_anisotropic")==0 ||
           std::strcmp(extension,"GL_ARB_texture_filter_anisotropic")==0) {
            GLfloat maximum=1;glGetFloatv(0x84FF,&maximum);
            return std::min(maximum,8.0f);
        }
    }
    return 1.0f;
}
// Manifest schema is deliberately fixed: no general-purpose JSON parser needed.
std::string StringValue(const std::string& line) {
    const auto start=line.find('"',line.find(':')+1);
    const auto end=line.find('"',start+1);
    if(start==std::string::npos||end==std::string::npos) throw std::runtime_error("Invalid manifest string");
    return line.substr(start+1,end-start-1);
}
}


namespace {
struct PreparedModel {
    glm::vec3 min,max;
    std::vector<tinyobj::material_t> materials;
    std::map<int,std::vector<Vertex>> grouped;
};

PreparedModel PrepareModel(const std::string& path,bool map,collision::CollisionWorld* physicalWorld) {
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    PreparedModel result;
    auto& materials=result.materials;
    std::string warning,error,base=Directory(path);
    if(!tinyobj::LoadObj(&attributes,&shapes,&materials,&warning,&error,path.c_str(),base.c_str(),true))
        throw std::runtime_error(path+": "+error);
    result.min=glm::vec3(std::numeric_limits<float>::max());
    result.max=glm::vec3(std::numeric_limits<float>::lowest());
    auto& grouped=result.grouped;
    // Reuse the OBJ read: the physical world retains positions/material kinds,
    // while GPU batches retain normals/UVs. Neither depends on camera visibility.
    std::vector<collision::WorldTriangle> physical;
    std::vector<collision::GeometryKind> kinds;
    if(physicalWorld)for(const auto& material:materials) {
        const auto policy=ClassifyMaterial(material.name,material.diffuse_texname,!material.diffuse_texname.empty(),map);
        kinds.push_back(collision::ClassifyCollisionMaterial(material.name,material.diffuse_texname,
                                                            policy.layer==MaterialLayer::DECAL));
    }
    struct Tree {glm::dvec3 base{0};double height=0,width=0;};
    std::map<std::tuple<long long,long long,long long,long long>,Tree> trees;
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
            if(physicalWorld) {
                auto kind=id>=0&&size_t(id)<kinds.size()?kinds[id]:collision::GeometryKind::Solid;
                // CONCRETE also appears in buildings: only a horizontal apron
                // at the map's ground level is support, not an elevated roof.
                if(kind==collision::GeometryKind::Solid&&id>=0&&size_t(id)<materials.size()&&
                   materials[id].name.compare(0,8,"CONCRETE")==0&&std::abs(normal.y)>.95f&&
                   std::abs(triangle[0].position.y)<.05f&&std::abs(triangle[1].position.y)<.05f&&
                   std::abs(triangle[2].position.y)<.05f)kind=collision::GeometryKind::Ground;
                if(kind==collision::GeometryKind::Tree) {
                    // Each triangle of an OSM tree billboard spans the quad's
                    // bounds. Crossed quads share a center and become one proxy.
                    const glm::dvec3 low=glm::min(glm::dvec3(triangle[0].position),glm::min(glm::dvec3(triangle[1].position),glm::dvec3(triangle[2].position)));
                    const glm::dvec3 high=glm::max(glm::dvec3(triangle[0].position),glm::max(glm::dvec3(triangle[1].position),glm::dvec3(triangle[2].position)));
                    const auto center=(low+high)*.5;
                    const double height=high.y-low.y,width=std::max(high.x-low.x,high.z-low.z);
                    if(height>.1&&width>.1) {
                        const auto key=std::make_tuple(std::llround(center.x*10),std::llround(center.z*10),
                                                      std::llround(low.y*10),std::llround(height*10));
                        auto& tree=trees[key];tree.base=glm::dvec3(center.x,low.y,center.z);
                        tree.height=std::max(tree.height,height);tree.width=std::max(tree.width,width);
                    }
                } else if(kind!=collision::GeometryKind::Ignore) {
                    collision::WorldTriangle face;
                    face.a=triangle[0].position;face.b=triangle[1].position;face.c=triangle[2].position;
                    face.surface=kind==collision::GeometryKind::Ground?collision::Surface::Ground:
                                 kind==collision::GeometryKind::Water?collision::Surface::Water:collision::Surface::Solid;
                    physical.push_back(face);
                }
            }
            // Repeating textures are invariant under a shared integer UV shift.
            // Keep the original span/derivatives while avoiding huge world UVs.
            if(id>=0 && size_t(id)<materials.size() && !materials[id].diffuse_texopt.clamp) {
                const glm::vec2 origin(std::floor(triangle[0].uv.x),std::floor(triangle[0].uv.y));
                for(auto& v:triangle)v.uv-=origin;
            }
            for(auto& v:triangle) {if(glm::dot(v.normal,v.normal)<1e-12f)v.normal=normal;grouped[id].push_back(v);}
            offset+=3;
        }
    }
    if(physicalWorld) {
        for(const auto& entry:trees)collision::AppendTreeProxy(physical,entry.second.base,entry.second.height,entry.second.width);
        physicalWorld->AddChunk(path,std::move(physical));
    }
    if(result.grouped.empty())throw std::runtime_error("Empty OBJ: "+path);
    return result;
}
}

GLuint ModelRenderer::LoadTexture(const std::string& path,const LoadCallbacks& callbacks) {
    auto found=textures.find(path);
    if(found!=textures.end())return found->second;
    struct Image {int width=0,height=0;std::shared_ptr<unsigned char> pixels;};
    auto image=RunLoadCpu(callbacks,[&](){
        Image decoded;int channels;
        stbi_set_flip_vertically_on_load(true);
        decoded.pixels=std::shared_ptr<unsigned char>(stbi_load(path.c_str(),&decoded.width,&decoded.height,&channels,4),stbi_image_free);
        if(!decoded.pixels)throw std::runtime_error("Cannot load texture: "+path);
        return decoded;
    });
    if(callbacks.pump)callbacks.pump();
    GLuint texture;glGenTextures(1,&texture);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,image.width,image.height,0,GL_RGBA,GL_UNSIGNED_BYTE,image.pixels.get());
    glGenerateMipmap(GL_TEXTURE_2D);
    textures[path]=texture;
    if(callbacks.pump)callbacks.pump();
    return texture;
}

RenderModel ModelRenderer::Load(const std::string& path,bool map,collision::CollisionWorld* physicalWorld,
                                const LoadCallbacks& callbacks) {
    auto prepared=RunLoadCpu(callbacks,[&](){return PrepareModel(path,map,physicalWorld);});
    RenderModel result;result.min=prepared.min;result.max=prepared.max;
    auto& materials=prepared.materials;
    auto& grouped=prepared.grouped;
    const auto base=Directory(path);
    try {
    for(auto& group:grouped) {
        if(callbacks.pump)callbacks.pump();
        result.batches.emplace_back();
        auto& batch=result.batches.back();
        if(group.first>=0 && size_t(group.first)<materials.size()) {
            const auto& m=materials[group.first];
            batch.material.diffuse=glm::vec3(m.diffuse[0],m.diffuse[1],m.diffuse[2]);
            batch.material.opacity=m.dissolve;
            batch.material.clamp=m.diffuse_texopt.clamp;
            if(!m.diffuse_texname.empty())batch.material.texture=LoadTexture(Join(base,m.diffuse_texname),callbacks);
            batch.material.policy=ClassifyMaterial(m.name,m.diffuse_texname,batch.material.texture!=0,map);
        } else {
            batch.material.policy=ClassifyMaterial("","",false,map);
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
        if(callbacks.pump)callbacks.pump();
    }
    glBindVertexArray(0);
    if(result.batches.empty())throw std::runtime_error("Empty OBJ: "+path);
    } catch(...) {
        glBindVertexArray(0);glBindBuffer(GL_ARRAY_BUFFER,0);
        Release(result);throw;
    }
    return result;
}

namespace {
std::vector<std::string> MapFiles(const std::string& manifest) {
    std::ifstream input(manifest);
    if(!input)throw std::runtime_error("Cannot open map manifest: "+manifest);
    std::vector<std::string> result;
    std::string line;bool chunks=false;
    while(std::getline(input,line)) {
        if(line.find("\"chunks\"")!=std::string::npos)chunks=true;
        if(line.find("\"textures\"")!=std::string::npos)break;
        if(chunks&&line.find("\"file\"")!=std::string::npos) {
            const auto file=StringValue(line);
            if(file.size()<4||file.substr(file.size()-4)!=".obj")throw std::runtime_error("Invalid map chunk");
            result.push_back(file);
        }
    }
    if(result.empty())throw std::runtime_error("No map chunks in manifest");
    return result;
}
}
std::size_t ModelRenderer::CountMapChunks(const std::string& manifest) {return MapFiles(manifest).size();}
std::vector<MapChunk> ModelRenderer::LoadMap(const std::string& manifest,const std::function<void(size_t)>& progress,
                                          collision::CollisionWorld* physicalWorld,const LoadCallbacks& callbacks) {
    const auto files=MapFiles(manifest);
    std::vector<MapChunk> result;
    result.reserve(files.size());
    try {
        if(callbacks.progress)callbacks.progress(LoadProgress("Cenario",0,files.size()));
        for(const auto& file:files) {
            MapChunk chunk;chunk.file=file;
            chunk.model=Load(Join(Directory(manifest),file),true,physicalWorld,callbacks);
            chunk.min=chunk.model.min;chunk.max=chunk.model.max;
            try {result.push_back(std::move(chunk));}
            catch(...) {Release(chunk.model);throw;}
            std::printf("Map chunk %zu: %s\n",result.size(),file.c_str());std::fflush(stdout);
            if(progress)progress(result.size());
            if(callbacks.progress)callbacks.progress(LoadProgress("Cenario",result.size(),files.size()));
            if(callbacks.pump)callbacks.pump();
        }
    } catch(...) {
        for(auto& chunk:result)Release(chunk.model);
        throw;
    }
    return result;
}

void ModelRenderer::SetProgram(GLuint program) {
    modelUniform=glGetUniformLocation(program,"model");
    colorUniform=glGetUniformLocation(program,"material_diffuse");
    opacityUniform=glGetUniformLocation(program,"material_opacity");
    hasTextureUniform=glGetUniformLocation(program,"has_diffuse_texture");
    textureUniform=glGetUniformLocation(program,"diffuse_texture");
    cameraUniform=glGetUniformLocation(program,"camera_position");
    proceduralUniform=glGetUniformLocation(program,"procedural_surface");
    alphaCutoffUniform=glGetUniformLocation(program,"alpha_cutoff");
    if(!samplers[0]) {
        glGenSamplers(2,samplers);
        const float anisotropy=MaxAnisotropy();
        for(int i=0;i<2;++i) {
            glSamplerParameteri(samplers[i],GL_TEXTURE_WRAP_S,i?GL_CLAMP_TO_EDGE:GL_REPEAT);
            glSamplerParameteri(samplers[i],GL_TEXTURE_WRAP_T,i?GL_CLAMP_TO_EDGE:GL_REPEAT);
            glSamplerParameteri(samplers[i],GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
            glSamplerParameteri(samplers[i],GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            // FONTE: https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_filter_anisotropic.txt
            if(anisotropy>1)glSamplerParameterf(samplers[i],0x84FE,anisotropy);
        }
    }
}

void ModelRenderer::Draw(const RenderModel& model,const glm::mat4& transform,const glm::vec3& camera,RenderPass pass) {
    glUniformMatrix4fv(modelUniform,1,GL_FALSE,glm::value_ptr(transform));
    glUniform3fv(cameraUniform,1,glm::value_ptr(camera));
    glUniform1f(alphaCutoffUniform,pass==RenderPass::Solid?.2f:.001f);
    glUniform1i(textureUniform,0);glActiveTexture(GL_TEXTURE0);
    std::vector<const RenderBatch*> batches;
    for(const auto& batch:model.batches) {
        const RenderPass batchPass=batch.material.policy.layer==MaterialLayer::DECAL?RenderPass::Decal:
            batch.material.opacity<.999f?RenderPass::Transparent:RenderPass::Solid;
        if(batchPass==pass)batches.push_back(&batch);
    }
    // Bounds-center sorting is approximate for intersecting glass surfaces.
    if(pass==RenderPass::Transparent)std::sort(batches.begin(),batches.end(),[&](const RenderBatch* a,const RenderBatch* b){
        glm::vec3 da=glm::vec3(transform*glm::vec4(a->center,1))-camera;
        glm::vec3 db=glm::vec3(transform*glm::vec4(b->center,1))-camera;
        return glm::dot(da,da)>glm::dot(db,db);
    });
    for(const auto* batch:batches) {
        const auto& material=batch->material;
        glUniform3fv(colorUniform,1,glm::value_ptr(material.diffuse));
        glUniform1f(opacityUniform,material.opacity);
        glUniform1i(hasTextureUniform,material.texture!=0);
        glUniform1i(proceduralUniform,static_cast<int>(material.policy.procedural));
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
