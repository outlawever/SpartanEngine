/*
Copyright(c) 2016-2017 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES ============================
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <vector>
#include "../Core/Scene.h"
#include "ModelImporter.h"
#include "../FileSystem/FileSystem.h"
#include "../Logging/Log.h"
#include "../Components/Transform.h"
#include "../Components/MeshRenderer.h"
#include "../Components/MeshFilter.h"
#include "../Core/GameObject.h"
#include "../Core/Context.h"
#include "../Multithreading/ThreadPool.h"
#include "../Resource/ResourceCache.h"
//=======================================

//= NAMESPACES ====================
using namespace std;
using namespace Directus::Math;
using namespace Directus::Resource;
//=================================

// default pp steps
static auto ppsteps =
aiProcess_CalcTangentSpace |
aiProcess_GenSmoothNormals |
aiProcess_JoinIdenticalVertices |
aiProcess_ImproveCacheLocality |
aiProcess_LimitBoneWeights |
aiProcess_SplitLargeMeshes |
aiProcess_Triangulate |
aiProcess_GenUVCoords |
aiProcess_SortByPType |
aiProcess_FindDegenerates |
aiProcess_FindInvalidData |
aiProcess_FindInstances |
aiProcess_ValidateDataStructure |
aiProcess_OptimizeMeshes |
aiProcess_Debone |
aiProcess_ConvertToLeftHanded;

static int smoothAngle = 80;
vector<string> materialNames;

ModelImporter::ModelImporter(Context* context) : Subsystem(context)
{
	m_rootGameObject = nullptr;
}

ModelImporter::~ModelImporter()
{
}

void ModelImporter::LoadAsync(const string& filePath)
{
	ThreadPool* threadPool = g_context->GetSubsystem<ThreadPool>();
	threadPool->AddTask(std::bind(&ModelImporter::Load, this, filePath));
}

bool ModelImporter::Load(const string& filePath)
{
	m_isLoading = true;
	m_filePath = filePath;
	m_rootGameObject = nullptr;
	m_modelName = FileSystem::GetFileNameFromPath(m_filePath);

	// Set up Assimp importer
	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_ICL_PTCACHE_SIZE, 64); // Optimize mesh
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT); // Remove points and lines.
	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_CAMERAS | aiComponent_LIGHTS); // Remove cameras and lights
	importer.SetPropertyInteger(AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE, smoothAngle);

	// Read the 3D model file
	const aiScene* scene = importer.ReadFile(m_filePath, ppsteps);
	if (!scene) // Someting went wrong. Print it.
	{
		LOG_ERROR("Failed to load \"" + FileSystem::GetFileNameNoExtensionFromPath(m_filePath) + "\". " + importer.GetErrorString());
		return false;
	}

	// Create all the appropriate directories
	FileSystem::CreateDirectory_("Standard Assets/Models/");
	FileSystem::CreateDirectory_("Standard Assets/Models/" + FileSystem::GetFileNameNoExtensionFromPath(m_modelName));
	FileSystem::CreateDirectory_("Standard Assets/Models/" + FileSystem::GetFileNameNoExtensionFromPath(m_modelName) + "/Meshes/");
	FileSystem::CreateDirectory_("Standard Assets/Models/" + FileSystem::GetFileNameNoExtensionFromPath(m_modelName) + "/Materials/");
	FileSystem::CreateDirectory_("Standard Assets/Models/" + FileSystem::GetFileNameNoExtensionFromPath(m_modelName) + "/Textures/");

	// Copy the source model file to an appropriate directory
	string modelDestination = "Standard Assets/Models/" + FileSystem::GetFileNameNoExtensionFromPath(m_modelName) + "/" + FileSystem::GetFileNameFromPath(m_modelName);
	FileSystem::CopyFileFromTo(m_filePath, modelDestination);

	// Copy any material files (used be obj models)
	auto files = FileSystem::GetFilesInDirectory(FileSystem::GetPathWithoutFileName(m_filePath));
	for (const auto& file : files)
		if (FileSystem::GetExtensionFromPath(file) == ".mtl")
			FileSystem::CopyFileFromTo(m_filePath, "Standard Assets/Models/" + FileSystem::GetFileNameNoExtensionFromPath(m_modelName) + "/" + FileSystem::GetFileNameFromPath(file));

	// This function will recursively process the entire model
	ProcessNode(scene, scene->mRootNode, nullptr, nullptr);

	// Normalize the scale of the model
	g_context->GetSubsystem<ResourceCache>()->NormalizeModelScale(m_rootGameObject);

	m_isLoading = false;
	return true;
}

//= HELPER FUNCTIONS ========================================================================
Matrix aiMatrix4x4ToMatrix(const aiMatrix4x4& transform)
{
	return Matrix(
		transform.a1, transform.b1, transform.c1, transform.d1,
		transform.a2, transform.b2, transform.c2, transform.d2,
		transform.a3, transform.b3, transform.c3, transform.d3,
		transform.a4, transform.b4, transform.c4, transform.d4
	);
}

void SetGameObjectTransform(GameObject* gameObject, aiNode* node)
{
	aiMatrix4x4 mAssimp = node->mTransformation;
	Vector3 position;
	Quaternion rotation;
	Vector3 scale;

	// Decompose the transformation matrix
	Matrix mEngine = aiMatrix4x4ToMatrix(mAssimp);
	mEngine.Decompose(scale, rotation, position);

	// Apply position, rotation and scale
	gameObject->GetTransform()->SetPositionLocal(position);
	gameObject->GetTransform()->SetRotationLocal(rotation);
	gameObject->GetTransform()->SetScaleLocal(scale);
}

Vector4 ToVector4(const aiColor4D& aiColor)
{
	return Vector4(aiColor.r, aiColor.g, aiColor.b, aiColor.a);
}

Vector3 ToVector3(const aiVector3D& aiVector)
{
	return Vector3(aiVector.x, aiVector.y, aiVector.z);
}

Vector2 ToVector2(const aiVector2D& aiVector)
{
	return Vector2(aiVector.x, aiVector.y);
}
//============================================================================================

//= PROCESSING ===============================================================================
void ModelImporter::ProcessNode(const aiScene* scene, aiNode* assimpNode, GameObject* parentNode, GameObject* newNode)
{
	if (!newNode)
		newNode = g_context->GetSubsystem<Scene>()->CreateGameObject();;

	if (!assimpNode->mParent)
		m_rootGameObject = newNode;

	//= GET NODE NAME ===========================================================
	// Note: In case this is the root node, aiNode.mName will be "RootNode". 
	// To get a more descriptive name we instead get the name from the file path.
	if (assimpNode->mParent)
		newNode->SetName(assimpNode->mName.C_Str());
	else
		newNode->SetName(FileSystem::GetFileNameNoExtensionFromPath(m_filePath));
	//===========================================================================

	// Set the transform of parentNode as the parent of the newNode's transform
	Transform* parentTrans = parentNode ? parentNode->GetTransform() : nullptr;
	newNode->GetTransform()->SetParent(parentTrans);

	// Set the transformation matrix of the assimp node to the new node
	SetGameObjectTransform(newNode, assimpNode);

	// Process all the node's meshes
	for (unsigned int i = 0; i < assimpNode->mNumMeshes; i++)
	{
		GameObject* gameobject = newNode; // set the current gameobject
		aiMesh* mesh = scene->mMeshes[assimpNode->mMeshes[i]]; // get mesh
		string name = assimpNode->mName.C_Str(); // get name

		// if this node has many meshes, then assign a new gameobject for each one of them
		if (assimpNode->mNumMeshes > 1)
		{
			gameobject = g_context->GetSubsystem<Scene>()->CreateGameObject(); // create
			gameobject->GetTransform()->SetParent(newNode->GetTransform()); // set parent
			name += "_" + to_string(i + 1); // set name
		}

		// Set gameobject's name
		gameobject->SetName(name);

		// Process mesh
		ProcessMesh(mesh, scene, gameobject);
	}

	// Process children
	for (unsigned int i = 0; i < assimpNode->mNumChildren; i++)
	{
		GameObject* child = g_context->GetSubsystem<Scene>()->CreateGameObject();
		ProcessNode(scene, assimpNode->mChildren[i], newNode, child);
	}
}

void ModelImporter::ProcessMesh(aiMesh* mesh, const aiScene* scene, GameObject* gameobject)
{
	vector<VertexPositionTextureNormalTangent> vertices;
	vector<unsigned int> indices;

	VertexPositionTextureNormalTangent vertex;
	for (unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices; vertexIndex++)
	{
		// get the position
		vertex.position = ToVector3(mesh->mVertices[vertexIndex]);

		// get the normal
		if (NULL != mesh->mNormals)
			vertex.normal = ToVector3(mesh->mNormals[vertexIndex]);

		// get the tangent
		if (NULL != mesh->mTangents)
			vertex.tangent = ToVector3(mesh->mTangents[vertexIndex]);

		// get the texture coordinates
		if (mesh->HasTextureCoords(0))
			vertex.uv = ToVector2(aiVector2D(mesh->mTextureCoords[0][vertexIndex].x, mesh->mTextureCoords[0][vertexIndex].y));

		// save the vertex
		vertices.push_back(vertex);

		// reset the vertex for use in the next loop
		vertex.normal = Vector3::Zero;
		vertex.tangent = Vector3::Zero;
		vertex.uv = Vector2::Zero;
	}

	// get the indices by iterating through each face of the mesh.
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		if (face.mNumIndices < 3)
			continue;

		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	// Add a mesh component and pass the data
	MeshFilter* meshComp = gameobject->AddComponent<MeshFilter>();
	meshComp->CreateAndSet(mesh->mName.C_Str(), m_rootGameObject->GetID(), vertices, indices);

	// No need to save the mesh as a file here, when the model importer performs a scale normalization on the entire model
	// this will cause the mesh to update and save itself, thus I only pass the directory to do so.
	if (meshComp->HasMesh()) meshComp->GetMesh().lock()->SetDirectory("Standard Assets/Models/" + FileSystem::GetFileNameNoExtensionFromPath(m_modelName) + "/Meshes/");

	// process materials
	if (scene->HasMaterials())
	{
		// Get assimp material
		aiMaterial* assimpMaterial = scene->mMaterials[mesh->mMaterialIndex];

		// Convert AiMaterial to Material and add it to the pool
		auto material = g_context->GetSubsystem<ResourceCache>()->AddResource(GenerateMaterialFromAiMaterial(assimpMaterial));

		// Set it in the mesh renderer component
		gameobject->AddComponent<MeshRenderer>()->SetMaterial(material);

		// Save the material in our custom format
		if (!material.expired())
			material.lock()->Save("Standard Assets/Models/" + FileSystem::GetFileNameNoExtensionFromPath(m_modelName) + "/Materials/" + material.lock()->GetName(), false);
	}

	// free memory
	vertices.clear();
	indices.clear();
}

shared_ptr<Material> ModelImporter::GenerateMaterialFromAiMaterial(aiMaterial* material)
{
	shared_ptr<Material> engineMaterial = make_shared<Material>(g_context);

	//= NAME ====================================================================
	aiString name;
	aiGetMaterialString(material, AI_MATKEY_NAME, &name);
	engineMaterial->SetName(name.C_Str());
	engineMaterial->SetModelID(m_modelName);

	//= CullMode ===============================================================================================
	// Specifies whether meshes using this material must be rendered without backface CullMode. 0 for false, !0 for true.
	bool isTwoSided = false;
	int r = material->Get(AI_MATKEY_TWOSIDED, isTwoSided);
	if (r == aiReturn_SUCCESS && isTwoSided)
	{
		LOG_INFO("two-sided");
		engineMaterial->SetFaceCullMode(CullNone);
	}

	//= DIFFUSE COLOR ======================================================================================
	aiColor4D colorDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
	aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &colorDiffuse);
	engineMaterial->SetColorAlbedo(ToVector4(colorDiffuse));

	//= OPACITY ==============================================
	aiColor4D opacity(1.0f, 1.0f, 1.0f, 1.0f);
	aiGetMaterialColor(material, AI_MATKEY_OPACITY, &opacity);
	engineMaterial->SetOpacity(opacity.r);

	// FIX: materials that have a diffuse texture should not be tinted black
	if (engineMaterial->GetColorAlbedo() == Vector4(0.0f, 0.0f, 0.0f, 1.0f))
		engineMaterial->SetColorAlbedo(Vector4(1.0f, 1.0f, 1.0f, 1.0f));

	//= ALBEDO TEXTURE ======================================================================================================
	aiString texturePath;
	if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
			AddTextureToMaterial(engineMaterial, Albedo, texturePath.data);

	//= SPECULAR (used as ROUGHNESS) TEXTURE =================================================================================
	if (material->GetTextureCount(aiTextureType_SHININESS) > 0)
		if (material->GetTexture(aiTextureType_SHININESS, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
			AddTextureToMaterial(engineMaterial, Roughness, texturePath.data);

	//= AMBIENT (used as METALLIC) TEXTURE ===================================================================================
	if (material->GetTextureCount(aiTextureType_AMBIENT) > 0)
		if (material->GetTexture(aiTextureType_AMBIENT, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
			AddTextureToMaterial(engineMaterial, Metallic, texturePath.data);

	//= NORMAL TEXTURE ======================================================================================================
	if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
		if (material->GetTexture(aiTextureType_NORMALS, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
			AddTextureToMaterial(engineMaterial, Normal, texturePath.data);

	//= OCCLUSION TEXTURE ====================================================================================================
	if (material->GetTextureCount(aiTextureType_LIGHTMAP) > 0)
		if (material->GetTexture(aiTextureType_LIGHTMAP, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
			AddTextureToMaterial(engineMaterial, Occlusion, texturePath.data);

	//= EMISSIVE TEXTURE ====================================================================================================
	if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0)
		if (material->GetTexture(aiTextureType_EMISSIVE, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
			AddTextureToMaterial(engineMaterial, Emission, texturePath.data);

	//= HEIGHT TEXTURE =====================================================================================================
	if (material->GetTextureCount(aiTextureType_HEIGHT) > 0)
		if (material->GetTexture(aiTextureType_HEIGHT, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
			AddTextureToMaterial(engineMaterial, Height, texturePath.data);

	//= MASK TEXTURE ========================================================================================================
	if (material->GetTextureCount(aiTextureType_OPACITY) > 0)
		if (material->GetTexture(aiTextureType_OPACITY, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
			AddTextureToMaterial(engineMaterial, Mask, texturePath.data);

	return engineMaterial;
}
//============================================================================================

//= HELPER FUNCTIONS =========================================================================
void ModelImporter::AddTextureToMaterial(weak_ptr<Material> material, TextureType textureType, const string& texturePath)
{
	string textureSource = FindTexture(texturePath);
	if (textureSource == DATA_NOT_ASSIGNED)
	{
		LOG_WARNING("Failed to find \"" + texturePath + "\".");
		return;
	}

	// Copy the source texture to an appropriate directory
	string textureDestination = "Standard Assets/Models/" + FileSystem::GetFileNameNoExtensionFromPath(m_modelName) + "/Textures/" + FileSystem::GetFileNameFromPath(textureSource);
	FileSystem::CopyFileFromTo(textureSource, textureDestination);

	// Set the texture to the material
	material.lock()->SetTexture(textureDestination, textureType);
}

string ModelImporter::FindTexture(string texturePath)
{
	// The texture path is relative to the model, something like "Textures\Alan_Wake_Jacket.jpg" which is too 
	// arbitrary to load a texture from it. This is why we get the model's directory (which is relative to the engine)...
	string modelRootDirectory = FileSystem::GetPathWithoutFileName(m_filePath);

	// ... and marge it with the texture path, Assets\Models\Alan_Wake\" + "Textures\Alan_Wake_Jacket.jpg".
	texturePath = modelRootDirectory + texturePath;

	// 1. Check if the texture path is valid
	if (FileSystem::FileExists(texturePath))
		return texturePath;

	// 2. Check the same texture path as previously but 
	// this time with different file extensions (jpg, png and so on).
	texturePath = TryPathWithMultipleExtensions(texturePath);
	if (FileSystem::FileExists(texturePath))
		return texturePath;

	// At this point we know the provided path is wrong, we will make a few guesses.
	// The most common mistake is that the artist provided a path which is absolute to his computer.

	// 3. Check if the texture is in the same folder as the model
	texturePath = FileSystem::GetFileNameFromPath(texturePath);
	if (FileSystem::FileExists(texturePath))
		return texturePath;

	// 4. Check the same texture path as previously but 
	// this time with different file extensions (jpg, png and so on).
	texturePath = TryPathWithMultipleExtensions(texturePath);
	if (FileSystem::FileExists(texturePath))
		return texturePath;

	// Give up, no valid texture path was found
	return DATA_NOT_ASSIGNED;
}

string ModelImporter::TryPathWithMultipleExtensions(const string& fullpath)
{
	// Remove extension
	int lastindex = fullpath.find_last_of(".");
	string fileName = fullpath.substr(0, lastindex);

	// create path for a couple of different extensions
	vector<string> supportedImageExtensions = FileSystem::GetSupportedImageFileFormats(true);

	for (auto i = 0; i < supportedImageExtensions.size(); i++)
		if (FileSystem::FileExists(fileName + supportedImageExtensions[i]))
			return fileName + supportedImageExtensions[i];

	return fullpath;
}
