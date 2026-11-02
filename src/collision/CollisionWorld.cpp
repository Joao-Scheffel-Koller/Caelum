#include "collision/CollisionWorld.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace collision {
namespace {
const std::uint32_t LeafSize=8;
const double TimeTieEpsilon=1e-7;

bool Finite(const glm::dvec3& p) {
    return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
}
Triangle DoubleTriangle(const WorldTriangle& triangle) {
    return Triangle{glm::dvec3(triangle.a),glm::dvec3(triangle.b),glm::dvec3(triangle.c)};
}
std::string Lower(std::string text) {
    std::transform(text.begin(),text.end(),text.begin(),[](unsigned char c){return char(std::tolower(c));});
    return text;
}
bool Contains(const std::string& text,const char* token) {return text.find(token)!=std::string::npos;}
int Priority(Surface surface) {
    return surface==Surface::Water?2:surface==Surface::Solid?1:0;
}
double Center(const WorldTriangle& t,int axis) {
    return (double(t.a[axis])+double(t.b[axis])+double(t.c[axis]))/3;
}
void Include(Aabb& bounds,const Aabb& other) {
    bounds.min=glm::min(bounds.min,other.min);
    bounds.max=glm::max(bounds.max,other.max);
}
}

GeometryKind ClassifyCollisionMaterial(const std::string& originalName,
                                      const std::string& diffusePath,bool decal) {
    if(decal)return GeometryKind::Ignore;
    const std::string name=Lower(originalName),path=Lower(diffusePath);
    if(Contains(name,"water")||Contains(path,"water"))return GeometryKind::Water;
    if(Contains(name,"tree")||Contains(path,"tree"))return GeometryKind::Tree;
    // Generic construction materials may also appear vertically. Keep them
    // solid unless their surface role is unambiguous; landing needs a runway
    // zone and a suitable normal in addition to this material classification.
    if(Contains(name,"building")||Contains(name,"roof")||Contains(name,"wall")||
       Contains(name,"sandstone")||Contains(path,"sandstone"))
        return GeometryKind::Solid;
    for(const char* token:{"asphalt","earth","grass","scrub","gravel","sand",
                          "paving","cobblestone","pebblestone","sett","pitch_"})
        if(Contains(name,token)||Contains(path,token))return GeometryKind::Ground;
    return GeometryKind::Solid;
}

void AppendTreeProxy(std::vector<WorldTriangle>& output,const glm::dvec3& base,
                     double height,double width) {
    if(!Finite(base)||!std::isfinite(height)||!std::isfinite(width)||height<=0||width<=0)return;
    const double floatLimit=std::numeric_limits<float>::max();
    if(glm::any(glm::greaterThan(glm::abs(base)+glm::dvec3(width,height,width),glm::dvec3(floatLimit))))return;
    const int sides=8;
    const double turn=2*std::acos(-1.0)/sides;
    const double trunkRadius=std::min(width*.07,height*.035),trunkHeight=height*.58;
    const double crownRadius=width*.30,crownBottom=height*.32,crownTop=height*.95;
    const auto ring=[&](double angle,double radius,double y) {
        return glm::vec3(base+glm::dvec3(std::cos(angle)*radius,y,std::sin(angle)*radius));
    };
    const glm::vec3 trunkBottom(base),trunkTop(base+glm::dvec3(0,trunkHeight,0));
    const glm::vec3 crownBase(base+glm::dvec3(0,crownBottom,0));
    const glm::vec3 tip(base+glm::dvec3(0,crownTop,0));
    for(int i=0;i<sides;++i) {
        const double a=i*turn,b=(i+1)*turn;
        const auto lowA=ring(a,trunkRadius,0),lowB=ring(b,trunkRadius,0);
        const auto highA=ring(a,trunkRadius,trunkHeight),highB=ring(b,trunkRadius,trunkHeight);
        output.emplace_back(lowA,highA,highB);
        output.emplace_back(lowA,highB,lowB);
        output.emplace_back(trunkBottom,lowA,lowB);
        output.emplace_back(trunkTop,highB,highA);
        const auto crownA=ring(a,crownRadius,crownBottom),crownB=ring(b,crownRadius,crownBottom);
        output.emplace_back(crownA,tip,crownB);
        output.emplace_back(crownBase,crownA,crownB);
    }
}

