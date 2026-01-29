#include "stdafx.h"
#include "ResourceManager.h"
#include "LUT.h"
using namespace std;
using namespace DirectX;

XMFLOAT4X4 ResourceManager::ToXMFLOAT4X4(const aiMatrix4x4& matrix)
{
	return XMFLOAT4X4(
		matrix.a1, matrix.b1, matrix.c1, matrix.d1,
		matrix.a2, matrix.b2, matrix.c2, matrix.d2,
		matrix.a3, matrix.b3, matrix.c3, matrix.d3,
		matrix.a4, matrix.b4, matrix.c4, matrix.d4
	);
}

XMFLOAT3 ResourceManager::ToXMFLOAT3(const aiVector3D& vec3)
{
	return XMFLOAT3(vec3.x, vec3.y, vec3.z);
}

XMFLOAT4 ResourceManager::ToXMFLOAT4(const aiQuaternion& quar)
{
	return XMFLOAT4(quar.x, quar.y, quar.z, quar.w);
}

bool ResourceManager::SceneHasBones(const aiScene* scene)
{
	if (!scene) return false;
	for (UINT i = 0; i < scene->mNumMeshes; ++i)
	{
		if (scene->mMeshes[i]->HasBones()) return true;
	}
	return false;
}


void ResourceManager::Initialize(com_ptr<ID3D11Device> device, com_ptr<ID3D11DeviceContext> deviceContext)
{
	m_device = device;
	m_deviceContext = deviceContext;
	m_spriteBatch = make_unique<SpriteBatch>(m_deviceContext.Get());

	CreateDepthStencilStates();
	CreateBlendStates();
	CreateRasterStates();

	CreateConstantBuffers();
	SetAllConstantBuffers();
	CreateSamplerStates();
	SetAllSamplerStates();

	CacheAllTexture();
	LUT& lut = LUT::GetInstance();
}

void ResourceManager::SetDepthStencilState(DepthStencilState state)
{
	if (m_currentDepthStencilState == state) return;

	m_deviceContext->OMSetDepthStencilState(m_depthStencilStates[static_cast<size_t>(state)].Get(), 0);
	m_currentDepthStencilState = state;
}

void ResourceManager::SetBlendState(BlendState state)
{
	if (m_currentBlendState == state) return;

	constexpr array<FLOAT, 4> blendFactor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 나중에 따로 받도록 수정?
	m_deviceContext->OMSetBlendState(m_blendStates[static_cast<size_t>(state)].Get(), blendFactor.data(), 0xFFFFFFFF);
	m_currentBlendState = state;
}

void ResourceManager::SetRasterState(RasterState state)
{
	if (m_currentRasterState == state) return;

	m_deviceContext->RSSetState(m_rasterStates[static_cast<size_t>(state)].Get());
	m_currentRasterState = state;
}

void ResourceManager::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
{
	if (m_currentTopology == topology) return;

	m_deviceContext->IASetPrimitiveTopology(topology);
	m_currentTopology = topology;
}

void ResourceManager::SetAllConstantBuffers()
{
	// 정점 셰이더용 상수 버퍼 설정
	// 뷰-투영 상수 버퍼
	m_deviceContext->VSSetConstantBuffers(static_cast<UINT>(VSConstBuffers::ViewProjection), 1, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::ViewProjection)].GetAddressOf());
	// 스카이박스 뷰-투영 역행렬 상수 버퍼
	m_deviceContext->VSSetConstantBuffers(static_cast<UINT>(VSConstBuffers::SkyboxViewProjection), 1, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::SkyboxViewProjection)].GetAddressOf());
	// 객체 월드, 스케일 역행렬 적용한 월드 상수 버퍼
	m_deviceContext->VSSetConstantBuffers(static_cast<UINT>(VSConstBuffers::WorldNormal), 1, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::WorldNormal)].GetAddressOf());

	// 애니메이션 관련 상수 버퍼 설정
	// 시간 상수 버퍼
	m_deviceContext->VSSetConstantBuffers(static_cast<UINT>(VSConstBuffers::Time), 1, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::Time)].GetAddressOf());
	// 뼈 버퍼
	m_deviceContext->VSSetConstantBuffers(static_cast<UINT>(VSConstBuffers::Bone), 1, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::Bone)].GetAddressOf());

	//선 그리기용 상수 버퍼
	m_deviceContext->VSSetConstantBuffers(static_cast<UINT>(VSConstBuffers::Line), 1, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::Line)].GetAddressOf());
	// 파티클 상수 버퍼
	m_deviceContext->VSSetConstantBuffers(static_cast<UINT>(VSConstBuffers::Particle), 1, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::Particle)].GetAddressOf());


	// 픽셀 셰이더용 상수 버퍼 설정
	// 후처리 상수 버퍼
	m_deviceContext->PSSetConstantBuffers(static_cast<UINT>(PSConstBuffers::PostProcessing), 1, m_psConstantBuffers[static_cast<size_t>(PSConstBuffers::PostProcessing)].GetAddressOf());
	// 카메라 위치 상수 버퍼
	m_deviceContext->PSSetConstantBuffers(static_cast<UINT>(PSConstBuffers::CameraPosition), 1, m_psConstantBuffers[static_cast<size_t>(PSConstBuffers::CameraPosition)].GetAddressOf());
	// 방향광 상수 버퍼
	m_deviceContext->PSSetConstantBuffers(static_cast<UINT>(PSConstBuffers::GlobalLight), 1, m_psConstantBuffers[static_cast<size_t>(PSConstBuffers::GlobalLight)].GetAddressOf());
	// 재질 팩터 상수 버퍼
	m_deviceContext->PSSetConstantBuffers(static_cast<UINT>(PSConstBuffers::MaterialFactor), 1, m_psConstantBuffers[static_cast<size_t>(PSConstBuffers::MaterialFactor)].GetAddressOf());

}

