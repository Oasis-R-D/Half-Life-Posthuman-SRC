/*
*
//
/// Copyright PackMail Industries 2025-2025, no rights reserved. (I'll sue you breh)
//
*
*/

#include "engine_particles.h"
//8========================================================D
// ENGINE_PARTICLES.CPP: File to store code defined particles
// WHY: Most of these are only here so they can accept inputs
//8========================================================D

char bloodgibcloud[] = R"(
randomdir 1
systemsize 5

life 1
lifevar 0.2

fadedelay 0.1

minvel 175
maxvel 225

maxofs 11

texture smoke03
rendermode 2

scale 10
scalevar 5
scaledampdelay 0.1
scaledampfactor -10

rotationvel 80
rotationdamp 0.02

veldamp 2.3
veldampdelay 0.01

collision 4
gravity -0.1
mainalpha 0.25

startparticles 45

lightmaps 2

pcolr %d
pcolg %d
pcolb %d
)";

char bulletholeglow[] = R"(
life 1.5

fadedelay 0.5

minvel 0
maxvel 0

display 2

texture gloworg
rendermode 0

scale 5

pcolr 255
pcolg 255
pcolb 255

gravity 0

startparticles 1

lightmaps 0
mainalpha 0.33
)";

char bloodsprite[] = R"(
life 0.33

fadedelay 0.3

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

minvel 100
maxvel 180

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

gravity 1

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

startparticles 20

collision 2

lightmaps 1

)";

// replace "sprite muzzleflash1" with a input so we can use this one
char particle_muzzleflash[] = R"(
life 0.1
lifevar 0.025

fadedelay 0.05

sprite %s
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
