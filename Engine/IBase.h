#pragma once

class IBase
{
public:
	virtual ~IBase() = default;

	virtual void Initialize() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Render() = 0;
	virtual void Finalize() = 0;
};