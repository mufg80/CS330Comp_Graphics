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
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	// destroy the created OpenGL textures
	DestroyGLTextures();

	// Clear materials.
	m_objectMaterials.clear();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
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
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
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

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
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

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
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

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
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
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
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
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
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

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	// Load first texture from jpg files.
	// Use bool to know if there was a failure and notify user.
	bool loadFailed = false;

	// Blue Grey... pic courtesy Sisters Seamless 06-25-2020
	loadFailed = CreateGLTexture("BackgroundTile.jpg", "background");
	if (!loadFailed) {
		std::cout << "JPG [background] failed to load.\n";
		loadFailed = false;
	}

	// Created this texture using screenshot of original project picture and
	// using free website to process it.
	loadFailed = CreateGLTexture("PotGold.jpg", "pot");
	if (!loadFailed) {
		std::cout << "JPG [pot] failed to load.\n";
		loadFailed = false;
	}
	// Used provided image for handle of pot.
	loadFailed = CreateGLTexture("gold-seamless-texture.jpg", "gold");
	if (!loadFailed) {
		std::cout << "JPG [gold] failed to load.\n";
		loadFailed = false;
	}
	// Used provided image for handle of pot.
	loadFailed = CreateGLTexture("BlueRusticWood2.png", "rustic");
	if (!loadFailed) {
		std::cout << "JPG [rustic] failed to load.\n";
		loadFailed = false;
	}

	// Used provided image for handle of pot.
	loadFailed = CreateGLTexture("melon.bmp", "melon");
	if (!loadFailed) {
		std::cout << "JPG [melon] failed to load.\n";
		loadFailed = false;
	}

	// Used provided image for handle of pot.
	loadFailed = CreateGLTexture("leaf.bmp", "leaf");
	if (!loadFailed) {
		std::cout << "JPG [leaf] failed to load.\n";
		loadFailed = false;
	}

	// Used provided image for handle of pot.
	loadFailed = CreateGLTexture("knife_handle.jpg", "knife");
	if (!loadFailed) {
		std::cout << "JPG [knife] failed to load.\n";
		loadFailed = false;
	}
	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}


/***********************************************************
 *  DefineObjectMaterial()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the materials to be applied to the various meshes.
 ***********************************************************/

void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	// Adding material objects, some made up and others used from provided opengl sample program.

	// Silver material.
	OBJECT_MATERIAL silver;
	silver.ambientColor = glm::vec3(0.09225f, 0.09225f, 0.09225f);
	silver.ambientStrength = 0.1f;
	silver.diffuseColor = glm::vec3(0.40754f, 0.40754f, 0.40754f);
	silver.specularColor = glm::vec3(0.408273f, 0.408273f, 0.408273f);
	silver.shininess = 1.0;
	silver.tag = "silver";


	m_objectMaterials.push_back(silver);

	// Gold material.
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.ambientStrength = 0.3f;
	goldMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	// BlackMetal material.
	OBJECT_MATERIAL blackMetalMaterial;
	blackMetalMaterial.ambientColor = glm::vec3(0.02f, 0.02f, 0.02f);
	blackMetalMaterial.ambientStrength = 0.01f;
	blackMetalMaterial.diffuseColor = glm::vec3(0.01f, 0.01f, 0.01f);
	blackMetalMaterial.specularColor = glm::vec3(0.01f, 0.01f, 0.01f);
	blackMetalMaterial.shininess = 0.01;
	blackMetalMaterial.tag = "blackmetal";

	m_objectMaterials.push_back(blackMetalMaterial);

	// Blue Wood material.
	OBJECT_MATERIAL blueWoodMaterial;
	blueWoodMaterial.ambientColor = glm::vec3(0.01f, 0.01f, 0.01f);
	blueWoodMaterial.ambientStrength = 0.1f;
	blueWoodMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.2f);
	blueWoodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.3f);
	blueWoodMaterial.shininess = 0.1;
	blueWoodMaterial.tag = "bluewood";

	m_objectMaterials.push_back(blueWoodMaterial);

	// Cheese material.
	OBJECT_MATERIAL cheeseMaterial;
	cheeseMaterial.ambientColor = glm::vec3(0.01f, 0.01f, 0.01f);
	cheeseMaterial.ambientStrength = 0.1f;
	cheeseMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	cheeseMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cheeseMaterial.shininess = 0.3;
	cheeseMaterial.tag = "cheese";

	m_objectMaterials.push_back(cheeseMaterial);

	// Turqoise material.
	OBJECT_MATERIAL turqoiseMaterial;
	turqoiseMaterial.ambientColor = glm::vec3(0.1f, 0.18725f, 0.1745f);
	turqoiseMaterial.ambientStrength = 0.2f;
	turqoiseMaterial.diffuseColor = glm::vec3(0.396f, 0.74151f, 0.69102f);
	turqoiseMaterial.specularColor = glm::vec3(0.297254f, 0.30829f, 0.306678f);
	turqoiseMaterial.shininess = 0.1;
	turqoiseMaterial.tag = "turqoise";

	m_objectMaterials.push_back(turqoiseMaterial);


}


