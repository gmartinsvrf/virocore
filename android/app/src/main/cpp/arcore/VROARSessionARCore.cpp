//
//  VROARSessionARCore.cpp
//  ViroKit
//
//  Created by Raj Advani on 9/27/17.
//  Copyright © 2017 Viro Media. All rights reserved.
//

#include "VROARSessionARCore.h"
#include "VROARAnchor.h"
#include "VROARPlaneAnchor.h"
#include "VROTexture.h"
#include "VRODriver.h"
#include "VROScene.h"
#include "VROTextureSubstrateOpenGL.h"
#include "VROLog.h"
#include <algorithm>
#include <VROCameraTexture.h>
#include "VROPlatformUtil.h"
#include <VROStringUtil.h>
#include "VROARHitTestResult.h"

VROARSessionARCore::VROARSessionARCore(jni::Object<arcore::Session> sessionJNI,
                                       jni::Object<arcore::ViroViewARCore> viroViewJNI,
                                       std::shared_ptr<VRODriverOpenGL> driver) :
    VROARSession(VROTrackingType::DOF6, VROWorldAlignment::Gravity),
    _lightingMode(arcore::config::LightingMode::AmbientIntensity),
    _planeFindingMode(arcore::config::PlaneFindingMode::Horizontal),
    _updateMode(arcore::config::UpdateMode::Blocking),
    _cameraTextureId(0) {
    _sessionJNI = sessionJNI.NewWeakGlobalRef(*VROPlatformGetJNIEnv());
    _viroViewJNI = viroViewJNI.NewWeakGlobalRef(*VROPlatformGetJNIEnv());
}

GLuint VROARSessionARCore::getCameraTextureId() const {
    return _cameraTextureId;
}

void VROARSessionARCore::initGL(std::shared_ptr<VRODriverOpenGL> driver) {
    // Generate the background texture
    glGenTextures(1, &_cameraTextureId);
    
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, _cameraTextureId);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::unique_ptr<VROTextureSubstrate> substrate = std::unique_ptr<VROTextureSubstrateOpenGL>(
            new VROTextureSubstrateOpenGL(GL_TEXTURE_EXTERNAL_OES, _cameraTextureId, driver, true));
    _background = std::make_shared<VROTexture>(VROTextureType::TextureEGLImage, std::move(substrate));

    arcore::session::setCameraTextureName( *_sessionJNI.get(), _cameraTextureId);
}

VROARSessionARCore::~VROARSessionARCore() {
    _sessionJNI.release();
    _viroViewJNI.release();
}

#pragma mark - VROARSession implementation

void VROARSessionARCore::run() {
    // TODO We can make this resume(), but on Android this is controlled externally
    //      by way of the activity lifecycle (we invoke pause and resume upon receiving
    //      lifecycle callbacks
}

void VROARSessionARCore::pause() {
    arcore::session::pause(*_sessionJNI.get());
}

bool VROARSessionARCore::isReady() const {
    return getScene() != nullptr;
}

void VROARSessionARCore::resetSession(bool resetTracking, bool removeAnchors) {
    return; // no-op
}


void VROARSessionARCore::setAnchorDetection(std::set<VROAnchorDetection> types) {
    std::set<VROAnchorDetection>::iterator it;
    for (it = types.begin(); it != types.end(); it++) {
        VROAnchorDetection type = *it;
        switch (type) {
            case VROAnchorDetection::None:
                _planeFindingMode = arcore::config::PlaneFindingMode::Disabled;
                break;
            case VROAnchorDetection::PlanesHorizontal:
                _planeFindingMode = arcore::config::PlaneFindingMode::Horizontal;
                break;
        }
    }

    if (types.size() == 0) {
        _planeFindingMode = arcore::config::PlaneFindingMode::Disabled;
    }

    updateARCoreConfig();
}

void VROARSessionARCore::updateARCoreConfig() {
    jni::Object<arcore::ViroViewARCore> view = *_viroViewJNI.get();
    if (view) {
        arcore::viroview::setConfig(view, arcore::config::getConfig(*_sessionJNI.get(), _lightingMode, _planeFindingMode, _updateMode));
    }
}

void VROARSessionARCore::setScene(std::shared_ptr<VROScene> scene) {
    VROARSession::setScene(scene);
}

void VROARSessionARCore::setDelegate(std::shared_ptr<VROARSessionDelegate> delegate) {
    VROARSession::setDelegate(delegate);
    // When we add a new delegate, notify it of all the anchors we've found thus far
    if (delegate) {
        for (auto it = _anchors.begin(); it != _anchors.end();it++) {
            delegate->anchorWasDetected(*it);
        }
    }
}

