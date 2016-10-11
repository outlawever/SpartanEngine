/*
Copyright(c) 2016 Panos Karabelas

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

//= INCLUDES ==========
#include "ShaderPool.h"
#include "../IO/Log.h"
//=====================

//= NAMESPACES =====
using namespace std;
//==================

ShaderPool::ShaderPool(Graphics* graphicsDevice)
{
	m_graphics = graphicsDevice;
}

ShaderPool::~ShaderPool()
{
	Clear();
}

ShaderVariation* ShaderPool::CreateShaderBasedOnMaterial(
	bool albedo,
	bool roughness,
	bool metallic,
	bool normal,
	bool height,
	bool occlusion,
	bool emission,
	bool mask,
	bool cubemap
)
{
	// If an appropriate shader already exists, return it's ID
	ShaderVariation* existingShader = FindMatchingShader(
		albedo,
		roughness,
		metallic,
		normal,
		height,
		occlusion,
		emission,
		mask,
		cubemap
	);

	if (existingShader)
		return existingShader;

	// If not, create a new one
	ShaderVariation* shader = new ShaderVariation();
	shader->Initialize(
		albedo,
		roughness,
		metallic,
		normal,
		height,
		occlusion,
		emission,
		mask,
		cubemap,
		m_graphics
	);

	// Add the shader to the pool and return it
	m_shaders.push_back(shader);
	return m_shaders.back();
}

ShaderVariation* ShaderPool::GetShaderByID(const string& shaderID)
{
	for (ShaderVariation* shader : m_shaders)
		if (shader->GetID() == shaderID)
			return shader;

	return nullptr;
}

const vector<ShaderVariation*>& ShaderPool::GetAllShaders() const
{
	return m_shaders;
}

ShaderVariation* ShaderPool::FindMatchingShader(
	bool albedo,
	bool roughness,
	bool metallic,
	bool normal,
	bool height,
	bool occlusion,
	bool emission,
	bool mask,
	bool cubemap
)
{
	for (ShaderVariation* shader : m_shaders)
	{
		if (shader->HasAlbedoTexture() != albedo) continue;
		if (shader->HasRoughnessTexture() != roughness) continue;
		if (shader->HasMetallicTexture() != metallic) continue;
		if (shader->HasNormalTexture() != normal) continue;
		if (shader->HasHeightTexture() != height) continue;
		if (shader->HasOcclusionTexture() != occlusion) continue;
		if (shader->HasEmissionTexture() != emission) continue;	
		if (shader->HasMaskTexture() != mask) continue;
		if (shader->HasCubeMapTexture() != cubemap) continue;

		return shader;
	}

	return nullptr;
}

void ShaderPool::Clear()
{
	for (ShaderVariation* shader : m_shaders)
		delete shader;

	m_shaders.clear();
	m_shaders.shrink_to_fit();
}
