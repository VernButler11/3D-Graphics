///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
    const char* g_ModelName = "model";
    const char* g_ColorValueName = "objectColor";
    const char* g_TextureValueName = "objectTexture";
    const char* g_UseTextureName = "bUseTexture";
    const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 ***********************************************************/
SceneManager::~SceneManager()
{
    m_pShaderManager = NULL;
    delete m_basicMeshes;
    m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
    int width = 0;
    int height = 0;
    int colorChannels = 0;
    GLuint textureID = 0;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* image = stbi_load(
        filename,
        &width,
        &height,
        &colorChannels,
        0);

    if (image)
    {
        std::cout << "Successfully loaded image:" << filename
            << ", width:" << width
            << ", height:" << height
            << ", channels:" << colorChannels << std::endl;

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (colorChannels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0,
                GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (colorChannels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            std::cout << "Not implemented to handle image with "
                << colorChannels << " channels" << std::endl;
            stbi_image_free(image);
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_textureIDs[m_loadedTextures].ID = textureID;
        m_textureIDs[m_loadedTextures].tag = tag;
        m_loadedTextures++;

        return true;
    }

    std::cout << "Could not load image:" << filename << std::endl;
    return false;
}

/***********************************************************
 *  BindGLTextures()
 ***********************************************************/
void SceneManager::BindGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
    }
}

/***********************************************************
 *  DestroyGLTextures()
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glGenTextures(1, &m_textureIDs[i].ID);
    }
}

/***********************************************************
 *  FindTextureID()
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
    int textureID = -1;
    int index = 0;
    bool bFound = false;

    while ((index < m_loadedTextures) && (bFound == false))
    {
        if (m_textureIDs[index].tag.compare(tag) == 0)
        {
            textureID = m_textureIDs[index].ID;
            bFound = true;
        }
        else
            index++;
    }

    return textureID;
}

/***********************************************************
 *  FindTextureSlot()
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
    int textureSlot = -1;
    int index = 0;
    bool bFound = false;

    while ((index < m_loadedTextures) && (bFound == false))
    {
        if (m_textureIDs[index].tag.compare(tag) == 0)
        {
            textureSlot = index;
            bFound = true;
        }
        else
            index++;
    }

    return textureSlot;
}

/***********************************************************
 *  FindMaterial()
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
    if (m_objectMaterials.size() == 0)
    {
        return false;
    }

    int index = 0;
    bool bFound = false;
    while ((index < m_objectMaterials.size()) && (bFound == false))
    {
        if (m_objectMaterials[index].tag.compare(tag) == 0)
        {
            bFound = true;
            material.ambientColor = m_objectMaterials[index].ambientColor;
            material.ambientStrength = m_objectMaterials[index].ambientStrength;
            material.diffuseColor = m_objectMaterials[index].diffuseColor;
            material.specularColor = m_objectMaterials[index].specularColor;
            material.shininess = m_objectMaterials[index].shininess;
        }
        else
        {
            index++;
        }
    }

    return true;
}

/***********************************************************
 *  SetTransformations()
 ***********************************************************/
void SceneManager::SetTransformations(
    glm::vec3 scaleXYZ,
    float XrotationDegrees,
    float YrotationDegrees,
    float ZrotationDegrees,
    glm::vec3 positionXYZ)
{
    glm::mat4 modelView;
    glm::mat4 scale;
    glm::mat4 rotationX;
    glm::mat4 rotationY;
    glm::mat4 rotationZ;
    glm::mat4 translation;

    scale = glm::scale(scaleXYZ);
    rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
    rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(positionXYZ);

    modelView = translation * rotationX * rotationY * rotationZ * scale;

    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setMat4Value(g_ModelName, modelView);
    }
}

/***********************************************************
 *  SetShaderColor()
 ***********************************************************/
