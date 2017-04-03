//
//  VROBox.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 12/7/15.
//  Copyright © 2015 Viro Media. All rights reserved.
//

#include "VROBox.h"
#include "VROData.h"
#include "VROGeometrySource.h"
#include "VROGeometryElement.h"
#include "VROMaterial.h"
#include "stdlib.h"

static const int kNumBoxVertices = 36;

std::shared_ptr<VROBox> VROBox::createBox(float width, float height, float length) {
    int numVertices = kNumBoxVertices;
    
    int varSizeBytes = sizeof(VROShapeVertexLayout) * numVertices;
    VROShapeVertexLayout var[varSizeBytes];
    
    VROBox::buildBox(var, width, height, length);
    
    int indices[kNumBoxVertices];
    for (int i = 0; i < kNumBoxVertices; i++) {
        indices[i] = i;
    }
    VROShapeUtilComputeTangents(var, numVertices, indices, numVertices);
    
    std::shared_ptr<VROData> indexData = std::make_shared<VROData>((void *) indices, sizeof(int) * kNumBoxVertices);
    std::shared_ptr<VROData> vertexData = std::make_shared<VROData>((void *) var, varSizeBytes);
    
    std::vector<std::shared_ptr<VROGeometrySource>> sources = VROShapeUtilBuildGeometrySources(vertexData, numVertices);
    std::shared_ptr<VROGeometryElement> element = std::make_shared<VROGeometryElement>(indexData,
                                                                                       VROGeometryPrimitiveType::Triangle,
                                                                                       kNumBoxVertices / 3,
                                                                                       sizeof(int));
    std::vector<std::shared_ptr<VROGeometryElement>> elements = { element };
    
    std::shared_ptr<VROBox> box = std::shared_ptr<VROBox>(new VROBox(sources, elements));
    
    std::shared_ptr<VROMaterial> material = std::make_shared<VROMaterial>();
    box->getMaterials().push_back(material);    
    return box;
}

void VROBox::buildBox(VROShapeVertexLayout *vertexLayout, float width, float height, float length) {
    float w = width  / 2;
    float h = height / 2;
    float l = length / 2;
    
    const float cubeVertices[] = {
        // Front face
        -w,  h, l,
        -w, -h, l,
         w,  h, l,
        -w, -h, l,
         w, -h, l,
         w,  h, l,
        
        // Right face
         w,  h,  l,
         w, -h,  l,
         w,  h, -l,
         w, -h,  l,
         w, -h, -l,
         w,  h, -l,
        
        // Back face
         w,  h, -l,
         w, -h, -l,
        -w,  h, -l,
         w, -h, -l,
        -w, -h, -l,
        -w,  h, -l,
        
        // Left face
        -w,  h, -l,
        -w, -h, -l,
        -w,  h,  l,
        -w, -h, -l,
        -w, -h,  l,
        -w,  h,  l,
        
        // Top face
        -w, h, -l,
        -w, h,  l,
         w, h, -l,
        -w, h,  l,
         w, h,  l,
         w, h, -l,
        
        // Bottom face
         w, -h, -l,
         w, -h,  l,
        -w, -h, -l,
         w, -h,  l,
        -w, -h,  l,
        -w, -h, -l,
    };
    
    const float cubeTex[] = {
        // Front face
        0, 0,
        0, 1,
        1, 0,
        0, 1,
        1, 1,
        1, 0,
        
        // Right face
        0, 0,
        0, 1,
        1, 0,
        0, 1,
        1, 1,
        1, 0,
        
        // Back face
        0, 0,
        0, 1,
        1, 0,
        0, 1,
        1, 1,
        1, 0,
        
        // Left face
        0, 0,
        0, 1,
        1, 0,
        0, 1,
        1, 1,
        1, 0,
        
        // Top face
        0, 0,
        0, 1,
        1, 0,
        0, 1,
        1, 1,
        1, 0,
        
        // Bottom face
        0, 0,
        0, 1,
        1, 0,
        0, 1,
        1, 1,
        1, 0,
    };
    
    const float cubeNormals[] = {
        // Front face
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        
        // Right face
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        
        // Back face
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        
        // Left face
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        
        // Top face
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        
        // Bottom face
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f
    };
    
    for (int i = 0; i < kNumBoxVertices; i++) {
        vertexLayout[i].x =  cubeVertices[i * 3 + 0];
        vertexLayout[i].y =  cubeVertices[i * 3 + 1];
        vertexLayout[i].z =  cubeVertices[i * 3 + 2];
        vertexLayout[i].u =  cubeTex[i * 2 + 0];
        vertexLayout[i].v =  cubeTex[i * 2 + 1];
        vertexLayout[i].nx = cubeNormals[i * 3 + 0];
        vertexLayout[i].ny = cubeNormals[i * 3 + 1];
        vertexLayout[i].nz = cubeNormals[i * 3 + 2];
    }
}

VROBox::~VROBox() {
    
}
