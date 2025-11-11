/*
*
//
/// Copyright PackMail Industries 2025-2025, no rights reserved.
//
*
*/

char bloodsprite[] = R"(
life 0.5
lifevar 0.1

fadedelay 4.9

minvel 0
maxvel 0

sprite %s
framerate 30
rendermode 2

scale 4.0

rotationvel 4
rotationvar 4

pcolr %d
pcolg %d
pcolb %d

gravity 0

startparticles 1

lightmaps 0
)";

char bloodchunks[] = R"(
life 5.5
lifevar 0.1

randomdir 1

fadedelay 2

minvel 80
maxvel 105

sprite %s
framerate 0
rendermode 2
startframe %d

scale %d

collision 2

rotationvel 10
rotationvar 10

pcolr %d
pcolg %d
pcolb %d

gravity 0.5

startparticles 1

lightmaps 0
)";

char particle_bulletimpact[] = R"(
life 1.5
lifevar 0.1

systemshape 1
systemsize 1
randomdir 1

fadedelay 1.3

minvel 50
maxvel 200

sprite _particletexture
rendermode 2

scale 1.5

pcolr 47
pcolg 47
pcolb 47


gravity 1

startparticles 30

collision 2

lightmaps 0

)";

// replace "sprite muzzleflash1" with a input so we can use this one
char particle_muzzleflash[] = R"(
life 0.1
lifevar 0.025

fadedelay 0.05

sprite %d
rendermode 0

scale %d
scalevar 0.1

rotationvel 4
rotationvar 4

pcolr 255
pcolg 255
pcolb 255


gravity 0

startparticles 1

collision 0

lightmaps 0

)";

// what is this one?
char particle_explosion[] = R"(
life 1.5
lifevar 0.1

systemshape 1
systemsize 1
randomdir 1

fadedelay 1.3

minvel -512
maxvel 512

sprite _particletexture
rendermode 1

scale 1.5

pcolr 255
pcolg 243
pcolb 27

gravity 0.1

startparticles 1024

lightmaps 0

)";
