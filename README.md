***** 193541T GDEV *****

> ** Completion Time: 4 days **

Game Controls:

Esc - Exit programme
Left Mouse Button (LMB) - Shoot
Right Mouse Button (RMB) - Reduce Field Of View (FOV)
F1 - Toggle fullscreen
F2 - Change polygon mode
F3 - Change minimap view
WASD - Move player
Space - Prone --> Crouch --> Stand --> Jump
C - Stand --> Crouch --> Prone
Shift - Toggle sprint
1 - Select inv[0]
2 - Select inv[1]
3 - Select inv[2]
4 - Select inv[3]
5 - Select inv[4]
I - Toggle distortion
O - Toggle echo
P - Toggle wavesReverb
L - Reset sound
R - Reload currently selected gun

Non-Game Controls:

Left Mouse Button (LMB) - select different options

Features:

Walking
Sprinting
Jumping
Crouching
Proning

Shooting weapons
Changing weapons
Diff weapon attributes
Collecting of ammo from the ground

Collision between player and boundary
Collision between player and sphere (player and enemy, player and spherical walls)
Collision between player and cuboid
Collision between sphere and sphere (bullet and enemy)

Different pre-set camera positions for the minimap:
		TopFollowingOrtho
		TopFollowingPerspective
		TopStaticOrtho
		TopStaticPerspective
		IsometricOrtho
		IsometricPerspective
		ThirdPersonFollowingOrtho
		ThirdPersonFollowingPerspective
		ThirdPersonStaticOrtho
		ThirdPersonStaticPerspective

Score system (file I/O for saving and loading scores, storing and sorting scores in a container)
Lives
Health
Inventory
Minimap

Camera collision with terrain
Night vision (combined with scoped mode for sniper)
Scoped mode (for sniper)

2D audio
3D audio (coin spinning music and fire burning music)

Object pooling
Skybox with bounds checking
Ray casting to check for ray-sphere intersection
HDR rendering
Bloom
Deferred rendering pipeline + Forward rendering pipeline
Terrain with normals
Sprite Animation (coin and fire)
All meshes used are UV-mapped

No warnings (except for the ones from the external libraries)
No memory leaks