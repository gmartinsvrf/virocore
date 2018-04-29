//
//  TransformDelegate_JNI.cpp
//  ViroRenderer
//
//  Copyright © 2016 Viro Media. All rights reserved.
//

#include <jni.h>
#include <memory>
#include <VROPlatformUtil.h>
#include "TransformDelegate_JNI.h"

TransformDelegate_JNI::TransformDelegate_JNI(VRO_OBJECT javaDelegateObject, double distanceFilter):VROTransformDelegate(distanceFilter){
    _javaObject  = reinterpret_cast<jclass>(VROPlatformGetJNIEnv()->NewWeakGlobalRef(javaDelegateObject));
}

TransformDelegate_JNI::~TransformDelegate_JNI() {
    VROPlatformGetJNIEnv()->DeleteWeakGlobalRef(_javaObject);
}


void TransformDelegate_JNI::onPositionUpdate(VROVector3f position){
    VRO_ENV env = VROPlatformGetJNIEnv();
    jweak weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, position] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (localObj == NULL) {
            return;
        }

        VROPlatformCallJavaFunction(localObj, "onPositionUpdate", "(FFF)V",
                                    position.x, position.y, position.z);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}