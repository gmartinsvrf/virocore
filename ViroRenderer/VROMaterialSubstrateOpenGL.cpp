//
//  VROMaterialSubstrateOpenGL.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 5/2/16.
//  Copyright © 2016 Viro Media. All rights reserved.
//

#include "VROMaterialSubstrateOpenGL.h"
#include "VRODriverOpenGL.h"
#include "VROMaterial.h"
#include "VROShaderProgram.h"
#include "VROAllocationTracker.h"
#include "VROEye.h"
#include "VROLight.h"
#include "VROSortKey.h"
#include <sstream>

static std::shared_ptr<VROShaderModifier> sDiffuseTextureModifier;
static std::shared_ptr<VROShaderModifier> sNormalMapTextureModifier;
static std::shared_ptr<VROShaderModifier> sReflectiveTextureModifier;

void VROMaterialSubstrateOpenGL::hydrateProgram(VRODriverOpenGL &driver) {
    _program->hydrate();
}

VROMaterialSubstrateOpenGL::VROMaterialSubstrateOpenGL(const VROMaterial &material, VRODriverOpenGL &driver) :
    _material(material),
    _lightingModel(material.getLightingModel()),
    _program(nullptr),
    _diffuseSurfaceColorUniform(nullptr),
    _diffuseIntensityUniform(nullptr),
    _alphaUniform(nullptr),
    _shininessUniform(nullptr),
    _normalMatrixUniform(nullptr),
    _modelMatrixUniform(nullptr),
    _modelViewMatrixUniform(nullptr),
    _modelViewProjectionMatrixUniform(nullptr),
    _cameraPositionUniform(nullptr) {

    switch (material.getLightingModel()) {
        case VROLightingModel::Constant:
            loadConstantLighting(material, driver);
            break;
                
        case VROLightingModel::Blinn:
            loadBlinnLighting(material, driver);
            break;
                
        case VROLightingModel::Lambert:
            loadLambertLighting(material, driver);
            break;
                
        case VROLightingModel::Phong:
            loadPhongLighting(material, driver);
            break;
                
        default:
            break;
    }
        
    ALLOCATION_TRACKER_ADD(MaterialSubstrates, 1);
}
    
VROMaterialSubstrateOpenGL::~VROMaterialSubstrateOpenGL() {
    ALLOCATION_TRACKER_SUB(MaterialSubstrates, 1);
}

void VROMaterialSubstrateOpenGL::loadConstantLighting(const VROMaterial &material, VRODriverOpenGL &driver) {
    VROMaterialVisual &diffuse = material.getDiffuse();
    
    std::string vertexShader = "standard_vsh";
    std::string fragmentShader;
    
    std::vector<std::string> samplers;
    std::vector<std::shared_ptr<VROShaderModifier>> modifiers = material.getShaderModifiers();

    if (diffuse.getTextureType() == VROTextureType::None) {
        fragmentShader = "constant_fsh";
    }
    else if (diffuse.getTextureType() == VROTextureType::Texture2D) {
        _textures.push_back(diffuse.getTexture());
        samplers.push_back("diffuse_texture");
        modifiers.push_back(createDiffuseTextureModifier());

        fragmentShader = "constant_fsh";
    }
    else if (diffuse.getTextureType() == VROTextureType::TextureEGLImage) {
        _textures.push_back(diffuse.getTexture());
        samplers.push_back("diffuse_texture");
        modifiers.push_back(createDiffuseTextureModifier());

        fragmentShader = "constant_fsh";
        modifiers.push_back(createEGLImageModifier());
    }
    else { // TextureCube
        _textures.push_back(diffuse.getTexture());
        samplers.push_back("diffuse_texture");

        fragmentShader = "constant_q_fsh";
    }
    
    _program = driver.getPooledShader(vertexShader, fragmentShader, samplers,
                                      modifiers);
    if (!_program->isHydrated()) {
        addUniforms();
        hydrateProgram(driver);
    }
    else {
        loadUniforms();
    }
}