/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/
	m_pShaderManager->setBoolValue(g_UseLightingName, true);
	
	// Main white light positioned and settings applied.
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 100.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientC", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseC", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularC", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStr", 25.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularInt", 0.9f);

	// Softer blue light in foreground for specular reflection on pot handle.

	m_pShaderManager->setVec3Value("lightSources[1].position", -2.0f, 0.0f, 10.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientC", 0.01f, 0.01f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseC", 0.5f, 0.5f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[1].specularC", 0.05f, 0.05f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStr", 1.5f);
	m_pShaderManager->setFloatValue("lightSources[1].specularInt", 0.9f);


}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{

	// define the materials for objects in the scene
	DefineObjectMaterials();

	// add and define the light sources for the scene
	SetupSceneLights();

	// load the textures for the 3D scene
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	// Added in items needed in final project to load them into memory.
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{

	// Call functions to draw each part of scene.
	DrawBackDrop();
	DrawVase();
	DrawChest();
	DrawMelon();
	DrawLeaves();
}


// Draw the main background plane.
void SceneManager::DrawBackDrop() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// Set transformations for perpendicular Backdrop as plane.

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 50.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);

	// Apply uv scale to texture.
	SetTextureUVScale(1.0, 1.0);

	// Load background texture on plane.
	SetShaderTexture("background");

	// Load material for backplane.
	SetShaderMaterial("turqoise");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}

// Draw the vase made up of 3 meshes.
void SceneManager::DrawVase() {

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/

	// Set transformations for Vase torus 1 (large)
	// This will be the base of the vase and will be the largest. The
	// tapered cylinder will rest atop it.

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 2.5f, 10.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 3.0f, 0.0f);


	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);

	// Apply uv scale to help match texture on shapes.
	SetTextureUVScale(2.5, 2.5);

	// Load background texture on plane.
	SetShaderTexture("pot");

	// Set the active shader material with silver tag.
	SetShaderMaterial("silver");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/

	// Set transformations for Vase tapered cylinder
	// This cylinder will create the top of the vase.

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 1.5f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 5.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);

	// Apply uv scale to help match texture on shapes.
	SetTextureUVScale(2.5, 0.5);



	// Set the active shader material with silver tag.
	SetShaderMaterial("silver");

	// Load texture on cylinder.
	SetShaderTexture("pot");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/

	// Set transformations for Vase half torus
	// this torus will act as the handle. 
	// TODO - diameter must be reduced, if not, this mesh
	// can be removed.

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.8f, 3.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 130.0f;
	YrotationDegrees = 35.0f;
	ZrotationDegrees = 40.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.8f, 5.3f, 0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 1, 0.4, 1);

	// Load background texture on plane.
	SetShaderTexture("gold");

	// Set the active shader material with gold tag.
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();
}

// Draw chest object with two straps on front.
void SceneManager::DrawChest() {


	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/

	

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	
	/****************************************************************/

	// Set transformations for chest
	// This cylinder will create the top of the vase.

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(24.0f, 12.0f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -5.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	

	// Apply uv scale to help match texture on shapes.
	SetTextureUVScale(1.0, 1.0);



	// Set the active shader material with bluewood
	SetShaderMaterial("bluewood");

	// Load texture on box
	SetShaderTexture("rustic");
	//SetShaderColor(0.0, 0.0, 1, 0.3);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	// Set transformations for strap 1
	

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 11.8f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.0f, -5.0f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 1, 0.4, 1);

	// Load background texture on strap 1
	SetShaderTexture("knife");

	// Set the active shader material with blackmetal
	SetShaderMaterial("blackmetal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	/****************************************************************/

	// Set transformations for strap 2
	

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 11.8f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, -5.0f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 1, 0.4, 1);

	// Load background texture on strap2
	SetShaderTexture("knife");

	// Set the active shader material with blackmetal
	SetShaderMaterial("blackmetal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}

// Draw melon 
void SceneManager::DrawMelon() {

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/

	// Set transformations for melon
	
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	/****************************************************************/

	// Set transformations for sphere

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 2.5f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -70.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -40.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 3.5f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	// Apply uv scale to help match texture on shapes.
	SetTextureUVScale(1.0, 1.0);



	// Set the active shader material with cheese tag
	SetShaderMaterial("cheese");

	// Load texture on sphere.
	SetShaderTexture("melon");
	//SetShaderColor(0.0, 0.0, 1, 0.3);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

}

// Function to draw the multiple leaves.
void SceneManager::DrawLeaves() {

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/

	// Set transformations for leaf

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;


	/****************************************************************/

	// Set transformations for leaf

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.3f, 0.8f, 0.01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.5f, 2.0f, 0.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);



	// Apply uv scale to help match texture on shapes.
	SetTextureUVScale(1.0, 1.0);



	// Set the active shader material with turquoise
	SetShaderMaterial("turqoise");

	// Load texture on sphere.
	SetShaderTexture("leaf");
	//SetShaderColor(0.0, 0.0, 1, 0.3);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.6f, 0.01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -5.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 2.0f, 0.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.6f, 0.01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 75.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.1f, 3.0f, 0.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();



	// Second cluster

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.6f, 0.01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -25.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.5f, 1.7f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.6f, 0.01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 75.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 1.7f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();



	// Third 

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.6f, 0.01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 25.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.3f, 5.7f, 2.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();


	// Last Cluster

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.6f, 0.01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -75.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.3f, 1.7f, 2.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();



	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.6f, 0.01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 55.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.8f, 1.7f, 2.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

}
