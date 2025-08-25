///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
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

	modelView = translation * rotationZ * rotationY * rotationX * scale;

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

/*************************************************************
*   LoadSceneTextures()
*
* This method is used for preparing the 3D scene by loading
* shapes and textures into memory to support 3D scene
* rendering.
*/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;
	bReturn = CreateGLTexture(
		"textures/steel.jpg",
		"steel"
	);
	bReturn = CreateGLTexture(
		"textures/obsidian.png",
		"obsidian"
	);
	bReturn = CreateGLTexture(
		"textures/orange.png",
		"orange"
	);
	bReturn = CreateGLTexture(
		"textures/black.jpg",
		"black"
	);
	bReturn = CreateGLTexture(
		"textures/tan.png",
		"tan"
	);
	bReturn = CreateGLTexture(
		"textures/taupe.jpg",
		"taupe"
	);
	bReturn = CreateGLTexture(
		"textures/white_wood.jpg",
		"white_wood"
	);
	bReturn = CreateGLTexture(
		"textures/redsq.png",
		"red_ceramic"
	);
	bReturn = CreateGLTexture(
		"textures/tileable_wood.jpg",
		"wood_log"
	);
	bReturn = CreateGLTexture(
		"textures/flower_tile.jpg",
		"flower_tile"
	);
	bReturn = CreateGLTexture(
		"textures/white_plastic.jpg",
		"white_plastic"
	);

	//After the textures are loaded into memeory,
	//they are bound to texture slots.
	BindGLTextures();
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
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}
/***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	// Gold material definition
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 01.f);
	metalMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	metalMaterial.shininess = 22.0f;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	// Cement material definition
	OBJECT_MATERIAL cementMaterial;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";
	m_objectMaterials.push_back(cementMaterial);

	// Wood material definition
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	// Tile material definition
	OBJECT_MATERIAL tileMaterial;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";

	m_objectMaterials.push_back(tileMaterial);

	// Glass material definition
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	// Clay material definition
	OBJECT_MATERIAL clayMaterial;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5;
	clayMaterial.tag = "clay";

	m_objectMaterials.push_back(clayMaterial);

	// Ceramic material definition
	OBJECT_MATERIAL ceramicMaterial;
	ceramicMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	ceramicMaterial.specularColor = glm::vec3(0.45f, 0.45f, 0.45f);
	ceramicMaterial.shininess = 50.0;
	ceramicMaterial.tag = "ceramic";

	m_objectMaterials.push_back(ceramicMaterial);
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
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Point Light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 16.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.75f, 0.75f, 0.75f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	m_pShaderManager->setVec3Value("pointLights[1].position", 20.0f, 16.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.75f, 0.75f, 0.75f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	m_pShaderManager->setVec3Value("pointLights[2].position", -20.0f, 16.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.75f, 0.75f, 0.75f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);

	// directional light to emulate overhead lights
	m_pShaderManager->setVec3Value("directionalLight.direction", 4.0f, 20.0f, -4.0f);
	m_pShaderManager->setVec3Value("directionLight.ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("directionLight.diffuse", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("directionLight.specular", 0.0f, 0.0f, 0.0f);
	//m_pShaderManager->setBoolValue("directionalLight.bActive", true);
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// loads the textures for the 3D scene
	LoadSceneTextures();

	// define the materials that will be used for the objects in the 3D scene
	DefineObjectMaterials();

	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
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
	// Countertop plane
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(25.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ,XrotationDegrees,YrotationDegrees,ZrotationDegrees,positionXYZ);

	// set the texture for the next draw command
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("taupe");
	SetShaderMaterial("tile");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// Splash plane
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(25.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// set the texture for the next draw command
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("white_wood");
	SetShaderMaterial("tile");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	// Lighter
	{
		// Handle Sphere
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.6f, 0.62f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(21.9f, 0.6f, -8.6f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		//SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("black");
		SetShaderMaterial("wood");

		// draw the mesh with transformationw values
		m_basicMeshes->DrawSphereMesh();

		// handle tapered cylinder
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 3.0f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(22.0f, 0.5f, -8.6f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("black");
		SetShaderMaterial("wood");

		// draw the mesh with transformation values
		m_basicMeshes->DrawTaperedCylinderMesh();

		// handle cylinder
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 3.0f, 0.35f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(22.0f, 0.7f, -8.6f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("black");
		SetShaderMaterial("wood");

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();

		// switch sphere
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.15f, 0.3f, 0.15f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(19.9f, 1.0f, -8.6f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();

		// Handle Taper to Extentsion Tube
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 1.0f, 0.36f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(19.0f, 0.7f, -8.6f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("black");
		SetShaderMaterial("wood");

		// draw the mesh with transformation values
		m_basicMeshes->DrawTaperedCylinderMesh();

		// Extension Tube 
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.2f, 5.0f, 0.2f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(19.0f, 0.7f, -8.6f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("steel");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();

		// Taper to End Cylinder
		// Handle Taper to Extentsion Tube
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 270.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(14.0f, 0.7f, -8.6f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("steel");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawTaperedCylinderMesh();

		// End Cylinder
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.3f, 1.0f, 0.3f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(14.0f, 0.7f, -8.6f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);
		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("steel");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();
	}

	// Candle
	{
		// Base Cylinder
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(3.0f, 5.0f, 3.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.0f, 0.01f, -6.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(3.0f, 3.0f);
		SetShaderTexture("orange");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();
		
		// Top Cylinder
		// Base Cylinder
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(3.3f, 1.0f, 3.3f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.0f, 4.95f, -6.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("black");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();
	};
	// Butter tray
	{
		// Base Plate
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(9.0f, 0.5f, 4.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-3.0f, 0.3f, 3.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(3.0f, 3.0f);
		SetShaderTexture("red_ceramic");
		SetShaderMaterial("ceramic");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Right side
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 2.5f, 2.7f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.5f, 1.8f, 3.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(3.0f, 3.0f);
		SetShaderTexture("red_ceramic");
		SetShaderMaterial("ceramic");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
		
		// Left Side
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 2.5f, 2.7f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-6.5f, 1.8f, 3.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(3.0f, 3.0f);
		SetShaderTexture("red_ceramic");
		SetShaderMaterial("ceramic");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Front side
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 2.5f, 7.45f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-3.0f, 1.8f, 4.4001f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(3.0f, 3.0f);
		SetShaderTexture("red_ceramic");
		SetShaderMaterial("ceramic");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Back side
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 2.5f, 7.45f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-3.0f, 1.8f, 2.1999f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(3.0f, 3.0f);
		SetShaderTexture("red_ceramic");
		SetShaderMaterial("ceramic");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Top
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(7.0f, 0.5f, 2.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-3.0f, 2.8f, 3.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(3.0f, 3.0f);
		SetShaderTexture("red_ceramic");
		SetShaderMaterial("ceramic");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Chicken base
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.3f, 0.5f, 0.3f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-3.0f, 3.1f, 3.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(3.0f, 3.0f);
		SetShaderTexture("orange");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawTaperedCylinderMesh();

		// Chicken body
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.3f, 0.5f, 0.15f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 180.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-3.0f, 3.9f, 3.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(3.0f, 3.0f);
		SetShaderTexture("orange");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawTaperedCylinderMesh();

		// Chicken tail
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.2f, 0.4f, 0.1f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 40.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-3.11f, 3.9f, 3.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(3.0f, 3.0f);
		SetShaderTexture("orange");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawConeMesh();

		// Chicken neck
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.2f, 0.6f, 0.1f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = -40.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-3.0f, 3.9f, 3.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(3.0f, 3.0f);
		SetShaderTexture("orange");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawConeMesh();

		// Chicken neck
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.05f, 0.05f, 0.05f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = -40.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-2.65f, 4.35f, 3.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(3.0f, 3.0f);
		SetShaderTexture("orange");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();
	}

	//Toaster base plate
	{
		// Base wood
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(12.0f, 0.4f, 17.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-16.0f, 0.3f, -0.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("wood_log");
		SetShaderMaterial("wood");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Tile #1
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(5.0f, 0.2f, 5.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-18.5f, 0.45f, -5.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("flower_tile");
		SetShaderMaterial("ceramic");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Tile #2
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(5.0f, 0.2f, 5.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-13.5f, 0.45f, -5.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("flower_tile");
		SetShaderMaterial("ceramic");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// 2nd row
		// Tile #3
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(5.0f, 0.2f, 5.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-18.5f, 0.45f, -0.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("flower_tile");
		SetShaderMaterial("ceramic");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Tile #4
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(5.0f, 0.2f, 5.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-13.5f, 0.45f, -0.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("flower_tile");
		SetShaderMaterial("ceramic");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// 3rd row
		// Tile #5
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(5.0f, 0.2f, 5.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-18.5f, 0.45f, 4.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("flower_tile");
		SetShaderMaterial("ceramic");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Tile #6
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(5.0f, 0.2f, 5.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-13.5f, 0.45f, 4.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("flower_tile");
		SetShaderMaterial("ceramic");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	// Toaster
	{
		// Right side
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.5f, 10.5001f, 13.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-13.0f, 5.8f, -0.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Left side
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.5f, 10.50001f, 13.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-19.0f, 5.8f, -0.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Back side
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(6.5f, 10.5f, 2.2f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-16.0f, 5.799f, -6.1999f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();


		// Front bottom
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(6.8f, 4.5f, 13.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-16.0f, 2.8f, -0.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Back metal
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(4.5f, 10.5f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-16.0f, 6.0f, -4.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("steel");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Front metal
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(4.5f, 10.5f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-16.0f, 6.0f, 2.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("steel");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
		// Right metal
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 10.5f, 7.6f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-14.0f, 6.0f, -0.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("steel");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Left metal
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 10.5f, 7.6f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-18.0f, 6.0f, -0.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("steel");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Middle metal
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.3f, 10.5f, 7.6f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-16.0f, 5.9999f, -0.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("steel");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Right metal pusher outline
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.3f, 7.0f, 2.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-14.75f, 7.55f, 4.6999f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("steel");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Left metal pusher outline
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.3f, 7.0f, 2.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-17.25f, 7.55f, 4.6999f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("steel");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Behind metal pusher outline
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.3f, 7.0f, 2.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-16.0f, 7.55f, 3.85f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("steel");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Bottom metal pusher outline
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.699f, 0.3f, 2.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-16.0f, 5.2f, 4.85f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("steel");
		SetShaderMaterial("glass");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Front strip behind push down
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(6.5f, 10.5f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-16.0f, 5.799f, 3.1999f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Top right plastic
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.9f, 7.0f, 2.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-14.15f, 7.55f, 4.6999f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Top left plastic
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.9f, 7.0f, 2.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-17.85f, 7.55f, 4.6999f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Pusher cylinder
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 90.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-16.0f, 10.0f, 3.7f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();
		
		// Pusher box
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(2.0f, 1.0f, 2.4f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-16.0f, 9.5f, 4.499f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// Dial
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 0.75f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 90.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-16.0f, 3.5f, 5.499f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawTaperedCylinderMesh();

		// Bottom button
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.2f, 1.0f, 0.2f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 90.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-14.0f, 3.5f, 5.7f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();

		// Middle button
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.2f, 1.0f, 0.2f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 90.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-14.0f, 4.3f, 5.7f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();

		// Top button
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.2f, 1.0f, 0.2f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 90.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-14.0f, 5.1f, 5.7f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// set the color values into the shader
		SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

		// set the texture for the next draw command
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderTexture("white_plastic");
		SetShaderMaterial("cement");

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();
	}
}