void ResourceManager::SetAllSamplerStates()
{
	for (size_t i = 0; i < static_cast<size_t>(SamplerState::Count); ++i) m_deviceContext->PSSetSamplers(static_cast<UINT>(i), 1, m_samplerStates[i].GetAddressOf());
}

com_ptr<ID3D11Buffer> ResourceManager::CreateVertexBuffer(const void* data, UINT stride, UINT count, bool isDynamic)
{
	HRESULT hr = S_OK;
	com_ptr<ID3D11Buffer> vertexBuffer = nullptr;

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = stride * count;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	if (isDynamic)
	{
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC; 
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		bufferDesc.Usage = D3D11_USAGE_DEFAULT; 
		bufferDesc.CPUAccessFlags = 0;
	}

	if (data != nullptr)
	{
		D3D11_SUBRESOURCE_DATA initialData = {};
		initialData.pSysMem = data;
		initialData.SysMemPitch = 0;
		initialData.SysMemSlicePitch = 0;

		hr = m_device->CreateBuffer(&bufferDesc, &initialData, vertexBuffer.GetAddressOf());
	}
	else
	{
		hr = m_device->CreateBuffer(&bufferDesc, nullptr, vertexBuffer.GetAddressOf());
	}

	CheckResult(hr, "범용 정점 버퍼 생성 실패.");

	return vertexBuffer;
}

pair<com_ptr<ID3D11VertexShader>, com_ptr<ID3D11InputLayout>> ResourceManager::GetVertexShaderAndInputLayout(const string& shaderName, const vector<InputElement>& inputElements)
{
	// 기존에 생성된 셰이더 및 입력 레이아웃이 있으면 재사용
	auto it = m_vertexShadersAndInputLayouts.find(shaderName);
	if (it != m_vertexShadersAndInputLayouts.end()) return it->second;

	HRESULT hr = S_OK;

	// 정점 셰이더 컴파일
	com_ptr<ID3DBlob> VSCode = CompileShader(shaderName, "vs_5_0");
	hr = m_device->CreateVertexShader
	(
		VSCode->GetBufferPointer(),
		VSCode->GetBufferSize(),
		nullptr,
		m_vertexShadersAndInputLayouts[shaderName].first.GetAddressOf()
	);
	CheckResult(hr, "정점 셰이더 생성 실패.");

	// 입력 레이아웃 생성
	if (!inputElements.empty())
	{
		vector<D3D11_INPUT_ELEMENT_DESC> inputElementDescs;
		for (const auto& element : inputElements) inputElementDescs.push_back(INPUT_ELEMENT_DESC_TEMPLATES[static_cast<size_t>(element)]);

		hr = m_device->CreateInputLayout
		(
			inputElementDescs.data(),
			static_cast<UINT>(inputElementDescs.size()),
			VSCode->GetBufferPointer(),
			VSCode->GetBufferSize(),
			m_vertexShadersAndInputLayouts[shaderName].second.GetAddressOf()
		);
		CheckResult(hr, "입력 레이아웃 생성 실패.");
	}

	return m_vertexShadersAndInputLayouts[shaderName];
}

com_ptr<ID3D11GeometryShader> ResourceManager::GetGeometryShader(const string& shaderName)
{
	// 기존에 생성된 지오메트리 셰이더가 있으면 재사용
	auto it = m_geometryShaders.find(shaderName);
	if (it != m_geometryShaders.end()) return it->second;

	HRESULT hr = S_OK;

	// 지오메트리 셰이더 컴파일
	com_ptr<ID3DBlob> GSCode = CompileShader(shaderName, "gs_5_0");
	hr = m_device->CreateGeometryShader
	(
		GSCode->GetBufferPointer(),
		GSCode->GetBufferSize(),
		nullptr,
		m_geometryShaders[shaderName].GetAddressOf()
	);
	CheckResult(hr, "지오메트리 셰이더 생성 실패.");

	return m_geometryShaders[shaderName];
}

com_ptr<ID3D11PixelShader> ResourceManager::GetPixelShader(const string& shaderName)
{
	// 기존에 생성된 픽셀 셰이더가 있으면 재사용
	auto it = m_pixelShaders.find(shaderName);
	if (it != m_pixelShaders.end()) return it->second;

	HRESULT hr = S_OK;

	// 픽셀 셰이더 컴파일
	com_ptr<ID3DBlob> PSCode = CompileShader(shaderName, "ps_5_0");
	hr = m_device->CreatePixelShader
	(
		PSCode->GetBufferPointer(),
		PSCode->GetBufferSize(),
		nullptr,
		m_pixelShaders[shaderName].GetAddressOf()
	);
	CheckResult(hr, "픽셀 셰이더 생성 실패.");

	return m_pixelShaders[shaderName];
}

