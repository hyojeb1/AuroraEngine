#include "stdafx.h"

#include "ResourceManager.h"
#include "Renderer.h"

#include "UIBase.h"
#include "Button.h"
#include "Panel.h"
#include "Slider.h"
#include "UITextBase.h"

#include "TimeManager.h"

using namespace nlohmann;

void UIBase::SetTextureAndOffset(const std::string& idle)
{
	m_textureIdle = ResourceManager::GetInstance().GetTextureAndOffset(idle);
	UpdateRect();
}

json UIBase::Serialize() const
{
    json data;
    data["type"] = GetTypeName();
    data["name"] = m_name;
    data["active"] = m_isActive;

    data["pos"] = { m_localPosition.x, m_localPosition.y };
    data["scale"] = m_scale;

    data["depth"] = m_depth;

    data["pathIdle"] = m_pathIdle;

    // Animation
    data["animRows"] = m_rows;
    data["animCols"] = m_columns;
    data["animFPS"] = m_framesPerSecond;
    data["animLoop"] = m_loop;
    data["animAutoPlay"] = auto_play_;

    return data;
}

void UIBase::Deserialize(const json& data)
{
    if (data.contains("name")) m_name = data["name"];
    if (data.contains("active")) m_isActive = data["active"];

    if (data.contains("pos")) {
        m_localPosition.x = data["pos"][0];
        m_localPosition.y = data["pos"][1];
    }
    if (data.contains("scale")) m_scale = data["scale"];
    if (data.contains("depth")) m_depth = data["depth"];
    if (data.contains("pathIdle")) m_pathIdle = data.value("textureIdle", "");

    // Animation Data
    if (data.contains("animRows")) m_rows = data["animRows"];
    if (data.contains("animCols")) m_columns = data["animCols"];
    if (data.contains("animFPS")) m_framesPerSecond = data["animFPS"];
    if (data.contains("animLoop")) m_loop = data["animLoop"];
    if (data.contains("animAutoPlay")) auto_play_ = data["animAutoPlay"];

    m_playing = auto_play_;
    m_currentFrame = 0;
    m_accumulatedTime = 0.0f;

    UpdateRect();
}

UIBase* UIBase::CreateFactory(const std::string& typeName)
{
    if (typeName == "Button")      return new Button();
    if (typeName == "Panel")       return new Panel();
    if (typeName == "Slider")      return new Slider();
    if (typeName == "Text")        return new UITextBase();
    // if (typeName == "MovingPanel") return new MovingPanel();

    return nullptr;
}

void UIBase::Update()
{
    // 1. 재생 조건 체크
    if (!m_isActive || !m_playing) return;
    if (m_rows <= 0 || m_columns <= 0) return;

    const int maxFrames = GetMaxFrames();
    if (maxFrames <= 1) return;
    if (m_framesPerSecond <= 0.0f) return;
    if (playback_speed_ <= 0.0f) return;

    float dt = TimeManager::GetInstance().GetDeltaTime();


    // 2. 시간 누적
    m_accumulatedTime += dt * playback_speed_;
    const float frameDuration = 1.0f / m_framesPerSecond;

    // 3. 프레임 변경 로직
    bool updated = false;
    while (m_accumulatedTime >= frameDuration) {
        m_accumulatedTime -= frameDuration;
        ++m_currentFrame;
        updated = true;

        // 프레임 초과 처리
        if (m_currentFrame >= maxFrames) {
            if (m_loop) {
                m_currentFrame = 0;
            } else {
                m_currentFrame = maxFrames - 1;
                m_playing = false; // 재생 중지

                // 필요하다면 여기서 애니메이션 종료 콜백 호출
                // OnAnimationFinished(); 

                if (destroy_on_finish_) {
                    SetActive(false); // UI 비활성화 (삭제 대기)
                }
                break;
            }
        }
    }

    // 4. 프레임이 변했으면 RECT(이미지 자르는 영역) 업데이트
    if (updated) {
        UpdateFlipBookRect();
    }
}


