///bof LUT.h
#pragma once

class LUT
{
public:
//private:
	//get set은 나중에, 지금은 퍼블릭
	static constexpr size_t LUT_SIZE = 16;
	static constexpr size_t TOTAL_SIZE = LUT_SIZE * LUT_SIZE * LUT_SIZE;
	std::array<uint32_t, TOTAL_SIZE> volume_data_;
	com_ptr<ID3D11ShaderResourceView> texture3d_;

public:
	void MakeVolumeData(std::vector<uint8_t>& tex2d);
	void CreateTexture3D(ID3D11Device* device);
};
///eof LUT.h