com_ptr<ID3D11ShaderResourceView> ResourceManager::GetTexture(const string& fileName, TextureType type)
{
	// 기존에 생성된 텍스처가 있으면 재사용
	auto it = m_textures.find(fileName);
	if (it != m_textures.end()) return it->second;

	HRESULT hr = S_OK;

	const auto cacheIt = m_textureCaches.find(fileName);
	if (cacheIt == m_textureCaches.end())
	{
		cerr << "텍스처 캐시에서 파일을 찾을 수 없습니다: " << fileName << endl;

		switch (type)
		{
		case TextureType::BaseColor:
			return GetTexture("Fallback_BaseColor.png", TextureType::BaseColor);
			//return GetTexture("Fallback_BaseColor_Gray.png", TextureType::BaseColor);
		case TextureType::ORM:
			return GetTexture("Fallback_OcclusionRoughnessMetallic.png", TextureType::ORM);
		case TextureType::Normal:
			return GetTexture("Fallback_Normal.png", TextureType::Normal);
		case TextureType::Emissive:
			return GetTexture("Fallback_Emissive.png", TextureType::Emissive);
		default:
			return nullptr;
		}
	}

	// 파일 확장자 확인
	const string extension = fileName.substr(fileName.find_last_of('.') + 1);
	if (extension == "dds" || extension == "DDS")
	{
		// DDS 파일 (큐브맵 등)
		hr = CreateDDSTextureFromMemoryEx
		(
			m_device.Get(),
			m_deviceContext.Get(),
			cacheIt->second.data(),
			cacheIt->second.size(),
			0,
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE,
			0,
			0, // dds 는 mipmap 자동 생성 못함
			type == TextureType::BaseColor || type == TextureType::Emissive ? DDS_LOADER_FORCE_SRGB : DDS_LOADER_IGNORE_SRGB,
			nullptr,
			m_textures[fileName].GetAddressOf()
		);
		CheckResult(hr, "DDS 텍스처 생성 실패.");
	}
	else
	{
		// WIC 지원 이미지 (jpg, png 등)
		hr = CreateWICTextureFromMemoryEx
		(
			m_device.Get(),
			m_deviceContext.Get(),
			cacheIt->second.data(),
			cacheIt->second.size(),
			0,
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE,
			0,
			D3D11_RESOURCE_MISC_GENERATE_MIPS, // mipmap 자동 생성
			type == TextureType::BaseColor || type == TextureType::Emissive ? WIC_LOADER_FORCE_SRGB : WIC_LOADER_IGNORE_SRGB,
			nullptr,
			m_textures[fileName].GetAddressOf()
		);
		CheckResult(hr, "텍스처 생성 실패.");
	}

	return m_textures[fileName];
}

std::pair<com_ptr<ID3D11ShaderResourceView>, DirectX::XMFLOAT2> ResourceManager::GetTextureAndOffset(const std::string& fileName)
{
	com_ptr<ID3D11ShaderResourceView> textureSRV = GetTexture(fileName);
	XMFLOAT2 offset = {};

	// srv에서 크기 정보 얻기
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	textureSRV->GetDesc(&srvDesc);
	if (srvDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D)
	{
		D3D11_TEX2D_SRV tex2DSRV = srvDesc.Texture2D;
		com_ptr<ID3D11Resource> resource = nullptr;
		textureSRV->GetResource(resource.GetAddressOf());
		com_ptr<ID3D11Texture2D> texture2D = nullptr;
		resource.As(&texture2D);
		D3D11_TEXTURE2D_DESC textureDesc = {};
		texture2D->GetDesc(&textureDesc);
		offset.x = static_cast<float>(textureDesc.Width) * 0.5f;
		offset.y = static_cast<float>(textureDesc.Height) * 0.5f;
	}

	return { textureSRV, offset };
}

