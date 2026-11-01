#include "camera/CameraController.hpp"
// FONTE: codigo base do professor, include/matrices.h.
#include "matrices.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int checks=0;
void Check(bool condition,const std::string& message) {
    ++checks;
    if(!condition){std::cerr<<"FAIL: "<<message<<'\n';std::exit(EXIT_FAILURE);}
}
bool Near(const glm::vec3& a,const glm::vec3& b,float epsilon=.0001f) {
    return glm::length(a-b)<epsilon;
}
void Valid(const CameraPose& p,const std::string& label) {
    for(int i=0;i<3;++i)Check(std::isfinite(p.position[i])&&std::isfinite(p.forward[i])&&std::isfinite(p.up[i]),label+" finite basis");
    Check(std::abs(glm::length(p.forward)-1)<.0001f,label+" unit forward");
    Check(std::abs(glm::length(p.up)-1)<.0001f,label+" unit up");
    Check(std::abs(glm::dot(p.forward,p.up))<.0001f,label+" perpendicular basis");
    Check(p.nearPlane>0&&p.farPlane>p.nearPlane,label+" clipping planes");
    const glm::mat4 view=Matrix_Camera_View(glm::vec4(p.position,1),glm::vec4(p.forward,0),glm::vec4(p.up,0));
    const glm::mat4 projection=Matrix_Perspective(p.fieldOfView,4.0f/3.0f,-p.nearPlane,-p.farPlane);
    bool finite=true;
    for(int column=0;column<4;++column)for(int row=0;row<4;++row)
        finite=finite&&std::isfinite(view[column][row])&&std::isfinite(projection[column][row]);
    Check(finite,label+" finite view/projection matrices");
}
}

