#include "Scene.h"
#include "Vendor/stb_image.h"
#include <glm/gtx/color_space.hpp>

constexpr float terrainXScale = 1500.f;
constexpr float terrainYScale = 200.f;
constexpr float terrainZScale = 1500.f;

//#define DEBUGGING

extern bool LMB;
extern bool RMB;
extern bool endLoop;
extern float leftRightMB;
extern float angularFOV;
extern float dt;
extern int winWidth;
extern int winHeight;

void SetUpCubemap(uint& cubemapRefID, const std::vector<cstr>& faces);

glm::vec3 Light::globalAmbient = glm::vec3(.2f);

Scene::Scene():
	cam(glm::vec3(0.f, 0.f, 5.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), 0.f, 150.f),
	minimapCam(glm::vec3(0.f), glm::vec3(0.f), glm::vec3(0.f), 0.f, 0.f),
	soundEngine(nullptr),
	coinMusic({}),
	coinSoundFX({}),
	fireMusic({}),
	fireSoundFX({}),
	meshes{
		new Mesh(Mesh::MeshType::Quad, GL_TRIANGLES, {
			{"Imgs/BG.jpg", Mesh::TexType::Diffuse, 0},
			{"Imgs/Scope.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/Heart.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/Slot.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/ReticleMain.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/ReticleShotgun.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/ReticleScar.png", Mesh::TexType::Diffuse, 0},
		}),
		new Mesh(Mesh::MeshType::Cube, GL_TRIANGLES, {
		}),
		new Mesh(Mesh::MeshType::Sphere, GL_TRIANGLE_STRIP, {
			{"Imgs/Enemy.jpg", Mesh::TexType::Diffuse, 0},
		}),
		new Mesh(Mesh::MeshType::Cylinder, GL_TRIANGLE_STRIP, {
		}),
		new Terrain("Imgs/hMap.raw", 8.f, 8.f),
		new SpriteAni(1, 6),
		new SpriteAni(4, 8),
	},
	models{
		new Model("ObjsAndMtls/Shotgun.obj", {
			aiTextureType_DIFFUSE,
		}),
		new Model("ObjsAndMtls/Scar.obj", {
			aiTextureType_DIFFUSE,
		}),
		new Model("ObjsAndMtls/Sniper.obj", {
			aiTextureType_DIFFUSE,
		}),
	},
	blurSP{"Shaders/Quad.vs", "Shaders/Blur.fs"},
	forwardSP{"Shaders/Forward.vs", "Shaders/Forward.fs"},
	geoPassSP{"Shaders/GeoPass.vs", "Shaders/GeoPass.fs"},
	lightingPassSP{"Shaders/Quad.vs", "Shaders/LightingPass.fs"},
	normalsSP{"Shaders/Normals.vs", "Shaders/Normals.fs", "Shaders/Normals.gs"},
	screenSP{"Shaders/Quad.vs", "Shaders/Screen.fs"},
	textSP{"Shaders/Text.vs", "Shaders/Text.fs"},
	ptLights({}),
	directionalLights({}),
	spotlights({}),
	cubemapRefID(0),
	minimapView(MinimapViewType::TopFollowingOrtho),
	currSlot(0),
	inv{
		ItemType::Shotgun,
		ItemType::Scar,
		ItemType::Sniper,
		ItemType::None,
		ItemType::None,
	},
	playerStates((int)PlayerState::NoMovement | (int)PlayerState::Standing),
	screen(Screen::MainMenu),
	entityPool(std::vector<Entity*>(1000)),
	timeLeft(0.f),
	score(0),
	scores({}),
	textScaleFactors{
		1.f,
		1.f,
		1.f,
	},
	textColours{
		glm::vec4(1.f),
		glm::vec4(1.f),
		glm::vec4(1.f),
	},
	sprintOn(false),
	playerHealth(0.f),
	playerMaxHealth(100.f),
	playerLives(0),
	playerMaxLives(5),
	currGun(nullptr),
	guns{
		nullptr,
		nullptr,
		nullptr,
	},
	reticleColour(glm::vec4(1.f)),
	view(glm::mat4(1.f)),
	projection(glm::mat4(1.f)),
	elapsedTime(0.f),
	modelStack()
{
}

Scene::~Scene(){
	///Create save
	str line;
	try{
		std::ofstream stream("Data/scores.dat", std::ios::out);
		if(stream.is_open()){
			const size_t& mySize = scores.size();
			for(size_t i = 0; i < mySize; ++i){
				stream << (!i ? "" : "\n") + std::to_string(scores[i]);
			}
			stream.close();
		} else{
			throw("Failed to save scores!");
		}
	} catch(cstr const& errorMsg){
		(void)puts(errorMsg);
	}

	for(short i = 0; i < 3; ++i){
		if(guns[i]){
			delete guns[i];
			guns[i] = nullptr;
		}
	}
	for(Entity*& entity: entityPool){
		if(entity){
			delete entity;
			entity = nullptr;
		}
	}

	const size_t& pSize = ptLights.size();
	const size_t& dSize = directionalLights.size();
	const size_t& sSize = spotlights.size();
	for(size_t i = 0; i < pSize; ++i){
		delete ptLights[i];
		ptLights[i] = nullptr;
	}
	for(size_t i = 0; i < dSize; ++i){
		delete directionalLights[i];
		directionalLights[i] = nullptr;
	}
	for(size_t i = 0; i < sSize; ++i){
		delete spotlights[i];
		spotlights[i] = nullptr;
	}
	
	for(int i = 0; i < (int)MeshType::Amt; ++i){
		if(meshes[i]){
			delete meshes[i];
			meshes[i] = nullptr;
		}
	}
	for(int i = 0; i < (int)ModelType::Amt; ++i){
		if(models[i]){
			delete models[i];
			models[i] = nullptr;
		}
	}

	const size_t& coinMusicSize = coinMusic.size();
	for(size_t i = 0; i < coinMusicSize; ++i){
		coinMusic[i]->drop();
	}
	const size_t& fireMusicSize = fireMusic.size();
	for(size_t i = 0; i < fireMusicSize; ++i){
		fireMusic[i]->drop();
	}
	if(soundEngine){
		soundEngine->drop();
	}
}