const Model* ResourceManager::LoadModel(const string& fileName)
{
	auto it = m_models.find(fileName);
	if (it != m_models.end()) return &it->second;

	Assimp::Importer importer;
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
	const string fullPath = "../Asset/Model/" + fileName;

	const aiScene* scene = importer.ReadFile
	(
		fullPath,
		aiProcess_CalcTangentSpace | // 접선 공간 계산
		aiProcess_JoinIdenticalVertices | // 동일한 정점 결합 // 메모리 절약 // 좀 위험함
		aiProcess_Triangulate | // 삼각형화
		aiProcess_GenSmoothNormals | // 부드러운 법선 생성 // 조금 느릴 수 있다고 하니까 유의
		aiProcess_SplitLargeMeshes | // 큰 메쉬 분할 // 드로우 콜 최대치를 넘는 메쉬 방지 // 이 옵션이 쓸일이 생기면 뭔가 크게 잘못된거임
		aiProcess_ValidateDataStructure | // 데이터 구조 검증 // 큰 문제가 아니여도 경고는 남김
		aiProcess_ImproveCacheLocality | // 정점 캐시 지역성 향상
		aiProcess_RemoveRedundantMaterials | // 사용되지 않는 재질 제거
		aiProcess_FixInfacingNormals | // 뒤집힌 법선(내부를 향한 법선) 수정 // 만약 의도한 것이라면 이 옵션을 빼야함
		aiProcess_PopulateArmatureData | // 본 정보 채우기 // 애니메이션이 있는 모델에 필요 // 사실 뭐하는건지 잘 모르겠음
		aiProcess_SortByPType | // 프리미티브 타입별로 메쉬 정렬 // 삼각형, 선, 점 등으로 나눔 // 삼각형만 필요하면 나머지는 무시 가능
		aiProcess_FindDegenerates | // 엄청 작은(사실상 안보이는) 삼각형 제거
		aiProcess_FindInvalidData | // 잘못된 데이터(노말 값 = 0 같은거) 찾기 및 수정
		aiProcess_GenUVCoords | // 비UV 맵핑(구면, 원통 등)을 UV 좌표 채널로 변환
		aiProcess_TransformUVCoords | // UV 좌표 변환 적용 // 뭐하는건지 모르겠음
		aiProcess_FindInstances | // 중복 메쉬 찾기
		aiProcess_OptimizeMeshes | // 메쉬 최적화
		aiProcess_OptimizeGraph | // 씬 그래프 최적화 // 애니메이션이나 본이 없는 노드 병합 // 좀 위험할 수 있으니 유의
		aiProcess_SplitByBoneCount | // 본 개수로 메쉬 분할 // 한 메쉬에 본이 너무 많으면 여러 메쉬로 나눔 // 뭐하는건지 모르겠음
		aiProcess_Debone | // 사용하지 않는 더미 본 제거
		aiProcess_DropNormals | // aiProcess_JoinIdenticalVertices 와 같이 사용 // 정점 노말 제거
		aiProcess_GenBoundingBoxes | // 바운딩 박스 생성
		aiProcess_LimitBoneWeights |
		aiProcess_GlobalScale |
		aiProcess_ConvertToLeftHanded // DirectX 좌표계(왼손 좌표계)로 변환
	);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		cerr << "모델 로드 실패: " << importer.GetErrorString() << endl;
		exit(EXIT_FAILURE);
	}

	Model& model = m_models[fileName];

	// 1. 메쉬 및 노드 처리 (이 과정에서 본 정보가 있다면 Skeleton에 등록됨)
	ProcessNode(scene->mRootNode, scene, model);

	// 2. 모델 텍스처 로드 (Assimp 재질 정보 기반)
	const string fileNameWithoutExtension = filesystem::path(fileName).stem().string();

	const aiMaterial* material = (scene->HasMaterials() && scene->HasMeshes())
		? scene->mMaterials[scene->mMeshes[0]->mMaterialIndex]
		: nullptr;

	model.materialTexture.baseColorTextureSRV = LoadTextureHybrid(material, fileNameWithoutExtension, aiTextureType_DIFFUSE, "_BaseColor.png", TextureType::BaseColor);
	model.materialTexture.normalTextureSRV = LoadTextureHybrid(material, fileNameWithoutExtension, aiTextureType_NORMALS, "_Normal.png", TextureType::Normal);
	model.materialTexture.emissionTextureSRV = LoadTextureHybrid(material, fileNameWithoutExtension, aiTextureType_EMISSIVE, "_Emissive.png", TextureType::Emissive);
	model.materialTexture.ORMTextureSRV = LoadTextureHybrid(material, fileNameWithoutExtension, aiTextureType_METALNESS, "_OcclusionRoughnessMetallic.png", TextureType::ORM);

	// 3. 본 정보가 있다면 스켈레톤 구축
	if (SceneHasBones(scene))
	{
		aiMatrix4x4 inverse_root_transform = scene->mRootNode->mTransformation;
		inverse_root_transform.Inverse();
		model.skeleton.globalInverseTransform = ToXMFLOAT4X4(inverse_root_transform);
		model.skeleton.root = BuildSkeletonNode(scene->mRootNode, model.skeleton);
		model.type = ModelType::Skinned; // 필요하다면 타입 마킹
	}

	// 4. 애니메이션 로드
	if (scene->HasAnimations()) LoadAnimations(scene, model);

	return &m_models[fileName];
}

SpriteFont* ResourceManager::GetSpriteFont(const wstring& fontName)
{
	// 기존에 생성된 스프라이트 폰트가 있으면 재사용
	auto it = m_spriteFonts.find(fontName);
	if (it != m_spriteFonts.end()) return it->second.get();

	// 새로 생성
	m_spriteFonts[fontName] = make_unique<SpriteFont>(m_device.Get(), (L"../Asset/Font/" + fontName + L".spritefont").c_str());

	return m_spriteFonts[fontName].get();
}

/// <summary>
/// "SceneName/ImageName.png" 형태의 키 만들려고 만든 private 함수
/// </summary>
/// <param name="rawPath"></param>
/// <returns></returns>
string ResourceManager::FindTextureFromCache(const string& rawPath)
{
	if (rawPath.empty()) return "";

	// 1. 경로에서 파일명만 추출 (상위 디렉토리 제거)
	// 결과: "Wall_BaseColor.png"
	filesystem::path p(rawPath);
	string filename = p.filename().string();

	// 2. 캐시 맵에서 파일명이 일치하는지 검색
	for (const auto& pair : m_textureCaches)
	{
		filesystem::path cachePath(pair.first);
		if (cachePath.filename().string() == filename)
		{
			return pair.first; 
		}
	}

	return "";
}

