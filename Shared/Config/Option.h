#include <string>
#include <string_view>
using namespace std;

namespace Config
{
	//Render


	//etc.








//Sound Config
//Volume
	inline constexpr const float Master_Volume = 1.0f;											//0 ~ 1
	inline constexpr const float BGM_Volume = 1.0f;
	inline constexpr const float AMB_Volume = 1.0f;
	inline constexpr const float SFX_Volume = 1.0f;
	inline constexpr const float UI_Volume = 1.0f;


//BGM
	inline constexpr const char* Main_BGM = "10S_Sample";									//Node 생성은(Main_BGM + "_Beat")으로 자동 결정 ex) DOB Music_test2_Beat 
	inline constexpr const char* Ambience = "1.DOB_AMB";
//SFX
	//Player
	inline constexpr const char* Player_Dash = "1.Player_Dash";

	//Enemy
	inline constexpr const char* Enemy_Die = "8.Enemy_Die";

//UI
	inline constexpr const char* Player_Shoot = "2.Player_Shot";
	inline constexpr const char* Player_Reload_Spin = "4.Player_spin";

	inline constexpr const char* Player_Reload_Cocking = "4.Player_cocking";
	inline constexpr const int   Player_Reload_Cocking_Count = 1;
}