void SceneManager::SetShaderColor(
    float redColorValue,
    float greenColorValue,
    float blueColorValue,
    float alphaValue)
{
    glm::vec4 currentColor;

    currentColor.r = redColorValue;
    currentColor.g = greenColorValue;
    currentColor.b = blueColorValue;
    currentColor.a = alphaValue;

    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setIntValue(g_UseTextureName, false);
        m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
    }
}

/***********************************************************
 *  SetShaderTexture()
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setIntValue(g_UseTextureName, true);

        int textureSlot = FindTextureSlot(textureTag);
        m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
    }
}

/***********************************************************
 *  SetTextureUVScale()
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
    }
}

/***********************************************************
 *  SetShaderMaterial()
 ***********************************************************/
void SceneManager::SetShaderMaterial(std::string materialTag)
{
    if (m_objectMaterials.size() > 0)
    {
        OBJECT_MATERIAL material;
        bool bReturn = FindMaterial(materialTag, material);
        if (bReturn == true)
        {
            m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
            m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
            m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
            m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
            m_pShaderManager->setFloatValue("material.shininess", material.shininess);
        }
    }
}

/***********************************************************
 *  PrepareScene()
 *  Load meshes and textures for Milestone 4
 ***********************************************************/
void SceneManager::PrepareScene()
{
    // meshes
    m_basicMeshes->LoadPlaneMesh();
    m_basicMeshes->LoadCylinderMesh();
    m_basicMeshes->LoadSphereMesh();
    m_basicMeshes->LoadConeMesh();

    // textures (paths may be .jpg or .png depending on your files)
    CreateGLTexture("../../Utilities/textures/stainedglass.jpg", "stainedglass");
    CreateGLTexture("../../Utilities/textures/breadcrust.jpg", "breadcrust");
    CreateGLTexture("../../Utilities/textures/rusticwood.jpg", "rusticwood");
    CreateGLTexture("../../Utilities/textures/pavers.jpg", "pavers");
    CreateGLTexture("../../Utilities/textures/abstract.jpg", "abstract");
    CreateGLTexture("../../Utilities/textures/cheddar.jpg", "cheddar");
    CreateGLTexture("../../Utilities/textures/drywall.jpg", "drywall");
}

/***********************************************************
 *  RenderScene()
 *  Apply textures to vase object and plane (Milestone 4)
 ***********************************************************/