com_ptr<ID3D11ShaderResourceView> ResourceManager::LoadTextureHybrid(const aiMaterial* material, const std::string& model_name, aiTextureType aiType, const std::string& suffix, TextureType engine_type)
{
	// PRIORITY 1: Assimp 메타데이터 기반 로드
	if (material)
	{
		aiString texPath;
		// 1-1. 해당 슬롯에 텍스처 경로가 있는지 확인
		if (material->GetTexture(aiType, 0, &texPath) == AI_SUCCESS)
		{
			// 경로가 있다면 캐시에서 검색
			string cacheKey = FindTextureFromCache(texPath.C_Str());
			if (!cacheKey.empty())
			{
				return GetTexture(cacheKey, engine_type);
			}
		}

		// 1-2. ORM 텍스처 특별 처리 (FBX는 표준 슬롯이 없어 UNKNOWN에 들어가는 경우가 많음)
		if (engine_type == TextureType::ORM)
		{
			if (material->GetTexture(aiTextureType_UNKNOWN, 0, &texPath) == AI_SUCCESS)
			{
				string cacheKey = FindTextureFromCache(texPath.C_Str());
				if (!cacheKey.empty())
				{
					return GetTexture(cacheKey, engine_type);
				}
			}
		}
	}

	// PRIORITY 2: 기존 이름 규칙 기반 로드 (model_name + Suffix)
	string oldStyleName = model_name + suffix;

	// 캐시에 해당 파일이 실제로 존재하는지 확인
	if (m_textureCaches.find(oldStyleName) != m_textureCaches.end())
	{
		return GetTexture(oldStyleName, engine_type);
	}

	// PRIORITY 3: 모두 실패 -> GetTexture가 알아서 Fallback 반환
	return GetTexture(oldStyleName, engine_type);
}

void ResourceManager::CreateDepthStencilStates()
{
	HRESULT hr = S_OK;
	for (size_t i = 0; i < static_cast<size_t>(DepthStencilState::Count); ++i)
	{
		hr = m_device->CreateDepthStencilState(&DEPTH_STENCIL_DESC_TEMPLATES[i], m_depthStencilStates[i].GetAddressOf());
		CheckResult(hr, "깊이버퍼 상태 생성 실패.");
	}
}

void ResourceManager::CreateBlendStates()
{
	HRESULT hr = S_OK;
	for (size_t i = 0; i < static_cast<size_t>(BlendState::Count); ++i)
	{
		hr = m_device->CreateBlendState(&BLEND_DESC_TEMPLATES[i], m_blendStates[i].GetAddressOf());
		CheckResult(hr, "블렌드 상태 생성 실패.");
	}
}

void ResourceManager::CreateRasterStates()
{
	HRESULT hr = S_OK;

	for (size_t i = 0; i < static_cast<size_t>(RasterState::Count); ++i)
	{
		hr = m_device->CreateRasterizerState(&RASTERIZER_DESC_TEMPLATES[i], m_rasterStates[i].GetAddressOf());
		CheckResult(hr, "래스터 상태 생성 실패.");
	}
}

void ResourceManager::CreateConstantBuffers()
{
	HRESULT hr = S_OK;

	// 정점 셰이더용 상수 버퍼 생성
	// 뷰-투영 상수 버퍼
	hr = m_device->CreateBuffer(&VS_CONST_BUFFER_DESCS[static_cast<size_t>(VSConstBuffers::ViewProjection)], nullptr, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::ViewProjection)].GetAddressOf());
	CheckResult(hr, "ViewProjection 상수 버퍼 생성 실패.");
	// 스카이박스 뷰-투영 역행렬 상수 버퍼
	hr = m_device->CreateBuffer(&VS_CONST_BUFFER_DESCS[static_cast<size_t>(VSConstBuffers::SkyboxViewProjection)], nullptr, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::SkyboxViewProjection)].GetAddressOf());
	CheckResult(hr, "Object 상수 버퍼 생성 실패.");
	// 객체 월드, 스케일 역행렬 적용한 월드 상수 버퍼
	hr = m_device->CreateBuffer(&VS_CONST_BUFFER_DESCS[static_cast<size_t>(VSConstBuffers::WorldNormal)], nullptr, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::WorldNormal)].GetAddressOf());
	CheckResult(hr, "WorldNormal 상수 버퍼 생성 실패.");

	// 애니메이션 관련 상수 버퍼 생성
	// 시간 상수 버퍼
	hr = m_device->CreateBuffer(&VS_CONST_BUFFER_DESCS[static_cast<size_t>(VSConstBuffers::Time)], nullptr, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::Time)].GetAddressOf());
	CheckResult(hr, "Time 상수 버퍼 생성 실패.");
	// 뼈 버퍼
	hr = m_device->CreateBuffer(&VS_CONST_BUFFER_DESCS[static_cast<size_t>(VSConstBuffers::Bone)], nullptr,	m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::Bone)].GetAddressOf());
	CheckResult(hr, "Bone 상수 버퍼 생성 실패.");
	
	//선 그리기용 상수 버퍼
	hr = m_device->CreateBuffer(&VS_CONST_BUFFER_DESCS[static_cast<size_t>(VSConstBuffers::Line)], nullptr, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::Line)].GetAddressOf());
	CheckResult(hr, "LineColor 상수 버퍼 생성 실패.");
	
	//파티클 상수 버퍼
	hr = m_device->CreateBuffer(&VS_CONST_BUFFER_DESCS[static_cast<size_t>(VSConstBuffers::Particle)], nullptr, m_vsConstantBuffers[static_cast<size_t>(VSConstBuffers::Particle)].GetAddressOf());
	CheckResult(hr, "LineColor 상수 버퍼 생성 실패.");

	// 픽셀 셰이더용 상수 버퍼 생성
	// 후처리 상수 버퍼
	hr = m_device->CreateBuffer(&PS_CONST_BUFFER_DESCS[static_cast<size_t>(PSConstBuffers::PostProcessing)], nullptr, m_psConstantBuffers[static_cast<size_t>(PSConstBuffers::PostProcessing)].GetAddressOf());
	CheckResult(hr, "PostProcessing 상수 버퍼 생성 실패.");
	// 카메라 위치 상수 버퍼
	hr = m_device->CreateBuffer(&PS_CONST_BUFFER_DESCS[static_cast<size_t>(PSConstBuffers::CameraPosition)], nullptr, m_psConstantBuffers[static_cast<size_t>(PSConstBuffers::CameraPosition)].GetAddressOf());
	CheckResult(hr, "CameraPosition 상수 버퍼 생성 실패.");
	// 방향광 상수 버퍼
	hr = m_device->CreateBuffer(&PS_CONST_BUFFER_DESCS[static_cast<size_t>(PSConstBuffers::GlobalLight)], nullptr, m_psConstantBuffers[static_cast<size_t>(PSConstBuffers::GlobalLight)].GetAddressOf());
	CheckResult(hr, "DirectionalLight 상수 버퍼 생성 실패.");
	// 재질 팩터 상수 버퍼
	hr = m_device->CreateBuffer(&PS_CONST_BUFFER_DESCS[static_cast<size_t>(PSConstBuffers::MaterialFactor)], nullptr, m_psConstantBuffers[static_cast<size_t>(PSConstBuffers::MaterialFactor)].GetAddressOf());
	CheckResult(hr, "MaterialFactor 상수 버퍼 생성 실패.");
}

