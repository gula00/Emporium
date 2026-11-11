// Reconstructed Emporium scene 5. This is an independent implementation.
// Uses CHAI3D (BSD-licensed) and its ODE module; see CHAI3D-copyright.txt.
#include "chai3d.h"
#include "CODE.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <iostream>
#include <memory>
#include <thread>

using namespace chai3d;

namespace {
cWorld* world = nullptr;
cCamera* camera = nullptr;
cViewport* viewport = nullptr;
cODEWorld* physics = nullptr;
cHapticDeviceHandler* handler = nullptr;
std::array<cODEGenericBody*, 5> boxes{};
std::array<cToolCursor*, 2> tools{};
std::atomic<bool> running{false};
std::atomic<bool> resetRequested{false};
std::thread simulationThread;
GLFWwindow* window = nullptr;
int framebufferWidth = 1000;
int framebufferHeight = 700;

constexpr double boxSize = 0.20;
constexpr double toolRadius = 0.035;

void resetBoxes() {
    // Initial positions recovered from cDemo5::setup in the Linux binary.
    const cVector3d positions[5] = {
        cVector3d(0.0, -0.4, 0.5),
        cVector3d(0.0,  0.4, 0.5),
        cVector3d(0.0,  0.0, 0.5),
        cVector3d(-0.4, -0.2, 0.5),
        cVector3d( 0.4,  0.2, 0.5),
    };
    for (size_t i = 0; i < boxes.size(); ++i) {
        boxes[i]->setLocalPos(positions[i]);
    }
}

void addBox(size_t index, const cColorf& color, double height, double mass,
            double maxStiffness) {
    boxes[index] = new cODEGenericBody(physics);
    auto* mesh = new cMesh();
    cCreateBox(mesh, boxSize, boxSize, height);
    mesh->createAABBCollisionDetector(toolRadius);
    cMaterial material;
    material.m_diffuse = color;
    material.setStiffness(0.3 * maxStiffness);
    material.setDynamicFriction(0.6);
    material.setStaticFriction(0.6);
    mesh->setMaterial(material);
    boxes[index]->setImageModel(mesh);
    boxes[index]->createDynamicBox(boxSize, boxSize, height);
    boxes[index]->setMass(mass);
}

void createScene(double maxStiffness) {
    physics = new cODEWorld(world);
    world->addChild(physics);
    physics->setGravity(cVector3d(0.0, 0.0, -9.81));
    physics->setAngularDamping(0.00002);
    physics->setLinearDamping(0.00002);

    addBox(0, cColorf(0.80f, 0.25f, 0.20f), boxSize, 0.02, maxStiffness);
    addBox(1, cColorf(0.22f, 0.38f, 0.85f), boxSize, 0.02, maxStiffness);
    addBox(2, cColorf(0.25f, 0.70f, 0.38f), boxSize, 0.02, maxStiffness);
    addBox(3, cColorf(0.90f, 0.67f, 0.20f), 2 * boxSize, 0.05, maxStiffness);
    addBox(4, cColorf(0.65f, 0.32f, 0.78f), 2 * boxSize, 0.05, maxStiffness);
    resetBoxes();

    // Six ODE planes bound the box simulation. Only the floor is drawn.
    const cVector3d points[6] = {
        {0, 0, 2}, {0, 0, -1}, {0, 1, 0},
        {0, -1, 0}, {1, 0, 0}, {-0.8, 0, 0}
    };
    const cVector3d normals[6] = {
        {0, 0, -1}, {0, 0, 1}, {0, -1, 0},
        {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}
    };
    for (int i = 0; i < 6; ++i) {
        auto* plane = new cODEGenericBody(physics);
        plane->createStaticPlane(points[i], normals[i]);
    }

    auto* ground = new cMesh();
    physics->addChild(ground);
    cCreatePlane(ground, 3.0, 3.0);
    ground->setLocalPos(0, 0, -1);
    cMaterial groundMaterial;
    groundMaterial.setWhite();
    groundMaterial.setStiffness(0.3 * maxStiffness);
    groundMaterial.setDynamicFriction(0.2);
    groundMaterial.setStaticFriction(0.0);
    ground->setMaterial(groundMaterial);
    ground->createAABBCollisionDetector(toolRadius);
    auto texture = cTexture2d::create();
    if (texture->loadFromFile("../resources/images/ground.jpg")) {
        ground->setTexture(texture);
        ground->setUseTexture(true);
    }
}

void simulationLoop() {
    cPrecisionClock clock;
    clock.reset();
    clock.start();
    while (running.load()) {
        if (resetRequested.exchange(false)) resetBoxes();
        clock.stop();
        double dt = cClamp(clock.getCurrentTimeSeconds(), 0.0001, 0.001);
        clock.reset();
        clock.start();
        world->computeGlobalPositions(true);
        for (auto* tool : tools) {
            if (!tool) continue;
            tool->updateFromDevice();
            tool->computeInteractionForces();
            tool->applyToDevice();
            for (int i = 0; i < tool->getNumHapticPoints(); ++i) {
                cHapticPoint* point = tool->getHapticPoint(i);
                for (int j = 0; j < point->getNumCollisionEvents(); ++j) {
                    cCollisionEvent* contact = point->getCollisionEvent(j);
                    if (!contact || !contact->m_object) continue;
                    cGenericObject* owner = contact->m_object->getOwner();
                    if (owner) owner = owner->getOwner();
                    auto* body = dynamic_cast<cODEGenericBody*>(owner);
                    if (body) {
                        body->addExternalForceAtPoint(
                            -0.3 * point->getLastComputedForce(), contact->m_globalPos);
                    }
                }
            }
        }
        physics->updateDynamics(dt);
        if (!tools[0] && !tools[1]) cSleepMs(1);
    }
}

void onKey(GLFWwindow* handle, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)
        glfwSetWindowShouldClose(handle, GLFW_TRUE);
    if (key == GLFW_KEY_G) {
        if (physics->getGravity().length() > 0)
            physics->setGravity(cVector3d(0, 0, 0));
        else physics->setGravity(cVector3d(0, 0, -9.81));
    }
    if (key == GLFW_KEY_R) resetRequested.store(true);
}
} // namespace

