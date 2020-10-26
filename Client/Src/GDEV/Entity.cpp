#include "Entity.h"

Entity::Entity():
	type(EntityType::Amt),
	active(false),
	life(0.f),
	maxLife(0.f),
	colour(glm::vec4(.7f, .4f, .1f, 1.f)),
	diffuseTexIndex(-1),
	collisionNormal(glm::vec3(0.f)),
	scale(glm::vec3(1.f)),
	light(nullptr),

	pos(glm::vec3(0.f)),
	vel(glm::vec3(0.f)),
	mass(0.f),
	force(glm::vec3(0.f))
{
}