bool Scene::Init(){
	///Load save
	cstr const& fPath = "Data/scores.dat";
	str line;
	std::ifstream stream(fPath, std::ios::in);
	if(stream.is_open()){
		while(getline(stream, line)){
			try{
				scores.emplace_back(stoi(line));
			} catch(const std::invalid_argument& e){
				(void)puts(e.what());
			}
		}
		stream.close();
	}
	if(scores.size() > 1){
		std::sort(scores.begin(), scores.end(), std::greater<int>());
	}

	const std::vector<cstr> faces{
		"Imgs/Skybox/Right.png",
		"Imgs/Skybox/Left.png",
		"Imgs/Skybox/Top.png",
		"Imgs/Skybox/Bottom.png",
		"Imgs/Skybox/Front.png",
		"Imgs/Skybox/Back.png"
	};
	SetUpCubemap(cubemapRefID, faces);

	soundEngine = createIrrKlangDevice(ESOD_AUTO_DETECT, ESEO_MULTI_THREADED | ESEO_LOAD_PLUGINS | ESEO_USE_3D_BUFFERS | ESEO_PRINT_DEBUG_INFO_TO_DEBUGGER);
	if(!soundEngine){
		(void)puts("Failed to init soundEngine!\n");
	}

	meshes[(int)MeshType::CoinSpriteAni]->AddTexMap({"Imgs/Coin.png", Mesh::TexType::Diffuse, 0});
	static_cast<SpriteAni*>(meshes[(int)MeshType::CoinSpriteAni])->AddAni("CoinSpriteAni", 0, 6);
	static_cast<SpriteAni*>(meshes[(int)MeshType::CoinSpriteAni])->Play("CoinSpriteAni", -1, .5f);

	meshes[(int)MeshType::FireSpriteAni]->AddTexMap({"Imgs/Fire.png", Mesh::TexType::Diffuse, 0});
	static_cast<SpriteAni*>(meshes[(int)MeshType::FireSpriteAni])->AddAni("FireSpriteAni", 0, 32);
	static_cast<SpriteAni*>(meshes[(int)MeshType::FireSpriteAni])->Play("FireSpriteAni", -1, .5f);

	meshes[(int)MeshType::Terrain]->AddTexMap({"Imgs/Floor.jpg", Mesh::TexType::Diffuse, 0});

	const size_t& mySize = entityPool.size();
	for(size_t i = 0; i < mySize; ++i){
		entityPool[i] = new Entity();
	}

	///Create thick walls
	for(short i = 0; i < 20; ++i){
		const float xScale = PseudorandMinMax(60.f, 90.f);
		const float yScale = PseudorandMinMax(60.f, 90.f);
		const float zScale = 5.f;
		const float xPos = PseudorandMinMax(-terrainXScale / 2.f + 5.f + xScale, terrainXScale / 2.f - 5.f - xScale);
		const float zPos = PseudorandMinMax(-terrainZScale / 2.f + 5.f + zScale, terrainZScale / 2.f - 5.f - zScale);
		CreateThickWall({
			glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale) + yScale - 5.f, zPos),
			glm::vec3(1.f, 0.f, 0.f),
			glm::vec3(xScale, yScale, zScale),
			glm::vec4(5.f * glm::rgbColor(glm::vec3(PseudorandMinMax(0.f, 255.f), 1.f, 1.f)), 1.f),
			-1,
		});
	}

	///Create spheres
	for(short i = 0; i < 20; ++i){
		const float scaleFactor = PseudorandMinMax(30.f, 40.f);
		const float xPos = PseudorandMinMax(-terrainXScale / 2.f + 5.f + scaleFactor, terrainXScale / 2.f - 5.f - scaleFactor);
		const float zPos = PseudorandMinMax(-terrainZScale / 2.f + 5.f + scaleFactor, terrainZScale / 2.f - 5.f - scaleFactor);
		CreateSphere({
			glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale) + scaleFactor - 5.f, zPos),
			glm::vec3(1.f, 0.f, 0.f),
			glm::vec3(scaleFactor),
			glm::vec4(5.f * glm::rgbColor(glm::vec3(PseudorandMinMax(0.f, 255.f), 1.f, 1.f)), 1.f),
			-1,
		});
	}

	///Create coins
	for(short i = 0; i < 20; ++i){
		const float scaleFactor = 15.f;
		const float xPos = PseudorandMinMax(-terrainXScale / 2.f + 5.f + scaleFactor, terrainXScale / 2.f - 5.f - scaleFactor);
		const float zPos = PseudorandMinMax(-terrainZScale / 2.f + 5.f + scaleFactor, terrainZScale / 2.f - 5.f - scaleFactor);
		const glm::vec3 pos = glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale) + scaleFactor, zPos);
		CreateCoin({
			pos,
			glm::vec3(1.f, 0.f, 0.f),
			glm::vec3(scaleFactor),
			glm::vec4(5.f * glm::rgbColor(glm::vec3(PseudorandMinMax(0.f, 255.f), 1.f, 1.f)), .99f),
			-1,
		});

		ISound* music = soundEngine->play3D("Audio/Music/Spin.mp3", vec3df(pos.x, pos.y, pos.z), true, true, true, ESM_AUTO_DETECT, true);
		if(music){
			music->setMinDistance(3.f);
			music->setVolume(5);

			ISoundEffectControl* soundFX = music->getSoundEffectControl();
			if(!soundFX){
				(void)puts("No soundFX support!\n");
			}
			coinSoundFX.emplace_back(soundFX);

			coinMusic.emplace_back(music);
		} else{
			(void)puts("Failed to init music!\n");
		}
	}

	///Create fires
	for(short i = 0; i < 20; ++i){
		const float scaleFactor = 15.f;
		const float xPos = PseudorandMinMax(-terrainXScale / 2.f + 5.f + scaleFactor, terrainXScale / 2.f - 5.f - scaleFactor);
		const float zPos = PseudorandMinMax(-terrainZScale / 2.f + 5.f + scaleFactor, terrainZScale / 2.f - 5.f - scaleFactor);
		const glm::vec3 pos = glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale) + scaleFactor, zPos);
		CreateFire({
			pos,
			glm::vec3(1.f, 0.f, 0.f),
			glm::vec3(scaleFactor),
			glm::vec4(5.f * glm::rgbColor(glm::vec3(PseudorandMinMax(0.f, 255.f), 1.f, 1.f)), .99f),
			-1,
		});

		ISound* music = soundEngine->play3D("Audio/Music/Burn.wav", vec3df(pos.x, pos.y, pos.z), true, true, true, ESM_AUTO_DETECT, true);
		if(music){
			music->setMinDistance(3.f);
			music->setVolume(5);

			ISoundEffectControl* soundFX = music->getSoundEffectControl();
			if(!soundFX){
				(void)puts("No soundFX support!\n");
			}
			fireSoundFX.emplace_back(soundFX);

			fireMusic.emplace_back(music);
		} else{
			(void)puts("Failed to init music!\n");
		}
	}

	///Create enemies
	for(short i = 0; i < 10; ++i){
		const float scaleFactor = 20.f;
		const float xPos = PseudorandMinMax(-terrainXScale / 2.f + 5.f + scaleFactor, terrainXScale / 2.f - 5.f - scaleFactor);
		const float zPos = PseudorandMinMax(-terrainZScale / 2.f + 5.f + scaleFactor, terrainZScale / 2.f - 5.f - scaleFactor);
		const glm::vec3 pos = glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale) + scaleFactor + 9.f, zPos);
		CreateEnemy({
			pos,
			glm::vec3(1.f, 0.f, 0.f),
			glm::vec3(scaleFactor),
			glm::vec4(5.f * glm::rgbColor(glm::vec3(PseudorandMinMax(0.f, 255.f), 1.f, 1.f)), 1.f),
			0,
		});
	}

	///Create ammo
	for(short i = 0; i < 30; ++i){
		const float scaleFactor = 10.f;
		const float xPos = PseudorandMinMax(-terrainXScale / 2.f + 5.f + scaleFactor, terrainXScale / 2.f - 5.f - scaleFactor);
		const float zPos = PseudorandMinMax(-terrainZScale / 2.f + 5.f + scaleFactor, terrainZScale / 2.f - 5.f - scaleFactor);
		const glm::vec3 pos = glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale) + scaleFactor + 9.f, zPos);
		switch(PseudorandMinMax(0, 2)){
			case 0:
				CreateShotgunAmmo({
					pos,
					glm::vec3(1.f, 0.f, 0.f),
					glm::vec3(scaleFactor),
					glm::vec4(5.f * glm::vec4(1.f, 0.f, 0.f, 1.f)),
					-1,
				});
				break;
			case 1:
				CreateScarAmmo({
					pos,
					glm::vec3(1.f, 0.f, 0.f),
					glm::vec3(scaleFactor),
					glm::vec4(5.f * glm::vec4(0.f, 1.f, 0.f, 1.f)),
					-1,
				});
				break;
			case 2:
				CreateSniperAmmo({
					pos,
					glm::vec3(1.f, 0.f, 0.f),
					glm::vec3(scaleFactor),
					glm::vec4(5.f * glm::vec4(0.f, 0.f, 1.f, 1.f)),
					-1,
				});
				break;
		}
	}

	///Create pt lights
	for(short i = 0; i < 5; ++i){
		for(short j = 0; j < 5; ++j){
			Light* light = CreateLight(LightType::Pt);
			float& x = static_cast<PtLight*>(light)->pos.x;
			float& z = static_cast<PtLight*>(light)->pos.z;
			x = -terrainXScale / 2.f + (terrainXScale / 24.f) * (float(i) * 5.f + float(j));
			z = -terrainZScale / 2.f + (terrainZScale / 24.f) * (float(j) * 5.f + float(i));
			static_cast<PtLight*>(light)->pos.y = terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(x / terrainXScale, z / terrainZScale) + 5.f;
			light->diffuse = 50.f * glm::vec3(PseudorandMinMax(0.f, 1.f), PseudorandMinMax(0.f, 1.f), PseudorandMinMax(0.f, 1.f));
			ptLights.emplace_back(light);
		}
	}

	return true;
}