int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_SAMPLES, 4);
    window = glfwCreateWindow(1000, 700, "Emporium - Scene 5: Dynamic Bodies", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, onKey);
#ifdef GLEW_VERSION
    if (glewInit() != GLEW_OK) return 1;
#endif

    world = new cWorld();
    world->m_backgroundColor.setWhite();
    camera = new cCamera(world);
    world->addChild(camera);
    camera->set(cVector3d(2.5, 0.0, 0.8), cVector3d(0, 0, -0.4), cVector3d(0, 0, 1));
    camera->setClippingPlanes(0.01, 10.0);
    auto* light = new cSpotLight(world);
    world->addChild(light);
    light->setEnabled(true);
    light->setLocalPos(0, 0, 1.5);
    light->setDir(0, 0, -1);
    light->setCutOffAngleDeg(60);

    handler = new cHapticDeviceHandler();
    double maxStiffness = 1000.0;
    for (unsigned i = 0; i < std::min(handler->getNumDevices(), 2u); ++i) {
        cGenericHapticDevicePtr device;
        if (!handler->getDevice(device, i)) continue;
        auto* tool = new cToolCursor(world);
        world->addChild(tool);
        tool->setHapticDevice(device);
        tool->setWorkspaceRadius(1.3);
        tool->setRadius(toolRadius, toolRadius);
        tool->enableDynamicObjects(true);
        tool->setWaitForSmallForce(true);
        tool->start();
        tools[i] = tool;
        maxStiffness = std::min(maxStiffness,
            device->getSpecifications().m_maxLinearStiffness / tool->getWorkspaceScaleFactor());
    }
    createScene(maxStiffness);
    viewport = new cViewport(camera);

    running.store(true);
    simulationThread = std::thread(simulationLoop);
    while (!glfwWindowShouldClose(window)) {
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        viewport->renderView(framebufferWidth, framebufferHeight);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    running.store(false);
    simulationThread.join();
    for (auto* tool : tools) if (tool) tool->stop();
    delete viewport;
    delete world;
    delete handler;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