void VROMaterialSubstrateOpenGL::loadLambertLighting(const VROMaterial &material, VRODriverOpenGL &driver) {
    std::string vertexShader = "standard_vsh";
    std::string fragmentShader = "lambert_fsh";
    
    std::vector<std::string> samplers;
    std::vector<std::shared_ptr<VROShaderModifier>> modifiers = material.getShaderModifiers();
    
    VROMaterialVisual &diffuse    = material.getDiffuse();
    VROMaterialVisual &normal     = material.getNormal();
    VROMaterialVisual &reflective = material.getReflective();
    
    if (diffuse.getTextureType() != VROTextureType::None) {
        _textures.push_back(diffuse.getTexture());
        samplers.push_back("diffuse_texture");
        modifiers.push_back(createDiffuseTextureModifier());
        
        // For Android video
        if (diffuse.getTextureType() == VROTextureType::TextureEGLImage) {
            modifiers.push_back(createEGLImageModifier());
        }
    }
    
    if (normal.getTextureType() == VROTextureType::Texture2D) {
        _textures.push_back(normal.getTexture());
        samplers.push_back("normal_texture");
        modifiers.push_back(createNormalMapTextureModifier());
    }
    
    if (reflective.getTextureType() == VROTextureType::TextureCube) {
        _textures.push_back(reflective.getTexture());
        samplers.push_back("reflect_texture");
        modifiers.push_back(createReflectiveTextureModifier());
    }
    
    _program = driver.getPooledShader(vertexShader, fragmentShader, samplers,
                                      modifiers);
    if (!_program->isHydrated()) {
        addUniforms();
        hydrateProgram(driver);
    }
    else {
        loadUniforms();
    }
}

void VROMaterialSubstrateOpenGL::loadPhongLighting(const VROMaterial &material, VRODriverOpenGL &driver) {
    /*
     If there's no specular map, then we fall back to Lambert lighting.
     */
    VROMaterialVisual &specular = material.getSpecular();
    if (specular.getTextureType() != VROTextureType::Texture2D) {
        loadLambertLighting(material, driver);
    }
    else {
        std::string vertexShader = "standard_vsh";
        std::string fragmentShader = "phong_fsh";
        
        configureSpecularShader(vertexShader, fragmentShader, material, driver);
    }
}

void VROMaterialSubstrateOpenGL::loadBlinnLighting(const VROMaterial &material, VRODriverOpenGL &driver) {
    /*
     If there's no specular map, then we fall back to Lambert lighting.
     */
    VROMaterialVisual &specular = material.getSpecular();
    if (specular.getTextureType() != VROTextureType::Texture2D) {
        loadLambertLighting(material, driver);
    }
    else {
        std::string vertexShader = "standard_vsh";
        std::string fragmentShader = "blinn_fsh";
        
        configureSpecularShader(vertexShader, fragmentShader, material, driver);
    }
}

// Configures properties for both Blinn and Phong
void VROMaterialSubstrateOpenGL::configureSpecularShader(std::string vertexShader, std::string fragmentShader,
                                                         const VROMaterial &material, VRODriverOpenGL &driver) {
    std::vector<std::string> samplers;
    std::vector<std::shared_ptr<VROShaderModifier>> modifiers = material.getShaderModifiers();

    VROMaterialVisual &diffuse    = material.getDiffuse();
    VROMaterialVisual &specular   = material.getSpecular();
    VROMaterialVisual &normal     = material.getNormal();
    VROMaterialVisual &reflective = material.getReflective();
    
    if (diffuse.getTextureType() != VROTextureType::None) {
        _textures.push_back(diffuse.getTexture());
        samplers.push_back("diffuse_texture");
        modifiers.push_back(createDiffuseTextureModifier());
        
        // For Android video
        if (diffuse.getTextureType() == VROTextureType::TextureEGLImage) {
            modifiers.push_back(createEGLImageModifier());
        }
    }
    
    _textures.push_back(specular.getTexture());
    samplers.push_back("specular_texture");
    
    if (normal.getTextureType() == VROTextureType::Texture2D) {
        _textures.push_back(normal.getTexture());
        samplers.push_back("normal_texture");
        modifiers.push_back(createNormalMapTextureModifier());
    }

    if (reflective.getTextureType() == VROTextureType::TextureCube) {
        _textures.push_back(reflective.getTexture());
        samplers.push_back("reflect_texture");
        modifiers.push_back(createReflectiveTextureModifier());
    }
    
    _program = driver.getPooledShader(vertexShader, fragmentShader, samplers,
                                      modifiers);
    if (!_program->isHydrated()) {
        addUniforms();
        _shininessUniform = _program->addUniform(VROShaderProperty::Float, 1, "material_shininess");
        hydrateProgram(driver);
    }
    else {
        _shininessUniform = _program->getUniform("material_shininess");
        loadUniforms();
    }
}

