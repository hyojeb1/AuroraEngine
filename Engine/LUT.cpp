///bof LUT.cpp
#include "stdafx.h"
#include "LUT.h"

using namespace std;


void LUT::MakeVolumeData(std::vector<uint8_t>& tex2d)
{
	if (tex2d.size() != TOTAL_SIZE * 4) return;

	for (int z = 0; z < LUT_SIZE; ++z) //blue
	{
		for (int y = 0; y < LUT_SIZE; ++y) //green
		{
			for (int x = 0; x < LUT_SIZE; ++x) // red
			{
				int src_x = (z * LUT_SIZE) + x;
				int src_y = y;

				int src_width = LUT_SIZE * LUT_SIZE;
				int src_index = (src_x * src_width) + src_y;

				uint8_t r = tex2d[src_index * 4 + 0];
				uint8_t g = tex2d[src_index * 4 + 1];
				uint8_t b = tex2d[src_index * 4 + 2];
				uint8_t a = tex2d[src_index * 4 + 3];

				uint32_t pixel_color = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | ((uint32_t)r);

				int dest_index = (z * LUT_SIZE * LUT_SIZE) + (y * LUT_SIZE) + x;
				volume_data_[dest_index] = pixel_color;
			}
		}
	}

}

void LUT::CreateTexture3D(ID3D11Device* device)
{
	D3D11_TEXTURE3D_DESC desc = {};
	desc.Width = LUT_SIZE;
	desc.Height = LUT_SIZE;
	desc.Depth = LUT_SIZE;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // NO! SRGB 
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

}


///eof LUT.cpp