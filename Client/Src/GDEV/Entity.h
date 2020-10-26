#pragma once
#include <Core.h>
#include <Engine.h>

class Entity final{
	friend class Shotgun;
	friend class Scar;
	friend class Sniper;
	friend class Gun;
	friend class Scene;
private:
	enum class EntityType{
		Bullet = 0,
		Sphere,
		Wall,
		Pillar,
		Coin,
		Fire,
		Enemy,
		ShotgunAmmo,
		ScarAmmo,
		SniperAmmo,
		Amt
	};

	Entity();

	EntityType type;
	bool active;
	float life;
	float maxLife;
	glm::vec4 colour;
	int diffuseTexIndex;
	glm::vec3 collisionNormal;
	glm::vec3 scale;
	Light* light;

	glm::vec3 pos;
	glm::vec3 vel;
	float mass;
	glm::vec3 force;
};