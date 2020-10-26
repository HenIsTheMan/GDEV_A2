#pragma once
#include "Entity.h"

class Gun{
public:
	virtual ~Gun() = default;

	///Getters
	const bool& GetCanShoot() const;
	const float& GetReloadBT() const;
	const float& GetReloadTime() const;
	const float& GetShotCooldownTime() const;
	const int& GetLoadedBullets() const;
	const int& GetMaxLoadedBullets() const;
	const int& GetUnloadedBullets() const;
	const int& GetMaxUnloadedBullets() const;

	///Setters
	void SetCanShoot(const bool& canShoot);
	void SetReloadBT(const float& reloadBT);
	void SetReloadTime(const float& reloadTime);
	void SetShotCooldownTime(const float& shotCooldownTime);
	void SetLoadedBullets(const int& loadedBullets);
	void SetMaxLoadedBullets(const int& maxLoadedBullets);
	void SetUnloadedBullets(const int& unloadedBullets);
	void SetMaxUnloadedBullets(const int& maxUnloadedBullets);

	void Update();
	void Reload(ISoundEngine* const& soundEngine);
	virtual void Shoot(const float& elapsedTime, Entity* const& entity, const glm::vec3& camPos, const glm::vec3& camFront, ISoundEngine* const& soundEngine) = 0;
protected:
	Gun();

	bool canShoot;
	float reloadBT;
	float reloadTime;
	float shotCooldownTime;
	int loadedBullets;
	int maxLoadedBullets;
	int unloadedBullets;
	int maxUnloadedBullets;
private:
	bool reloading;
};

class Shotgun final: public Gun{
public:
	Shotgun();
	void Shoot(const float& elapsedTime, Entity* const& entity, const glm::vec3& camPos, const glm::vec3& camFront, ISoundEngine* const& soundEngine) override;
};

class Scar final: public Gun{
public:
	Scar();
	void Shoot(const float& elapsedTime, Entity* const& entity, const glm::vec3& camPos, const glm::vec3& camFront, ISoundEngine* const& soundEngine) override;
};

class Sniper final: public Gun{
public:
	Sniper();
	void Shoot(const float& elapsedTime, Entity* const& entity, const glm::vec3& camPos, const glm::vec3& camFront, ISoundEngine* const& soundEngine) override;
};