void Scene::Update(GLFWwindow* const& win){
	elapsedTime += dt;
	if(winHeight){ //Avoid division by 0 when win is minimised
		cam.SetDefaultAspectRatio(float(winWidth) / float(winHeight));
		cam.ResetAspectRatio();
	}

	POINT mousePos;
	if(GetCursorPos(&mousePos)){
		HWND hwnd = ::GetActiveWindow();
		(void)ScreenToClient(hwnd, &mousePos);
	} else{
		(void)puts("Failed to get mouse pos relative to screen!");
	}
	static float buttonBT = 0.f;

	switch(screen){
		case Screen::GameOver:
		case Screen::MainMenu: {
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			cam.SetPos(glm::vec3(0.f, 0.f, 5.f));
			cam.SetTarget(glm::vec3(0.f));
			cam.SetUp(glm::vec3(0.f, 1.f, 0.f));
			view = cam.LookAt();
			projection = glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f);

			if(mousePos.x >= 25.f && mousePos.x <= (screen == Screen::MainMenu ? 100.f : 230.f) && mousePos.y >= winHeight - 160.f && mousePos.y <= winHeight - 125.f){
				if(textScaleFactors[0] != 1.1f){
					soundEngine->play2D("Audio/Sounds/Pop.flac", false);
					textScaleFactors[0] = 1.1f;
					textColours[0] = glm::vec4(1.f, 1.f, 0.f, 1.f);
				}
				if(leftRightMB > 0.f && buttonBT <= elapsedTime){
					soundEngine->play2D("Audio/Sounds/Select.wav", false);
					timeLeft = 60.f;
					score = 0;
					playerHealth = playerMaxHealth;
					playerLives = playerMaxLives;
					if(guns[0]){
						delete guns[0];
						guns[0] = nullptr;
					}
					guns[0] = new Shotgun();
					if(guns[1]){
						delete guns[1];
						guns[1] = nullptr;
					}
					guns[1] = new Scar();
					if(guns[2]){
						delete guns[2];
						guns[2] = nullptr;
					}
					guns[2] = new Sniper();
					screen = Screen::Game;
					cam.SetPos(glm::vec3(0.f));
					cam.SetTarget(glm::vec3(0.f, 0.f, -1.f));
					cam.SetUp(glm::vec3(0.f, 1.f, 0.f));
					buttonBT = elapsedTime + .3f;
				}
			} else{
				textScaleFactors[0] = 1.f;
				textColours[0] = glm::vec4(1.f);
			}
			if(mousePos.x >= 25.f && mousePos.x <= 230.f && mousePos.y >= winHeight - 110.f && mousePos.y <= winHeight - 75.f){
				if(textScaleFactors[1] != 1.1f){
					soundEngine->play2D("Audio/Sounds/Pop.flac", false);
					textScaleFactors[1] = 1.1f;
					textColours[1] = glm::vec4(1.f, 1.f, 0.f, 1.f);
				}
				if(leftRightMB > 0.f && buttonBT <= elapsedTime){
					soundEngine->play2D("Audio/Sounds/Select.wav", false);
					screen = Screen::Scoreboard;
					buttonBT = elapsedTime + .3f;
				}
			} else{
				textScaleFactors[1] = 1.f;
				textColours[1] = glm::vec4(1.f);
			}
			if(mousePos.x >= 25.f && mousePos.x <= 100.f && mousePos.y >= winHeight - 60.f && mousePos.y <= winHeight - 25.f){
				if(textScaleFactors[2] != 1.1f){
					soundEngine->play2D("Audio/Sounds/Pop.flac", false);
					textScaleFactors[2] = 1.1f;
					textColours[2] = glm::vec4(1.f, 1.f, 0.f, 1.f);
				}
				if(leftRightMB > 0.f && buttonBT <= elapsedTime){
					soundEngine->play2D("Audio/Sounds/Select.wav", false);
					endLoop = true;
					buttonBT = elapsedTime + .3f;
				}
			} else{
				textScaleFactors[2] = 1.f;
				textColours[2] = glm::vec4(1.f);
			}

			break;
		}
		case Screen::Game: {
			if(playerHealth <= 0.f){
				--playerLives;
				playerHealth = playerMaxHealth;
			}
			if(playerLives <= 0){
				timeLeft = 0.f;
			} else{
				timeLeft -= dt;
			}
			if(timeLeft <= 0.f){
				screen = Screen::GameOver;
				const size_t& mySize = scores.size();
				if(mySize == 5){ //Max no. of scores saved
					std::sort(scores.begin(), scores.end(), std::greater<int>());
					if(score > scores.back()){
						scores.pop_back();
						scores.emplace_back(score);
					}
					std::sort(scores.begin(), scores.end(), std::greater<int>());
				} else{
					scores.emplace_back(score);
					std::sort(scores.begin(), scores.end(), std::greater<int>());
				}
			}
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			////Control player states
			static float sprintBT = 0.f;
			static float heightBT = 0.f;

			///Toggle sprint
			if(Key(VK_SHIFT) && sprintBT <= elapsedTime){
				sprintOn = !sprintOn;
				sprintBT = elapsedTime + .5f;
			}

			///Set movement state
			if(Key(GLFW_KEY_A) || Key(GLFW_KEY_D) || Key(GLFW_KEY_W) || Key(GLFW_KEY_S)){
				if(sprintOn){
					playerStates &= ~(int)PlayerState::NoMovement;
					playerStates &= ~(int)PlayerState::Walking;
					playerStates |= (int)PlayerState::Sprinting;
				} else{
					playerStates &= ~(int)PlayerState::NoMovement;
					playerStates |= (int)PlayerState::Walking;
					playerStates &= ~(int)PlayerState::Sprinting;
				}
			} else{
				playerStates |= (int)PlayerState::NoMovement;
				playerStates &= ~(int)PlayerState::Walking;
				playerStates &= ~(int)PlayerState::Sprinting;
			}

			///Set height state
			if(heightBT <= elapsedTime){
				if(Key(GLFW_KEY_C)){
					if(playerStates & (int)PlayerState::Standing){
						playerStates |= (int)PlayerState::Crouching;
						playerStates &= ~(int)PlayerState::Standing;
					} else if(playerStates & (int)PlayerState::Crouching){
						playerStates |= (int)PlayerState::Proning;
						playerStates &= ~(int)PlayerState::Crouching;
					}
					heightBT = elapsedTime + .5f;
				}
				if(Key(VK_SPACE)){
					if(playerStates & (int)PlayerState::Proning){
						playerStates |= (int)PlayerState::Crouching;
						playerStates &= ~(int)PlayerState::Proning;
					} else if(playerStates & (int)PlayerState::Crouching){
						playerStates |= (int)PlayerState::Standing;
						playerStates &= ~(int)PlayerState::Crouching;
					} else if(playerStates & (int)PlayerState::Standing){
						soundEngine->play2D("Audio/Sounds/Jump.wav", false);
						playerStates |= (int)PlayerState::Jumping;
						playerStates &= ~(int)PlayerState::Standing;
					}
					heightBT = elapsedTime + .5f;
				} else{
					if((playerStates & (int)PlayerState::Jumping)){
						playerStates |= (int)PlayerState::Falling;
						playerStates &= ~(int)PlayerState::Jumping;
						cam.SetVel(0.f);
					}
				}
			}

			float yMin = terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(cam.GetPos().x / terrainXScale, cam.GetPos().z / terrainZScale);
			float yMax = yMin;

			///Update player according to its states
			int playerStatesTemp = playerStates;
			int bitMask = 1;
			while(playerStatesTemp){
				switch(PlayerState(playerStatesTemp & bitMask)){
					case PlayerState::NoMovement:
						cam.SetSpd(0.f);
						break;
					case PlayerState::Walking:
						cam.SetSpd(100.f);
						break;
					case PlayerState::Sprinting:
						cam.SetSpd(250.f);
						break;
					case PlayerState::Standing:
						yMin += 30.f;
						yMax += 30.f;
						break;
					case PlayerState::Jumping:
						cam.SetVel(300.f);
					case PlayerState::Falling:
						cam.SetAccel(-1500.f);
						yMin += 30.f;
						yMax += 250.f;
						break;
					case PlayerState::Crouching:
						cam.SetSpd(cam.GetSpd() / 5.f);
						yMin += 5.f;
						yMax += 5.f;
						break;
					case PlayerState::Proning:
						cam.SetSpd(5.f);
						yMin += 1.f;
						yMax += 1.f;
						break;
				}
				playerStatesTemp &= ~bitMask;
				bitMask <<= 1;
			}

			if(playerStates & (int)PlayerState::Jumping){
				if(cam.GetPos().y >= yMax){
					playerStates |= (int)PlayerState::Falling;
					playerStates &= ~(int)PlayerState::Jumping;
					cam.SetVel(0.f);
				}
			}
			if(playerStates & (int)PlayerState::Falling){
				if(cam.GetPos().y <= yMin){
					playerStates |= (int)PlayerState::Standing;
					playerStates &= ~(int)PlayerState::Falling;
					cam.SetAccel(0.f);
					cam.SetVel(0.f);
				}
			}
			cam.UpdateJumpFall();
			cam.Update(GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_W, GLFW_KEY_S, -terrainXScale / 2.f + 5.f, terrainXScale / 2.f - 5.f, yMin, yMax, -terrainZScale / 2.f + 5.f, terrainZScale / 2.f - 5.f);
			view = cam.LookAt();

			///Control FOV of perspective projection based on item selected in inv
			if(RMB){
				switch(inv[currSlot]){
					case ItemType::Shotgun:
						angularFOV = 40.f;
						break;
					case ItemType::Scar:
						angularFOV = 30.f;
						break;
					case ItemType::Sniper:
						if(angularFOV != 15.f){
							soundEngine->play2D("Audio/Sounds/Scope.wav", false);
						}
						angularFOV = 15.f;
						break;
				}
			} else{
				angularFOV = 45.f;
			}
			projection = glm::perspective(glm::radians(angularFOV), cam.GetAspectRatio(), .1f, 9999.f);

			const glm::vec3& camPos = cam.GetPos();
			const glm::vec3& camFront = cam.CalcFront();
			soundEngine->setListenerPosition(vec3df(camPos.x, camPos.y, camPos.z), vec3df(camFront.x, camFront.y, camFront.z));

			static_cast<SpriteAni*>(meshes[(int)MeshType::CoinSpriteAni])->Update();
			static_cast<SpriteAni*>(meshes[(int)MeshType::FireSpriteAni])->Update();

			static float polyModeBT = 0.f;
			static float minimapViewBT = 0.f;
			static float distortionBT = 0.f;
			static float echoBT = 0.f;
			static float wavesReverbBT = 0.f;
			static float resetSoundFXBT = 0.f;

			///Track changing of inv slots
			int keys[]{
				GLFW_KEY_1,
				GLFW_KEY_2,
				GLFW_KEY_3,
				GLFW_KEY_4,
				GLFW_KEY_5,
			};
			for(int i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i){
				if(Key(keys[i])){
					currSlot = keys[i] - 49;
					break;
				}
			}
			switch(inv[currSlot]){
				case ItemType::Shotgun:
				case ItemType::Scar:
				case ItemType::Sniper:
					currGun = guns[int(inv[currSlot])];
					break;
				default:
					currGun = nullptr;
			}

			GLint polyMode;
			glGetIntegerv(GL_POLYGON_MODE, &polyMode);
			if(Key(VK_F2) && polyModeBT <= elapsedTime){
				glPolygonMode(GL_FRONT_AND_BACK, polyMode + (polyMode == GL_FILL ? -2 : 1));
				polyModeBT = elapsedTime + .5f;
			}

			if(Key(VK_F3) && minimapViewBT <= elapsedTime){
				minimapView = MinimapViewType((int)minimapView + 1);
				if(minimapView == MinimapViewType::Amt){
					minimapView = MinimapViewType(0);
				}
				minimapViewBT = elapsedTime + .5f;
			}

			if(score < 0){
				score = 0;
			}

			///Control soundFX
			const size_t& coinSoundFXSize = coinSoundFX.size();
			for(size_t i = 0; i < coinSoundFXSize; ++i){
				ISoundEffectControl* soundFX = coinSoundFX[i];
				if(soundFX){
					if(Key(KEY_I) && distortionBT <= elapsedTime){
						soundFX->isDistortionSoundEffectEnabled() ? soundFX->disableDistortionSoundEffect() : (void)soundFX->enableDistortionSoundEffect();
						distortionBT = elapsedTime + .5f;
					}
					if(Key(KEY_O) && echoBT <= elapsedTime){
						soundFX->isEchoSoundEffectEnabled() ? soundFX->disableEchoSoundEffect() : (void)soundFX->enableEchoSoundEffect();
						echoBT = elapsedTime + .5f;
					}
					if(Key(KEY_P) && wavesReverbBT <= elapsedTime){
						soundFX->isWavesReverbSoundEffectEnabled() ? soundFX->disableWavesReverbSoundEffect() : (void)soundFX->enableWavesReverbSoundEffect();
						wavesReverbBT = elapsedTime + .5f;
					}
					if(Key(KEY_L) && resetSoundFXBT <= elapsedTime){
						soundFX->disableAllEffects();
						resetSoundFXBT = elapsedTime + .5f;
					}
				}
			}
			const size_t& fireSoundFXSize = fireSoundFX.size();
			for(size_t i = 0; i < fireSoundFXSize; ++i){
				ISoundEffectControl* soundFX = fireSoundFX[i];
				if(soundFX){
					if(Key(KEY_I) && distortionBT <= elapsedTime){
						soundFX->isDistortionSoundEffectEnabled() ? soundFX->disableDistortionSoundEffect() : (void)soundFX->enableDistortionSoundEffect();
						distortionBT = elapsedTime + .5f;
					}
					if(Key(KEY_O) && echoBT <= elapsedTime){
						soundFX->isEchoSoundEffectEnabled() ? soundFX->disableEchoSoundEffect() : (void)soundFX->enableEchoSoundEffect();
						echoBT = elapsedTime + .5f;
					}
					if(Key(KEY_P) && wavesReverbBT <= elapsedTime){
						soundFX->isWavesReverbSoundEffectEnabled() ? soundFX->disableWavesReverbSoundEffect() : (void)soundFX->enableWavesReverbSoundEffect();
						wavesReverbBT = elapsedTime + .5f;
					}
					if(Key(KEY_L) && resetSoundFXBT <= elapsedTime){
						soundFX->disableAllEffects();
						resetSoundFXBT = elapsedTime + .5f;
					}
				}
			}

			///Control shooting and reloading of currGun
			if(currGun){
				if(LMB){
					currGun->Shoot(elapsedTime, FetchEntity(), cam.GetPos(), cam.CalcFront(), soundEngine);
				}
				if(Key(GLFW_KEY_R)){
					currGun->Reload(soundEngine);
				}
				currGun->Update();
			}
			UpdateEntities();

			break;
		}
		case Screen::Scoreboard: {
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			cam.SetPos(glm::vec3(0.f, 0.f, 5.f));
			cam.SetTarget(glm::vec3(0.f));
			cam.SetUp(glm::vec3(0.f, 1.f, 0.f));
			view = cam.LookAt();
			projection = glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f);

			if(mousePos.x >= 25.f && mousePos.x <= 110.f && mousePos.y >= winHeight - 60.f && mousePos.y <= winHeight - 25.f){
				if(textScaleFactors[2] != 1.1f){
					soundEngine->play2D("Audio/Sounds/Pop.flac", false);
					textScaleFactors[2] = 1.1f;
					textColours[2] = glm::vec4(1.f, 1.f, 0.f, 1.f);
				}
				if(leftRightMB > 0.f && buttonBT <= elapsedTime){
					soundEngine->play2D("Audio/Sounds/Select.wav", false);
					screen = Screen::MainMenu;
					buttonBT = elapsedTime + .3f;
				}
			} else{
				textScaleFactors[2] = 1.f;
				textColours[2] = glm::vec4(1.f);
			}

			break;
		}
	}
}

void Scene::CreateShotgunAmmo(const EntityCreationAttribs& attribs){
	Entity* const& entity = FetchEntity();

	entity->type = Entity::EntityType::ShotgunAmmo;
	entity->active = true;
	entity->life = 0.f;
	entity->maxLife = 0.f;
	entity->colour = attribs.colour;
	entity->diffuseTexIndex = attribs.diffuseTexIndex;
	entity->collisionNormal = attribs.collisionNormal;
	entity->scale = attribs.scale;

	entity->pos = attribs.pos;
	entity->vel = glm::vec3(0.f);
	entity->mass = 5.f;
	entity->force = glm::vec3(0.f);
}

void Scene::CreateScarAmmo(const EntityCreationAttribs& attribs){
	Entity* const& entity = FetchEntity();

	entity->type = Entity::EntityType::ScarAmmo;
	entity->active = true;
	entity->life = 0.f;
	entity->maxLife = 0.f;
	entity->colour = attribs.colour;
	entity->diffuseTexIndex = attribs.diffuseTexIndex;
	entity->collisionNormal = attribs.collisionNormal;
	entity->scale = attribs.scale;

	entity->pos = attribs.pos;
	entity->vel = glm::vec3(0.f);
	entity->mass = 5.f;
	entity->force = glm::vec3(0.f);
}

