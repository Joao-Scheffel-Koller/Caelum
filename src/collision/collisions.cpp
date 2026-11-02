#include "collision/collisions.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace collision {
namespace {
const double Epsilon=64*std::numeric_limits<double>::epsilon();
const double TimeTolerance=1e-10;

bool Finite(const glm::dvec3& v) {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
}
bool Finite(const Triangle& t) {return Finite(t.a)&&Finite(t.b)&&Finite(t.c);}
double MaxComponent(const glm::dvec3& v) {
    return std::max(std::abs(v.x),std::max(std::abs(v.y),std::abs(v.z)));
}
bool Unit(const glm::dvec3& v,glm::dvec3& unit) {
    const double scale=MaxComponent(v);
    if(!std::isfinite(scale)||scale==0)return false;
    const glm::dvec3 scaled=v/scale;
    unit=scaled/std::sqrt(glm::dot(scaled,scaled));
    return Finite(unit);
}
glm::dvec3 SegmentPoint(const glm::dvec3& p,const glm::dvec3& a,const glm::dvec3& b) {
    const glm::dvec3 edge=b-a;
    const double lengthSquared=glm::dot(edge,edge);
    if(!(lengthSquared>0)||!std::isfinite(lengthSquared))return a;
    const double fraction=glm::dot(p-a,edge)/lengthSquared;
    if(!std::isfinite(fraction))return a;
    return a+std::max(0.0,std::min(1.0,fraction))*edge;
}
bool Inside(const glm::dvec3& p,const Triangle& t,const glm::dvec3& normal) {
    const glm::dvec3 points[]={t.a,t.b,t.c};
    for(int i=0;i<3;++i) {
        const glm::dvec3 edge=points[(i+1)%3]-points[i],delta=p-points[i];
        const double value=glm::dot(glm::cross(edge,delta),normal);
        const double tolerance=Epsilon*std::max(1.0,std::sqrt(glm::dot(edge,edge)*glm::dot(delta,delta)));
        if(!std::isfinite(value)||value < -tolerance)return false;
    }
    return true;
}
glm::dvec3 Closest(const glm::dvec3& p,const Triangle& t,const glm::dvec3& normal,bool face) {
    if(face) {
        const glm::dvec3 projection=p-normal*glm::dot(p-t.a,normal);
        if(Finite(projection)&&Inside(projection,t,normal))return projection;
    }
    const glm::dvec3 points[]={t.a,t.b,t.c};
    glm::dvec3 best=t.a;
    double distance=std::numeric_limits<double>::infinity();
    for(int i=0;i<3;++i) {
        const glm::dvec3 candidate=SegmentPoint(p,points[i],points[(i+1)%3]);
        const glm::dvec3 delta=p-candidate;
        const double squared=glm::dot(delta,delta);
        if(squared<distance) {distance=squared;best=candidate;}
    }
    return best;
}
glm::dvec3 FallbackNormal(const glm::dvec3& normal,bool face,const glm::dvec3& motion) {
    if(face) {
        if(glm::dot(normal,motion)>0)return -normal;
        return normal;
    }
    glm::dvec3 direction;
    return Unit(motion,direction)?-direction:glm::dvec3(0,1,0);
}

// Solve the feature's distance equation without stepping along the trajectory.
// Scaling and the q formulation avoid overflow in the discriminant and the
// cancellation of the small root. Candidate contacts are verified geometrically.
template<class Candidate>
void Roots(double a,double b,double c,const Candidate& candidate) {
    if(!std::isfinite(a)||!std::isfinite(b)||!std::isfinite(c))return;
    const double scale=std::max(std::abs(a),std::max(std::abs(b),std::abs(c)));
    if(scale==0)return;
    a/=scale;b/=scale;c/=scale;
    if(a==0) {if(b!=0)candidate(-c/b);return;}
    const double product=4*a*c;
    double discriminant=std::fma(b,b,-product);
    const double tolerance=Epsilon*(b*b+std::abs(product));
    if(discriminant < -tolerance)return;
    discriminant=std::max(0.0,discriminant);
    const double root=std::sqrt(discriminant);
    const double q=-.5*(b+std::copysign(root,b));
    if(q==0) {candidate(-b/(2*a));return;}
    double first=q/a,second=c/q;
    if(first>second)std::swap(first,second);
    candidate(first);
    if(second!=first)candidate(second);
}
}

