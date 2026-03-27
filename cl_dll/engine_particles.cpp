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

// vv based on flames_smoke.txt
char impactsmoke[] = R"(
life 1
lifevar 0.25

fadedelay 0.25

minvel 8
maxvel 64

maxofs 15

texture smoke03
rendermode 2

scale 16
scalevar 4
scaledampdelay 0.1
scaledampfactor -15

rotationvel 80
rotationdamp 0.04

veldamp 0.1

collision 4
gravity -0.05
mainalpha 0.125

startparticles 1

lightmaps 2

pcolr %d
pcolg %d
pcolb %d
)";

// vvv could use a animated tex to make the drops fly outwards
char bloodgibcloud[] = R"(
randomdir 1
systemsize 5

life 1
lifevar 0.2

fadedelay 0.1

minvel 175
maxvel 225

maxofs 11

texture splash1
rendermode 2

scale 10
scalevar 5
scaledampdelay 0.1
scaledampfactor -10

rotationvel 80
rotationdamp 0.01

veldamp 2.3
veldampdelay 0.01

collision 2
impactdamp 0.75

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

char innacuracydebug[] = R"(
life 0.05

fadedelay 0.01

display 2

texture gloworg
rendermode 0

scale 5

pcolr 0
pcolg 255
pcolb 0

gravity 0

startparticles 1

lightmaps 0
mainalpha 1.0
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

char bloodspray[] = R"(
life 30

randomdir %d

minvel 100
maxvel 350
maxofs 90

sprite blooddrop
framerate 0
rendermode 2
startframe %d

scale 2
scalevar 1

collision 3

create %s
decalparticlecreate %s

fromwad  1
decalang -1

rotationvel 10
rotationvar 10

pcolr %d
pcolg %d
pcolb %d

gravity 1

startparticles 1

lightmaps 0
)";

char particle_gs[] = R"(
life 1.5
lifevar 0.1

systemshape 1
systemsize 1
randomdir 0

fadedelay 1.3

minvel 50
maxvel 200
rotationvel 90
rotationvar 30

texture gsparticle
rendermode 2

scale 1.5
scalevar 0.25

pcolr 47
pcolg 47
pcolb 47


gravity 1

startparticles 15

collision 2
impactdamp 0.25
lightmaps 1

)";

char particle_bulletdripimpact[] = R"(
life 1.5
lifevar 0.1

randomdir %d

maxofs 12

fadedelay 1.3

minvel 64
maxvel 200
rotationvel 90
rotationvar 30

texture gsparticle
rendermode 2

scale %f

pcolr %d
pcolg %d
pcolb %d


gravity 1

collision 2
impactdamp 0.75
lightmaps 1

systemfadetime %f
intensity %d

)";

// replace "sprite muzzleflash1" with a input so we can use this one
char particle_muzzleflash[] = R"(
life 0.025

scale %lf

sprite %s

rendermode 0

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