void ResourceManager::CreateSamplerStates()
{
	HRESULT hr = S_OK;

	for (size_t i = 0; i < static_cast<size_t>(SamplerState::Count); ++i)
	{
		hr = m_device->CreateSamplerState(&SAMPLER_DESC_TEMPLATES[i], m_samplerStates[i].GetAddressOf());
		CheckResult(hr, "샘플러 상태 생성 실패.");
	}
}

void ResourceManager::CacheAllTexture()
{
	const filesystem::path textureDirectory = "../Asset/Texture/";

	for (const auto& dirEntry : filesystem::recursive_directory_iterator(textureDirectory))
	{
		if (dirEntry.is_regular_file())
		{
			const string fileName = filesystem::relative(dirEntry.path(), textureDirectory).string();

			// 텍스처 파일 읽기
			ifstream fileStream(dirEntry.path(), ios::binary | ios::ate);

			if (fileStream)
			{
				const streamsize fileSize = fileStream.tellg();
				fileStream.seekg(0, ios::beg);
				vector<char> fileData(static_cast<size_t>(fileSize));

				if (fileStream.read(fileData.data(), fileSize)) m_textureCaches[fileName] = vector<uint8_t>(fileData.begin(), fileData.end());
				else cerr << "텍스처 파일 읽기 실패: " << fileName << endl;
			}
			else cerr << "텍스처 파일 열기 실패: " << fileName << endl;
		}
	}
}

void ResourceManager::ProcessNode(const aiNode* node, const aiScene* scene, Model& model)
{
	// 노드의 메쉬 처리
	for (UINT i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		model.meshes.push_back(ProcessMesh(mesh, scene, model));
	}

	// 자식 노드 재귀 처리
	for (UINT i = 0; i < node->mNumChildren; ++i) ProcessNode(node->mChildren[i], scene, model);
}

