#pragma once

class RNG : public Singleton<RNG>
{
	friend class Singleton<RNG>;

	std::mt19937 m_generator = {};

public:
	RNG() = default;
	~RNG() = default;
	RNG(const RNG&) = delete;
	RNG& operator=(const RNG&) = delete;
	RNG(RNG&&) = delete;
	RNG& operator=(RNG&&) = delete;

	void Initialize() { m_generator.seed(std::random_device{}()); }

	template <typename T> requires std::is_integral_v<T>
	T Range(T min, T max) { std::uniform_int_distribution<T> distribution(min, max); return distribution(m_generator); }
	template <typename T> requires std::is_floating_point_v<T>
	T Range(T min, T max) { std::uniform_real_distribution<T> distribution(min, max); return distribution(m_generator); }
};

template<typename T> requires std::is_integral_v<T>
constexpr T RANDOM(T min, T max) { return RNG::GetInstance().Range<T>(min, max); }
template<typename T> requires std::is_floating_point_v<T>
constexpr T RANDOM(T min, T max) { return RNG::GetInstance().Range<T>(min, max); }