std::uint32_t CollisionWorld::Build(Chunk& chunk,std::uint32_t begin,std::uint32_t end) {
    const std::uint32_t index=static_cast<std::uint32_t>(chunk.nodes.size());
    Node node;
    Aabb bounds=Bounds(DoubleTriangle(chunk.triangles[begin]));
    glm::dvec3 low(std::numeric_limits<double>::max()),high(std::numeric_limits<double>::lowest());
    for(std::uint32_t i=begin;i<end;++i) {
        const auto& triangle=chunk.triangles[i];
        Include(bounds,Bounds(DoubleTriangle(triangle)));
        const glm::dvec3 center(Center(triangle,0),Center(triangle,1),Center(triangle,2));
        low=glm::min(low,center);high=glm::max(high,center);
    }
    node.min=glm::vec3(bounds.min);node.max=glm::vec3(bounds.max);
    node.first=begin;node.count=end-begin;
    chunk.nodes.push_back(node);
    if(end-begin<=LeafSize)return index;
    const glm::dvec3 span=high-low;
    const int axis=span.y>span.x?(span.z>span.y?2:1):(span.z>span.x?2:0);
    const std::uint32_t middle=begin+(end-begin)/2;
    std::nth_element(chunk.triangles.begin()+begin,chunk.triangles.begin()+middle,
                     chunk.triangles.begin()+end,[axis](const WorldTriangle& a,const WorldTriangle& b){
        const double first=Center(a,axis),second=Center(b,axis);
        if(first!=second)return first<second;
        // Resolve equal centroids consistently without requiring OBJ object IDs.
        for(int vertex=0;vertex<3;++vertex)for(int coordinate=0;coordinate<3;++coordinate) {
            const glm::vec3* av=vertex==0?&a.a:vertex==1?&a.b:&a.c;
            const glm::vec3* bv=vertex==0?&b.a:vertex==1?&b.b:&b.c;
            if((*av)[coordinate]!=(*bv)[coordinate])return (*av)[coordinate]<(*bv)[coordinate];
        }
        return Priority(a.surface)<Priority(b.surface);
    });
    const std::uint32_t left=Build(chunk,begin,middle),right=Build(chunk,middle,end);
    chunk.nodes[index].first=left;chunk.nodes[index].right=right;chunk.nodes[index].count=0;
    return index;
}

void CollisionWorld::AddChunk(std::string name,std::vector<WorldTriangle> triangles) {
    for(const auto& triangle:triangles)
        if(!Finite(glm::dvec3(triangle.a))||!Finite(glm::dvec3(triangle.b))||!Finite(glm::dvec3(triangle.c)))
            throw std::invalid_argument("Nonfinite collision triangle in "+name);
    triangles.erase(std::remove_if(triangles.begin(),triangles.end(),[](const WorldTriangle& triangle){
        const auto t=DoubleTriangle(triangle);
        const auto n=glm::cross(t.b-t.a,t.c-t.a);
        return glm::dot(n,n)<=1e-20;
    }),triangles.end());
    if(triangles.empty())return;
    if(triangles.size()>std::numeric_limits<std::uint32_t>::max())
        throw std::length_error("Too many collision triangles in "+name);
    Chunk chunk;chunk.name=std::move(name);chunk.triangles=std::move(triangles);
    chunk.nodes.reserve(chunk.triangles.size()/4+1);
    Build(chunk,0,static_cast<std::uint32_t>(chunk.triangles.size()));
    triangleCount+=chunk.triangles.size();chunks.push_back(std::move(chunk));
}

bool CollisionWorld::SampleSurface(Surface kind,Triangle& triangle) const {
    for(const auto& chunk:chunks)for(const auto& candidate:chunk.triangles)
        if(candidate.surface==kind) {
            triangle=DoubleTriangle(candidate);
            return true;
        }
    return false;
}

Hit CollisionWorld::SweepSphere(const glm::dvec3& from,const glm::dvec3& to,double radius,
                                std::size_t* tested,bool ignoreSupport) const {
    if(tested)*tested=0;
    Hit best;
    if(!Finite(from)||!Finite(to)||!std::isfinite(radius)||radius<0)return best;
    Aabb query=SweptBounds(from,to,radius);
    for(const auto& chunk:chunks) {
        const auto& root=chunk.nodes.front();
        if(!Overlaps(query,Aabb{glm::dvec3(root.min),glm::dvec3(root.max)}))continue;
        // Median splits keep the depth below 32 for uint32 triangle counts.
        // Depth-first traversal needs at most one pending sibling per level.
        std::uint32_t stack[64],pending=0;stack[pending++]=0;
        while(pending) {
            const auto& node=chunk.nodes[stack[--pending]];
            if(!Overlaps(query,Aabb{glm::dvec3(node.min),glm::dvec3(node.max)}))continue;
            if(node.count==0) {
                stack[pending++]=node.right;stack[pending++]=node.first;
                continue;
            }
            for(std::uint32_t i=node.first;i<node.first+node.count;++i) {
                const auto& physical=chunk.triangles[i];
                const auto triangle=DoubleTriangle(physical);
                if(!Overlaps(query,Bounds(triangle)))continue;
                if(tested)++*tested;
                Hit contact=collision::SweepSphere(from,to,radius,triangle);
                if(!contact.hit)continue;
                contact.surface=physical.surface;
                if(ignoreSupport&&contact.surface==Surface::Ground&&contact.normal.y>=.85)continue;
                const double difference=contact.time-best.time;
                if(!best.hit||difference<-TimeTieEpsilon||
                   (std::abs(difference)<=TimeTieEpsilon&&
                    (Priority(contact.surface)>Priority(best.surface)||
                     (Priority(contact.surface)==Priority(best.surface)&&contact.penetration>best.penetration)))) {
                    best=contact;
                    const double end=std::min(1.0,best.time+TimeTieEpsilon);
                    query=SweptBounds(from,from+(to-from)*end,radius);
                }
            }
        }
    }
    return best;
}

} // namespace collision