void VROARSessionARCore::addARImageTarget(std::shared_ptr<VROARImageTarget> target) {
    // no-op
}

void VROARSessionARCore::removeARImageTarget(std::shared_ptr<VROARImageTarget> target) {
    // no-op
}

void VROARSessionARCore::addAnchor(std::shared_ptr<VROARAnchor> anchor) {
    std::shared_ptr<VROARSessionDelegate> delegate = getDelegate();
    if (delegate) {
        delegate->anchorWasDetected(anchor);
    }

    _anchors.push_back(anchor);
}

void VROARSessionARCore::removeAnchor(std::shared_ptr<VROARAnchor> anchor) {
    _anchors.erase(std::remove_if(_anchors.begin(), _anchors.end(),
                                 [anchor](std::shared_ptr<VROARAnchor> candidate) {
                                     return candidate == anchor;
                                 }), _anchors.end());

    for (auto it = _nativeAnchorMap.begin(); it != _nativeAnchorMap.end();) {
        if (it->second == anchor) {
            // TODO We should remove the anchor from the ARCore session, but unclear
            //      how to do this given just the identifier. Do we create a dummy
            //      ARAnchor and set its identifier?
            //[_session removeAnchor:it->first];
            it = _nativeAnchorMap.erase(it);
        }
        else {
            ++it;
        }
    }
    
    std::shared_ptr<VROARSessionDelegate> delegate = getDelegate();
    if (delegate) {
        delegate->anchorWasRemoved(anchor);
    }
}

void VROARSessionARCore::updateAnchor(std::shared_ptr<VROARAnchor> anchor) {
    std::shared_ptr<VROARSessionDelegate> delegate = getDelegate();
    if (delegate) {
        delegate->anchorWillUpdate(anchor);
    }
    anchor->updateNodeTransform();
    if (delegate) {
        delegate->anchorDidUpdate(anchor);
    }
}

std::shared_ptr<VROTexture> VROARSessionARCore::getCameraBackgroundTexture() {
    return _background;
}

std::unique_ptr<VROARFrame> &VROARSessionARCore::updateFrame() {
    jni::Object<arcore::Frame> frameJNI = arcore::session::update(*_sessionJNI.get());
    _currentFrame = std::make_unique<VROARFrameARCore>(frameJNI, _viewport, shared_from_this());

    VROARFrameARCore *arFrame = (VROARFrameARCore *) _currentFrame.get();
    processUpdatedAnchors(arFrame);
    return _currentFrame;
}

std::unique_ptr<VROARFrame> &VROARSessionARCore::getLastFrame() {
    return _currentFrame;
}

void VROARSessionARCore::setViewport(VROViewport viewport) {
    _viewport = viewport;
}

void VROARSessionARCore::setOrientation(VROCameraOrientation orientation) {
    _orientation = orientation;
}

void VROARSessionARCore::setWorldOrigin(VROMatrix4f relativeTransform) {
    // no-op on Android
}

#pragma mark - Internal Methods