Aabb Bounds(const Triangle& t) {
    if(!Finite(t))throw std::invalid_argument("Triangle bounds require finite coordinates");
    Aabb box;
    box.min=glm::min(t.a,glm::min(t.b,t.c));
    box.max=glm::max(t.a,glm::max(t.b,t.c));
    return box;
}
Aabb SweptBounds(const glm::dvec3& from,const glm::dvec3& to,double radius) {
    if(!Finite(from)||!Finite(to)||!std::isfinite(radius)||radius<0)
        throw std::invalid_argument("Swept bounds require finite endpoints and a nonnegative radius");
    Aabb box;
    box.min=glm::min(from,to)-glm::dvec3(radius);
    box.max=glm::max(from,to)+glm::dvec3(radius);
    if(!Finite(box.min)||!Finite(box.max))throw std::invalid_argument("Swept bounds overflow");
    return box;
}
bool Overlaps(const Aabb& a,const Aabb& b) {
    if(!Finite(a.min)||!Finite(a.max)||!Finite(b.min)||!Finite(b.max))return false;
    for(int axis=0;axis<3;++axis) {
        if(a.min[axis]>a.max[axis]||b.min[axis]>b.max[axis])return false;
        if(a.max[axis]<b.min[axis]||b.max[axis]<a.min[axis])return false;
    }
    return true;
}
glm::dvec3 ClosestPoint(const glm::dvec3& point,const Triangle& triangle) {
    if(!Finite(point)||!Finite(triangle))
        throw std::invalid_argument("Closest point requires finite coordinates");
    glm::dvec3 normal(0);
    const bool face=Unit(glm::cross(triangle.b-triangle.a,triangle.c-triangle.a),normal);
    return Closest(point,triangle,normal,face);
}

Hit SweepSphere(const glm::dvec3& from,const glm::dvec3& to,double radius,const Triangle& triangle) {
    Hit result;
    if(!Finite(from)||!Finite(to)||!Finite(triangle)||!std::isfinite(radius)||radius<0)return result;
    const glm::dvec3 motion=to-from;
    const double motionSquared=glm::dot(motion,motion);
    if(!Finite(motion)||!std::isfinite(motionSquared)||!std::isfinite(radius*radius))return result;
    glm::dvec3 normal(0);
    const bool face=Unit(glm::cross(triangle.b-triangle.a,triangle.c-triangle.a),normal);
    const double coordinateScale=std::max(1.0,std::max(radius,std::max(MaxComponent(from),
        std::max(MaxComponent(to),std::max(MaxComponent(triangle.a),
        std::max(MaxComponent(triangle.b),MaxComponent(triangle.c)))))));
    const double tolerance=Epsilon*coordinateScale;
    const auto consider=[&](double time) {
        if(!std::isfinite(time)||time < -TimeTolerance||time > 1+TimeTolerance)return;
        time=std::max(0.0,std::min(1.0,time));
        if(result.hit&&time>=result.time)return;
        const glm::dvec3 center=from+motion*time;
        const glm::dvec3 point=Closest(center,triangle,normal,face);
        const glm::dvec3 delta=center-point;
        const double distance=std::sqrt(glm::dot(delta,delta));
        if(!std::isfinite(distance)||distance>radius+tolerance)return;
        result.hit=true;result.time=time;result.point=point;
        result.normal=distance>tolerance?delta/distance:FallbackNormal(normal,face,motion);
        result.penetration=std::max(0.0,radius-distance);
    };

    // Initial overlap has precedence, including a resting or tangential contact.
    consider(0);
    if(result.hit||motionSquared==0)return result;

    if(face) {
        const double initial=glm::dot(from-triangle.a,normal);
        const double rate=glm::dot(motion,normal);
        if(rate!=0)for(double side:{-1.0,1.0}) {
            const double time=(side*radius-initial)/rate;
            if(time < -TimeTolerance||time > 1+TimeTolerance)continue;
            const glm::dvec3 contact=from+time*motion-side*radius*normal;
            if(Inside(contact,triangle,normal))consider(time);
        }
    }

    const glm::dvec3 points[]={triangle.a,triangle.b,triangle.c};
    for(int i=0;i<3;++i) {
        const glm::dvec3 edge=points[(i+1)%3]-points[i],offset=from-points[i];
        const double lengthSquared=glm::dot(edge,edge);
        if(lengthSquared>0&&std::isfinite(lengthSquared)) {
            const double along=glm::dot(offset,edge)/lengthSquared;
            const double rate=glm::dot(motion,edge)/lengthSquared;
            const glm::dvec3 perpendicular=offset-edge*along,velocity=motion-edge*rate;
            const auto edgeContact=[&](double time) {
                const double fraction=along+time*rate;
                if(fraction>=-TimeTolerance&&fraction<=1+TimeTolerance)consider(time);
            };
            Roots(glm::dot(velocity,velocity),2*glm::dot(perpendicular,velocity),
                  glm::dot(perpendicular,perpendicular)-radius*radius,edgeContact);
        }
        Roots(motionSquared,2*glm::dot(offset,motion),glm::dot(offset,offset)-radius*radius,consider);
    }
    return result;
}
} // namespace collision