Mesh ResourceManager::ProcessMesh(const aiMesh* mesh, const aiScene* scene, Model& model)
{
	Mesh resultMesh;

	switch (mesh->mPrimitiveTypes)
	{
	case aiPrimitiveType_POINT:
		resultMesh.topology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		break;

	case aiPrimitiveType_LINE:
		resultMesh.topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		break;

	case aiPrimitiveType_TRIANGLE:
		resultMesh.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	
	default:
		resultMesh.topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		break;
	}

	// 정점 처리
	resultMesh.vertices.reserve(mesh->mNumVertices);
	for (UINT i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex = {};
		// 위치
		vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f };

		// UV // 첫 번째 UV 채널만 사용
		if (mesh->mTextureCoords[0]) vertex.UV = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

		// 법선
		if (mesh->HasNormals()) vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

		// 쌍접선, 접선
		if (mesh->HasTangentsAndBitangents())
		{
			vertex.bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
			vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
		}

		// 본 데이터 초기화 (정적 모델은 0으로 유지)
		vertex.boneIndex = { 0, 0, 0, 0 };
		vertex.boneWeight = { 0.f, 0.f, 0.f, 0.f };

		resultMesh.vertices.push_back(vertex);
	}

	// 본(Bone) 가중치 처리 (본이 있는 경우에만 실행)
	if (mesh->HasBones())
	{
		// 람다: 정점에 본 ID와 가중치를 추가하는 함수
		auto addBoneData = [](Vertex& vertex, uint32_t boneIndex, float weight)
			{
				float* weights = &vertex.boneWeight.x;
				uint32_t* indices = vertex.boneIndex.data();

				// 빈 슬롯(가중치가 0인 곳)을 찾아 넣음
				for (int i = 0; i < 4; ++i)
				{
					if (weights[i] == 0.0f)
					{
						indices[i] = boneIndex;
						weights[i] = weight;
						return;
					}
				}
			};

		for (UINT i = 0; i < mesh->mNumBones; ++i)
		{
			const aiBone* bone = mesh->mBones[i];
			const string boneName = bone->mName.C_Str();

			uint32_t boneIndex = 0;

			// 모델의 스켈레톤에 본 등록
			auto mappingIt = model.skeleton.boneMapping.find(boneName);
			if (mappingIt == model.skeleton.boneMapping.end())
			{
				boneIndex = static_cast<uint32_t>(model.skeleton.bones.size());
				model.skeleton.boneMapping[boneName] = boneIndex;

				BoneInfo info = {};
				info.id = boneIndex;
				info.offset_matrix = ToXMFLOAT4X4(bone->mOffsetMatrix);
				model.skeleton.bones.push_back(info);
			}
			else
			{
				boneIndex = mappingIt->second;
			}

			// 해당 본의 영향을 받는 정점들에 가중치 기록
			for (UINT weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
			{
				const aiVertexWeight& weight = bone->mWeights[weightIndex];
				if (weight.mVertexId < resultMesh.vertices.size())
				{
					addBoneData(resultMesh.vertices[weight.mVertexId], boneIndex, weight.mWeight);
				}
			}
		}

		for (auto& vertex : resultMesh.vertices)
		{
			float* weights = &vertex.boneWeight.x;
			float sum = weights[0] + weights[1] + weights[2] + weights[3];
			if (sum > 0.0f)
			{
				for (int i = 0; i < 4; ++i) weights[i] /= sum;
			}
		}
	}

	for (UINT i = 0; i < mesh->mNumFaces; ++i)
	{
		const aiFace& face = mesh->mFaces[i];
		for (UINT j = 0; j < face.mNumIndices; ++j) resultMesh.indices.push_back(face.mIndices[j]);
	}
	resultMesh.indexCount = static_cast<UINT>(resultMesh.indices.size());

	// 바운딩 박스 처리
	resultMesh.boundingBox =
	{
		// 중심
		{
			(mesh->mAABB.mMin.x + mesh->mAABB.mMax.x) * 0.5f,
			(mesh->mAABB.mMin.y + mesh->mAABB.mMax.y) * 0.5f,
			(mesh->mAABB.mMin.z + mesh->mAABB.mMax.z) * 0.5f
		},
		// 꼭짓점까지의 거리
		{
			(mesh->mAABB.mMax.x - mesh->mAABB.mMin.x) * 0.5f,
			(mesh->mAABB.mMax.y - mesh->mAABB.mMin.y) * 0.5f,
			(mesh->mAABB.mMax.z - mesh->mAABB.mMin.z) * 0.5f
		}
	};
	// 모델 전체 바운딩 박스 갱신
	BoundingBox::CreateMerged(model.boundingBox, model.boundingBox, resultMesh.boundingBox);

	CreateMeshBuffers(resultMesh);

	return resultMesh;
}



void ResourceManager::CreateMeshBuffers(Mesh& mesh)
{
	HRESULT hr = S_OK;

	// 정점 버퍼 생성
	if (mesh.vertices.empty()) return;
	const D3D11_BUFFER_DESC vertexBufferDesc =
	{
		.ByteWidth = static_cast<UINT>(sizeof(Vertex) * mesh.vertices.size()),
		.Usage = D3D11_USAGE_DEFAULT, // 이거 D3D11_USAGE_IMMUTABLE로 바꿀 수 있나?
		.BindFlags = D3D11_BIND_VERTEX_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	};
	const D3D11_SUBRESOURCE_DATA vertexInitialData =
	{
		.pSysMem = mesh.vertices.data(),
		.SysMemPitch = 0,
		.SysMemSlicePitch = 0
	};
	hr = m_device->CreateBuffer(&vertexBufferDesc, &vertexInitialData, mesh.vertexBuffer.GetAddressOf());
	CheckResult(hr, "메쉬 정점 버퍼 생성 실패.");

	// 인덱스 버퍼 생성
	if (mesh.indices.empty()) return;
	const D3D11_BUFFER_DESC indexBufferDesc =
	{
		.ByteWidth = static_cast<UINT>(sizeof(UINT) * mesh.indices.size()),
		.Usage = D3D11_USAGE_DEFAULT, // 이것도
		.BindFlags = D3D11_BIND_INDEX_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	};
	const D3D11_SUBRESOURCE_DATA indexInitialData =
	{
		.pSysMem = mesh.indices.data(),
		.SysMemPitch = 0,
		.SysMemSlicePitch = 0
	};
	hr = m_device->CreateBuffer(&indexBufferDesc, &indexInitialData, mesh.indexBuffer.GetAddressOf());
	CheckResult(hr, "메쉬 인덱스 버퍼 생성 실패.");
}