void Scene::CreateSniperAmmo(const EntityCreationAttribs& attribs){
	Entity* const& entity = FetchEntity();

	entity->type = Entity::EntityType::SniperAmmo;
	entity->active = true;
	entity->life = 0.f;
	entity->maxLife = 0.f;
	entity->colour = attribs.colour;
	entity->diffuseTexIndex = attribs.diffuseTexIndex;
	entity->collisionNormal = attribs.collisionNormal;
	entity->scale = attribs.scale;

	entity->pos = attribs.pos;
	entity->vel = glm::vec3(0.f);
	entity->mass = 5.f;
	entity->force = glm::vec3(0.f);
}

void Scene::CreateCoin(const EntityCreationAttribs& attribs){
	Entity* const& entity = FetchEntity();

	entity->type = Entity::EntityType::Coin;
	entity->active = true;
	entity->life = 0.f;
	entity->maxLife = 0.f;
	entity->colour = attribs.colour;
	entity->diffuseTexIndex = attribs.diffuseTexIndex;
	entity->collisionNormal = attribs.collisionNormal;
	entity->scale = attribs.scale;

	entity->pos = attribs.pos;
	entity->vel = glm::vec3(0.f);
	entity->mass = 5.f;
	entity->force = glm::vec3(0.f);
}

void Scene::CreateFire(const EntityCreationAttribs& attribs){
	Entity* const& entity = FetchEntity();

	entity->type = Entity::EntityType::Fire;
	entity->active = true;
	entity->life = 0.f;
	entity->maxLife = 0.f;
	entity->colour = attribs.colour;
	entity->diffuseTexIndex = attribs.diffuseTexIndex;
	entity->collisionNormal = attribs.collisionNormal;
	entity->scale = attribs.scale;

	entity->pos = attribs.pos;
	entity->vel = glm::vec3(0.f);
	entity->mass = 5.f;
	entity->force = glm::vec3(0.f);
}

void Scene::CreateEnemy(const EntityCreationAttribs& attribs){
	Entity* const& entity = FetchEntity();

	entity->type = Entity::EntityType::Enemy;
	entity->active = true;
	entity->life = 100.f;
	entity->maxLife = 100.f;
	entity->colour = attribs.colour;
	entity->diffuseTexIndex = attribs.diffuseTexIndex;
	entity->collisionNormal = attribs.collisionNormal;
	entity->scale = attribs.scale;

	entity->pos = attribs.pos;
	entity->vel = glm::vec3(0.f);
	entity->mass = 5.f;
	entity->force = glm::vec3(0.f);
}

void Scene::CreateSphere(const EntityCreationAttribs& attribs){
	Entity* const& entity = FetchEntity();

	entity->type = Entity::EntityType::Sphere;
	entity->active = true;
	entity->life = 0.f;
	entity->maxLife = 0.f;
	entity->colour = attribs.colour;
	entity->diffuseTexIndex = attribs.diffuseTexIndex;
	entity->collisionNormal = attribs.collisionNormal;
	entity->scale = attribs.scale;

	entity->pos = attribs.pos;
	entity->vel = glm::vec3(0.f);
	entity->mass = 5.f;
	entity->force = glm::vec3(0.f);
}

void Scene::CreateThinWall(const EntityCreationAttribs& attribs){
	Entity* const& entity = FetchEntity();

	entity->type = Entity::EntityType::Wall;
	entity->active = true;
	entity->life = 0.f;
	entity->maxLife = 0.f;
	entity->colour = attribs.colour;
	entity->diffuseTexIndex = attribs.diffuseTexIndex;
	entity->collisionNormal = attribs.collisionNormal;
	entity->scale = attribs.scale;

	entity->pos = attribs.pos;
	entity->vel = glm::vec3(0.f);
	entity->mass = 5.f;
	entity->force = glm::vec3(0.f);
}

void Scene::CreatePillar(const EntityCreationAttribs& attribs){
	Entity* const& entity = FetchEntity();

	entity->type = Entity::EntityType::Pillar;
	entity->active = true;
	entity->life = 0.f;
	entity->maxLife = 0.f;
	entity->colour = attribs.colour;
	entity->diffuseTexIndex = attribs.diffuseTexIndex;
	entity->collisionNormal = attribs.collisionNormal;
	entity->scale = attribs.scale;

	entity->pos = attribs.pos;
	entity->vel = glm::vec3(0.f);
	entity->mass = 5.f;
	entity->force = glm::vec3(0.f);
}

void Scene::CreateThickWall(const EntityCreationAttribs& attribs){
	const glm::vec3 NP = glm::vec3(-attribs.collisionNormal.z, attribs.collisionNormal.y, attribs.collisionNormal.x);
	CreateThinWall({
		attribs.pos,
		attribs.collisionNormal,
		attribs.scale,
		attribs.colour,
		attribs.diffuseTexIndex,
	});
	CreateThinWall({
		attribs.pos,
		NP,
		glm::vec3(attribs.scale.y, attribs.scale.x, attribs.scale.z),
		attribs.colour,
		-2,
	});
	CreatePillar({
		attribs.pos + NP * attribs.scale.y + attribs.collisionNormal * attribs.scale.x,
		glm::vec3(0.f),
		glm::vec3(glm::vec2(.005f), attribs.scale.z),
		attribs.colour,
		-2,
	});
	CreatePillar({
		attribs.pos + -NP * attribs.scale.y + -attribs.collisionNormal * attribs.scale.x,
		glm::vec3(0.f),
		glm::vec3(glm::vec2(.005f), attribs.scale.z),
		attribs.colour,
		-2,
	});
	CreatePillar({
		attribs.pos + -NP * attribs.scale.y + attribs.collisionNormal * attribs.scale.x,
		glm::vec3(0.f),
		glm::vec3(glm::vec2(.005f), attribs.scale.z),
		attribs.colour,
		-2,
	});
	CreatePillar({
		attribs.pos + NP * attribs.scale.y + -attribs.collisionNormal * attribs.scale.x,
		glm::vec3(0.f),
		glm::vec3(glm::vec2(.005f), attribs.scale.z),
		attribs.colour,
		-2,
	});
}

void Scene::UpdateCollisionBetweenEntities(Entity* const& entity1, Entity* const& entity2){ //For collision between bullets and enemies
	const glm::vec3& displacementVec = entity2->pos - entity1->pos;
	if(glm::dot(-displacementVec, -displacementVec) <= (entity1->scale.x + entity2->scale.x) * (entity1->scale.y + entity2->scale.y)){
		entity1->active = false;
		if(entity1->colour == glm::vec4(glm::vec3(.4f), .3f)){ //If shotgun bullet...
			entity2->life -= 50.f * float(glm::length(cam.GetPos() - entity2->pos) <= 150.f);
		} else if(entity1->colour == glm::vec4(glm::vec3(1.f), .3f)){ //If scar bullet...
			entity2->life -= 5.f;
		} else{ //If sniper bullet...
			entity2->life = 0.f;
		}
		if(entity2->life <= 0.f){
			entity2->active = false;
			score += 200;
		}
	}
}

