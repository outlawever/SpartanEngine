/*
Copyright(c) 2016-2019 Panos Karabelas

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

//= INCLUDES ======================
#include "../RHI_BlendState.h"
#include "../RHI_Device.h"
#include "../RHI_Implementation.h"
#include "../../Logging/Log.h"
#include "D3D11_Common.h"
//=================================

//= NAMESPACES =====
using namespace std;
//==================

namespace Directus
{
	RHI_BlendState::RHI_BlendState(shared_ptr<RHI_Device> device, bool blendEnabled)
	{
		// Save properties
		m_blendEnabled = blendEnabled;

		// Create description
		D3D11_BLEND_DESC desc;
		desc.AlphaToCoverageEnable	= false;
		desc.IndependentBlendEnable = blendEnabled;
		for (UINT i = 0; i < 8; ++i)
		{
			desc.RenderTarget[i].BlendEnable			= blendEnabled;
			desc.RenderTarget[i].BlendOp				= D3D11_BLEND_OP_ADD;
			desc.RenderTarget[i].BlendOpAlpha			= D3D11_BLEND_OP_ADD;
			desc.RenderTarget[i].DestBlend				= D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[i].DestBlendAlpha			= D3D11_BLEND_ONE;
			desc.RenderTarget[i].RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[i].SrcBlend				= D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[i].SrcBlendAlpha			= D3D11_BLEND_ONE;
		}
		desc.RenderTarget[0].BlendEnable = blendEnabled;

		// Create blend state
		auto blendState	= (ID3D11BlendState*)m_buffer;
		auto result		= device->GetDevice<ID3D11Device>()->CreateBlendState(&desc, &blendState);

		// Handle result
		if (SUCCEEDED(result))
		{
			m_buffer		= (void*)blendState;
			m_initialized	= true;	
		}
		else
		{
			m_initialized = false;
			LOGF_ERROR("Failed to create blend state %s.", D3D11_Common::DxgiErrorToString(result));
		}
	}

	RHI_BlendState::~RHI_BlendState()
	{
		SafeRelease((ID3D11BlendState*)m_buffer);
	}
}