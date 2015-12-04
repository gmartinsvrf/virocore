//
//  VROScene.h
//  ViroRenderer
//
//  Created by Raj Advani on 10/19/15.
//  Copyright © 2015 Viro Media. All rights reserved.
//

#ifndef VROScene_h
#define VROScene_h

#include <stdio.h>
#include <vector>
#include <memory>

class VRONode;
class VRORenderContext;
class VROMaterialVisual;
class VROLayer;

class VROScene {
    
public:
    
    void render(const VRORenderContext &renderContext);
    
    void addNode(std::shared_ptr<VRONode> node);
    
    std::vector<std::shared_ptr<VRONode>> &getRootNodes() {
        return _nodes;
    }
    
private:
    
    /*
     The root nodes of the scene.
     */
    std::vector<std::shared_ptr<VRONode>> _nodes;
    
    /*
     The background visual to display.
     */
    std::shared_ptr<VROMaterialVisual> _background;
    
};

#endif /* VROScene_h */
