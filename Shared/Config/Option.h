#include <string>
#include <string_view>
using namespace std;

namespace Config
{
	//Render
		

//etc.
	//Input
		//청각 입력 보정
		inline constexpr float BeatHumanOffset = 0.24f;
	
		//오차 보정
		inline constexpr float InputCorrection = 0.08f;
	
		//노드 생성 -> 도착 시간
		inline constexpr float travelTime      = 1.2f;

		//시각적 보정
		inline constexpr float visualLead	   = 0.2f;

	//Effect
		inline constexpr float Player_Dash_Tilt = 10.0f;


//Sound Config
	//Volume for debug
		inline constexpr const float Master_Volume = 1.0f;											//0 ~ 1
		inline constexpr const float BGM_Volume = 1.0f;
		inline constexpr const float AMB_Volume = 1.0f;
		inline constexpr const float SFX_Volume = 1.0f;
		inline constexpr const float UI_Volume = 1.0f;

	//BGM
		inline constexpr const char* Main_BGM = "1.DOB_IngameBGM_1";									//Node 생성은(Main_BGM + "_Beat")으로 자동 결정 ex) DOB Music_test2_Beat 
		inline constexpr const char* Ambience = "1.DOB_AMB";

	//SFX
		//Player
		inline constexpr const char* Player_Dash = "1.Player_Dash";

		//Enemy
		inline constexpr const char* Enemy_Die = "8.Enemy_Die";

		//UI
		inline constexpr const char* Player_Shoot = "2.Player_Shot";
		inline constexpr const char* Player_DeadEye_Shoot = "10.DeadEye_Shoot_LV";
		inline constexpr const char* Player_Reload_Spin = "4.Player_spin";

		inline constexpr const char* Player_Reload_Cocking = "4.Player_cocking";
		inline constexpr const int   Player_Reload_Cocking_Count = 1;

//Score Config
		inline constexpr int		 BaseScore = 100;
		inline constexpr int		 ChainKillBonus = 50;
		inline constexpr float		 ChainKillWindowSec = 2.0f;
		inline constexpr int		 KillsPerLevel = 10;
		inline constexpr int		 MaxMultiplier = 8;
		inline constexpr float		 ScoreDecayIntervalSec = 1.0f;
		inline constexpr int		 ScoreDecayAmount = 1;
}