void UIBase::UpdateFlipBookRect()
{
    // 텍스처가 없으면 리턴
    if (!m_textureIdle.first) {
        m_UIAnimationRect = { 0, 0, 0, 0 };
        return;
    }

    // 1. 텍스처 전체 크기 가져오기
    int texWidth = 0, texHeight = 0;
    GetTextureResolution(texWidth, texHeight);

    // 2. 안전장치 (0으로 나누기 방지)
    if (m_columns <= 0) m_columns = 1;
    if (m_rows <= 0) m_rows = 1;

    // 3. 현재 프레임 인덱스 보정
    ClampFrame();

    // 4. 프레임 하나의 크기 계산 (픽셀 단위)
    const int frameWidth = texWidth / m_columns;
    const int frameHeight = texHeight / m_rows;

    // 5. 현재 프레임의 행/열 계산
    // 텍스처 아틀라스에서의 절대 인덱스 계산 (StartFrame 고려)
    // 보통 StartFrame은 0이지만, 특정 동작을 위해 오프셋을 줄 수 있음
    const int currentAbsoluteFrame = m_currentFrame;

    // 전체 그리드 내에서의 위치
    const int colIndex = currentAbsoluteFrame % m_columns;
    const int rowIndex = currentAbsoluteFrame / m_columns; // 행은 나누기 몫

    // 6. RECT 설정 (Source Rectangle)
   m_UIAnimationRect.left = colIndex * frameWidth;
   m_UIAnimationRect.top = rowIndex * frameHeight;
   m_UIAnimationRect.right = m_UIAnimationRect.left + frameWidth;
   m_UIAnimationRect.bottom = m_UIAnimationRect.top + frameHeight;
}

void UIBase::GetTextureResolution(int& outWidth, int& outHeight) const
{
    outWidth = 0;
    outHeight = 0;

    if (m_textureIdle.first) {
        // SRV에서 리소스(텍스처) 추출
        com_ptr<ID3D11Resource> resource;
        m_textureIdle.first->GetResource(resource.GetAddressOf());

        if (resource) {
            com_ptr<ID3D11Texture2D> texture2D;
            if (SUCCEEDED(resource.As(&texture2D))) {
                D3D11_TEXTURE2D_DESC desc;
                texture2D->GetDesc(&desc);
                outWidth = static_cast<int>(desc.Width);
                outHeight = static_cast<int>(desc.Height);
            }
        }
    }
}

int UIBase::GetMaxFrames() const
{
    if (m_rows <= 0 || m_columns <= 0) return 0;
    const int gridFrames = m_rows * m_columns;

    // StartFrame부터 EndFrame까지의 구간 길이 계산
    // 예: Start(0), End(5) -> 0,1,2,3,4 (5프레임)
    // 예: Start(0), End(0) -> 전체 프레임 사용

    int availableFrames = gridFrames;

    if (m_endFrame > 0 && m_endFrame <= gridFrames) {
        availableFrames = m_endFrame;
    }

    // StartFrame이 있으면 그만큼 뺌 (UI에서는 보통 0부터 시작)
    if (m_startFrame > 0 && m_startFrame < availableFrames) {
        // 복잡한 부분: FlipbookParticle은 range를 지정하지만, 
        // UI는 보통 0~End 혹은 Start~End로 씀. 
        // 여기서는 Particle 로직을 따라감 (Start부터 End까지의 갯수가 아니라, 인덱스 기준)
        return availableFrames - m_startFrame;
    }

    return availableFrames;
}

void UIBase::ClampFrame()
{
    if (m_rows <= 0) m_rows = 1;
    if (m_columns <= 0) m_columns = 1;

    const int maxFrames = GetMaxFrames(); // 현재 설정상 가능한 최대 프레임 수

    // 현재 프레임이 범위를 벗어나지 않게 조정
    if (maxFrames <= 0) {
        m_currentFrame = 0;
    } else {
        if (m_currentFrame < 0) m_currentFrame = 0;
        if (m_currentFrame >= maxFrames) m_currentFrame = maxFrames - 1;
    }
}