unique_ptr<SkeletonNode> ResourceManager::BuildSkeletonNode(const aiNode* node, Skeleton& skeleton)
{
	auto skeletonNode = make_unique<SkeletonNode>();
	skeletonNode->name = node->mName.C_Str();
	skeletonNode->localTransform = ToXMFLOAT4X4(node->mTransformation);

	auto mappingIt = skeleton.boneMapping.find(skeletonNode->name);
	if (mappingIt != skeleton.boneMapping.end())
	{
		skeletonNode->boneIndex = static_cast<int>(mappingIt->second);
	}

	for (UINT i = 0; i < node->mNumChildren; ++i)
	{
		skeletonNode->children.push_back(BuildSkeletonNode(node->mChildren[i], skeleton));
	}

	return skeletonNode;
}

/// <summary>
/// Assimp 라이브러리로 읽어들인 원본 데이터(aiScene)를 우리 엔진 전용 포맷(Model)으로 변환(Parsing)하는 함수
/// </summary>
/// <param name="scene">[in] 원본 데이터(aiScene)</param>
/// <param name="model">[out] 우리 엔진 전용 포맷(Model)</param>
void ResourceManager::LoadAnimations(const aiScene* scene, Model& model)
{
	model.animations.clear();
	if (!scene || scene->mNumAnimations == 0) return;

	model.animations.reserve(scene->mNumAnimations);

	//1. 애니메이션 순회 (겉 포장지 뜯기)
	for (UINT anim_index = 0; anim_index < scene->mNumAnimations; ++anim_index)
	{
		//aiAnimation: Assimp에서 애니메이션 하나(예: "Run", "Walk", "Attack")를 의미하는 구조체입니다.
		const aiAnimation* animation = scene->mAnimations[anim_index];
		if (!animation) continue;

		AnimationClip clip = {};
		clip.name					= animation->mName.C_Str();
		if (clip.name.empty()) clip.name = "Animation_" + to_string(anim_index);
		clip.duration				= static_cast<float>(animation->mDuration);
		clip.ticks_per_second		= static_cast<float>(animation->mTicksPerSecond);
		if (clip.ticks_per_second <= 0.0f) clip.ticks_per_second = AnimationClip::DEFAULT_FPS;
		

		//2. 채널 순회 (누가 움직이는가?)
		//aiNodeAnim (Channel): 이게 핵심입니다. "특정 뼈 하나에 대한 움직임 데이터 묶음"입니다.
		//하나의 애니메이션(예: 달리기) 안에는 수십 개의 채널(왼팔 채널, 오른다리 채널, 머리 채널...)이 들어있습니다.
		for (UINT channel_index = 0; channel_index < animation->mNumChannels; ++channel_index)
		{
			const aiNodeAnim* node_anim = animation->mChannels[channel_index];
			if (!node_anim) continue;

			BoneAnimationChannel channel = {};
			channel.boneName = node_anim->mNodeName.C_Str();

			//3. 본 매핑(이름표 확인)
			auto mapping_iter = model.skeleton.boneMapping.find(channel.boneName);
			if (mapping_iter != model.skeleton.boneMapping.end())
			{
				channel.boneIndex = static_cast<int>(mapping_iter->second);
			}


			//4. 키프레임 복사 (움직임 기록)
			//(1)
			channel.position_keys.reserve(node_anim->mNumPositionKeys);
			for (UINT i = 0; i < node_anim->mNumPositionKeys; ++i)
			{
				VectorKeyframe key = {};
				key.time_position	= static_cast<float>(node_anim->mPositionKeys[i].mTime);
				key.value			= ToXMFLOAT3(node_anim->mPositionKeys[i].mValue);
				channel.position_keys.push_back(key);
			}

			//(2)
			channel.rotation_keys.reserve(node_anim->mNumRotationKeys);
			for (UINT i = 0; i < node_anim->mNumRotationKeys; ++i)
			{
				QuaternionKeyframe key = {};
				key.time_position	= static_cast<float>(node_anim->mRotationKeys[i].mTime);
				key.value			= ToXMFLOAT4(node_anim->mRotationKeys[i].mValue);
				channel.rotation_keys.push_back(key);
			}

			//(3)
			channel.scale_keys.reserve(node_anim->mNumScalingKeys);
			for (UINT i = 0; i < node_anim->mNumScalingKeys; ++i)
			{
				VectorKeyframe key = {};
				key.time_position	= static_cast<float>(node_anim->mScalingKeys[i].mTime);
				key.value			= ToXMFLOAT3(node_anim->mScalingKeys[i].mValue);
				channel.scale_keys.push_back(key);
			}

			string keyName = channel.boneName; // 이름을 미리 복사
			clip.channels[keyName] = move(channel); // 안전하게 이동
		}
		
		//5. 저장 (가방에 넣기)
		model.animations.push_back(move(clip));
	}
}


com_ptr<ID3DBlob> ResourceManager::CompileShader(const string& shaderName, const char* shaderModel)
{
	HRESULT hr = S_OK;

	const filesystem::path shaderPath = "../Asset/Shader/" + shaderName;
	com_ptr<ID3DBlob> shaderCode = nullptr;
	com_ptr<ID3DBlob> errorBlob = nullptr;

	hr = D3DCompileFromFile
	(
		shaderPath.wstring().c_str(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",
		shaderModel,
		#ifdef _DEBUG
		D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		#else
		D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3,
		#endif
		0,
		shaderCode.GetAddressOf(),
		errorBlob.GetAddressOf()
	);
	if (errorBlob) cerr << shaderName << " 셰이더 컴파일 오류: " << static_cast<const char*>(errorBlob->GetBufferPointer()) << endl;

	return shaderCode;
}