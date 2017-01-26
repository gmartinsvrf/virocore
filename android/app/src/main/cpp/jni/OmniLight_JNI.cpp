//
// OmniLight_JNI.cpp
// ViroRenderer
//
// Copyright © 2016 Viro Media. All rights reserved.

#include <jni.h>
#include <VROLight.h>
#include <PersistentRef.h>
#include <VRONode.h>
#include "Node_JNI.h"

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_com_viro_renderer_jni_OmniLightJni_##method_name

namespace OmniLight {
    inline jlong jptr(std::shared_ptr<VROLight> shared_node) {
        PersistentRef<VROLight> *native_light = new PersistentRef<VROLight>(shared_node);
        return reinterpret_cast<intptr_t>(native_light);
    }

    inline std::shared_ptr<VROLight> native(jlong ptr) {
        PersistentRef<VROLight> *persistentBox = reinterpret_cast<PersistentRef<VROLight> *>(ptr);
        return persistentBox->get();
    }
}

extern "C" {


JNI_METHOD(jlong, nativeCreateOmniLight)(JNIEnv *env,
                                         jclass clazz,
                                         jlong color,
                                         jfloat attenuationStartDistance,
                                         jfloat attenuationEndDistance,
                                         jfloat positionX,
                                         jfloat positionY,
                                         jfloat positionZ) {

    std::shared_ptr<VROLight> omniLight = std::make_shared<VROLight>(VROLightType::Omni);

    // Get the color
    float r = ((color >> 16) & 0xFF) / 255.0;
    float g = ((color >> 8) & 0xFF) / 255.0;
    float b = (color & 0xFF) / 255.0;

    VROVector3f vecColor(r, g, b);
    omniLight->setColor(vecColor);
    omniLight->setAttenuationStartDistance(attenuationStartDistance);
    omniLight->setAttenuationEndDistance(attenuationEndDistance);

    VROVector3f vecPosition(positionX, positionY, positionZ);
    omniLight->setPosition(vecPosition);

    return OmniLight::jptr(omniLight);
}

JNI_METHOD(void, nativeDestroyOmniLight)(JNIEnv *env,
                                         jclass clazz,
                                         jlong native_light_ref) {
    delete reinterpret_cast<PersistentRef<VROLight> *>(native_light_ref);
}

JNI_METHOD(void, nativeAddToNode)(JNIEnv *env,
                                  jclass clazz,
                                  jlong native_light_ref,
                                  jlong native_node_ref) {
    std::shared_ptr<VROLight> light = OmniLight::native(native_light_ref);
    Node::native(native_node_ref)->addLight(light);
}

JNI_METHOD(void, nativeRemoveFromNode)(JNIEnv *env,
                                       jclass clazz,
                                       jlong native_light_ref,
                                       jlong native_node_ref) {
    std::shared_ptr<VROLight> light = OmniLight::native(native_light_ref);
    Node::native(native_node_ref)->removeLight(light);
}

// Setters


JNI_METHOD(void, nativeSetColor)(JNIEnv *env,
                                 jclass clazz,
                                 jlong native_light_ref,
                                 jlong color) {

    std::shared_ptr<VROLight> light = OmniLight::native(native_light_ref);

    // Get the color
    float r = ((color >> 16) & 0xFF) / 255.0;
    float g = ((color >> 8) & 0xFF) / 255.0;
    float b = (color & 0xFF) / 255.0;

    VROVector3f vecColor(r, g, b);

    light->setColor(vecColor);
}

JNI_METHOD(void, nativeSetAttenuationStartDistance)(JNIEnv *env,
                                                    jclass clazz,
                                                    jlong native_light_ref,
                                                    jfloat attenuationStartDistance) {

    std::shared_ptr<VROLight> light = OmniLight::native(native_light_ref);
    light->setAttenuationStartDistance(attenuationStartDistance);
}

JNI_METHOD(void, nativeSetAttenuationEndDistance)(JNIEnv *env,
                                                  jclass clazz,
                                                  jlong native_light_ref,
                                                  jfloat attenuationEndDistance) {

    std::shared_ptr<VROLight> light = OmniLight::native(native_light_ref);
    light->setAttenuationEndDistance(attenuationEndDistance);
}

JNI_METHOD(void, nativeSetPosition)(JNIEnv *env,
                                    jclass clazz,
                                    jlong native_light_ref,
                                    jfloat positionX,
                                    jfloat positionY,
                                    jfloat positionZ) {

    std::shared_ptr<VROLight> light = OmniLight::native(native_light_ref);
    VROVector3f vecPosition(positionX, positionY, positionZ);
    light->setPosition(vecPosition);
}
}