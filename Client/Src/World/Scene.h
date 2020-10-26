#pragma once
#include <Engine.h>
#include "Cam.h"
#include "../GDEV/Entity.h"
#include "../GDEV/Gun.h"

#define BIT(x) 1 << x

class Scene final{
public:
	Scene();
	~Scene();
	bool Init();
	void Update(GLFWwindow* const& win);
	void GeoRenderPass();
	void LightingRenderPass(const uint& posTexRefID, const uint& coloursTexRefID, const uint& normalsTexRefID, const uint& specTexRefID, const uint& reflectionTexRefID);
	void BlurRender(const uint& brightTexRefID, const bool& horizontal);
	void DefaultRender(const uint& screenTexRefID, const uint& blurTexRefID, const glm::vec3& translate, const glm::vec3& scale);
	void MinimapRender();
	void ForwardRender();
private:
	Cam cam;
	Cam minimapCam;
	ISoundEngine* soundEngine;
	std::vector<ISound*> coinMusic;
	std::vector<ISoundEffectControl*> coinSoundFX;
	std::vector<ISound*> fireMusic;
	std::vector<ISoundEffectControl*> fireSoundFX;
	TextChief textChief;

	enum struct MeshType{
		Quad = 0,
		Cube,
		Sphere,
		Cylinder,
		Terrain,
		CoinSpriteAni,
		FireSpriteAni,
		Amt
	};
	Mesh* meshes[(int)MeshType::Amt];

	enum struct ModelType{
		Shotgun = 0,
		Scar,
		Sniper,
		Amt
	};
	Model* models[(int)ModelType::Amt];

	ShaderProg blurSP;
	ShaderProg forwardSP;
	ShaderProg geoPassSP;
	ShaderProg lightingPassSP;
	ShaderProg normalsSP;
	ShaderProg screenSP;
	ShaderProg textSP;

	std::vector<Light*> ptLights;
	std::vector<Light*> directionalLights;
	std::vector<Light*> spotlights;
	uint cubemapRefID;

	enum struct MinimapViewType{
		TopFollowingOrtho = 0,
		TopFollowingPerspective,
		TopStaticOrtho,
		TopStaticPerspective,
		IsometricOrtho,
		IsometricPerspective,
		ThirdPersonFollowingOrtho,
		ThirdPersonFollowingPerspective,
		ThirdPersonStaticOrtho,
		ThirdPersonStaticPerspective,
		Amt
	};
	MinimapViewType minimapView;
	enum struct ItemType{
		None = -1,
		Shotgun,
		Scar,
		Sniper,
		ShotgunAmmo,
		ScarAmmo,
		SniperAmmo,
		HealthPack,
	};
	int currSlot;
	ItemType inv[5];
	enum struct PlayerState{
		NoMovement = BIT(1),
		Walking = BIT(2),
		Sprinting = BIT(3),

		Standing = BIT(4),
		Jumping = BIT(5),
		Falling = BIT(6),
		Crouching = BIT(7),
		Proning = BIT(8),
	};
	int playerStates;
	enum struct Screen{
		MainMenu = 0,
		Game,
		GameOver,
		Scoreboard,
		Amt
	};
	Screen screen;
	std::vector<Entity*> entityPool;
	float timeLeft;
	int score;
	std::vector<int> scores;
	float textScaleFactors[3];
	glm::vec4 textColours[3];
	bool sprintOn;
	float playerHealth;
	float playerMaxHealth;
	int playerLives;
	int playerMaxLives;
	Gun* currGun;
	Gun* guns[3];
	glm::vec4 reticleColour;
	Entity* const& FetchEntity();
	struct EntityCreationAttribs final{
		glm::vec3 pos;
		glm::vec3 collisionNormal;
		glm::vec3 scale;
		glm::vec4 colour;
		int diffuseTexIndex;
	};
	void CreateShotgunAmmo(const EntityCreationAttribs& attribs);
	void CreateScarAmmo(const EntityCreationAttribs& attribs);
	void CreateSniperAmmo(const EntityCreationAttribs& attribs);
	void CreateCoin(const EntityCreationAttribs& attribs);
	void CreateFire(const EntityCreationAttribs& attribs);
	void CreateEnemy(const EntityCreationAttribs& attribs);
	void CreateSphere(const EntityCreationAttribs& attribs);
	void CreateThinWall(const EntityCreationAttribs& attribs);
	void CreatePillar(const EntityCreationAttribs& attribs);
	void CreateThickWall(const EntityCreationAttribs& attribs);
	void UpdateCollisionBetweenEntities(Entity* const& entity1, Entity* const& entity2);
	void UpdateEntities();
	void RenderEntities(ShaderProg& SP, const bool& opaque);

	glm::mat4 view;
	glm::mat4 projection;
	//std::vector<Mesh::BatchRenderParams> params;

	float elapsedTime;
	mutable std::stack<glm::mat4> modelStack;
	glm::mat4 Translate(const glm::vec3& translate);
	glm::mat4 Rotate(const glm::vec4& rotate);
	glm::mat4 Scale(const glm::vec3& scale);
	glm::mat4 GetTopModel() const;
	void PushModel(const std::vector<glm::mat4>& vec) const;
	void PopModel() const;
};

enum struct Axis{
	x = 0,
	y,
	z,
	Amt
};

inline static glm::vec3 RotateVecIn2D(const glm::vec3& vec, const float& angleInRad, const Axis& axis){
	switch(axis){
		case Axis::x:
			return glm::vec3(vec.x, vec.y * cos(angleInRad) + vec.z * -sin(angleInRad), vec.y * sin(angleInRad) + vec.z * cos(angleInRad));
		case Axis::y:
			return glm::vec3(vec.x * cos(angleInRad) + vec.z * sin(angleInRad), vec.y, vec.x * -sin(angleInRad) + vec.z * cos(angleInRad));
		case Axis::z:
			return glm::vec3(vec.x * cos(angleInRad) + vec.y * -sin(angleInRad), vec.x * sin(angleInRad) + vec.y * cos(angleInRad), vec.z);
		default:
			(void)puts("Invalid axis input for vec rotation!");
			return glm::vec3();
	}
}