void Scene::UpdateEntities(){
	static float burnBT = 0.f;
	reticleColour = glm::vec4(1.f);
	cam.canMove = true;

	for(size_t i = 0; i < entityPool.size(); ++i){
		Entity* const& entity = entityPool[i];
		if(entity && entity->active){
			switch(entity->type){
				case Entity::EntityType::Enemy: {
					///Enemy movement
					entity->pos.y += float(sin(glfwGetTime())) / 5.f;
					if(glm::length(cam.GetPos() - entity->pos) >= 50.f){
						entity->vel = glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(PseudorandMinMax(-10.f, 10.f)), {0.f, 1.f, 0.f}) *
							glm::vec4(glm::normalize((cam.GetPos() - entity->pos)), 0.f)) * 20.f;
					} else{
						entity->vel = glm::vec3(0.f);
					}

					///Ray casting to check for Ray-Sphere intersection
					glm::vec3 vec = cam.GetPos() - entity->pos;
					const float b = glm::dot(cam.CalcFront(), vec);
					const float c = glm::dot(vec, vec) - entity->scale.x * entity->scale.x;
					if(b * b - c >= 0.f){
						reticleColour = glm::vec4(1.f, 0.f, 0.f, 1.f);
					}

					///Check for collision with cam
					const glm::vec3& relativeVel = cam.trueVel;
					const glm::vec3& displacementVec = entity->pos - cam.GetPos();
					if(glm::dot(relativeVel, displacementVec) > 0.f && glm::dot(-displacementVec, -displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						cam.canMove = false;
						playerHealth -= 2.f;
					}
					break;
				}
				case Entity::EntityType::Bullet: {
					entity->life -= dt;
					if(entity->life < 0.f){
						entity->active = false;
					}
					break;
				}
				case Entity::EntityType::Coin: {
					///Check for collision with cam
					const glm::vec3& displacementVec = entity->pos - cam.GetPos();
					if(glm::dot(-displacementVec, -displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						soundEngine->play2D("Audio/Sounds/Collect.wav", false);
						entity->active = false;
						score += 20;
					}
					break;
				}
				case Entity::EntityType::Fire: {
					const glm::vec3& displacementVec = entity->pos - cam.GetPos();
					if(glm::dot(-displacementVec, -displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						if(burnBT <= elapsedTime){
							playerHealth -= playerMaxHealth / 10.f;
							burnBT = elapsedTime + .5f;
						}
					}
					break;
				}
				case Entity::EntityType::Sphere: {
					///Check for collision with cam
					const glm::vec3& relativeVel = cam.trueVel;
					const glm::vec3& displacementVec = entity->pos - cam.GetPos();
					if(glm::dot(relativeVel, displacementVec) > 0.f && glm::dot(-displacementVec, -displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						cam.canMove = false;
					}
					break;
				}
				case Entity::EntityType::ShotgunAmmo: {
					glm::vec3 N = entity->collisionNormal;
					const glm::vec3 displacement = entity->pos - cam.GetPos();
					if(glm::dot(displacement, N) < 0.f){
						N = -N;
					}

					const glm::vec3& camPos = cam.GetPos();
					const float halfWidth = entity->scale.x;
					const float halfHeight = entity->scale.y;
					const float halfDepth = entity->scale.z;
					glm::vec3 closestPt;
					closestPt.x = std::max(entity->pos.x - halfWidth, std::min(camPos.x, entity->pos.x + halfWidth));
					closestPt.y = std::max(entity->pos.y - halfHeight, std::min(camPos.y, entity->pos.y + halfHeight));
					closestPt.z = std::max(entity->pos.z - halfDepth, std::min(camPos.z, entity->pos.z + halfDepth));
					if(glm::dot(cam.trueVel, N) > 0.f && glm::length(closestPt - camPos) < 20.f){
						entity->active = false;
						if(guns[0]){
							guns[0]->SetUnloadedBullets(guns[0]->GetUnloadedBullets() + guns[0]->GetMaxLoadedBullets() / 2);
							guns[0]->SetUnloadedBullets(std::min(guns[0]->GetUnloadedBullets(), guns[0]->GetMaxUnloadedBullets()));
						}
					}
					break;
				}
				case Entity::EntityType::ScarAmmo: {
					glm::vec3 N = entity->collisionNormal;
					const glm::vec3 displacement = entity->pos - cam.GetPos();
					if(glm::dot(displacement, N) < 0.f){
						N = -N;
					}

					const glm::vec3& camPos = cam.GetPos();
					const float halfWidth = entity->scale.x;
					const float halfHeight = entity->scale.y;
					const float halfDepth = entity->scale.z;
					glm::vec3 closestPt;
					closestPt.x = std::max(entity->pos.x - halfWidth, std::min(camPos.x, entity->pos.x + halfWidth));
					closestPt.y = std::max(entity->pos.y - halfHeight, std::min(camPos.y, entity->pos.y + halfHeight));
					closestPt.z = std::max(entity->pos.z - halfDepth, std::min(camPos.z, entity->pos.z + halfDepth));
					if(glm::dot(cam.trueVel, N) > 0.f && glm::length(closestPt - camPos) < 20.f){
						entity->active = false;
						if(guns[1]){
							guns[1]->SetUnloadedBullets(guns[1]->GetUnloadedBullets() + guns[1]->GetMaxLoadedBullets());
							guns[1]->SetUnloadedBullets(std::min(guns[1]->GetUnloadedBullets(), guns[1]->GetMaxUnloadedBullets()));
						}
					}
					break;
				}
				case Entity::EntityType::SniperAmmo: {
					glm::vec3 N = entity->collisionNormal;
					const glm::vec3 displacement = entity->pos - cam.GetPos();
					if(glm::dot(displacement, N) < 0.f){
						N = -N;
					}

					const glm::vec3& camPos = cam.GetPos();
					const float halfWidth = entity->scale.x;
					const float halfHeight = entity->scale.y;
					const float halfDepth = entity->scale.z;
					glm::vec3 closestPt;
					closestPt.x = std::max(entity->pos.x - halfWidth, std::min(camPos.x, entity->pos.x + halfWidth));
					closestPt.y = std::max(entity->pos.y - halfHeight, std::min(camPos.y, entity->pos.y + halfHeight));
					closestPt.z = std::max(entity->pos.z - halfDepth, std::min(camPos.z, entity->pos.z + halfDepth));
					if(glm::dot(cam.trueVel, N) > 0.f && glm::length(closestPt - camPos) < 20.f){
						entity->active = false;
						if(guns[2]){
							guns[2]->SetUnloadedBullets(guns[2]->GetUnloadedBullets() + guns[2]->GetMaxLoadedBullets() * 2);
							guns[2]->SetUnloadedBullets(std::min(guns[2]->GetUnloadedBullets(), guns[2]->GetMaxUnloadedBullets()));
						}
					}
					break;
				}
				case Entity::EntityType::Wall: {
					glm::vec3 N = entity->collisionNormal;
					const glm::vec3 displacement = entity->pos - cam.GetPos();
					if(glm::dot(displacement, N) < 0.f){
						N = -N;
					}

					const glm::vec3& camPos = cam.GetPos();
					const float halfWidth = entity->scale.x;
					const float halfHeight = entity->scale.y;
					const float halfDepth = entity->scale.z;
					glm::vec3 closestPt;
					closestPt.x = std::max(entity->pos.x - halfWidth, std::min(camPos.x, entity->pos.x + halfWidth));
					closestPt.y = std::max(entity->pos.y - halfHeight, std::min(camPos.y, entity->pos.y + halfHeight));
					closestPt.z = std::max(entity->pos.z - halfDepth, std::min(camPos.z, entity->pos.z + halfDepth));
					if(glm::dot(cam.trueVel, N) > 0.f && glm::length(closestPt - camPos) < 20.f){
						cam.canMove = false;
					}
					break;
				}
			}

			for(size_t j = 0; j < entityPool.size(); ++j){
				Entity* const& instance = entityPool[j];
				if(instance && instance->active){
					if(entity->type == Entity::EntityType::Bullet && instance->type == Entity::EntityType::Enemy){
						UpdateCollisionBetweenEntities(entity, instance);
					} else if(entity->type == Entity::EntityType::Enemy && instance->type == Entity::EntityType::Bullet){
						UpdateCollisionBetweenEntities(instance, entity);
					} else{
						continue;
					}
				}
			}
			entity->pos += entity->vel * dt;
		}
	}
}

void Scene::RenderEntities(ShaderProg& SP, const bool& opaque){
	SP.Use();
	const size_t& mySize = entityPool.size();
	for(size_t i = 0; i < mySize; ++i){
		Entity* const& entity = entityPool[i];
		if(entity && entity->active && (opaque ? (entity->colour.a == 1.f) : (entity->colour.a != 1.f)) && entity->diffuseTexIndex != -2){
			SP.Set1i("noNormals", 1);
			SP.Set1i("useCustomColour", 1);
			SP.Set4fv("customColour", entity->colour);
			SP.Set1i("useCustomDiffuseTexIndex", 1);
			SP.Set1i("customDiffuseTexIndex", entity->diffuseTexIndex);
			switch(entity->type){
				case Entity::EntityType::Bullet:
				case Entity::EntityType::Sphere:
				case Entity::EntityType::Enemy:
					PushModel({
						Translate(entity->pos),
						Scale(entity->scale),
					});
					meshes[(int)MeshType::Sphere]->SetModel(GetTopModel());
					meshes[(int)MeshType::Sphere]->Render(SP);
					break;
				case Entity::EntityType::ShotgunAmmo:
				case Entity::EntityType::ScarAmmo:
				case Entity::EntityType::SniperAmmo:
				case Entity::EntityType::Wall:
					PushModel({
						Translate(entity->pos),
						//Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(entity->collisionNormal.z, entity->collisionNormal.x)))),
						Scale(entity->scale),
					});
					meshes[(int)MeshType::Cube]->SetModel(GetTopModel());
					meshes[(int)MeshType::Cube]->Render(SP);
					break;
				case Entity::EntityType::Coin:
					PushModel({
						Translate(entity->pos),
						Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(cam.GetPos().x - entity->pos.x, cam.GetPos().z - entity->pos.z)))),
						Scale(entity->scale),
					});
					SP.Set1i("useCustomColour", 0);
					SP.Set1i("useCustomDiffuseTexIndex", 0);
					meshes[(int)MeshType::CoinSpriteAni]->SetModel(GetTopModel());
					meshes[(int)MeshType::CoinSpriteAni]->Render(SP);
					break;
				case Entity::EntityType::Fire:
					PushModel({
						Translate(entity->pos + glm::vec3(0.f, entity->scale.y / 2.f, 0.f)),
						Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(cam.GetPos().x - entity->pos.x, cam.GetPos().z - entity->pos.z)))),
						Scale(glm::vec3(entity->scale.x, entity->scale.y * 2.f, entity->scale.z)),
					});
					SP.Set1i("useCustomColour", 0);
					SP.Set1i("useCustomDiffuseTexIndex", 0);
					meshes[(int)MeshType::FireSpriteAni]->SetModel(GetTopModel());
					meshes[(int)MeshType::FireSpriteAni]->Render(SP);
					break;
			}
			SP.Set1i("useCustomDiffuseTexIndex", 0);
			SP.Set1i("useCustomColour", 0);
			SP.Set1i("noNormals", 0);
			PopModel();
		}
	}
}