std::shared_ptr<VROARAnchor> VROARSessionARCore::getAnchorForNative(jni::Object<arcore::Anchor> anchor) {
    std::string key = VROStringUtil::toString(arcore::anchor::getHashCode(anchor));
    auto it = _nativeAnchorMap.find(key);
    if (it != _nativeAnchorMap.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

void VROARSessionARCore::processUpdatedAnchors(VROARFrameARCore *frame) {
    jni::Object<arcore::Frame> frameJni = frame->getFrameJNI();

    jni::Object<arcore::Collection> anchorCollection = arcore::frame::getUpdatedAnchors(frameJni);
    std::vector<jni::UniqueObject<arcore::Anchor>> anchorsJni = arcore::collection::toAnchorArray(anchorCollection);

    jni::Object<arcore::Collection> planeCollection = arcore::frame::getUpdatedPlanes(frameJni);
    std::vector<jni::UniqueObject<arcore::Plane>> planesJni = arcore::collection::toPlaneArray(planeCollection);

    // Find all new and updated anchors, update/create new ones and notify this class.
    // Note: this should be 0 until we allow users to add their own anchors to the system.
    if (anchorsJni.size() > 0) {

        for (int i = 0; i < anchorsJni.size(); i++) {
            jni::Object<arcore::Anchor> anchorJni = *anchorsJni[i].get();

            std::shared_ptr<VROARAnchor> vAnchor;
            std::string key = arcore::anchor::getId(anchorJni);

            auto it = _nativeAnchorMap.find(key);
            if (it != _nativeAnchorMap.end()) {
                vAnchor = it->second;
                updateAnchorFromJni(vAnchor, anchorJni);
                updateAnchor(vAnchor);
            } else {
                vAnchor = std::make_shared<VROARAnchor>();
                _nativeAnchorMap[key] = vAnchor;
                updateAnchorFromJni(vAnchor, anchorJni);
                vAnchor->setId(key);
                addAnchor(vAnchor);
            }
        }
    }

    // Find all new and updated planes, update/create new ones and notify this class
    if (planesJni.size() > 0) {

        for (int i = 0; i < planesJni.size(); i++) {
            jni::Object<arcore::Plane> planeJni = *planesJni[i].get();

            jni::Object<arcore::Plane> newPlane = arcore::plane::getSubsumedBy(planeJni);

            std::shared_ptr<VROARPlaneAnchor> vAnchor;
            // ARCore doesn't use ID for planes, but rather they simply return the same object, so
            // the hashcodes (from Java) should be reliable.
            std::string key = VROStringUtil::toString(arcore::plane::getHashCode(planeJni));

            // If the plane wasn't subsumed by a new plane, then don't remove it.
            if (newPlane == NULL) {
                auto it = _nativeAnchorMap.find(key);
                if (it != _nativeAnchorMap.end()) {
                    vAnchor = std::dynamic_pointer_cast<VROARPlaneAnchor>(it->second);
                    if (vAnchor) {
                        updatePlaneFromJni(vAnchor, planeJni);
                        updateAnchor(vAnchor);
                    } else {
                        pwarn("[Viro] expected to find a Plane.");
                    }
                } else {
                    vAnchor = std::make_shared<VROARPlaneAnchor>();
                    _nativeAnchorMap[key] = vAnchor;
                    updatePlaneFromJni(vAnchor, planeJni);
                    vAnchor->setId(key);
                    addAnchor(vAnchor);
                }
            } else {
                auto it = _nativeAnchorMap.find(key);
                if (it != _nativeAnchorMap.end()) {
                    vAnchor = std::dynamic_pointer_cast<VROARPlaneAnchor>(it->second);
                    if (vAnchor) {
                        removeAnchor(vAnchor);
                    }
                }
            }
        }
    }
}

void VROARSessionARCore::updateAnchorFromJni(std::shared_ptr<VROARAnchor> anchor, jni::Object<arcore::Anchor> anchorJni) {
    anchor->setTransform(arcore::pose::toMatrix(arcore::anchor::getPose(anchorJni)));
}

void VROARSessionARCore::updatePlaneFromJni(std::shared_ptr<VROARPlaneAnchor> plane, jni::Object<arcore::Plane> planeJni) {
    VROMatrix4f newTransform = arcore::pose::toMatrix(arcore::plane::getCenterPose(planeJni));
    VROVector3f newTranslation = newTransform.extractTranslation();

    VROMatrix4f oldTransform = plane->getTransform();
    VROVector3f oldTranslation = oldTransform.extractTranslation();

    // If the old translation is NOT the zero vector, then we want to preserve the old translation
    // and set the "center" instead.
    if (!oldTranslation.isEqual(VROVector3f())) {
        // set the center to (Position(new) - Position(old).
        plane->setCenter(newTranslation - oldTranslation);
        // translate the newTransform by the difference of (Position(old) - Position(new)) to
        // keep the same oldTranslation.
        newTransform.translate(oldTranslation - newTranslation);
    }

    plane->setTransform(newTransform);

    switch (arcore::plane::getType(planeJni)) {
        case arcore::PlaneType::HorizontalUpward :
            plane->setAlignment(VROARPlaneAlignment::HorizontalUpward);
            break;
        case arcore::PlaneType::HorizontalDownward :
            plane->setAlignment(VROARPlaneAlignment::HorizontalDownward);
            break;
        default:
            plane->setAlignment(VROARPlaneAlignment::NonHorizontal);
    }

    // the center is 0, because in ARCore, planes only have a position (at their center) vs
    // ARKit's (initial) position & center.
    plane->setCenter(VROVector3f());

    float extentX = arcore::plane::getExtentX(planeJni);
    float extentZ = arcore::plane::getExtentZ(planeJni);
    plane->setExtent(VROVector3f(extentX, 0, extentZ));
}
