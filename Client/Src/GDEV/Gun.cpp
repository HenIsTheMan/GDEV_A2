#include "Gun.h"

extern float dt;

Gun::Gun():
	canShoot(true),
	reloadBT(0.f),
	reloadTime(0.f),
	shotCooldownTime(0.f),
	loadedBullets(0),
	maxLoadedBullets(0),
	unloadedBullets(0),
	maxUnloadedBullets(0),
	reloading(false)
{
}

const bool& Gun::GetCanShoot() const{
	return canShoot;
}

const float& Gun::GetReloadBT() const{
	return reloadBT;
}

const float& Gun::GetReloadTime() const{
	return reloadTime;
}

const float& Gun::GetShotCooldownTime() const{
	return shotCooldownTime;
}

const int& Gun::GetLoadedBullets() const{
	return loadedBullets;
}

const int& Gun::GetMaxLoadedBullets() const{
	return maxLoadedBullets;
}

const int& Gun::GetUnloadedBullets() const{
	return unloadedBullets;
}

const int& Gun::GetMaxUnloadedBullets() const{
	return maxUnloadedBullets;
}

void Gun::SetCanShoot(const bool& canShoot){
	this->canShoot = canShoot;
}

void Gun::SetReloadBT(const float& reloadBT){
	this->reloadBT = reloadBT;
}

void Gun::SetReloadTime(const float& reloadTime){
	this->reloadTime = reloadTime;
}

void Gun::SetShotCooldownTime(const float& shotCooldownTime){
	this->shotCooldownTime = shotCooldownTime;
}

void Gun::SetLoadedBullets(const int& loadedBullets){
	this->loadedBullets = loadedBullets;
}

void Gun::SetMaxLoadedBullets(const int& maxLoadedBullets){
	this->maxLoadedBullets = maxLoadedBullets;
}

void Gun::SetUnloadedBullets(const int& unloadedBullets){
	this->unloadedBullets = unloadedBullets;
}

void Gun::SetMaxUnloadedBullets(const int& maxUnloadedBullets){
	this->maxUnloadedBullets = maxUnloadedBullets;
}

void Gun::Update(){
	if(reloading){
		if(reloadBT <= reloadTime){
			reloadBT += dt;
		} else{
			canShoot = true;
			loadedBullets += unloadedBullets;
			unloadedBullets = loadedBullets < maxLoadedBullets ? 0 : loadedBullets - maxLoadedBullets;
			loadedBullets = std::min(maxLoadedBullets, loadedBullets);
			reloading = false;
		}
	}
}

void Gun::Reload(ISoundEngine* const& soundEngine){
	if(loadedBullets == maxLoadedBullets){
		return;
	} else if(!reloading){
		soundEngine->play2D("Audio/Sounds/Reload.wav", false);
		canShoot = false;
		reloadBT = 0.f;
		reloading = true;
	}
}

Shotgun::Shotgun(){
	reloadTime = 2.f;
	shotCooldownTime = .7f;
	loadedBullets = 12;
	maxLoadedBullets = 12;
	unloadedBullets = 18;
	maxUnloadedBullets = 360;
}

void Shotgun::Shoot(const float& elapsedTime, Entity* const& entity, const glm::vec3& camPos, const glm::vec3& camFront, ISoundEngine* const& soundEngine){
	static float bulletBT = 0.f;
	if(canShoot && loadedBullets && bulletBT <= elapsedTime){
		soundEngine->play2D("Audio/Sounds/Shotgun.wav", false);

		entity->type = Entity::EntityType::Bullet;
		entity->active = true;
		entity->life = 5.f;
		entity->maxLife = 5.f;
		entity->colour = glm::vec4(glm::vec3(.4f), .3f);
		entity->diffuseTexIndex = -1;
		entity->collisionNormal = glm::vec3(1.f, 0.f, 0.f);
		entity->scale = glm::vec3(.2f);
		entity->light = nullptr;

		entity->pos = camPos;
		entity->vel = 200.f * glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(PseudorandMinMax(-1.f, 1.f)), {0.f, 1.f, 0.f}) * glm::vec4(camFront, 0.f)); //With bullet bloom
		entity->mass = 5.f;
		entity->force = glm::vec3(0.f);

		--loadedBullets;
		bulletBT = elapsedTime + shotCooldownTime;
	}
}

Scar::Scar(){
	reloadTime = 1.f;
	shotCooldownTime = .1f;
	loadedBullets = 40;
	maxLoadedBullets = 40;
	unloadedBullets = 120;
	maxUnloadedBullets = 2400;
}

void Scar::Shoot(const float& elapsedTime, Entity* const& entity, const glm::vec3& camPos, const glm::vec3& camFront, ISoundEngine* const& soundEngine){
	static float bulletBT = 0.f;
	if(canShoot && loadedBullets && bulletBT <= elapsedTime){
		soundEngine->play2D("Audio/Sounds/Sniper.wav", false);

		entity->type = Entity::EntityType::Bullet;
		entity->active = true;
		entity->life = 5.f;
		entity->maxLife = 5.f;
		entity->colour = glm::vec4(glm::vec3(1.f), .3f);
		entity->diffuseTexIndex = -1;
		entity->collisionNormal = glm::vec3(1.f, 0.f, 0.f);
		entity->scale = glm::vec3(.2f);
		entity->light = nullptr;

		entity->pos = camPos + 10.f * camFront;
		entity->vel = 180.f * glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(PseudorandMinMax(-2.f, 2.f)), {0.f, 1.f, 0.f}) * glm::vec4(camFront, 0.f)); //With bullet bloom
		entity->mass = 5.f;
		entity->force = glm::vec3(0.f);

		--loadedBullets;
		bulletBT = elapsedTime + shotCooldownTime;
	}
}

Sniper::Sniper(){
	reloadTime = 3.f;
	shotCooldownTime = 0.f;
	loadedBullets = 1;
	maxLoadedBullets = 1;
	unloadedBullets = 10;
	maxUnloadedBullets = 30;
}

void Sniper::Shoot(const float& elapsedTime, Entity* const& entity, const glm::vec3& camPos, const glm::vec3& camFront, ISoundEngine* const& soundEngine){
	static float bulletBT = 0.f;
	if(canShoot && loadedBullets && bulletBT <= elapsedTime){
		soundEngine->play2D("Audio/Sounds/Sniper.wav", false);

		entity->type = Entity::EntityType::Bullet;
		entity->active = true;
		entity->life = 5.f;
		entity->maxLife = 5.f;
		entity->colour = glm::vec4(0.f, 0.f, 1.f, .3f);
		entity->diffuseTexIndex = -1;
		entity->collisionNormal = glm::vec3(1.f, 0.f, 0.f);
		entity->scale = glm::vec3(.2f);
		entity->light = nullptr;

		entity->pos = camPos + 10.f * camFront;
		entity->vel = 500.f * camFront;
		entity->mass = 5.f;
		entity->force = glm::vec3(0.f);

		--loadedBullets;
		bulletBT = elapsedTime + shotCooldownTime;
	}
}