void Scene::GeoRenderPass(){
	geoPassSP.Use();

	switch(screen){
		case Screen::Scoreboard:
		case Screen::GameOver:
		case Screen::MainMenu: {
			geoPassSP.SetMat4fv("PV", &(projection * view)[0][0]);

			PushModel({
				Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
			});
				geoPassSP.Set1i("noNormals", 1);
				meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
				meshes[(int)MeshType::Quad]->Render(geoPassSP);
				geoPassSP.Set1i("noNormals", 0);
			PopModel();

			break;
		}
		case Screen::Game: {
			const glm::vec3 OGPos = cam.GetPos();
			const glm::vec3 OGTarget = cam.GetTarget();
			const glm::vec3 OGUp = cam.GetUp();

			cam.SetPos(glm::vec3(0.f, 0.f, 5.f));
			cam.SetTarget(glm::vec3(0.f));
			cam.SetUp(glm::vec3(0.f, 1.f, 0.f));
			view = cam.LookAt();
			projection = glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f);
			geoPassSP.SetMat4fv("PV", &(projection * view)[0][0]);

			if(!(RMB && inv[currSlot] == ItemType::Sniper)){
				///Render healthbar
				PushModel({
					Translate(glm::vec3(-float(winWidth) / 2.5f, float(winHeight) / 2.5f, -10.f)),
					Scale(glm::vec3(float(winWidth) / 15.f, float(winHeight) / 50.f, 1.f)),
				});
					geoPassSP.Set1i("noNormals", 1);
					geoPassSP.Set1i("useCustomColour", 1);
					geoPassSP.Set4fv("customColour", glm::vec4(glm::vec3(1.f, 0.f, 0.f), 1.f));
					geoPassSP.Set1i("useCustomDiffuseTexIndex", 1);
					geoPassSP.Set1i("customDiffuseTexIndex", -1);
						meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
						meshes[(int)MeshType::Quad]->Render(geoPassSP);
					geoPassSP.Set1i("useCustomDiffuseTexIndex", 0);
					geoPassSP.Set1i("useCustomColour", 0);
					geoPassSP.Set1i("noNormals", 0);

					PushModel({
						Translate(glm::vec3(-(playerMaxHealth - playerHealth) / playerMaxHealth, 0.f, 1.f)),
						Scale(glm::vec3(playerHealth / playerMaxHealth, 1.f, 1.f)),
					});
						geoPassSP.Set1i("noNormals", 1);
						geoPassSP.Set1i("useCustomColour", 1);
						geoPassSP.Set4fv("customColour", glm::vec4(glm::vec3(0.f, 1.f, 0.f), 1.f));
						geoPassSP.Set1i("useCustomDiffuseTexIndex", 1);
						geoPassSP.Set1i("customDiffuseTexIndex", -1);
							meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
							meshes[(int)MeshType::Quad]->Render(geoPassSP);
						geoPassSP.Set1i("useCustomDiffuseTexIndex", 0);
						geoPassSP.Set1i("useCustomColour", 0);
						geoPassSP.Set1i("noNormals", 0);
					PopModel();
				PopModel();

				///Render items in inv
				for(short i = 0; i < 5; ++i){
					geoPassSP.Set1i("noNormals", 1);
					PushModel({
						Translate(glm::vec3(float(i) * 100.f - 300.f, -float(winHeight) / 2.3f, -10.f)),
					});
					switch(inv[i]){
						case ItemType::Shotgun:
							PushModel({
								Translate(glm::vec3(18.f, -18.f, 0.f)),
								Rotate(glm::vec4(0.f, 0.f, 1.f, 45.f)),
								Rotate(glm::vec4(0.f, 1.f, 0.f, 90.f)),
								Scale(glm::vec3(21.f)),
							});
								models[(int)ModelType::Shotgun]->SetModelForAll(GetTopModel());
								models[(int)ModelType::Shotgun]->Render(geoPassSP);
							PopModel();
							break;
						case ItemType::Scar:
							PushModel({
								Rotate(glm::vec4(0.f, 0.f, 1.f, 45.f)),
								Rotate(glm::vec4(0.f, 1.f, 0.f, 90.f)),
								Scale(glm::vec3(18.f)),
							});
								models[(int)ModelType::Scar]->SetModelForAll(GetTopModel());
								models[(int)ModelType::Scar]->Render(geoPassSP);
							PopModel();
							break;
						case ItemType::Sniper:
							PushModel({
								Translate(glm::vec3(16.f, -15.f, 0.f)),
								Rotate(glm::vec4(0.f, 0.f, 1.f, 45.f)),
								Scale(glm::vec3(10.f)),
							});
								models[(int)ModelType::Sniper]->SetModelForAll(GetTopModel());
								models[(int)ModelType::Sniper]->Render(geoPassSP);
							PopModel();
							break;
						case ItemType::ShotgunAmmo:
							break;
						case ItemType::ScarAmmo:
							break;
						case ItemType::SniperAmmo:
							break;
						case ItemType::HealthPack:
							break;
					}
					PopModel();
					geoPassSP.Set1i("noNormals", 0);
				}
			}

			cam.SetPos(OGPos);
			cam.SetTarget(OGTarget);
			cam.SetUp(OGUp);
			view = cam.LookAt();
			projection = glm::perspective(glm::radians(angularFOV), cam.GetAspectRatio(), .1f, 9999.f);

			geoPassSP.SetMat4fv("PV", &(projection * glm::mat4(glm::mat3(view)))[0][0]);

			glDepthFunc(GL_LEQUAL); //Modify comparison operators used for depth test such that frags with depth <= 1.f are shown
			glCullFace(GL_FRONT);
			geoPassSP.Set1i("sky", 1);
			geoPassSP.Set1i("skybox", 1);
			geoPassSP.UseTex(cubemapRefID, "cubemapSampler", GL_TEXTURE_CUBE_MAP);
				meshes[(int)MeshType::Cube]->SetModel(GetTopModel());
				meshes[(int)MeshType::Cube]->Render(geoPassSP);
			geoPassSP.Set1i("skybox", 0);
			geoPassSP.Set1i("sky", 0);
			glCullFace(GL_BACK);
			glDepthFunc(GL_LESS);

			geoPassSP.SetMat4fv("PV", &(projection * view)[0][0]);

			///Render item held
			const glm::vec3 front = cam.CalcFront();
			const float sign = front.y < 0.f ? -1.f : 1.f;
			switch(inv[currSlot]){
				case ItemType::Shotgun: {
					auto rotationMat = glm::rotate(glm::mat4(1.f), sign * acosf(glm::dot(front, glm::normalize(glm::vec3(front.x, 0.f, front.z)))),
						glm::normalize(glm::vec3(-front.z, 0.f, front.x)));
					PushModel({
						Translate(cam.GetPos() +
							glm::vec3(rotationMat * glm::vec4(RotateVecIn2D(glm::vec3(5.5f, -7.f, -13.f), atan2(front.x, front.z) + glm::radians(180.f), Axis::y), 1.f))
						),
						Rotate(glm::vec4(glm::vec3(-front.z, 0.f, front.x), sign * glm::degrees(acosf(glm::dot(front, glm::normalize(glm::vec3(front.x, 0.f, front.z))))))), 
						Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(front.x, front.z)))), 
						Scale(glm::vec3(3.f)),
					});
						models[(int)ModelType::Shotgun]->SetModelForAll(GetTopModel());
						models[(int)ModelType::Shotgun]->Render(geoPassSP);
					PopModel();
					break;
				}
				case ItemType::Scar: {
					auto rotationMat = glm::rotate(glm::mat4(1.f), sign * acosf(glm::dot(front, glm::normalize(glm::vec3(front.x, 0.f, front.z)))),
						glm::normalize(glm::vec3(-front.z, 0.f, front.x)));
					PushModel({
						Translate(cam.GetPos() +
							glm::vec3(rotationMat * glm::vec4(RotateVecIn2D(glm::vec3(5.f, -4.f, -12.f), atan2(front.x, front.z) + glm::radians(180.f), Axis::y), 1.f))
						),
						Rotate(glm::vec4(glm::vec3(-front.z, 0.f, front.x), sign * glm::degrees(acosf(glm::dot(front, glm::normalize(glm::vec3(front.x, 0.f, front.z))))))), 
						Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(front.x, front.z)))), 
						Scale(glm::vec3(3.f)),
					});
						models[(int)ModelType::Scar]->SetModelForAll(GetTopModel());
						models[(int)ModelType::Scar]->Render(geoPassSP);
					PopModel();
					break;
				}
				case ItemType::Sniper: {
					auto rotationMat = glm::rotate(glm::mat4(1.f), sign * acosf(glm::dot(front, glm::normalize(glm::vec3(front.x, 0.f, front.z)))),
						glm::normalize(glm::vec3(-front.z, 0.f, front.x)));
					PushModel({
						Translate(cam.GetPos() +
							glm::vec3(rotationMat * glm::vec4(RotateVecIn2D(glm::vec3(5.f, -6.f, -13.f), atan2(front.x, front.z) + glm::radians(180.f), Axis::y), 1.f))
						),
						Rotate(glm::vec4(glm::vec3(-front.z, 0.f, front.x), sign * glm::degrees(acosf(glm::dot(front, glm::normalize(glm::vec3(front.x, 0.f, front.z))))))), 
						Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(front.x, front.z)) - 90.f)), 
						Scale(glm::vec3(2.f)),
					});
						models[(int)ModelType::Sniper]->SetModelForAll(GetTopModel());
						models[(int)ModelType::Sniper]->Render(geoPassSP);
					PopModel();
					break;
				}
			}

			///Terrain
			PushModel({
				Scale(glm::vec3(terrainXScale, terrainYScale, terrainZScale)),
			});
				meshes[(int)MeshType::Terrain]->SetModel(GetTopModel());
				meshes[(int)MeshType::Terrain]->Render(geoPassSP);
			PopModel();

			RenderEntities(geoPassSP, true);
			break;
		}
	}
}

constexpr float Divisor = 5.f;

void Scene::MinimapRender(){
	if(screen == Screen::Game){
		forwardSP.Use();
		const int& pAmt = 0;
		const int& dAmt = 0;
		const int& sAmt = 0;
		forwardSP.Set1i("nightVision", 0);
		forwardSP.Set1f("shininess", 32.f); //More light scattering if lower
		forwardSP.Set3fv("globalAmbient", Light::globalAmbient);
		forwardSP.Set3fv("camPos", cam.GetPos());
		forwardSP.Set1i("pAmt", pAmt);
		forwardSP.Set1i("dAmt", dAmt);
		forwardSP.Set1i("sAmt", sAmt);

		switch(minimapView){
			case MinimapViewType::TopFollowingOrtho:
				minimapCam.SetPos(glm::vec3(cam.GetPos().x, 1000.f, cam.GetPos().z));
				minimapCam.SetTarget(glm::vec3(cam.GetPos().x, 0.f, cam.GetPos().z));
				minimapCam.SetUp(glm::vec3(0.f, 0.f, -1.f));
				view = minimapCam.LookAt();
				projection = glm::ortho(-float(winWidth) / Divisor, float(winWidth) / Divisor, -float(winHeight) / Divisor, float(winHeight) / Divisor, .1f, 99999.f);
				break;
			case MinimapViewType::TopFollowingPerspective:
				minimapCam.SetPos(glm::vec3(cam.GetPos().x, 1000.f, cam.GetPos().z));
				minimapCam.SetTarget(glm::vec3(cam.GetPos().x, 0.f, cam.GetPos().z));
				minimapCam.SetUp(glm::vec3(0.f, 0.f, -1.f));
				view = minimapCam.LookAt();
				projection = glm::perspective(angularFOV, cam.GetAspectRatio(), .1f, 99999.f);
				break;
			case MinimapViewType::TopStaticOrtho:
				minimapCam.SetPos(glm::vec3(0.f, 1000.f, 0.f));
				minimapCam.SetTarget(glm::vec3(0.f));
				minimapCam.SetUp(glm::vec3(0.f, 0.f, -1.f));
				view = minimapCam.LookAt();
				projection = glm::ortho(-float(winWidth) * .9f, float(winWidth) * .9f, -float(winHeight) * .9f, float(winHeight) * .9f, .1f, 99999.f);
				break;
			case MinimapViewType::TopStaticPerspective:
				minimapCam.SetPos(glm::vec3(0.f, 2000.f, 0.f));
				minimapCam.SetTarget(glm::vec3(0.f));
				minimapCam.SetUp(glm::vec3(0.f, 0.f, -1.f));
				view = minimapCam.LookAt();
				projection = glm::perspective(angularFOV, cam.GetAspectRatio(), .1f, 99999.f);
				break;
			case MinimapViewType::IsometricOrtho:
				minimapCam.SetPos(glm::vec3(1000.f));
				minimapCam.SetTarget(glm::vec3(0.f));
				minimapCam.SetUp(glm::normalize(glm::vec3(-1.f, 1.f, -1.f)));
				view = minimapCam.LookAt();
				projection = glm::ortho(-float(winWidth) * .9f, float(winWidth) * .9f, -float(winHeight) * .9f, float(winHeight) * .9f, .1f, 99999.f);
				break;
			case MinimapViewType::IsometricPerspective:
				minimapCam.SetPos(glm::vec3(1000.f));
				minimapCam.SetTarget(glm::vec3(0.f));
				minimapCam.SetUp(glm::normalize(glm::vec3(-1.f, 1.f, -1.f)));
				view = minimapCam.LookAt();
				projection = glm::perspective(angularFOV, cam.GetAspectRatio(), .1f, 99999.f);
				break;
			case MinimapViewType::ThirdPersonFollowingOrtho:
				minimapCam.SetPos(glm::vec3(cam.GetPos().x, 500.f, cam.GetPos().z + 500.f));
				minimapCam.SetTarget(glm::vec3(cam.GetPos().x, 0.f, cam.GetPos().z));
				minimapCam.SetUp(glm::normalize(glm::vec3(0.f, 1.f, -1.f)));
				view = minimapCam.LookAt();
				projection = glm::ortho(-float(winWidth) * .8f, float(winWidth) * .8f, -float(winHeight) * .8f, float(winHeight) * .8f, .1f, 99999.f);
				break;
			case MinimapViewType::ThirdPersonFollowingPerspective:
				minimapCam.SetPos(glm::vec3(cam.GetPos().x, 500.f, cam.GetPos().z + 500.f));
				minimapCam.SetTarget(glm::vec3(cam.GetPos().x, 0.f, cam.GetPos().z));
				minimapCam.SetUp(glm::normalize(glm::vec3(0.f, 1.f, -1.f)));
				view = minimapCam.LookAt();
				projection = glm::perspective(angularFOV, cam.GetAspectRatio(), .1f, 99999.f);
				break;
			case MinimapViewType::ThirdPersonStaticOrtho:
				minimapCam.SetPos(glm::vec3(0.f, 1000.f, 1000.f));
				minimapCam.SetTarget(glm::vec3(0.f));
				minimapCam.SetUp(glm::normalize(glm::vec3(0.f, 1.f, -1.f)));
				view = minimapCam.LookAt();
				projection = glm::ortho(-float(winWidth) * .8f, float(winWidth) * .8f, -float(winHeight) * .8f, float(winHeight) * .8f, .1f, 99999.f);
				break;
			case MinimapViewType::ThirdPersonStaticPerspective:
				minimapCam.SetPos(glm::vec3(0.f, 1000.f, 1000.f));
				minimapCam.SetTarget(glm::vec3(0.f));
				minimapCam.SetUp(glm::normalize(glm::vec3(0.f, 1.f, -1.f)));
				view = minimapCam.LookAt();
				projection = glm::perspective(angularFOV, cam.GetAspectRatio(), .1f, 99999.f);
				break;
		}
		forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);

		RenderEntities(forwardSP, true);

		PushModel({
			Translate(glm::vec3(cam.GetPos().x, 0.f, cam.GetPos().z)),
			Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(cam.CalcFront().x, cam.CalcFront().z)))),
			Scale(glm::vec3(20.f)),
		});
			models[(int)ModelType::Scar]->SetModelForAll(GetTopModel());
			models[(int)ModelType::Scar]->Render(forwardSP);
		PopModel();
	}
}

