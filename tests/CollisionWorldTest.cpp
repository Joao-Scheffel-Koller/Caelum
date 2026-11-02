// AI-assisted CPU regressions for physical map classification and BVH queries.
#include "collision/CollisionWorld.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {
int checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double tolerance=1e-6) {return std::abs(a-b)<tolerance;}
collision::WorldTriangle Wall(float x,collision::Surface surface=collision::Surface::Solid) {
    return {{x,-5,-5},{x,5,-5},{x,0,5},surface};
}
void Materials() {
    using collision::ClassifyCollisionMaterial;
    using collision::GeometryKind;
    Check(ClassifyCollisionMaterial("ASPHALT_0","",false)==GeometryKind::Ground,"runway asphalt is physical ground");
    Check(ClassifyCollisionMaterial("ROAD_MARKING_DASHED_0","textures/Asphalt010_Color.png",false)==GeometryKind::Ground,
          "base asphalt beneath road paint remains physical");
    Check(ClassifyCollisionMaterial("MAT_13_1","textures/runway_centerline_stripes.png",true)==GeometryKind::Ignore,
          "paint decals do not become duplicate physical floors");
    Check(ClassifyCollisionMaterial("WATER_0","textures/water.jpg",false)==GeometryKind::Water,"water keeps its hazardous surface type");
    Check(ClassifyCollisionMaterial("mat_0_0","TEXTURES/P1140080E_WATER.JPG",false)==GeometryKind::Water,
          "generic OSM material can classify water by image name ignoring case");
    Check(ClassifyCollisionMaterial("TREE_BILLBOARD_BROAD_LEAVED_0","",false)==GeometryKind::Tree&&
          ClassifyCollisionMaterial("TREE_BILLBOARD_CONIFEROUS_0","",false)==GeometryKind::Tree,
          "both exported tree families request proxies");
    Check(ClassifyCollisionMaterial("MAT_0_0","textures/arbaro_tree_broad_leaved.png",false)==GeometryKind::Tree,
          "tree proxy classification also recognizes a generated material's image");
    Check(ClassifyCollisionMaterial("BUILDING_DEFAULT_0","",false)==GeometryKind::Solid&&
          ClassifyCollisionMaterial("ROOF_DEFAULT_0","textures/grass.jpg",false)==GeometryKind::Solid,
          "building and roof labels remain obstacles regardless of a ground-like image");
    Check(ClassifyCollisionMaterial("CHAIN_LINK_FENCE_0","",false)==GeometryKind::Solid,
          "fence alpha cutouts are not paint or floor");
    Check(ClassifyCollisionMaterial("SANDSTONE_0","",false)==GeometryKind::Solid,
          "sandstone construction material is not mistaken for sand terrain");
    Check(ClassifyCollisionMaterial("GRASS_PAVER_0","",false)==GeometryKind::Ground&&
          ClassifyCollisionMaterial("EARTH_0","",false)==GeometryKind::Ground,
          "exported ordinary land families remain physical ground");
    Check(ClassifyCollisionMaterial("unknown","",false)==GeometryKind::Solid,"unknown material defaults to a solid obstacle");
}
void ContactsAndBounds() {
    using namespace collision;
    CollisionWorld world;
    Check(world.TriangleCount()==0&&world.ChunkCount()==0,"empty physical world starts without geometry");
    Triangle sample{{1,2,3},{4,5,6},{7,8,9}};
    Check(!world.SampleSurface(Surface::Water,sample)&&sample.a==glm::dvec3(1,2,3)&&
          sample.b==glm::dvec3(4,5,6)&&sample.c==glm::dvec3(7,8,9),
          "missing QA sample leaves the output triangle unchanged");
    std::size_t tested=999;
    Check(!world.SweepSphere({0,2,0},{0,-2,0},.5,&tested).hit&&tested==0,"empty query reports no tested triangles");
    world.AddChunk("empty",{});
    Check(world.ChunkCount()==0,"empty chunks do not create invalid BVH roots");
    world.AddChunk("floor",{{{-10,0,-10},{10,0,-10},{10,0,10},Surface::Ground},
                            {{-10,0,-10},{10,0,10},{-10,0,10},Surface::Ground}});
    auto hit=world.SweepSphere({0,2,0},{0,-2,0},.5);
    Check(world.SampleSurface(Surface::Ground,sample)&&sample.a.y==0&&sample.b.y==0&&sample.c.y==0,
          "QA sample returns a retained triangle with the requested surface kind");
    Check(hit.hit&&Near(hit.time,.375)&&hit.surface==Surface::Ground,"falling sphere reaches the ground at the correct time");
    Check(hit.normal.y>.99&&Near(hit.point.y,0),"ground contact preserves an upward normal and surface point");
    world.AddChunk("distant-wall",{Wall(9)});
    world.AddChunk("near-wall",{Wall(5)});
    hit=world.SweepSphere({0,2,0},{10,2,0},.5);
    Check(hit.hit&&Near(hit.time,.45)&&hit.surface==Surface::Solid,"query selects the nearest wall independent of chunk insertion order");
    Check(hit.normal.x<-.99,"obstacle contact normal opposes the incoming sphere");

    CollisionWorld crossing;
    crossing.AddChunk("nominal-negative-centroid-cell",{{{-5000,0,-5000},{10000,0,-5000},{-5000,0,10000},Surface::Ground}});
    hit=crossing.SweepSphere({4000,2,0},{4000,-2,0},.5);
    Check(hit.hit&&Near(hit.time,.375),"actual bounds retain a triangle that extends far beyond its centroid cell");

    CollisionWorld coplanar;
    coplanar.AddChunk("ground",{{{-10,0,-10},{10,0,-10},{0,0,10},Surface::Ground}});
    coplanar.AddChunk("water",{{{-10,0,-10},{10,0,-10},{0,0,10},Surface::Water}});
    hit=coplanar.SweepSphere({0,2,0},{0,-2,0},.5);
    Check(hit.hit&&hit.surface==Surface::Water,"coplanar water takes priority over land to prevent a false water landing");
    CollisionWorld reverse;
    reverse.AddChunk("water",{{{-10,0,-10},{10,0,-10},{0,0,10},Surface::Water}});
    reverse.AddChunk("solid",{{{-10,0,-10},{10,0,-10},{0,0,10},Surface::Solid}});
    Check(reverse.SweepSphere({0,2,0},{0,-2,0},.5).surface==Surface::Water,
          "hazard tie priority does not depend on chunk order");
}
void LocalBvh() {
    using namespace collision;
    CollisionWorld world;
    std::vector<WorldTriangle> triangles;
    for(int i=0;i<4096;++i)triangles.push_back(Wall(float(i*10+5)));
    world.AddChunk("many-walls",std::move(triangles));
    world.AddChunk("remote",{Wall(100000)});
    std::size_t tested=0;
    const auto hit=world.SweepSphere({0,1,0},{10,1,0},.25,&tested);
    Check(hit.hit&&Near(hit.time,.475),"BVH returns the same analytic first contact as a local wall query");
    Check(tested>0&&tested<20,"local BVH traversal does not test every triangle of a large chunk");
    Check(world.TriangleCount()==4097&&world.ChunkCount()==2,"world exposes retained physical geometry counts");
    world.SweepSphere({0,2,10000},{1,2,10000},.25,&tested);
    Check(tested==0,"non-overlapping actual chunk bounds reject remote geometry before narrow phase");
}
void SupportedWheelHazards() {
    using namespace collision;
    const auto floor=std::vector<WorldTriangle>{
        {{-10,0,-10},{10,0,-10},{10,0,10},Surface::Ground},
        {{-10,0,-10},{10,0,10},{-10,0,10},Surface::Ground}};
    CollisionWorld world;
    world.AddChunk("floor",floor);
    world.AddChunk("low-wall",{{{5,0,-5},{5,.4f,-5},{5,.4f,5},Surface::Solid},
                               {{5,0,-5},{5,.4f,5},{5,0,5},Surface::Solid}});
    const glm::dvec3 from(0,.25,0),to(10,.25,0);
    const auto first=world.SweepSphere(from,to,.25);
    Check(first.hit&&first.time==0&&first.surface==Surface::Ground,
          "ordinary sweep preserves the initial upward ground tangency");
    const auto obstacle=world.SweepSphere(from,to,.25,nullptr,true);
    Check(obstacle.hit&&Near(obstacle.time,.475)&&obstacle.surface==Surface::Solid,
          "supported wheel sweep still finds a later low obstacle after initial ground contact");
    CollisionWorld groundEdge;
    groundEdge.AddChunk("floor",floor);
    groundEdge.AddChunk("vertical-ground-edge",{{{5,0,-5},{5,.4f,-5},{5,.4f,5},Surface::Ground},
                                               {{5,0,-5},{5,.4f,5},{5,0,5},Surface::Ground}});
    const auto edge=groundEdge.SweepSphere(from,to,.25,nullptr,true);
    Check(edge.hit&&Near(edge.time,.475)&&edge.surface==Surface::Ground&&edge.normal.x<-.99,
          "ignoring upward support never removes an unsafe vertical ground edge");
}
void TreesAndInvalidData() {
    using namespace collision;
    std::vector<WorldTriangle> tree;
    AppendTreeProxy(tree,{0,0,0},10,8);
    Check(tree.size()==48,"tree proxy uses a small closed trunk and crown mesh");
    for(const auto& triangle:tree)for(const auto& vertex:{triangle.a,triangle.b,triangle.c})
        Check(std::isfinite(vertex.x)&&std::isfinite(vertex.y)&&std::isfinite(vertex.z)&&
              vertex.y>=0&&vertex.y<=10&&std::abs(vertex.x)<=4&&std::abs(vertex.z)<=4,
              "tree proxy stays finite and inside the source billboard extents");
    const auto original=tree.size();
    AppendTreeProxy(tree,{0,0,0},-1,8);
    AppendTreeProxy(tree,{0,0,0},10,0);
    AppendTreeProxy(tree,{std::numeric_limits<double>::quiet_NaN(),0,0},10,8);
    Check(tree.size()==original,"invalid tree dimensions do not introduce corrupt geometry");
    CollisionWorld world;world.AddChunk("tree",std::move(tree));
    Check(world.SweepSphere({-4,1,0},{4,1,0},.2).hit,"tree trunk is a physical obstacle");
    Check(world.SweepSphere({-4,5,0},{4,5,0},.2).hit,"tree crown is a physical obstacle");
    Check(!world.SweepSphere({3.6,9,-2},{3.6,9,2},.1).hit,
          "empty transparent billboard corner does not become a solid rectangular wall");
    const auto count=world.TriangleCount();
    bool rejected=false;
    try {world.AddChunk("bad",{{{std::numeric_limits<float>::infinity(),0,0},{0,1,0},{0,0,1}}});}
    catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected&&world.TriangleCount()==count,"invalid chunk is rejected without changing existing geometry");
    world.AddChunk("degenerate",{{{1,1,1},{1,1,1},{1,1,1}}});
    Check(world.TriangleCount()==count,"degenerate faces do not enter the BVH");
    std::size_t tested=999;
    Check(!world.SweepSphere({0,0,0},{1,0,0},-1,&tested).hit&&tested==0,"invalid query radius yields no contact or narrow-phase tests");
}
}
int main() {
    Materials();ContactsAndBounds();LocalBvh();SupportedWheelHazards();TreesAndInvalidData();
    std::printf("PASS: %d physical collision world checks\n",checks);
}