void SceneManager::RenderScene()
{

    // make sure all loaded textures are bound
    BindGLTextures();
    // ------------------------------------------------------------
// LIGHTING SETUP — Module Six Requirement
// Two distinct light sources: one white, one colored
// This block must run BEFORE drawing any objects
// ------------------------------------------------------------

// Enable lighting system globally (individual objects may disable it)
    m_pShaderManager->setIntValue("bUseLighting", false);

    // Camera position for specular highlights
    m_pShaderManager->setVec3Value("viewPos", m_cameraPosition);

    // ------------------------------------------------------------
    // Light 1 — White Key Light (main illumination)
    // ------------------------------------------------------------
    m_pShaderManager->setVec3Value("light1.position", glm::vec3(2.0f, 4.0f, 2.0f));
    m_pShaderManager->setVec3Value("light1.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    m_pShaderManager->setVec3Value("light1.diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
    m_pShaderManager->setVec3Value("light1.specular", glm::vec3(1.0f, 1.0f, 1.0f));

    // ------------------------------------------------------------
    // Light 2 — Colored Fill Light (adds depth + color variation)
    // ------------------------------------------------------------
    m_pShaderManager->setVec3Value("light2.position", glm::vec3(-3.0f, 1.0f, -2.0f));
    m_pShaderManager->setVec3Value("light2.ambient", glm::vec3(0.1f, 0.0f, 0.0f));   // subtle red tint
    m_pShaderManager->setVec3Value("light2.diffuse", glm::vec3(0.9f, 0.1f, 0.1f));   // strong red fill
    m_pShaderManager->setVec3Value("light2.specular", glm::vec3(0.6f, 0.1f, 0.1f));   // red-tinted highlights
    glm::vec3 scaleXYZ;
    float XrotationDegrees = 0.0f;
    float YrotationDegrees = 0.0f;
    float ZrotationDegrees = 0.0f;
    glm::vec3 positionXYZ;

    /****************************************************************/
    /*** GROUND PLANE — textured with drywall                      ***/
    /****************************************************************/
    scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
    XrotationDegrees = 0.0f;
    YrotationDegrees = 0.0f;
    ZrotationDegrees = 0.0f;
    positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

    SetTransformations(
        scaleXYZ,
        XrotationDegrees,
        YrotationDegrees,
        ZrotationDegrees,
        positionXYZ);

    //  ENABLE LIGHTING FOR THE PLANE (Milestone Five requirement)
    m_pShaderManager->setIntValue("bUseLighting", true);

    // drywall texture, lightly tiled
    SetShaderTexture("drywall");
    SetTextureUVScale(4.0f, 2.0f);

    m_basicMeshes->DrawPlaneMesh();

    /****************************************************************/
    /*** VASE STEP 1 — VASE BODY (CYLINDER)                        ***/
    /*** Textured with stainedglass, strongly tiled (complex)      ***/
    /****************************************************************/
    scaleXYZ = glm::vec3(1.0f, 3.0f, 1.0f);
    XrotationDegrees = 0.0f;
    YrotationDegrees = 0.0f;
    ZrotationDegrees = 0.0f;
    positionXYZ = glm::vec3(0.0f, 2.0f, 5.0f);

    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

    // COMPLEX OBJECT → LIGHTING OFF
    m_pShaderManager->setIntValue("bUseLighting", true);

    // complex texturing: tiling stainedglass around the vase
    SetShaderTexture("stainedglass");
    SetTextureUVScale(6.0f, 3.0f);   // strong tiling for rubric "complex technique"

    m_basicMeshes->DrawCylinderMesh();

    /****************************************************************/
    /*** VASE STEP 2 — SOIL / PLANT BASE (SPHERE)                  ***/
    /*** Textured with breadcrust                                  ***/
    /****************************************************************/
    scaleXYZ = glm::vec3(0.8f, 0.8f, 0.8f);
    XrotationDegrees = 0.0f;
    YrotationDegrees = 0.0f;
    ZrotationDegrees = 0.0f;
    positionXYZ = glm::vec3(0.0f, 4.3f, 5.0f);

    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

    // COMPLEX OBJECT → LIGHTING OFF
    m_pShaderManager->setIntValue("bUseLighting", true);

    SetShaderTexture("breadcrust");
    SetTextureUVScale(1.5f, 1.5f);

    m_basicMeshes->DrawSphereMesh();


    /****************************************************************/
    /*** VASE STEP 3 — SLIM STEM (CYLINDER)                        ***/
    /*** Textured with rusticwood                                  ***/
    /****************************************************************/
    scaleXYZ = glm::vec3(0.15f, 2.0f, 0.15f);
    XrotationDegrees = 0.0f;
    YrotationDegrees = 0.0f;
    ZrotationDegrees = 0.0f;
    positionXYZ = glm::vec3(0.0f, 5.0f, 5.0f);

    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

    //  COMPLEX OBJECT → LIGHTING OFF
    m_pShaderManager->setIntValue("bUseLighting", true);

    SetShaderTexture("rusticwood");
    SetTextureUVScale(1.0f, 2.0f);

    m_basicMeshes->DrawCylinderMesh();

    //****************************************************************/
    /*** VASE STEP 4 — LEFT LEAF (SPHERE)                          ***/
    /*** Textured with pavers                                      ***/
    /****************************************************************/
    scaleXYZ = glm::vec3(0.6f, 0.2f, 0.3f);
    XrotationDegrees = 0.0f;
    YrotationDegrees = -30.0f;
    ZrotationDegrees = 0.0f;
    positionXYZ = glm::vec3(-0.5f, 6.6f, 5.0f);

    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

    // COMPLEX OBJECT → LIGHTING OFF
    m_pShaderManager->setIntValue("bUseLighting", true);

    SetShaderTexture("pavers");
    SetTextureUVScale(2.0f, 2.0f);

    m_basicMeshes->DrawSphereMesh();


    /****************************************************************/
    /*** VASE STEP 4 — RIGHT LEAF (SPHERE)                         ***/
    /*** Textured with pavers                                      ***/
    /****************************************************************/
    scaleXYZ = glm::vec3(0.6f, 0.2f, 0.3f);
    XrotationDegrees = 0.0f;
    YrotationDegrees = 30.0f;
    ZrotationDegrees = 0.0f;
    positionXYZ = glm::vec3(0.5f, 6.6f, 5.0f);

    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

    // COMPLEX OBJECT → LIGHTING OFF
    m_pShaderManager->setIntValue("bUseLighting", true);

    SetShaderTexture("pavers");
    SetTextureUVScale(2.0f, 2.0f);

    m_basicMeshes->DrawSphereMesh();


    /****************************************************************/
    /*** VASE STEP 5 — FLOWER CENTER (SPHERE)                      ***/
    /*** Textured with cheddar                                     ***/
    /****************************************************************/
    scaleXYZ = glm::vec3(0.4f, 0.4f, 0.4f);
    XrotationDegrees = 0.0f;
    YrotationDegrees = 0.0f;
    ZrotationDegrees = 0.0f;
    positionXYZ = glm::vec3(0.0f, 7.5f, 5.0f);

    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

    // COMPLEX OBJECT → LIGHTING OFF
    m_pShaderManager->setIntValue("bUseLighting", true);

    SetShaderTexture("cheddar");
    SetTextureUVScale(1.0f, 1.0f);

    m_basicMeshes->DrawSphereMesh();



    /****************************************************************/
    /*** VASE STEP 5 — UPRIGHT OUTWARD-FACING PETAL RING           ***/
    /*** Textured with abstract                                    ***/
    /****************************************************************/
    {
        float radius = 0.45f;
        float petalY = 7.4f;
        float angles[5] = { 0.0f, 72.0f, 144.0f, 216.0f, 288.0f };

        for (int i = 0; i < 5; i++)
        {
            float angleDeg = angles[i];
            float rad = glm::radians(angleDeg);

            scaleXYZ = glm::vec3(0.50f, 0.18f, 0.32f);
            XrotationDegrees = -90.0f;
            YrotationDegrees = angleDeg;
            ZrotationDegrees = 0.0f;

            positionXYZ = glm::vec3(
                radius * sinf(rad),
                petalY,
                5.0f + radius * cosf(rad));

            SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

            //  COMPLEX OBJECT → LIGHTING OFF
            m_pShaderManager->setIntValue("bUseLighting", true);

            SetShaderTexture("abstract");
            SetTextureUVScale(1.0f, 1.0f);

            m_basicMeshes->DrawSphereMesh();
        }
    }

    /****************************************************************/
    /*** FLOWER RECEPTACLE — anchors petals to the center          ***/
    /*** Keep as solid color for contrast                          ***/
    /****************************************************************/
    scaleXYZ = glm::vec3(0.35f, 0.35f, 0.35f);
    XrotationDegrees = 0.0f;
    YrotationDegrees = 0.0f;
    ZrotationDegrees = 0.0f;
    positionXYZ = glm::vec3(0.0f, 7.4f, 5.0f - 0.15f);

    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

    // COMPLEX OBJECT → LIGHTING OFF
    m_pShaderManager->setIntValue("bUseLighting", false);

    // disable texture and use a simple green color
    SetShaderColor(0.2f, 0.6f, 0.2f, 1.0f);

    m_basicMeshes->DrawSphereMesh();
}