void Scene::ForwardRender(){
	forwardSP.Use();
	const int& pAmt = 0;
	const int& dAmt = 0;
	const int& sAmt = 0;

	forwardSP.Set1f("shininess", 32.f); //More light scattering if lower
	forwardSP.Set3fv("globalAmbient", Light::globalAmbient);
	forwardSP.Set3fv("camPos", cam.GetPos());
	forwardSP.Set1i("pAmt", pAmt);
	forwardSP.Set1i("dAmt", dAmt);
	forwardSP.Set1i("sAmt", sAmt);

	int i;
	for(i = 0; i < pAmt; ++i){
		const PtLight* const& ptLight = static_cast<PtLight*>(ptLights[i]);
		forwardSP.Set3fv(("ptLights[" + std::to_string(i) + "].ambient").c_str(), ptLight->ambient);
		forwardSP.Set3fv(("ptLights[" + std::to_string(i) + "].diffuse").c_str(), ptLight->diffuse);
		forwardSP.Set3fv(("ptLights[" + std::to_string(i) + "].spec").c_str(), ptLight->spec);
		forwardSP.Set3fv(("ptLights[" + std::to_string(i) + "].pos").c_str(), ptLight->pos);
		forwardSP.Set1f(("ptLights[" + std::to_string(i) + "].constant").c_str(), ptLight->constant);
		forwardSP.Set1f(("ptLights[" + std::to_string(i) + "].linear").c_str(), ptLight->linear);
		forwardSP.Set1f(("ptLights[" + std::to_string(i) + "].quadratic").c_str(), ptLight->quadratic);
	}
	for(i = 0; i < dAmt; ++i){
		const DirectionalLight* const& directionalLight = static_cast<DirectionalLight*>(directionalLights[i]);
		forwardSP.Set3fv(("directionalLights[" + std::to_string(i) + "].ambient").c_str(), directionalLight->ambient);
		forwardSP.Set3fv(("directionalLights[" + std::to_string(i) + "].diffuse").c_str(), directionalLight->diffuse);
		forwardSP.Set3fv(("directionalLights[" + std::to_string(i) + "].spec").c_str(), directionalLight->spec);
		forwardSP.Set3fv(("directionalLights[" + std::to_string(i) + "].dir").c_str(), directionalLight->dir);
	}
	for(i = 0; i < sAmt; ++i){
		const Spotlight* const& spotlight = static_cast<Spotlight*>(spotlights[i]);
		forwardSP.Set3fv(("spotlights[" + std::to_string(i) + "].ambient").c_str(), spotlight->ambient);
		forwardSP.Set3fv(("spotlights[" + std::to_string(i) + "].diffuse").c_str(), spotlight->diffuse);
		forwardSP.Set3fv(("spotlights[" + std::to_string(i) + "].spec").c_str(), spotlight->spec);
		forwardSP.Set3fv(("spotlights[" + std::to_string(i) + "].pos").c_str(), spotlight->pos);
		forwardSP.Set3fv(("spotlights[" + std::to_string(i) + "].dir").c_str(), spotlight->dir);
		forwardSP.Set1f(("spotlights[" + std::to_string(i) + "].cosInnerCutoff").c_str(), spotlight->cosInnerCutoff);
		forwardSP.Set1f(("spotlights[" + std::to_string(i) + "].cosOuterCutoff").c_str(), spotlight->cosOuterCutoff);
	}

	forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	switch(screen){
		case Screen::MainMenu: {
			forwardSP.Set1i("nightVision", 0);

			glDepthFunc(GL_GREATER);
			textChief.RenderText(textSP, {
				"Play",
				25.f,
				125.f,
				textScaleFactors[0],
				textColours[0],
				0,
			});
			textChief.RenderText(textSP, {
				"Scoreboard",
				25.f,
				75.f,
				textScaleFactors[1],
				textColours[1],
				0,
				});
			textChief.RenderText(textSP, {
				"Exit",
				25.f,
				25.f,
				textScaleFactors[2],
				textColours[2],
				0,
				});
			textChief.RenderText(textSP, {
				"Total",
				30.f,
				float(winHeight) / 1.2f,
				2.f,
				glm::vec4(glm::vec3(0.f, 1.f, 1.f), 1.f),
				0,
				});
			textChief.RenderText(textSP, {
				"Offense",
				30.f,
				float(winHeight) / 1.2f - 100.f,
				2.f,
				glm::vec4(glm::vec3(0.f, 1.f, 1.f), 1.f),
				0,
				});
			glDepthFunc(GL_LESS);

			break;
		}
		case Screen::Game: {
			view = cam.LookAt();
			projection = glm::perspective(glm::radians(angularFOV), cam.GetAspectRatio(), .1f, 9999.f);
			forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);

			RenderEntities(forwardSP, false);

			const size_t& coinMusicSize = coinMusic.size();
			for(size_t i = 0; i < coinMusicSize; ++i){
				ISound* music = coinMusic[i];
				if(music && music->getIsPaused()){
					music->setIsPaused(false);
				}
			}
			const size_t& fireMusicSize = fireMusic.size();
			for(size_t i = 0; i < fireMusicSize; ++i){
				ISound* music = fireMusic[i];
				if(music && music->getIsPaused()){
					music->setIsPaused(false);
				}
			}

			const glm::vec3 OGPos = cam.GetPos();
			const glm::vec3 OGTarget = cam.GetTarget();
			const glm::vec3 OGUp = cam.GetUp();

			cam.SetPos(glm::vec3(0.f, 0.f, 5.f));
			cam.SetTarget(glm::vec3(0.f));
			cam.SetUp(glm::vec3(0.f, 1.f, 0.f));
			view = cam.LookAt();
			projection = glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f);
			forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);

			if(RMB && inv[currSlot] == ItemType::Sniper){
				forwardSP.Set1i("nightVision", 1);

				PushModel({
					Translate(glm::vec3(0.f, 0.f, -100.f)),
					Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
				});
					forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
					forwardSP.Set1i("customDiffuseTexIndex", 1);
						meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
						meshes[(int)MeshType::Quad]->Render(forwardSP);
					forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
				PopModel();
			} else{
				forwardSP.Set1i("nightVision", 0);

				///Render red hearts
				for(int i = 0; i < playerLives; ++i){
					PushModel({
						Translate(glm::vec3(-float(winWidth) / 2.2f + 75.f * float(i), float(winHeight) / 2.2f, -9.f)),
						Scale(glm::vec3(25.f, 25.f, 1.f)),
					});
						forwardSP.Set1i("noNormals", 1);
						forwardSP.Set1i("useCustomColour", 1);
						forwardSP.Set4fv("customColour", glm::vec4(1.f, 0.f, 0.f, 1.f));
						forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
						forwardSP.Set1i("customDiffuseTexIndex", 2);
							meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
							meshes[(int)MeshType::Quad]->Render(forwardSP);
						forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
						forwardSP.Set1i("useCustomColour", 0);
						forwardSP.Set1i("noNormals", 0);
					PopModel();
				}

				///Render grey hearts
				for(int i = 0; i < playerMaxLives; ++i){
					PushModel({
						Translate(glm::vec3(-float(winWidth) / 2.2f + 75.f * float(i), float(winHeight) / 2.2f, -10.f)),
						Scale(glm::vec3(25.f, 25.f, 1.f)),
					});
						forwardSP.Set1i("noNormals", 1);
						forwardSP.Set1i("useCustomColour", 1);
						forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(.3f), 1.f));
						forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
						forwardSP.Set1i("customDiffuseTexIndex", 2);
							meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
							meshes[(int)MeshType::Quad]->Render(forwardSP);
						forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
						forwardSP.Set1i("useCustomColour", 0);
						forwardSP.Set1i("noNormals", 0);
					PopModel();
				}

				///Render inv slots
				for(short i = 0; i < 5; ++i){
					PushModel({
						Translate(glm::vec3(float(i) * 100.f - 300.f, -float(winHeight) / 2.3f, -10.f)),
						Scale(glm::vec3(50.f, 50.f, 1.f)),
					});
						forwardSP.Set1i("noNormals", 1);
						if(i == currSlot){
							forwardSP.Set1i("useCustomColour", 1);
							forwardSP.Set4fv("customColour", glm::vec4(1.f, 1.f, 0.f, 1.f));
						}
						forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
						forwardSP.Set1i("customDiffuseTexIndex", 3);
							meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
							meshes[(int)MeshType::Quad]->Render(forwardSP);
						forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
						if(i == currSlot){
							forwardSP.Set1i("useCustomColour", 0);
						}
						forwardSP.Set1i("noNormals", 0);
					PopModel();
				}

				///Render reticle
				if(inv[currSlot] == ItemType::Shotgun || inv[currSlot] == ItemType::Scar){
					PushModel({
						Scale(glm::vec3(40.f, 40.f, 1.f)),
					});
						forwardSP.Set1i("noNormals", 1);
						forwardSP.Set1i("useCustomColour", 1);
						forwardSP.Set4fv("customColour", reticleColour);
						forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
						forwardSP.Set1i("customDiffuseTexIndex", 4);
							meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
							meshes[(int)MeshType::Quad]->Render(forwardSP);
						forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
						forwardSP.Set1i("useCustomColour", 0);
						forwardSP.Set1i("noNormals", 0);

						PushModel({
							Translate(glm::vec3(0.f, 0.f, 1.f)),
						});
						if(RMB){
							PushModel({
								Scale(glm::vec3(.7f, .7f, 1.f)),
							});
						}
							forwardSP.Set1i("noNormals", 1);
							forwardSP.Set1i("useCustomColour", 1);
							forwardSP.Set4fv("customColour", reticleColour);
							forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
							forwardSP.Set1i("customDiffuseTexIndex", inv[currSlot] == ItemType::Shotgun ? 5 : 6);
								meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
								meshes[(int)MeshType::Quad]->Render(forwardSP);
							forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
							forwardSP.Set1i("useCustomColour", 0);
							forwardSP.Set1i("noNormals", 0);
						PopModel();
						PopModel();
					PopModel();
				}
			}

			cam.SetPos(OGPos);
			cam.SetTarget(OGTarget);
			cam.SetUp(OGUp);
			view = cam.LookAt();
			projection = glm::perspective(glm::radians(angularFOV), cam.GetAspectRatio(), .1f, 9999.f);
			forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);

			#ifdef DEBUGGING
			textChief.RenderText(textSP, {
				"CamTarget: " + std::to_string(cam.GetTarget().x) + " " + std::to_string(cam.GetTarget().y) + " " + std::to_string(cam.GetTarget().z),
				25.f,
				25.f,
				.5f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});
			textChief.RenderText(textSP, {
				"CamFront: " + std::to_string(cam.CalcFront().x) + " " + std::to_string(cam.CalcFront().y) + " " + std::to_string(cam.CalcFront().z),
				25.f,
				75.f,
				.5f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});
			#else
			///Render bullet info
			if(currGun){
				textChief.RenderText(textSP, {
					std::to_string(currGun->GetLoadedBullets()) + " / " + std::to_string(currGun->GetUnloadedBullets()),
					float(winWidth) / 2.4f,
					125.f,
					1.f,
					glm::vec4(1.f, 1.f, 0.f, 1.f),
					0,
				});
			}
			textChief.RenderText(textSP, {
				"Time Left: " + std::to_string(timeLeft).substr(0, 5),
				25.f,
				25.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});
			textChief.RenderText(textSP, {
				"Score: " + std::to_string(score),
				25.f,
				75.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});
			#endif
			textChief.RenderText(textSP, {
				"FPS: " + std::to_string(1.f / dt).substr(0, 2),
				25.f,
				125.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});

			break;
		}
		case Screen::GameOver: {
			forwardSP.Set1i("nightVision", 0);

			glDepthFunc(GL_GREATER);
			textChief.RenderText(textSP, {
				"Play Again",
				25.f,
				125.f,
				textScaleFactors[0],
				textColours[0],
				0,
				});
			textChief.RenderText(textSP, {
				"Scoreboard",
				25.f,
				75.f,
				textScaleFactors[1],
				textColours[1],
				0,
				});
			textChief.RenderText(textSP, {
				"Exit",
				25.f,
				25.f,
				textScaleFactors[2],
				textColours[2],
				0,
				});

			textChief.RenderText(textSP, {
				"Game Over",
				30.f,
				float(winHeight) / 1.2f,
				2.f,
				glm::vec4(1.f, .5f, 0.f, 1.f),
				0,
				});
			textChief.RenderText(textSP, {
				"Final Score: " + std::to_string(score),
				30.f,
				float(winHeight) / 1.2f - 100.f,
				2.f,
				glm::vec4(1.f, .5f, 0.f, 1.f),
				0,
				});
			if(scores.size() == 1 || (score == scores.front() && score != scores[1])){
				textChief.RenderText(textSP, {
					"New High Score!",
					30.f,
					float(winHeight) / 1.2f - 200.f,
					2.f,
					glm::vec4(1.f, .5f, 0.f, 1.f),
					0,
					});
			}
			glDepthFunc(GL_LESS);
			break;
		}
		case Screen::Scoreboard: {
			forwardSP.Set1i("nightVision", 0);

			glDepthFunc(GL_GREATER);
			textChief.RenderText(textSP, {
				"Back",
				25.f,
				25.f,
				textScaleFactors[2],
				textColours[2],
				0,
			});

			float currOffset = 0.f;
			textChief.RenderText(textSP, {
				"Scoreboard",
				30.f,
				float(winHeight) / 1.2f,
				1.f,
				glm::vec4(1.f, .5f, 0.f, 1.f),
				0,
			});
			const size_t& mySize = scores.size();
			for(size_t i = 0; i < mySize; ++i){
				currOffset += 80.f;
				textChief.RenderText(textSP, {
					std::to_string(scores[i]),
					30.f,
					float(winHeight) / 1.2f - currOffset,
					1.f,
					glm::vec4(1.f, .5f, 0.f, 1.f),
					0,
				});
			}
			glDepthFunc(GL_LESS);
			break;
		}
	}

	glBlendFunc(GL_ONE, GL_ZERO);
}