int main() {
    CameraFrame frame;
    frame.aircraftModel=Matrix_Translate(5000,20,6000)*Matrix_Rotate_Y(.65f)*Matrix_Rotate_X(.5f);
    frame.orbitTarget=glm::vec3(5000,24,6000);
    CameraController camera;
    const CameraPose initialOrbit=camera.Pose(frame);
    Valid(initialOrbit,"initial orbit");
    CameraController orbitUp;
    orbitUp.Look(0,-100);
    Check(orbitUp.Pose(frame).position.y>initialOrbit.position.y,
          "upward mouse movement raises the external camera");
    camera.SetMode(CameraType::COCKPIT,frame);
    const CameraPose cockpit=camera.Pose(frame);
    Check(Near(cockpit.position,glm::vec3(frame.aircraftModel*glm::vec4(frame.cockpitEyeLocal,1))),"eye follows exact model transform");
    Check(Near(cockpit.forward,glm::vec3(frame.aircraftModel[0])),"cockpit looks along model nose");
    Check(Near(cockpit.up,glm::vec3(frame.aircraftModel[1])),"cockpit inherits aircraft roll");
    Check(cockpit.nearPlane==.05f,"cockpit has short near plane");
    Valid(cockpit,"cockpit");
    camera.SetMode(CameraType::FREE,frame);
    const CameraPose detached=camera.Pose(frame);
    Check(Near(cockpit.position,detached.position)&&Near(cockpit.forward,detached.forward)&&Near(cockpit.up,detached.up),"entering free preserves all pose vectors including bank");
    CameraFrame moved=frame;
    moved.aircraftModel=Matrix_Translate(100,100,100)*frame.aircraftModel;
    moved.orbitTarget+=glm::vec3(100);
    Check(Near(camera.Pose(moved).position,detached.position),"free camera stays detached from aircraft");
    camera.SetMode(CameraType::FREE,moved);
    Check(Near(camera.Pose(moved).position,detached.position),"pressing F again never teleports");
    camera.Look(1000000,1000000);
    Valid(camera.Pose(frame),"free pitch limit");
    camera.Look(-1000000,-1000000);
    Valid(camera.Pose(frame),"opposite free pitch limit");
    camera.Scroll(1e6f);Check(camera.FreeSpeed()==2000,"free speed upper limit");
    camera.Scroll(-1e6f);Check(camera.FreeSpeed()==1,"free speed lower limit");
    camera.Reset(frame);Check(camera.FreeSpeed()==50,"free reset speed");
    Check(Near(camera.Pose(frame).position,initialOrbit.position),"free reset returns to aircraft overview");

    // Integrating one second at different frame rates must cover the same distance.
    CameraFrame origin;
    CameraController oneStep,manySteps,diagonal,fast;
    oneStep.SetMode(CameraType::FREE,origin);manySteps.SetMode(CameraType::FREE,origin);
    diagonal.SetMode(CameraType::FREE,origin);fast.SetMode(CameraType::FREE,origin);
    const glm::vec3 start=oneStep.Pose(origin).position;
    CameraMovement motion;motion.forward=1;
    oneStep.Update(motion,1,origin);
    for(int i=0;i<60;++i)manySteps.Update(motion,1.0f/60,origin);
    Check(Near(oneStep.Pose(origin).position,manySteps.Pose(origin).position,.001f),"movement is frame-rate independent");
    Check(std::abs(glm::length(oneStep.Pose(origin).position-start)-50)<.001f,"free speed uses meters per second");
    motion.right=1;motion.vertical=1;diagonal.Update(motion,1,origin);
    Check(std::abs(glm::length(diagonal.Pose(origin).position-start)-50)<.001f,"diagonal movement is normalized");
    motion.right=0;motion.vertical=0;motion.fast=true;fast.Update(motion,1,origin);
    Check(std::abs(glm::length(fast.Pose(origin).position-start)-250)<.001f,"Shift provides fivefold speed");

    camera.SetMode(CameraType::COCKPIT,frame);
    camera.Look(200,25);
    const CameraPose turnedHead=camera.Pose(frame);
    camera.SetMode(CameraType::LOOK_AT,frame);camera.Look(40,30);
    const CameraPose adjustedOrbit=camera.Pose(frame);
    camera.SetMode(CameraType::COCKPIT,frame);
    Check(Near(camera.Pose(frame).forward,turnedHead.forward),"switching modes retains cockpit head state");
    camera.SetMode(CameraType::LOOK_AT,frame);
    Check(Near(camera.Pose(frame).position,adjustedOrbit.position),"switching modes retains orbit state");
    camera.Look(1e6f,1e6f);Valid(camera.Pose(frame),"orbit pole limit");
    camera.Scroll(1e6f);Check(camera.OrbitDistance()==5,"orbit minimum zoom");
    camera.Scroll(-1e6f);Check(camera.OrbitDistance()==15000,"orbit maximum zoom");
    camera.SetMode(CameraType::COCKPIT,frame);
    motion.fast=false;motion.forward=1;motion.right=1;motion.vertical=1;
    camera.Update(motion,1000,frame);
    const glm::vec3 delta=camera.Pose(frame).position-glm::vec3(frame.aircraftModel[3]);
    const glm::vec3 local=glm::vec3(glm::dot(delta,glm::vec3(frame.aircraftModel[0])),
        glm::dot(delta,glm::vec3(frame.aircraftModel[1])),glm::dot(delta,glm::vec3(frame.aircraftModel[2])))-frame.cockpitEyeLocal;
    Check(local.x<=.201f&&local.y<=.121f&&local.z<=.181f,"head movement stays within cabin bounds");
    camera.Look(1e6f,1e6f);Valid(camera.Pose(frame),"cockpit look bounds");
    camera.Reset(frame);Check(Near(camera.Pose(frame).position,cockpit.position)&&Near(camera.Pose(frame).forward,cockpit.forward),"cockpit Home centers position and look");
    CameraFrame vertical;
    vertical.aircraftModel=Matrix_Rotate_Z(1.57079632679f);
    camera.SetMode(CameraType::COCKPIT,vertical);Valid(camera.Pose(vertical),"vertical aircraft cockpit");
    camera.SetMode(CameraType::FREE,vertical);Valid(camera.Pose(vertical),"vertical entry to free");
    camera.Look(20,20);Valid(camera.Pose(vertical),"free rotation after vertical entry");
    std::cout<<"PASS: "<<checks<<" camera checks\n";
    return EXIT_SUCCESS;
}