void VROMaterialSubstrateOpenGL::addUniforms() {
    _normalMatrixUniform = _program->addUniform(VROShaderProperty::Mat4, 1, "normal_matrix");
    _modelMatrixUniform = _program->addUniform(VROShaderProperty::Mat4, 1, "model_matrix");
    _modelViewMatrixUniform = _program->addUniform(VROShaderProperty::Mat4, 1, "modelview_matrix");
    _modelViewProjectionMatrixUniform = _program->addUniform(VROShaderProperty::Mat4, 1, "modelview_projection_matrix");
    _cameraPositionUniform = _program->addUniform(VROShaderProperty::Vec3, 1, "camera_position");
    
    _diffuseSurfaceColorUniform = _program->addUniform(VROShaderProperty::Vec4, 1, "material_diffuse_surface_color");
    _diffuseIntensityUniform = _program->addUniform(VROShaderProperty::Float, 1, "material_diffuse_intensity");
    _alphaUniform = _program->addUniform(VROShaderProperty::Float, 1, "material_alpha");
    
    for (const std::shared_ptr<VROShaderModifier> &modifier : _material.getShaderModifiers()) {
        std::vector<std::string> uniformNames = modifier->getUniforms();
        
        for (std::string &uniformName : uniformNames) {
            VROUniform *uniform = _program->getUniform(uniformName);
            _shaderModifierUniforms.push_back(uniform);
        }
    }
}

void VROMaterialSubstrateOpenGL::loadUniforms() {
    _diffuseSurfaceColorUniform = _program->getUniform("material_diffuse_surface_color");
    _diffuseIntensityUniform = _program->getUniform("material_diffuse_intensity");
    _alphaUniform = _program->getUniform("material_alpha");
    
    _normalMatrixUniform = _program->getUniform("normal_matrix");
    _modelMatrixUniform = _program->getUniform("model_matrix");
    _modelViewMatrixUniform = _program->getUniform("modelview_matrix");
    _modelViewProjectionMatrixUniform = _program->getUniform("modelview_projection_matrix");
    _cameraPositionUniform = _program->getUniform("camera_position");
    
    for (const std::shared_ptr<VROShaderModifier> &modifier : _material.getShaderModifiers()) {
        std::vector<std::string> uniformNames = modifier->getUniforms();
        
        for (std::string &uniformName : uniformNames) {
            VROUniform *uniform = _program->getUniform(uniformName);
            _shaderModifierUniforms.push_back(uniform);
        }
    }
}

void VROMaterialSubstrateOpenGL::bindShader() {
    _program->bind();
}

void VROMaterialSubstrateOpenGL::bindLights(int lightsHash,
                                            const std::vector<std::shared_ptr<VROLight>> &lights,
                                            const VRORenderContext &context,
                                            VRODriver &driver) {
    
    if (lights.empty()) {
        VROLightingUBO::unbind(_program);
        _lightingUBO.reset();
        return;
    }
    
    VRODriverOpenGL &glDriver = (VRODriverOpenGL &)driver;
    for (const std::shared_ptr<VROLight> &light : lights) {
        light->propagateUpdates();
    }

    if (!_lightingUBO || _lightingUBO->getHash() != lightsHash) {
        _lightingUBO = glDriver.getLightingUBO(lightsHash);
        if (!_lightingUBO) {
            _lightingUBO = glDriver.createLightingUBO(lightsHash, lights);
        }
    }
    
    _lightingUBO->bind(_program);
}

void VROMaterialSubstrateOpenGL::bindDepthSettings() {
    if (_material.getWritesToDepthBuffer()) {
        glDepthMask(GL_TRUE);
    }
    else {
        glDepthMask(GL_FALSE);
    }
    
    if (_material.getReadsFromDepthBuffer()) {
        glDepthFunc(GL_LEQUAL);
    }
    else {
        glDepthFunc(GL_ALWAYS);
    }
}

void VROMaterialSubstrateOpenGL::bindCullingSettings() {
    VROCullMode cullMode = _material.getCullMode();
    
    if (cullMode == VROCullMode::None) {
        glDisable(GL_CULL_FACE);
    }
    else if (cullMode == VROCullMode::Back) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else if (cullMode == VROCullMode::Front) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    }
}

