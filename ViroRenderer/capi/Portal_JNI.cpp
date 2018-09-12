/**
 * Copyright © 2017 Viro Media. All rights reserved.
 */

#include <memory>
#include <VROPortalFrame.h>
#include "Portal_JNI.h"

#if VRO_PLATFORM_ANDROID
#define VRO_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_com_viro_core_Portal_##method_name
#else
#define VRO_METHOD(return_type, method_name) \
    return_type Portal_##method_name
#endif

extern "C" {

VRO_METHOD(VRO_REF(VROPortalFrame), nativeCreatePortal)(VRO_NO_ARGS) {
    std::shared_ptr<VROPortalFrame> portal = std::make_shared<VROPortalFrame>();
    return VRO_REF_NEW(VROPortalFrame, portal);
}

VRO_METHOD(void, nativeDestroyPortal)(VRO_ARGS
                                      VRO_REF(VROPortalFrame) native_ref) {
    VRO_REF_DELETE(VROPortalFrame, native_ref);
}

}