void Scene::LightingRenderPass(const uint& posTexRefID, const uint& coloursTexRefID, const uint& normalsTexRefID, const uint& specTexRefID, const uint& reflectionTexRefID){
	lightingPassSP.Use();
	const int& pAmt = (int)ptLights.size();
	const int& dAmt = (int)directionalLights.size();
	const int& sAmt = (int)spotlights.size();

	lightingPassSP.Set1f("shininess", 32.f); //More light scattering if lower //??
	lightingPassSP.Set3fv("globalAmbient", Light::globalAmbient);
	lightingPassSP.Set3fv("camPos", cam.GetPos());
	lightingPassSP.Set1i("pAmt", pAmt);
	lightingPassSP.Set1i("dAmt", dAmt);
	lightingPassSP.Set1i("sAmt", sAmt);
	lightingPassSP.UseTex(posTexRefID, "posTex");
	lightingPassSP.UseTex(coloursTexRefID, "coloursTex");
	lightingPassSP.UseTex(normalsTexRefID, "normalsTex");
	lightingPassSP.UseTex(specTexRefID, "specTex");
	lightingPassSP.UseTex(reflectionTexRefID, "reflectionTex");

	int i;
	for(i = 0; i < pAmt; ++i){
		const PtLight* const& ptLight = static_cast<PtLight*>(ptLights[i]);
		lightingPassSP.Set3fv(("ptLights[" + std::to_string(i) + "].ambient").c_str(), ptLight->ambient);
		lightingPassSP.Set3fv(("ptLights[" + std::to_string(i) + "].diffuse").c_str(), ptLight->diffuse);
		lightingPassSP.Set3fv(("ptLights[" + std::to_string(i) + "].spec").c_str(), ptLight->spec);
		lightingPassSP.Set3fv(("ptLights[" + std::to_string(i) + "].pos").c_str(), ptLight->pos);
		lightingPassSP.Set1f(("ptLights[" + std::to_string(i) + "].constant").c_str(), ptLight->constant);
		lightingPassSP.Set1f(("ptLights[" + std::to_string(i) + "].linear").c_str(), ptLight->linear);
		lightingPassSP.Set1f(("ptLights[" + std::to_string(i) + "].quadratic").c_str(), ptLight->quadratic);
	}
	for(i = 0; i < dAmt; ++i){
		const DirectionalLight* const& directionalLight = static_cast<DirectionalLight*>(directionalLights[i]);
		lightingPassSP.Set3fv(("directionalLights[" + std::to_string(i) + "].ambient").c_str(), directionalLight->ambient);
		lightingPassSP.Set3fv(("directionalLights[" + std::to_string(i) + "].diffuse").c_str(), directionalLight->diffuse);
		lightingPassSP.Set3fv(("directionalLights[" + std::to_string(i) + "].spec").c_str(), directionalLight->spec);
		lightingPassSP.Set3fv(("directionalLights[" + std::to_string(i) + "].dir").c_str(), directionalLight->dir);
	}
	for(i = 0; i < sAmt; ++i){
		const Spotlight* const& spotlight = static_cast<Spotlight*>(spotlights[i]);
		lightingPassSP.Set3fv(("spotlights[" + std::to_string(i) + "].ambient").c_str(), spotlight->ambient);
		lightingPassSP.Set3fv(("spotlights[" + std::to_string(i) + "].diffuse").c_str(), spotlight->diffuse);
		lightingPassSP.Set3fv(("spotlights[" + std::to_string(i) + "].spec").c_str(), spotlight->spec);
		lightingPassSP.Set3fv(("spotlights[" + std::to_string(i) + "].pos").c_str(), spotlight->pos);
		lightingPassSP.Set3fv(("spotlights[" + std::to_string(i) + "].dir").c_str(), spotlight->dir);
		lightingPassSP.Set1f(("spotlights[" + std::to_string(i) + "].cosInnerCutoff").c_str(), spotlight->cosInnerCutoff);
		lightingPassSP.Set1f(("spotlights[" + std::to_string(i) + "].cosOuterCutoff").c_str(), spotlight->cosOuterCutoff);
	}

	meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
	meshes[(int)MeshType::Quad]->Render(lightingPassSP, false);
	lightingPassSP.ResetTexUnits();
}

void Scene::BlurRender(const uint& brightTexRefID, const bool& horizontal){
	blurSP.Use();
	blurSP.Set1i("horizontal", horizontal);
	blurSP.UseTex(brightTexRefID, "texSampler");
	meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
	meshes[(int)MeshType::Quad]->Render(blurSP, false);
	blurSP.ResetTexUnits();
}

void Scene::DefaultRender(const uint& screenTexRefID, const uint& blurTexRefID, const glm::vec3& translate, const glm::vec3& scale){
	if(!glm::length(translate) || screen == Screen::Game){
		screenSP.Use();
		screenSP.Set1f("exposure", .5f);
		screenSP.UseTex(screenTexRefID, "screenTexSampler");
		screenSP.UseTex(blurTexRefID, "blurTexSampler");
		if(screen == Screen::Game){
			screenSP.Set1i("nightVision", int(RMB && inv[currSlot] == ItemType::Sniper));
		} else{
			screenSP.Set1i("nightVision", 0);
		}
		PushModel({
			Translate(translate),
			Scale(scale),
		});
			meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
			meshes[(int)MeshType::Quad]->Render(screenSP, false);
		PopModel();
		screenSP.ResetTexUnits();
	}
}

Entity* const& Scene::FetchEntity(){
	for(Entity* const& entity: entityPool){
		if(!entity->active){
			return entity;
		}
	}
	entityPool.emplace_back(new Entity());
	(void)puts("1 entity was added to entityPool!\n");
	return entityPool.back();
}

glm::mat4 Scene::Translate(const glm::vec3& translate){
	return glm::translate(glm::mat4(1.f), translate);
}

glm::mat4 Scene::Rotate(const glm::vec4& rotate){
	return glm::rotate(glm::mat4(1.f), glm::radians(rotate.w), glm::vec3(rotate));
}

glm::mat4 Scene::Scale(const glm::vec3& scale){
	return glm::scale(glm::mat4(1.f), scale);
}

glm::mat4 Scene::GetTopModel() const{
	return modelStack.empty() ? glm::mat4(1.f) : modelStack.top();
}

void Scene::PushModel(const std::vector<glm::mat4>& vec) const{
	modelStack.push(modelStack.empty() ? glm::mat4(1.f) : modelStack.top());
	const size_t& size = vec.size();
	for(size_t i = 0; i < size; ++i){
		modelStack.top() *= vec[i];
	}
}

void Scene::PopModel() const{
	if(!modelStack.empty()){
		modelStack.pop();
	}
}