void VROMaterialSubstrateOpenGL::bindViewUniforms(VROMatrix4f transform, VROMatrix4f modelview,
                                                  VROMatrix4f projectionMatrix, VROMatrix4f normalMatrix,
                                                  VROVector3f cameraPosition) {
    
    if (_normalMatrixUniform != nullptr) {
        _normalMatrixUniform->setMat4(normalMatrix);
    }
    if (_modelMatrixUniform != nullptr) {
        _modelMatrixUniform->setMat4(transform);
    }
    if (_modelViewMatrixUniform != nullptr) {
        _modelViewMatrixUniform->setMat4(modelview);
    }
    if (_modelViewProjectionMatrixUniform != nullptr) {
        _modelViewProjectionMatrixUniform->setMat4(projectionMatrix.multiply(modelview));
    }
    if (_cameraPositionUniform != nullptr) {
        _cameraPositionUniform->setVec3(cameraPosition);
    }
}

void VROMaterialSubstrateOpenGL::bindMaterialUniforms(float opacity) {
    if (_diffuseSurfaceColorUniform != nullptr) {
        _diffuseSurfaceColorUniform->setVec4(_material.getDiffuse().getColor());
    }
    if (_diffuseIntensityUniform != nullptr) {
        _diffuseIntensityUniform->setFloat(_material.getDiffuse().getIntensity());
    }
    if (_alphaUniform != nullptr) {
        _alphaUniform->setFloat(_material.getTransparency() * opacity);
    }
    if (_shininessUniform != nullptr) {
        _shininessUniform->setFloat(_material.getShininess());
    }
    
    for (VROUniform *uniform : _shaderModifierUniforms) {
        uniform->set(nullptr);
    }
}

void VROMaterialSubstrateOpenGL::updateSortKey(VROSortKey &key) const {
    key.shader = _program->getShaderId();
    key.textures = hashTextures(_textures);
}

std::shared_ptr<VROShaderModifier> VROMaterialSubstrateOpenGL::createDiffuseTextureModifier() {
    /*
     Modifier that multiplies the material's surface color by a diffuse texture.
     */
    if (!sDiffuseTextureModifier) {
        std::vector<std::string> modifierCode =  {
            "uniform sampler2D diffuse_texture;",
            "_surface.diffuse_color *= texture(diffuse_texture, _surface.diffuse_texcoord);"
        };
        sDiffuseTextureModifier = std::make_shared<VROShaderModifier>(VROShaderEntryPoint::Surface,
                                                                      modifierCode);
    }
   
    return sDiffuseTextureModifier;
}

std::shared_ptr<VROShaderModifier> VROMaterialSubstrateOpenGL::createNormalMapTextureModifier() {
    /*
     Modifier that samples a normal map to determine the direction of the normal to use at each
     fragment.
     */
    if (!sNormalMapTextureModifier) {
        std::vector<std::string> modifierCode =  {
            "uniform sampler2D normal_texture;",
            "_surface.normal = v_tbn * normalize( texture(normal_texture, _surface.diffuse_texcoord).xyz * 2.0 - 1.0 );"
        };
        sNormalMapTextureModifier = std::make_shared<VROShaderModifier>(VROShaderEntryPoint::Surface,
                                                                        modifierCode);
    }
    
    return sNormalMapTextureModifier;
}

std::shared_ptr<VROShaderModifier> VROMaterialSubstrateOpenGL::createReflectiveTextureModifier() {
    /*
     Modifier that adds reflective color to the final light computation.
     */
    if (!sReflectiveTextureModifier) {
        std::vector<std::string> modifierCode =  {
            "uniform samplerCube reflect_texture;",
            "lowp vec4 reflective_color = compute_reflection(_surface.position, camera_position, _surface.normal, reflect_texture);",
            "_output_color.xyz += reflective_color.xyz;"
        };
        sReflectiveTextureModifier = std::make_shared<VROShaderModifier>(VROShaderEntryPoint::Fragment,
                                                                         modifierCode);
    }
    
    return sReflectiveTextureModifier;
}

std::shared_ptr<VROShaderModifier> VROMaterialSubstrateOpenGL::createEGLImageModifier() {
    std::vector<std::string> input;
    std::shared_ptr<VROShaderModifier> modifier = std::make_shared<VROShaderModifier>(VROShaderEntryPoint::Surface, input);
    modifier->addReplacement("uniform sampler2D diffuse_texture;", "uniform samplerExternalOES diffuse_texture;");
    
    return modifier;
}

uint32_t VROMaterialSubstrateOpenGL::hashTextures(const std::vector<std::shared_ptr<VROTexture>> &textures) const {
    uint32_t h = 0;
    for (const std::shared_ptr<VROTexture> &texture : textures) {
        h = 31 * h + texture->getTextureId();
    }
    return h;
}
