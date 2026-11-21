// HAUNTED HOUSE 3D - V143
// C++ / SDL2 / OpenGL ES 2.0
// Controls: WASD/arrows move, mouse/touch drag look, E = nearby gate/shutter/use, F flashlight,
// SPACE shoot, R reload. Touch: left side joystick, right side look,
// bottom-right fire, E = mount/dismount horse or use nearby objects; joystick = ride; yellow button = car headlights.
//
// This is a procedural prototype: no external models or textures required.

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cctype>

static const float PI = 3.14159265358979323846f;
static float hillHeightAt(float x,float z);
static float stuntRampHeightAt(float x,float z) {
    // A long, gentle incline from z=18 to z=52, 4.2 units high.
    if(x < -124.7f || x > -115.3f || z < 18.0f || z > 52.0f) return 0.0f;
    float t=(z-18.0f)/34.0f;
    return 4.2f*t;
}
static float bridgeHeightAt(float x,float z) {
    // Low bridge deck with shallow approach ramps on both river banks.
    if(z < 95.0f || z > 105.0f || x < -177.0f || x > -133.0f) return 0.0f;
    if(x < -170.0f) return 0.22f*(x+177.0f)/7.0f;
    if(x > -140.0f) return 0.22f*(-133.0f-x)/7.0f;
    return 0.22f;
}
static float carRideHeightAt(float x,float z) {
    return std::max(stuntRampHeightAt(x,z),bridgeHeightAt(x,z));
}
static float carVisualHeight = 0.0f;

static SDL_Window* gWindow = nullptr;
static SDL_GLContext gContext = nullptr;
static GLuint gProgram = 0, gCubeVBO = 0, gSphereVBO = 0;
static GLint gPosition = -1, gMVP = -1, gColor = -1, gLight = -1;
static int gWidth = 960, gHeight = 540;
static bool gRunning = true, gDoorOpen = true, gFlashlight = true;
static bool ayushGateOpen = false;
static float ayushGateAmount = 0.0f;
static bool onAyushBike = false, ayushRideArrived = false;
static bool ayushWalkingWithPlayer = false, ayushReturningToCollege = false;
static bool ayushBikeParked = false;
static float ayushFriendX = -145.0f, ayushFriendZ = -30.0f;
static float ayushBikeX = 25.0f, ayushBikeZ = 13.5f;
static float ayushBikeYaw = 1.30f, ayushRideProgress = 0.0f;
static float ayushReturnProgress = 0.0f;
static float ayushWalkPhase = 0.0f, ayushWalkBlend = 0.0f;

static bool mansionGateOpen = false, garageShutterOpen = false;
static float mansionGateAmount = 0.0f, garageShutterAmount = 0.0f;
static float playerX = 0.0f, playerZ = 6.0f;
static float playerHeight = 0.0f;
// Rocket / solar-system exploration mode.
static bool inRocket=false, rocketLaunched=false, rocketLanded=false, onPlanetSurface=false;
static float rocketAltitude=0.0f, rocketTravelX=0.0f, rocketTravelZ=0.0f;
static float rocketVerticalSpeed=0.0f, rocketYaw=0.0f, rocketPitch=0.0f, rocketRoll=0.0f;
static int currentPlanet=-1;
static float rocketEngineGlow=0.0f;
static float rocketThrottle=0.0f, rocketLaunchHold=0.0f, rocketShake=0.0f;
static float rocketExhaustPhase=0.0f;
static bool rocketIgnitionArmed=false, rocketStageSeparated=false;

static const char* planetNames[8]={"Mercury","Venus","Earth","Mars","Jupiter","Saturn","Uranus","NEBULON - ALIEN HOMEWORLD"};

static bool onUpperFloor = false;
// Rooftop complex: 0=main terrace, 1=supermarket + gym, 2=rental rooms, 3=top roof.
static int rooftopLevel = 0;
static float rooftopWalkHeight(int level) {
    // Slab tops: commercial 3.94, rental 9.50, top roof 12.84.
    // A small 0.10-unit clearance keeps the avatar's feet above the mesh.
    if(level==1) return 4.04f;
    if(level==2) return 9.60f;
    if(level==3) return 12.94f;
    return 2.60f;
}
static bool rentalDoorOpen[4] = {false,false,false,false};
static bool rentalDoorUnlocked[4] = {false,false,false,false};
static bool swimming = false;
static bool sleeping = false;
static bool sittingInGardenChair = false;
static int gardenChairIndex = -1;
static float yaw = 0.0f, pitch = 0.0f;
static float joystickX = 0.0f, joystickY = 0.0f;
static float smoothX = 0.0f, smoothY = 0.0f;
static int ammo = 12, reserveAmmo = 48, keysFound = 0;
static float health = 100.0f, fireCooldown = 0.0f;
static bool alienInvasionStarted=false, alienShipLanded=false;
static bool alienPrisonUnlocked=false;
static int abductedNPCCount=0;
static float alienArrivalTimer=0.0f, alienShipY=34.0f, alienShipDoor=0.0f;
static bool ak47Owned=false, scopeMode=false;
static bool gameOver=false;
static float akPickupX=3.8f, akPickupZ=13.0f;
static float alienSpawnRadius=27.0f;
struct AlienSoldier {
    float x=0,z=0,yaw=0,fireTimer=1.0f,hitFlash=0.0f;
    int hp=100; bool alive=true;
};
struct AlienBullet { float x=0,y=1.1f,z=0,vx=0,vy=0,vz=0,life=4.0f; };
static std::vector<AlienSoldier> alienSoldiers;
static std::vector<AlienBullet> alienBullets;
struct HomeworldGuard { float x=0,z=0,yaw=0,flash=0; int hp=100; bool alive=true; };
static std::vector<HomeworldGuard> homeworldGuards;
static bool homeworldMissionStarted=false, homeworldPrisonOpen=false;
static int rescuedStudents=0, studentsAboardRocket=0;
static float homeworldTime=0.0f;

static float gunSoundPhase=0.0f;
static int gunSoundSamples=0;

static SDL_FingerID leftFinger = -1, rightFinger = -1;
static float lastTouchX = 0.0f, lastTouchY = 0.0f;
static bool mouseLooking = false;
static int inventoryCount = 0;
static int cashBalance = 100000;
static int groceryBags = 0;
static bool groceriesInCar = false;
static bool supermarketCheckoutOpen = false;
static int supermarketSelected = 0;
static int supermarketCart[8] = {0,0,0,0,0,0,0,0};
static int supermarketLastBill = 0;
static bool deliveryActive=false, deliveryBellRang=false, deliveryCompleted=false;
static float deliveryX=94.0f, deliveryZ=78.0f, deliveryYaw=0.0f;
static float deliveryBellTimer=0.0f;
static int deliveryOrderCount=0, deliveryPathIndex=0;
static const float deliveryPathX[6]={6.0f,6.0f,6.0f,6.0f,6.0f,6.0f};
static const float deliveryPathZ[6]={36.0f,31.0f,26.0f,21.0f,16.0f,12.0f};
static const char* supermarketNames[8] = {
    "MILK", "BREAD", "APPLE", "BANANA", "RICE", "CHIPS", "JUICE", "SOAP"
};
static const int supermarketPrices[8] = {60,40,30,10,120,25,45,35};
static bool drivingCar = false;
static bool nightMode = false;
static bool carHeadlightsOn = false;
static float nightBlend = 0.0f;
static bool inAirplane = false, planeEngineOn = false, planeFlying = false;
static float planeX=46.0f, planeZ=124.0f, planeYaw=0.0f;
static float planeSpeed=0.0f, planeAltitude=0.0f;
static float planeAnimTime=0.0f;
static bool parachuting=false;
static float parachuteDrift=0.0f;
static bool ridingHorse=false;
static float horseX=17.0f, horseZ=29.0f, horseYaw=0.0f, horsePhase=0.0f;
static float companionX=2.0f, companionZ=9.0f, companionPhase=0.0f;
static bool companionCooking=false;
static bool companionReturningWithFood=false;
static bool foodReady=false;
static float cookingTimer=0.0f;
static bool childrenGoingStudy=false;
static float childrenStudyProgress=0.0f;
static float childWorldX[2]={-11.82f,-10.18f};
static float childWorldZ[2]={50.0f,50.0f};
static int childPathSegment[2]={0,0};
static float childWalkPhase=0.0f;
static bool childrenAtStudyDesk=false;
static Uint32 companionLastTapMs=0;
static float companionLastTapX=-1.0f, companionLastTapY=-1.0f;
static int planePassengers=3;
static float carX0 = -10.0f, carZ0 = 34.5f, carYaw = 0.0f;
static float carWheelSpin = 0.0f;
static float carCurrentSpeed = 0.0f;
static float steeringWheelAngle = 0.0f;
static float enterCarCooldown = 0.0f;
static float walkPhase = 0.0f;
static float walkBlend = 0.0f;
static float jumpVelocity = 0.0f;
static bool jumping = false;
static float jumpBaseHeight = 0.0f;
static bool fallingFromRoof = false;
static float roofFallStartHeight = 0.0f;
static int score = 0, fruitCount = 0;
static float throwCooldown = 0.0f;
struct Animal { float x,z,dir,speed,phase; int kind; };

// Forward declarations: NPC AI/render code appears before these helpers are defined.
static bool collides(float x,float z);
static void cube(const float* vp,float x,float y,float z,float sx,float sy,float sz,float r,float g,float b);
static void sphere(const float* vp,float x,float y,float z,float sx,float sy,float sz,float r,float g,float b);
static void cubeYaw(const float* vp,float x,float y,float z,float sx,float sy,float sz,float a,float r,float g,float b);
static void drawBlockText(const float* v,float cx,float cy,float frontZ,const char* text,float scale);

// Ten autonomous residents. Their compact episodic memory is persisted between runs.
struct NPCMemory {
    int meetings=0, gifts=0, conflicts=0, daysSeen=0;
    int observedWalk=0, observedJump=0, observedSit=0, observedDrive=0;
    float lastPlayerX=0.0f, lastPlayerZ=0.0f;
    Uint32 lastSeenTick=0;
    std::string lastEvent="New to this place";
};
struct NPC {
    std::string name;
    int role=0; // 0 resident, 1 shopkeeper, 2 guard, 3 student, 4 worker
    float x=0,z=0,yaw=0,speed=1.0f;
    float homeX=0,homeZ=0,workX=0,workZ=0,targetX=0,targetZ=0;
    float houseX=0,houseZ=0,houseProgress=0.0f;
    bool houseBuilt=false, playerOrdered=false;
    bool alienAbducting=false, alienCaptured=false;
    float alienLift=0.0f; // animated tractor-beam lift above the ground
    bool alienBeingLifted=false;
    float thinkTimer=0,waitTimer=0,phase=0,actionTimer=0,workPhase=0;
    float hitStun=0.0f, knockVX=0.0f, knockVZ=0.0f;
    int health=100;
    int mood=0,task=0; // 0 roam, 1 walk-to-work, 2 return-home, 3 observe, 4 dig, 5 build, 6 farm, 7 sleep
    NPCMemory memory;
};
static std::vector<NPC> npcs;
static std::string npcCommandBuffer;
static std::string npcCommandStatus="PRESS T TO TALK TO NPCs";
static bool collegeBookOpen=false;
static std::string collegeBookMessage="";
static bool npcCommandTyping=false;
static float npcBuildProgress=0.0f;
static const float npcBuildX=13.8f; // construction site beside the main house, not in front of its entrance
static const float npcBuildZ=-1.5f;
static float npcSaveTimer=0.0f;
static const char* npcMemoryFile="npc_memory.dat";
static float learningObserveTimer=0.0f;
static bool previousDrivingState=false, previousSittingState=false;
static bool previousJumpState=false;
static void recordPlayerLearning(float dt) {
    learningObserveTimer += dt;
    if(learningObserveTimer < 1.0f) return;
    learningObserveTimer = 0.0f;
    for(NPC& n:npcs) {
        if(joystickX!=0.0f || joystickY!=0.0f) n.memory.observedWalk++;
        if(jumping && !previousJumpState) n.memory.observedJump++;
        if(sittingInGardenChair && !previousSittingState) n.memory.observedSit++;
        if(drivingCar && !previousDrivingState) n.memory.observedDrive++;
    }
    previousJumpState=jumping;
    previousSittingState=sittingInGardenChair;
    previousDrivingState=drivingCar;
}

static void initNPCs() {
    if(!npcs.empty()) return;
    const char* names[10]={"Aarav","Meera","Kabir","Anaya","Rohan","Isha","Dev","Priya","Arjun","Tara"};
    const int roles[10]={0,1,2,3,4,0,1,3,4,0};
    const float hx[10]={-15,-12,-9,-6,-3,3,6,9,12,15};
    const float hz[10]={25,28,31,34,37,37,34,31,28,25};
    const float wx[10]={-8,94,-2,-5,8,12,94,5,-12,15};
    const float wz[10]={18,82,16,42,20,24,82,45,18,30};
    for(int i=0;i<10;i++) {
        NPC n; n.name=names[i]; n.role=roles[i];
        n.homeX=hx[i]; n.homeZ=hz[i]; n.workX=wx[i]; n.workZ=wz[i];
        // A separate village far from the player's mansion: two rows, five plots.
        n.houseX=-78.0f+(i%5)*15.0f; n.houseZ=142.0f+(i/5)*17.0f;
        n.x=hx[i]+(i%2?0.7f:-0.7f); n.z=hz[i]+1.0f;
        n.speed=0.75f+0.08f*(i%5); n.targetX=n.workX; n.targetZ=n.workZ;
        n.task=(i%3==0)?1:0;
        npcs.push_back(n);
    }
}
static void loadNPCMemory() {
    initNPCs();
    std::ifstream in(npcMemoryFile);
    if(!in) return;
    for(NPC& n:npcs) {
        int meetings,gifts,conflicts,days;
        float px,pz;
        if(!(in>>meetings>>gifts>>conflicts>>days>>px>>pz)) break;
        n.memory.meetings=meetings; n.memory.gifts=gifts;
        n.memory.conflicts=conflicts; n.memory.daysSeen=days;
        n.memory.lastPlayerX=px; n.memory.lastPlayerZ=pz;
        std::string line;
        std::getline(in,line); // consume end of numeric line
        if(!std::getline(in,line)) break;
        std::istringstream extra(line);
        if(extra >> n.memory.observedWalk >> n.memory.observedJump
                 >> n.memory.observedSit >> n.memory.observedDrive) {
            std::getline(in,n.memory.lastEvent);
        } else {
            n.memory.lastEvent=line; // accept older two-line save files
        }
        if(n.memory.lastEvent.empty()) n.memory.lastEvent="Remembering previous visit";
    }
}
static void saveNPCMemory() {
    std::ofstream out(npcMemoryFile,std::ios::trunc);
    if(!out) return;
    for(const NPC& n:npcs) {
        out<<n.memory.meetings<<' '<<n.memory.gifts<<' '
           <<n.memory.conflicts<<' '<<n.memory.daysSeen<<' '
           <<n.memory.lastPlayerX<<' '<<n.memory.lastPlayerZ<<'\n'
           <<n.memory.observedWalk<<' '<<n.memory.observedJump<<' '
           <<n.memory.observedSit<<' '<<n.memory.observedDrive<<'\n'
           <<n.memory.lastEvent<<'\n';
    }
}
static float playerAttackCooldown=0.0f;
static void attackNearestNPC(bool kick) {
    if(drivingCar || inAirplane || ridingHorse || sleeping || playerAttackCooldown>0.0f) return;
    float fx=sinf(yaw), fz=-cosf(yaw);
    NPC* target=nullptr;
    float best=kick?3.1f:2.25f;
    for(NPC& n:npcs) {
        float dx=n.x-playerX, dz=n.z-playerZ;
        float d=sqrtf(dx*dx+dz*dz);
        float facing=(dx*fx+dz*fz)/(d>0.01f?d:1.0f);
        if(d<best && facing>0.10f) { best=d; target=&n; }
    }
    if(target) {
        float dx=target->x-playerX, dz=target->z-playerZ;
        float d=sqrtf(dx*dx+dz*dz); if(d<0.01f) d=1.0f;
        float force=kick?10.5f:7.0f;
        target->knockVX=dx/d*force; target->knockVZ=dz/d*force;
        target->hitStun=kick?1.05f:0.78f;
        target->health=std::max(0,target->health-(kick?35:20));
        target->task=3; target->playerOrdered=true;
        target->memory.conflicts++;
        target->memory.lastEvent=kick?"The player kicked me":"The player slapped me";
    }
    playerAttackCooldown=kick?0.48f:0.34f;
}
static void updateNPCs(float dt) {
    initNPCs();
    npcSaveTimer+=dt;
    playerAttackCooldown=std::max(0.0f,playerAttackCooldown-dt);
    for(size_t i=0;i<npcs.size();++i) {
        NPC& n=npcs[i];
        if(n.alienAbducting || n.alienCaptured) continue;
        n.phase+=dt*5.0f;
        n.workPhase+=dt;
        // Simple experience-based adaptation from the player's repeated actions.
        int learnedTotal=n.memory.observedWalk+n.memory.observedJump+
                         n.memory.observedSit+n.memory.observedDrive;
        if(learnedTotal>20 && n.thinkTimer<=0.0f && !n.playerOrdered) {
            if(n.memory.observedSit>n.memory.observedWalk/2) {
                n.task=7; n.waitTimer=2.5f;
            } else if(n.memory.observedWalk>12) {
                n.task=0;
            }
            n.thinkTimer=3.0f;
        }
        n.thinkTimer=std::max(0.0f,n.thinkTimer-dt);
        n.actionTimer-=dt;
        if(n.hitStun>0.0f) {
            n.hitStun=std::max(0.0f,n.hitStun-dt);
            float nx=n.x+n.knockVX*dt, nz=n.z+n.knockVZ*dt;
            if(!collides(nx,n.z)) n.x=nx;
            if(!collides(n.x,nz)) n.z=nz;
            n.knockVX*=0.88f; n.knockVZ*=0.88f;
            continue;
        }
        if(n.health<=0) { n.health=100; n.playerOrdered=false; n.task=0; n.hitStun=1.2f; }
        float dx=playerX-n.x,dz=playerZ-n.z;
        float playerDist=sqrtf(dx*dx+dz*dz);
        if(playerDist<5.0f && n.waitTimer<=0.0f && !n.playerOrdered) {
            n.memory.meetings++;
            n.memory.lastPlayerX=playerX; n.memory.lastPlayerZ=playerZ;
            n.memory.lastSeenTick=SDL_GetTicks();
            n.memory.lastEvent="Met the player near ("+
                std::to_string((int)playerX)+","+std::to_string((int)playerZ)+")";
            n.task=3; n.targetX=playerX; n.targetZ=playerZ;
            n.waitTimer=18.0f;
        }
        n.waitTimer=std::max(0.0f,n.waitTimer-dt);
        n.thinkTimer-=dt;
        if(n.thinkTimer<=0.0f) {
            n.thinkTimer=5.0f+(float)(rand()%60)/10.0f;
            if(n.task==3 && playerDist>7.0f && !n.playerOrdered) n.task=0;
            if(n.task!=3 && !n.playerOrdered) {
                // Each resident independently chooses a nearby activity.
                int choice=rand()%100;
                if(choice<22) { // dig a small garden plot
                    n.task=4; n.targetX=n.homeX+((int)i%3-1)*1.1f; n.targetZ=n.homeZ+2.0f;
                    n.memory.lastEvent="Started digging a garden plot";
                } else if(choice<42) { // build/repair
                    n.task=5; n.targetX=n.houseX; n.targetZ=n.houseZ;
                    n.memory.lastEvent="Started building a personal home in the distant village";
                } else if(choice<62) { // farm
                    n.task=6; n.targetX=n.homeX-2.0f; n.targetZ=n.homeZ+1.5f;
                    n.memory.lastEvent="Started tending a garden";
                } else if(choice<78) { // sleep/rest
                    n.task=7; n.targetX=n.homeX+0.9f; n.targetZ=n.homeZ+0.2f;
                    n.memory.lastEvent="Went home to rest";
                } else { // explore nearby
                    n.task=0;
                    float a=(float)(rand()%628)/100.0f;
                    float r=3.0f+(float)(rand()%45)/10.0f;
                    n.targetX=n.homeX+sinf(a)*r;
                    n.targetZ=n.homeZ+cosf(a)*r;
                    n.memory.lastEvent="Went exploring nearby";
                }
                n.actionTimer=7.0f+(float)(rand()%100)/10.0f;
            }
        }
        // Build each resident's own house only after reaching their distant plot.
        if(n.task==5) {
            float hx=n.x-n.houseX, hz=n.z-n.houseZ;
            if(hx*hx+hz*hz<5.0f) {
                n.houseProgress=std::min(100.0f,n.houseProgress+dt*2.0f);
                if(n.houseProgress>=100.0f) { n.houseBuilt=true; n.playerOrdered=false; n.task=0; n.thinkTimer=2.0f; }
            }
        }
        // An activity completes after its work interval; choose another on the next think.
        if(n.actionTimer<=0.0f && n.task>=4 && n.task<=7) n.thinkTimer=0.0f;
        dx=n.targetX-n.x; dz=n.targetZ-n.z;
        float d=sqrtf(dx*dx+dz*dz);
        if(d>0.32f) {
            float step=std::min(d,n.speed*dt);
            float nx=n.x+dx/d*step, nz=n.z+dz/d*step;
            if(!collides(nx,nz)) { n.x=nx; n.z=nz; }
            else if(!collides(nx,n.z)) n.x=nx;
            else if(!collides(n.x,nz)) n.z=nz;
            else {
                // Don't freeze against a wall: pick a new nearby reachable point.
                float a=(float)(rand()%628)/100.0f;
                n.targetX=n.x+sinf(a)*2.0f; n.targetZ=n.z+cosf(a)*2.0f;
                n.thinkTimer=1.0f;
            }
            n.yaw=atan2f(dx,dz);
        }
    }
    if(npcSaveTimer>=8.0f) { saveNPCMemory(); npcSaveTimer=0.0f; }
}
static std::string upperNPC(std::string v) {
    for(char& c:v) c=(char)std::toupper((unsigned char)c);
    return v;
}
static void executeNPCCommand() {
    std::string cmd=upperNPC(npcCommandBuffer);
    npcCommandBuffer.clear();
    if(cmd.empty()) return;
    bool all=(cmd.find("ALL")!=std::string::npos || cmd.find("SAB")!=std::string::npos);
    bool found=false;
    for(NPC& n:npcs) {
        if(!all && cmd.find(upperNPC(n.name))==std::string::npos) continue;
        found=true;
        n.playerOrdered=true;
        if(cmd.find("GHAR")!=std::string::npos || cmd.find("BUILD")!=std::string::npos || cmd.find("HOUSE")!=std::string::npos) {
            n.task=5; n.targetX=n.houseX; n.targetZ=n.houseZ;
            n.actionTimer=9999.0f; n.thinkTimer=9999.0f;
            n.memory.lastEvent="The player ordered me to build my own home in the distant village";
            npcCommandStatus=(all?"ALL RESIDENTS":"NPC")+std::string(" GOING TO THEIR OWN HOUSE PLOTS");
        } else if(cmd.find("CHAL")!=std::string::npos || cmd.find("JAA")!=std::string::npos || cmd.find("JAO")!=std::string::npos || cmd.find("LEAVE")!=std::string::npos || cmd.find("DOOR")!=std::string::npos) {
            float ax=n.x-playerX, az=n.z-playerZ; float len=sqrtf(ax*ax+az*az);
            if(len<0.1f) { ax=1.0f; az=0.0f; len=1.0f; }
            n.task=0; n.targetX=n.x+(ax/len)*22.0f; n.targetZ=n.z+(az/len)*22.0f;
            n.actionTimer=9999.0f; n.thinkTimer=9999.0f;
            n.memory.lastEvent="The player ordered me to leave this area";
            npcCommandStatus=(all?"ALL RESIDENTS":"NPC")+std::string(" LEAVING THE AREA");
        } else if(cmd.find("PEECHE")!=std::string::npos || cmd.find("FOLLOW")!=std::string::npos || cmd.find("IDHAR")!=std::string::npos || cmd.find("HERE")!=std::string::npos || cmd.find("AAO")!=std::string::npos) {
            n.task=3; n.targetX=playerX; n.targetZ=playerZ;
            n.actionTimer=9999.0f; n.thinkTimer=9999.0f;
            n.memory.lastEvent="The player asked me to follow or approach";
            npcCommandStatus=n.name+" IS FOLLOWING YOU";
        } else if(cmd.find("SO")!=std::string::npos || cmd.find("SLEEP")!=std::string::npos) {
            n.task=7; n.targetX=n.homeX+0.9f; n.targetZ=n.homeZ+0.2f;
            n.actionTimer=9999.0f; n.thinkTimer=9999.0f;
            n.memory.lastEvent="The player told me to sleep";
            npcCommandStatus=n.name+" IS GOING TO SLEEP";
        } else {
            n.playerOrdered=false;
            npcCommandStatus=n.name+": TRY BUILD HOUSE, CHAL JAO, FOLLOW ME OR SLEEP";
        }
        if(!all) break;
    }
    if(!found) npcCommandStatus="NAME NOT FOUND - TRY AARAV, MEERA, KABIR OR TYPE ALL";
}
static void drawNPCs(const float* v) {
    for(const NPC& n:npcs) {
        if(n.alienCaptured) continue;
        if(n.alienAbducting) {
            if(n.alienBeingLifted) {
                float liftY=0.65f+n.alienLift;
                // Pulsing cyan tractor beam and the resident suspended inside it.
                float pulse=0.62f+0.18f*sinf((float)SDL_GetTicks()*0.012f);
                cube(v,n.x,liftY*0.50f,n.z,0.62f, liftY*0.50f,0.62f,
                     0.08f,0.88f,1.0f);
                cubeYaw(v,n.x,liftY,n.z,0.27f,0.43f,0.19f,n.yaw,
                        0.18f,0.62f,0.88f);
                sphere(v,n.x,liftY+0.65f,n.z,0.24f,0.26f,0.24f,
                       0.72f,0.52f,0.38f);
                sphere(v,n.x,liftY+0.72f,n.z-0.20f,0.07f,0.06f,0.05f,
                       0.10f,pulse,1.0f);
            } else {
                // Struggling pose while an alien escorts the resident to its ship.
                cubeYaw(v,n.x,0.82f,n.z,0.30f,0.20f,0.72f,n.yaw+1.5707f,
                        0.18f,0.62f,0.88f);
                sphere(v,n.x,1.03f,n.z-0.60f,0.22f,0.22f,0.22f,
                       0.72f,0.52f,0.38f);
            }
            continue;
        }
        float bob=0.035f*sinf(n.phase);
        float walk=sinf(n.phase)*0.48f;
        float work=sinf(n.workPhase*7.0f);
        float cr=n.role==1?0.15f:(n.role==2?0.12f:(n.role==3?0.18f:0.22f));
        float cg=n.role==1?0.45f:(n.role==2?0.22f:(n.role==3?0.35f:0.42f));
        float cb=n.role==1?0.82f:(n.role==2?0.78f:(n.role==3?0.88f:0.32f));

        // Every resident has a visible little bed at their home.
        cube(v,n.homeX+0.9f,0.22f,n.homeZ+0.2f,0.72f,0.18f,1.25f,0.34f,0.19f,0.12f);
        cube(v,n.homeX+0.9f,0.39f,n.homeZ-0.65f,0.66f,0.12f,0.32f,0.88f,0.86f,0.78f);

        if(n.task==7) {
            // Lying down on the bed while resting.
            cubeYaw(v,n.x,0.72f,n.z,0.30f,0.22f,0.72f,1.5707f,cr,cg,cb);
            sphere(v,n.x,0.91f,n.z-0.60f,0.22f,0.22f,0.22f,0.72f,0.52f,0.38f);
            continue;
        }

        // A slap or kick makes the resident fall backward before standing again.
        if(n.hitStun>0.0f) {
            cubeYaw(v,n.x,0.48f,n.z,0.30f,0.18f,0.72f,1.5707f,cr,cg,cb);
            sphere(v,n.x,0.67f,n.z-0.62f,0.22f,0.22f,0.22f,0.72f,0.52f,0.38f);
            continue;
        }
        // Torso, head, face.
        cubeYaw(v,n.x,1.05f+bob,n.z,0.27f,0.43f,0.19f,n.yaw,cr,cg,cb);
        sphere(v,n.x,1.70f+bob,n.z,0.24f,0.26f,0.24f,0.72f,0.52f,0.38f);
        sphere(v,n.x-0.085f,1.75f+bob,n.z-0.205f,0.032f,0.04f,0.025f,0.03f,0.02f,0.02f);
        sphere(v,n.x+0.085f,1.75f+bob,n.z-0.205f,0.032f,0.04f,0.025f,0.03f,0.02f,0.02f);

        float armL=0.0f,armR=0.0f,legL=walk,legR=-walk;
        if(n.task==4) { armL=-0.9f+work*0.45f; armR=-0.9f-work*0.45f; }
        else if(n.task==5) { armL=-1.1f+work*0.25f; armR=-1.1f-work*0.25f; }
        else if(n.task==6) { armL=-0.65f+work*0.3f; armR=-0.65f-work*0.3f; }
        else { armL=walk*0.55f; armR=-walk*0.55f; }

        // Animated arms and legs.
        cubeYaw(v,n.x-0.34f,1.08f+bob,n.z,0.085f,0.34f,0.09f,n.yaw+armL,0.72f,0.52f,0.38f);
        cubeYaw(v,n.x+0.34f,1.08f+bob,n.z,0.085f,0.34f,0.09f,n.yaw+armR,0.72f,0.52f,0.38f);
        cubeYaw(v,n.x-0.13f,0.43f+bob,n.z,0.09f,0.34f,0.10f,n.yaw+legL,0.12f,0.12f,0.16f);
        cubeYaw(v,n.x+0.13f,0.43f+bob,n.z,0.09f,0.34f,0.10f,n.yaw+legR,0.12f,0.12f,0.16f);

        // Work props and animated tools.
        if(n.task==4) { // shovel + freshly turned soil
            cubeYaw(v,n.x+0.48f,0.72f,n.z-0.12f,0.045f,0.48f,0.045f,-0.35f,0.48f,0.27f,0.12f);
            cubeYaw(v,n.x+0.58f,0.40f,n.z-0.23f,0.18f,0.06f,0.10f,-0.35f,0.35f,0.20f,0.10f);
            sphere(v,n.targetX,0.08f,n.targetZ,0.65f,0.10f,0.48f,0.28f,0.18f,0.09f);
        } else if(n.task==5) { // stacking blocks
            cube(v,npcBuildX,0.20f,npcBuildZ,0.48f,0.20f,0.42f,0.55f,0.31f,0.18f);
            cube(v,npcBuildX+0.22f,0.55f,npcBuildZ,0.42f,0.18f,0.40f,0.62f,0.38f,0.22f);
            cubeYaw(v,n.x+0.48f,1.18f+fabsf(work)*0.12f,n.z-0.18f,0.13f,0.13f,0.13f,n.yaw,0.75f,0.55f,0.28f);
        } else if(n.task==6) { // planting/watering
            cube(v,n.targetX,0.12f,n.targetZ,0.58f,0.10f,0.48f,0.25f,0.16f,0.08f);
            cube(v,n.targetX,0.43f,n.targetZ,0.07f,0.30f,0.07f,0.12f,0.42f,0.12f);
            sphere(v,n.targetX,0.62f,n.targetZ,0.22f,0.16f,0.22f,0.18f,0.62f,0.16f);
        }
    }
    // Each resident owns a separate remote plot and visibly constructs a personal house.
    for(const NPC& n:npcs) {
        cube(v,n.houseX,0.05f,n.houseZ,4.2f,0.08f,4.2f,0.30f,0.27f,0.22f);
        if(n.houseProgress>0.0f || n.houseBuilt) {
            float h=std::min(3.2f,n.houseProgress*0.032f);
            float wallH=std::max(0.12f,h);
            cube(v,n.houseX-1.75f,wallH*0.5f,n.houseZ,0.12f,wallH*0.5f,1.9f,0.66f,0.40f,0.25f);
            cube(v,n.houseX+1.75f,wallH*0.5f,n.houseZ,0.12f,wallH*0.5f,1.9f,0.66f,0.40f,0.25f);
            cube(v,n.houseX,wallH*0.5f,n.houseZ-1.75f,1.75f,wallH*0.5f,0.12f,0.62f,0.36f,0.22f);
            cube(v,n.houseX,wallH*0.5f,n.houseZ+1.75f,1.75f,wallH*0.5f,0.12f,0.62f,0.36f,0.22f);
            if(n.houseProgress>65.0f || n.houseBuilt)
                cube(v,n.houseX,wallH+0.16f,n.houseZ,1.95f,0.16f,1.95f,0.32f,0.15f,0.10f);
        }
    }
}
static std::vector<Animal> animals = {
    {-11.0f,22.0f,0.7f,0.75f,0.0f,0}, {11.0f,20.0f,-1.2f,0.65f,1.0f,0},
    {-18.0f,38.0f,1.8f,0.55f,2.0f,1}, {18.0f,40.0f,-2.0f,0.60f,0.5f,1},
    {-4.0f,31.0f,0.4f,0.50f,1.5f,0}, {5.0f,38.0f,-0.8f,0.45f,2.5f,1}
};
struct ThrownFruit { float x,z,vx,vz,life; };
static std::vector<ThrownFruit> thrownFruits;
static SDL_AudioDeviceID gAudioDevice = 0;
static volatile int gEngineAudioOn = 0;
static volatile int gEngineStartSamples = 0;
static volatile float gEngineThrottle = 0.0f;
static double gEnginePhase = 0.0;
static double gTurboPhase = 0.0;

struct Item {
    std::string name;
    float x, z;
    bool active;
    float r, g, b;
};
static std::vector<Item> items = {
    {"House key", -5.8f, -4.5f, true, 0.95f, 0.78f, 0.12f},
    {"Medkit", 5.7f, -4.8f, true, 0.9f, 0.12f, 0.12f},
    {"Battery", -6.5f, -10.2f, true, 0.15f, 0.75f, 0.95f},
    {"Ammo box", 6.0f, -10.0f, true, 0.9f, 0.65f, 0.12f},
    {"Grocery: Milk", 91.0f, 82.0f, true, 0.92f, 0.92f, 0.82f},
    {"Grocery: Bread", 93.0f, 82.0f, true, 0.72f, 0.48f, 0.22f},
    {"Grocery: Apple", 95.0f, 82.0f, true, 0.88f, 0.12f, 0.10f},
    {"Grocery: Juice", 97.0f, 82.0f, true, 0.96f, 0.58f, 0.08f},
    {"Grocery: Cereal", 91.0f, 85.0f, true, 0.92f, 0.72f, 0.18f},
    {"Grocery: Chips", 93.0f, 85.0f, true, 0.92f, 0.32f, 0.08f},
    {"Grocery: Water", 95.0f, 85.0f, true, 0.18f, 0.68f, 0.92f},
    {"Grocery: Pizza", 97.0f, 85.0f, true, 0.80f, 0.22f, 0.12f}
};

struct Box { float x,z,hx,hz; };
static std::vector<Box> obstacles;

static void multiply(float* out, const float* a, const float* b) {
    float t[16];
    for (int c=0;c<4;c++) for (int r=0;r<4;r++) {
        t[c*4+r]=0.0f;
        for (int k=0;k<4;k++) t[c*4+r]+=a[k*4+r]*b[c*4+k];
    }
    for(int i=0;i<16;i++) out[i]=t[i];
}

static void perspective(float* m,float fov,float aspect,float n,float f) {
    for(int i=0;i<16;i++) m[i]=0.0f;
    float q=1.0f/tanf(fov*0.5f);
    m[0]=q/aspect; m[5]=q;
    m[10]=(f+n)/(n-f); m[11]=-1.0f;
    m[14]=(2.0f*f*n)/(n-f);
}

static void viewMatrix(float* m) {
    float cameraYaw=inRocket?rocketYaw:(drivingCar?carYaw:yaw);
    // On upper rooftop levels, look down more steeply so the parapet
    // cannot hide the player's torso and legs from the follow camera.
    float cameraPitch = inRocket ? rocketPitch :
                        (onAyushBike ? -0.38f :
                         pitch - (rooftopLevel==1 ? 0.48f : (rooftopLevel>0 ? 0.32f : 0.0f)));
    float cp=cosf(cameraPitch), sp=sinf(cameraPitch), cy=cosf(cameraYaw), sy=sinf(cameraYaw);
    float fx=-sy*cp, fy=sp, fz=-cy*cp;
    float rx=cy, rz=-sy;
    float ux=sy*sp, uy=cp, uz=cy*sp;
    for(int i=0;i<16;i++) m[i]=0.0f;
    m[0]=rx; m[4]=0; m[8]=rz;
    m[1]=ux; m[5]=uy; m[9]=uz;
    m[2]=-fx; m[6]=-fy; m[10]=-fz;
    // First-person driver's seat camera while inside the car.
    // Other movement modes keep the original third-person follow camera.
    float camX, camZ, eyeY;
    if(inRocket) {
        camX=playerX+sinf(rocketYaw)*7.0f;
        camZ=playerZ+cosf(rocketYaw)*7.0f;
        eyeY=rocketAltitude+5.2f;
    } else if(drivingCar) {
        camX=carX0+sinf(carYaw)*0.12f;
        camZ=carZ0+cosf(carYaw)*0.12f;
        eyeY=1.43f+hillHeightAt(carX0,carZ0)+carVisualHeight;
    } else if(sleeping) {
        camX=playerX+sinf(yaw)*4.2f;
        camZ=playerZ+cosf(yaw)*4.2f;
        eyeY=1.55f+playerHeight;
    } else {
        float followDist=onAyushBike?7.2f:(inAirplane?9.0f:(rooftopLevel==1?3.2f:5.0f));
        float viewX=inAirplane?planeX:playerX;
        float viewZ=inAirplane?planeZ:playerZ;
        float viewYaw=inAirplane?planeYaw:yaw;
        float viewHeight=inAirplane?planeAltitude:playerHeight;
        camX=viewX+sinf(viewYaw)*followDist;
        camZ=viewZ+cosf(viewYaw)*followDist;
        if(onAyushBike) {
            camX += cosf(viewYaw)*2.2f;
            camZ -= sinf(viewYaw)*2.2f;
        }
        eyeY=onAyushBike?3.65f+viewHeight:
             1.65f+viewHeight+(inAirplane?2.5f:(rooftopLevel==1?1.95f:(rooftopLevel>0?2.7f:1.6f)));
    }
    m[12]=-(rx*camX+rz*camZ);
    m[13]=-(ux*camX+uy*eyeY+uz*camZ);
    m[14]=fx*camX+fy*eyeY+fz*camZ;
    m[15]=1.0f;
}


static bool companionScreenHit(float touchX,float touchY) {
    if(gWidth<=0 || gHeight<=0) return false;
    float proj[16],view[16],vp[16];
    perspective(proj,PI/3.0f,(float)gWidth/(float)gHeight,0.1f,420.0f);
    viewMatrix(view);
    multiply(vp,proj,view);
    // Test the visible body area, not an arbitrary part of the screen.
    float wx=companionX, wy=1.12f, wz=companionZ;
    float cx=vp[0]*wx+vp[4]*wy+vp[8]*wz+vp[12];
    float cy=vp[1]*wx+vp[5]*wy+vp[9]*wz+vp[13];
    float cw=vp[3]*wx+vp[7]*wy+vp[11]*wz+vp[15];
    if(cw<=0.05f) return false;
    float sx=(cx/cw+1.0f)*0.5f;
    float sy=(1.0f-cy/cw)*0.5f;
    // Compact tap target centered on the companion's on-screen body.
    float rx=std::max(0.035f,std::min(0.11f,0.22f/cw));
    float ry=std::max(0.055f,std::min(0.17f,0.34f/cw));
    float dx=(touchX-sx)/rx, dy=(touchY-sy)/ry;
    return dx*dx+dy*dy<=1.0f;
}

static bool childrenScreenHit(float touchX,float touchY) {
    if(gWidth<=0 || gHeight<=0 || childrenGoingStudy) return false;
    float proj[16],view[16],vp[16];
    perspective(proj,PI/3.0f,(float)gWidth/(float)gHeight,0.1f,420.0f);
    viewMatrix(view);
    multiply(vp,proj,view);
    for(int child=0;child<2;child++) {
        float wx=childWorldX[child];
        float wy=1.55f, wz=childWorldZ[child];
        float cx=vp[0]*wx+vp[4]*wy+vp[8]*wz+vp[12];
        float cy=vp[1]*wx+vp[5]*wy+vp[9]*wz+vp[13];
        float cw=vp[3]*wx+vp[7]*wy+vp[11]*wz+vp[15];
        if(cw<=0.05f) continue;
        float sx=(cx/cw+1.0f)*0.5f, sy=(1.0f-cy/cw)*0.5f;
        float rx=std::max(0.045f,std::min(0.14f,0.30f/cw));
        float ry=std::max(0.07f,std::min(0.20f,0.42f/cw));
        float dx=(touchX-sx)/rx, dy=(touchY-sy)/ry;
        if(dx*dx+dy*dy<=1.0f) return true;
    }
    return false;
}

static void makeModel(float* m,float x,float y,float z,float sx,float sy,float sz) {
    for(int i=0;i<16;i++) m[i]=0.0f;
    m[0]=sx; m[5]=sy; m[10]=sz;
    m[12]=x; m[13]=y; m[14]=z; m[15]=1.0f;
}

static GLuint compileShader(GLenum type,const char* src) {
    GLuint s=glCreateShader(type);
    glShaderSource(s,1,&src,nullptr);
    glCompileShader(s);
    GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok) {
        char log[1024]={0}; glGetShaderInfoLog(s,sizeof(log),nullptr,log);
        SDL_Log("Shader compile error: %s",log);
    }
    return s;
}

static void initShaders() {
    const char* vs =
        "attribute vec3 aPosition;"
        "uniform mat4 uMVP;"
        "uniform vec3 uLight;"
        "varying float vShade;"
        "void main(){"
        " vec3 n=normalize(aPosition);"
        " vShade=0.68+0.32*max(dot(n,normalize(uLight)),0.0);"
        " gl_Position=uMVP*vec4(aPosition,1.0);"
        "}";
    const char* fs =
        "precision mediump float;"
        "uniform vec4 uColor;"
        "varying float vShade;"
        "void main(){ gl_FragColor=vec4(uColor.rgb*vShade,uColor.a); }";
    GLuint v=compileShader(GL_VERTEX_SHADER,vs);
    GLuint f=compileShader(GL_FRAGMENT_SHADER,fs);
    gProgram=glCreateProgram();
    glAttachShader(gProgram,v); glAttachShader(gProgram,f);
    glBindAttribLocation(gProgram,0,"aPosition");
    glLinkProgram(gProgram);
    GLint ok=0; glGetProgramiv(gProgram,GL_LINK_STATUS,&ok);
    if(!ok) SDL_Log("Shader link failed");
    gPosition=glGetAttribLocation(gProgram,"aPosition");
    gMVP=glGetUniformLocation(gProgram,"uMVP");
    gColor=glGetUniformLocation(gProgram,"uColor");
    gLight=glGetUniformLocation(gProgram,"uLight");
}

static void initMeshes() {
    const float cube[] = {
        -1,-1,1, 1,-1,1, 1,1,1, -1,-1,1, 1,1,1, -1,1,1,
        1,-1,-1, -1,-1,-1, -1,1,-1, 1,-1,-1, -1,1,-1, 1,1,-1,
        -1,-1,-1, -1,-1,1, -1,1,1, -1,-1,-1, -1,1,1, -1,1,-1,
        1,-1,1, 1,-1,-1, 1,1,-1, 1,-1,1, 1,1,-1, 1,1,1,
        -1,1,1, 1,1,1, 1,1,-1, -1,1,1, 1,1,-1, -1,1,-1,
        -1,-1,-1, 1,-1,-1, 1,-1,1, -1,-1,-1, 1,-1,1, -1,-1,1
    };
    glGenBuffers(1,&gCubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER,gCubeVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(cube),cube,GL_STATIC_DRAW);

    std::vector<float> s;
    const int rings=18, segs=24;
    for(int i=0;i<rings;i++) {
        float p1=-PI*0.5f+PI*i/rings, p2=-PI*0.5f+PI*(i+1)/rings;
        for(int j=0;j<segs;j++) {
            float t1=2*PI*j/segs, t2=2*PI*(j+1)/segs;
            float v[4][3]={
                {cosf(p1)*cosf(t1),sinf(p1),cosf(p1)*sinf(t1)},
                {cosf(p1)*cosf(t2),sinf(p1),cosf(p1)*sinf(t2)},
                {cosf(p2)*cosf(t2),sinf(p2),cosf(p2)*sinf(t2)},
                {cosf(p2)*cosf(t1),sinf(p2),cosf(p2)*sinf(t1)}
            };
            int idx[6]={0,1,2,0,2,3};
            for(int k=0;k<6;k++) for(int a=0;a<3;a++) s.push_back(v[idx[k]][a]);
        }
    }
    glGenBuffers(1,&gSphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER,gSphereVBO);
    glBufferData(GL_ARRAY_BUFFER,s.size()*sizeof(float),s.data(),GL_STATIC_DRAW);
}

static void drawMesh(GLuint buffer,int count,const float* vp,
                     float x,float y,float z,float sx,float sy,float sz,
                     float r,float g,float b) {
    float md[16],mvp[16]; makeModel(md,x,y,z,sx,sy,sz);
    multiply(mvp,vp,md);
    glUniformMatrix4fv(gMVP,1,GL_FALSE,mvp);
    glUniform4f(gColor,r,g,b,1.0f);
    glBindBuffer(GL_ARRAY_BUFFER,buffer);
    glEnableVertexAttribArray(gPosition);
    glVertexAttribPointer(gPosition,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glDrawArrays(GL_TRIANGLES,0,count);
}
static void cube(const float* vp,float x,float y,float z,float sx,float sy,float sz,float r,float g,float b) {
    drawMesh(gCubeVBO,36,vp,x,y,z,sx,sy,sz,r,g,b);
}
static void sphere(const float* vp,float x,float y,float z,float sx,float sy,float sz,float r,float g,float b) {
    drawMesh(gSphereVBO,18*24*6,vp,x,y,z,sx,sy,sz,r,g,b);
}

static void makeModelYaw(float* m,float x,float y,float z,float sx,float sy,float sz,float a) {
    float c=cosf(a), q=sinf(a);
    for(int i=0;i<16;i++) m[i]=0.0f;
    m[0]=c*sx; m[2]=-q*sx;
    m[5]=sy;
    m[8]=q*sz; m[10]=c*sz;
    m[12]=x; m[13]=y; m[14]=z; m[15]=1.0f;
}
static void drawMeshYaw(GLuint buffer,int count,const float* vp,
                        float x,float y,float z,float sx,float sy,float sz,
                        float a,float r,float g,float b) {
    float md[16],mvp[16];
    makeModelYaw(md,x,y,z,sx,sy,sz,a);
    multiply(mvp,vp,md);
    glUniformMatrix4fv(gMVP,1,GL_FALSE,mvp);
    glUniform4f(gColor,r,g,b,1.0f);
    glBindBuffer(GL_ARRAY_BUFFER,buffer);
    glEnableVertexAttribArray(gPosition);
    glVertexAttribPointer(gPosition,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glDrawArrays(GL_TRIANGLES,0,count);
}
static void cubeYaw(const float* vp,float x,float y,float z,float sx,float sy,float sz,
                    float a,float r,float g,float b) {
    drawMeshYaw(gCubeVBO,36,vp,x,y,z,sx,sy,sz,a,r,g,b);
}
static void cubeSlopeX(const float* vp,float x,float y,float z,float sx,float sy,float sz,
                       float angle,float r,float g,float b) {
    float model[16]={0},mvp[16];
    float c=cosf(angle),q=sinf(angle);
    model[0]=sx; model[5]=c*sy; model[6]=q*sy;
    model[9]=-q*sz; model[10]=c*sz;
    model[12]=x; model[13]=y; model[14]=z; model[15]=1.0f;
    multiply(mvp,vp,model);
    glUniformMatrix4fv(gMVP,1,GL_FALSE,mvp);
    glUniform4f(gColor,r,g,b,1.0f);
    glBindBuffer(GL_ARRAY_BUFFER,gCubeVBO);
    glEnableVertexAttribArray(gPosition);
    glVertexAttribPointer(gPosition,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glDrawArrays(GL_TRIANGLES,0,36);
}

static void drawMeshAlpha(GLuint buffer,int count,const float* vp,
                           float x,float y,float z,float sx,float sy,float sz,
                           float a,float r,float g,float b,float alpha) {
    float md[16],mvp[16];
    makeModelYaw(md,x,y,z,sx,sy,sz,a);
    multiply(mvp,vp,md);
    glUniformMatrix4fv(gMVP,1,GL_FALSE,mvp);
    glUniform4f(gColor,r,g,b,alpha);
    glBindBuffer(GL_ARRAY_BUFFER,buffer);
    glEnableVertexAttribArray(gPosition);
    glVertexAttribPointer(gPosition,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDrawArrays(GL_TRIANGLES,0,count);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
static void cubeYawAlpha(const float* vp,float x,float y,float z,float sx,float sy,float sz,
                         float a,float r,float g,float b,float alpha) {
    drawMeshAlpha(gCubeVBO,36,vp,x,y,z,sx,sy,sz,a,r,g,b,alpha);
}
static void sphereYaw(const float* vp,float x,float y,float z,float sx,float sy,float sz,
                      float a,float r,float g,float b) {
    drawMeshYaw(gSphereVBO,18*24*6,vp,x,y,z,sx,sy,sz,a,r,g,b);
}

static void addObstacle(float x,float z,float hx,float hz) {
    obstacles.push_back({x,z,hx,hz});
}
static float hillHeightAt(float x,float z);

static bool onCollegeAccessCorridor(float x,float z) {
    // Wide drivable/walkable corridor from the mansion road to GL Bajaj.
    const float x1=-2.0f, z1=49.0f, x2=-145.0f, z2=-33.0f;
    const float dx=x2-x1, dz=z2-z1;
    const float len2=dx*dx+dz*dz;
    float t=((x-x1)*dx+(z-z1)*dz)/len2;
    t=std::max(0.0f,std::min(1.0f,t));
    float px=x1+t*dx, pz=z1+t*dz;
    float ex=x-px, ez=z-pz;
    return (ex*ex+ez*ez) <= 9.0f*9.0f;
}

static bool collides(float x,float z) {
    const float radius=0.28f;
    // Keep the long access road open, but do not disable collision once the
    // player reaches the college grounds.
    const bool insideCollegeDistrict=(x>-172.0f && x<-118.0f && z>-61.0f && z<-5.0f);
    if(onCollegeAccessCorridor(x,z) && !insideCollegeDistrict) return false;
    // Dedicated stairwell passage: ignore invisible collision inside the open shaft.
    if(x>=4.05f && x<=7.45f && z>=-8.65f && z<=-1.10f)
        return false;
    if(rooftopLevel>0) {
        // Solid perimeter of the two-storey rooftop building.
        if(x < -7.55f || x > 7.55f || z < -11.55f || z > 7.55f) return true;
        // Locked/closed rental-room doors on level two.
        if(rooftopLevel==2) {
            const float doorX[4]={-4.0f,4.0f,-4.0f,4.0f};
            const float doorZ[4]={7.65f,7.65f,-11.65f,-11.65f};
            for(int i=0;i<4;i++) if(!rentalDoorOpen[i]) {
                float dx=x-doorX[i], dz=z-doorZ[i];
                if(dx*dx+dz*dz<0.62f*0.62f) return true;
            }
        }
        return false;
    }
    // The green garden is a large open world, not a narrow strip around the house.
    // Expanded world bounds include the remote airport and its runway.
    // Open-world: do not create invisible walls at the edge of the old map.
    // Solid buildings, gates, trees and vehicles are still checked below.

    // Ayush's entrance corridor: once the bell opens the gate, bypass any
    // leftover nearby house/yard obstacle boxes through the full doorway.
    // This keeps a clear walkable path from outside the gate into the house.
    if(ayushGateAmount >= 0.75f &&
       x >= 21.8f && x <= 28.2f &&
       z >= 7.8f && z <= 13.2f) return false;

    // While on the raised terrace, prevent walking off its edges.
    // The only exit is the open stair corridor in the middle of the front rail.
    const bool insideMansionFootprint =
        (x >= -9.0f && x <= 9.0f && z >= -14.5f && z <= 11.5f);
    if(onUpperFloor && insideMansionFootprint) {
        const bool onLanding = (x >= 1.15f && x <= 8.85f &&
                                z >= -4.10f && z <= 1.35f);
        // Continue allowing the player through the foot of the stairs.
        // The previous limit (z<=5.35) trapped the player at the last step:
        // onUpperFloor was still true during collision testing for that frame.
        const bool stairExitCorridor = (x >= 4.15f && x <= 7.85f &&
                                        z > 1.20f && z <= 9.45f);
        const bool upperStoreyStairCorridor = (x >= 3.85f && x <= 7.65f &&
                                               z >= -10.05f && z <= -1.10f);
        if(!onLanding && !stairExitCorridor && !upperStoreyStairCorridor) return true;
    }

    // The single front entrance is aligned with the staircase (x ~= 6).
    if(z >= 9.45f && z <= 10.55f && (fabsf(x-6.0f) > 1.55f)) return true;

    // Closed mansion gate blocks the opening; it becomes passable only
    // after the two leaves have slid fully aside.
    if(mansionGateAmount < 0.88f &&
       z >= 9.48f && z <= 10.08f &&
       x >= 4.62f && x <= 7.38f) return true;

    // Ayush's front sliding gate: block only while its leaves are closed.
    // This check is dynamic; after ringing the bell and opening the gate,
    // the center entrance becomes walkable immediately.
    if(ayushGateAmount < 0.75f &&
       z >= 10.75f && z <= 11.55f &&
       x >= 22.5f && x <= 27.5f) return true;

    // Closed garage shutter blocks both the player and the car at the front.
    if(garageShutterAmount < 0.88f &&
       z >= 28.82f && z <= 29.48f &&
       x >= -13.75f && x <= -6.25f) return true;

    for(const Box& b:obstacles) {
        float cx=std::max(b.x-b.hx,std::min(x,b.x+b.hx));
        float cz=std::max(b.z-b.hz,std::min(z,b.z+b.hz));
        float dx=x-cx,dz=z-cz;
        if(dx*dx+dz*dz<radius*radius) return true;
    }
    return false;
}

// Delivery bike needs a wider clearance than the walking player.
// Check the bike footprint and nearby characters before moving.
static bool deliveryBlocked(float x,float z) {
    const float bikeClearance=0.78f;
    if(collides(x,z)) return true;
    const float offsets[8][2] = {
        {1.0f,0.0f},{-1.0f,0.0f},{0.0f,1.0f},{0.0f,-1.0f},
        {0.7071f,0.7071f},{-0.7071f,0.7071f},
        {0.7071f,-0.7071f},{-0.7071f,-0.7071f}
    };
    for(const auto& o:offsets) {
        if(collides(x+o[0]*bikeClearance,z+o[1]*bikeClearance)) return true;
    }
    float px=x-playerX, pz=z-playerZ;
    if(px*px+pz*pz < 1.45f*1.45f) return true;
    if(ayushWalkingWithPlayer || ayushReturningToCollege) {
        float fx=x-ayushFriendX, fz=z-ayushFriendZ;
        if(fx*fx+fz*fz < 1.35f*1.35f) return true;
    }
    for(const Animal& animal:animals) {
        float ax=x-animal.x, az=z-animal.z;
        if(ax*ax+az*az < 1.10f*1.10f) return true;
    }
    return false;
}


// Curving mountain road centerline, smoothly interpolated between waypoints.
static float mountainRoadX(float z) {
    const float zs[7]={47.0f,54.0f,61.0f,68.0f,75.0f,82.0f,90.0f};
    const float xs[7]={0.0f,-4.0f,-13.0f,-19.0f,-15.0f,-6.0f,1.0f};
    if(z<=zs[0]) return xs[0];
    if(z>=zs[6]) return xs[6];
    for(int i=0;i<6;i++) if(z>=zs[i] && z<=zs[i+1]) {
        float t=(z-zs[i])/(zs[i+1]-zs[i]);
        t=t*t*(3.0f-2.0f*t);
        return xs[i]+(xs[i+1]-xs[i])*t;
    }
    return 0.0f;
}
static bool onMountainRoad(float x,float z) {
    if(z<46.0f || z>91.0f) return false;
    float dx=x-mountainRoadX(z);
    return dx*dx<4.6f*4.6f;
}

// Wider vehicle footprint: check center and four corners against mountain slopes.
static bool vehicleCollides(float x,float z) {
    // Keep full vehicle SAT obstacle checks even on the campus access road.
    // The corridor opens player movement only; it must never disable car-vs-
    // building, parked-car, tree, gate, or terrain collision.
    // Car footprint is an oriented rectangle. Test it against every solid
    // building/obstacle with the Separating Axis Theorem (SAT), not just a
    // handful of points; this prevents corners from clipping into buildings.
    const float halfW=1.48f;
    const float halfL=2.65f;
    const float margin=0.06f;
    const float cs=cosf(carYaw), sn=sinf(carYaw);
    if(collides(x,z)) return true;

    for(const Box& b:obstacles) {
        float dx=x-b.x, dz=z-b.z;
        // World X and Z axes.
        if(fabsf(dx) > b.hx + halfW*fabsf(sn) + halfL*fabsf(cs) + margin) continue;
        if(fabsf(dz) > b.hz + halfW*fabsf(cs) + halfL*fabsf(sn) + margin) continue;
        // Match the car's actual movement basis:
        // forward=(-sin(yaw),-cos(yaw)), side=(cos(yaw),-sin(yaw)).
        // The previous SAT used the opposite Z signs, so a turning car
        // could pass through obstacles at many steering angles.
        float carSide=fabsf(dx*cs - dz*sn);
        float obstacleSide=b.hx*fabsf(cs) + b.hz*fabsf(sn);
        if(carSide > halfW + obstacleSide + margin) continue;
        float carForward=fabsf(-dx*sn - dz*cs);
        float obstacleForward=b.hx*fabsf(sn) + b.hz*fabsf(cs);
        if(carForward > halfL + obstacleForward + margin) continue;
        return true;
    }

    // Keep the complete car footprint out of steep terrain.
    const float samples[9][2]={{0,0},{-halfW,-halfL},{halfW,-halfL},
        {-halfW,halfL},{halfW,halfL},{0,-halfL},{0,halfL},{-halfW,0},{halfW,0}};
    for(int i=0;i<9;i++) {
        float wx=x+samples[i][0]*cs+samples[i][1]*sn;
        float wz=z-samples[i][0]*sn+samples[i][1]*cs;
        if(hillHeightAt(wx,wz)>0.42f && !onMountainRoad(wx,wz) && carRideHeightAt(wx,wz)<0.01f) return true;
    }
    return false;
}

// Sweep the car along its attempted movement so high speed cannot skip
// through a wall or building between two frames.
static bool vehiclePathCollides(float fromX,float fromZ,float toX,float toZ) {
    float dx=toX-fromX, dz=toZ-fromZ;
    float distance=sqrtf(dx*dx+dz*dz);
    int steps=std::max(1,(int)ceilf(distance/0.15f));
    for(int i=1;i<=steps;i++) {
        float t=(float)i/(float)steps;
        if(vehicleCollides(fromX+dx*t,fromZ+dz*t)) return true;
    }
    return false;
}

static void setupCollision() {
    obstacles.clear();
    // Main academic block perimeter is solid; front doorway remains open.
    addObstacle(-170.0f,-48.0f,0.22f,7.0f);
    addObstacle(-120.0f,-48.0f,0.22f,7.0f);
    addObstacle(-145.0f,-55.0f,25.0f,0.22f);
    addObstacle(-158.0f,-41.0f,12.0f,0.22f);
    addObstacle(-132.0f,-41.0f,12.0f,0.22f);
    // GL BAJAJ COLLEGE: solid campus boundary, classroom and furniture.
    // Campus center is (-145,-48). Classroom is an attached, open-top interior
    // in front of the academic block; the rear doorway faces the arrival road.
    addObstacle(-155.2f,-27.5f,0.22f,9.0f);  // classroom left wall
    addObstacle(-134.8f,-27.5f,0.22f,9.0f);  // classroom right wall
    addObstacle(-145.0f,-36.5f,10.0f,0.22f); // classroom wall at academic-block side
    addObstacle(-151.5f,-18.5f,3.5f,0.22f);  // entrance wall left of doorway
    addObstacle(-138.5f,-18.5f,3.5f,0.22f);  // entrance wall right of doorway
    // Solid teacher platform.
    addObstacle(-145.0f,-33.9f,3.8f,0.55f);
    // Classroom desks/chairs (collision boxes match visible furniture).
    for(int row=0;row<3;row++) for(int col=0;col<3;col++) {
        float dx=-150.0f+col*5.0f;
        float dz=-22.5f-row*3.0f;
        addObstacle(dx,dz,0.92f,0.62f);
    }
    // Water cooler and book shelf.
    addObstacle(-135.8f,-32.2f,0.42f,0.42f);
    addObstacle(-154.0f,-32.2f,0.65f,0.42f);
    // Main campus entrance gate posts and two fixed gate wings.
    addObstacle(-154.0f,-10.0f,0.38f,0.38f);
    addObstacle(-136.0f,-10.0f,0.38f,0.38f);
    addObstacle(-151.3f,-10.0f,2.2f,0.16f);
    addObstacle(-138.7f,-10.0f,2.2f,0.16f);

    // Outer walls: front wall is split around the entrance.
    addObstacle(-8.9f,-2.0f,0.18f,12.0f);
    addObstacle( 8.9f,-2.0f,0.18f,12.0f);
    addObstacle(0.0f,-14.0f,9.0f,0.18f);
    // One front entrance aligned with the staircase.
    addObstacle(-2.25f,10.0f,6.75f,0.18f);
    addObstacle( 8.25f,10.0f,0.75f,0.18f);
    // Internal partitions, matching visible walls.
    addObstacle(-4.0f,-6.90f,0.14f,1.10f);
    addObstacle(-4.0f,-1.90f,0.14f,3.10f); // doorway gap near z=-5
    addObstacle( 4.0f,-3.0f,0.14f,5.0f);
    addObstacle(0.0f,-7.0f,3.8f,0.14f);
    // Furniture collision.
    addObstacle(-6.0f,-5.0f,1.55f,2.05f); // bed
    addObstacle(-7.5f,-10.0f,0.75f,0.65f); // wardrobe
    addObstacle(6.0f,-5.0f,1.55f,0.60f); // counter
    addObstacle(0.0f,2.0f,2.05f,1.05f); // table
    // Garden trees: collision only around trunks, leaving the central path clear.
    addObstacle(-13.0f,17.0f,0.45f,0.45f);
    addObstacle(13.0f,17.0f,0.45f,0.45f);
    addObstacle(-17.0f,29.0f,0.45f,0.45f);
    addObstacle(17.0f,29.0f,0.45f,0.45f);
    // Garden armchairs and swing: solid to vehicles.
    addObstacle(-13.8f,50.0f,0.20f,0.20f);
    addObstacle(-8.2f,50.0f,0.20f,0.20f);
    addObstacle(-11.0f,50.0f,2.9f,0.20f);

    // Pine-tree trunks on the mountain slopes.
    addObstacle(-39.0f,57.0f,0.55f,0.55f);
    addObstacle(-39.0f,70.0f,0.55f,0.55f);
    addObstacle(-31.0f,82.0f,0.55f,0.55f);
    addObstacle(39.0f,57.0f,0.55f,0.55f);
    addObstacle(39.0f,70.0f,0.55f,0.55f);
    addObstacle(29.0f,83.0f,0.55f,0.55f);

    // The nearby houses use their visible wall segments for collision.
    // Do not add full-footprint invisible blockers here: they trap the player
    // when walking around the sides and behind the houses.

    // Ayush Mishra House: solid side/rear walls and split front wall.
    // The central entrance corridor (x=22.7..27.3) stays clear.
    addObstacle(16.0f,-2.0f,0.18f,12.0f);
    addObstacle(34.0f,-2.0f,0.18f,12.0f);
    // Rear exit is open in the middle for Ayush's bike.
    addObstacle(19.25f,-14.0f,3.25f,0.18f);
    addObstacle(30.75f,-14.0f,3.25f,0.18f);
    addObstacle(19.2f,10.0f,3.2f,0.18f);
    addObstacle(30.8f,10.0f,3.2f,0.18f);
    // Ayush's sliding gate collision is checked dynamically in collides(),
    // because setupCollision() is called only once at startup.

    // Garage side walls and rear wall. The front entrance remains open.
    addObstacle(-14.0f,34.5f,0.20f,5.50f);
    addObstacle(-6.0f,34.5f,0.20f,5.50f);
    addObstacle(-10.0f,39.78f,4.20f,0.20f);

    // Solid parked cars: body footprint plus bumpers.
    // The red car is a vehicle the player can enter; it is not a static obstacle.
    addObstacle(12.0f,9.0f,1.35f,2.35f);
    addObstacle(0.0f,40.0f,1.35f,2.35f);

    // MEGA CITY: collision footprints match the visible tower blocks.
    // These stop both the player and vehicles from driving/walking through buildings.
    for(int row=0;row<5;row++) for(int col=0;col<5;col++) {
        if((row==1&&col==1)||(row==2&&col==2)||(row==3&&col==3)) continue;
        float x=70.0f+col*21.0f;
        float z=45.0f+row*25.0f;
        float w=5.0f+(float)((row+col)%3)*1.0f;
        float d=5.0f+(float)((row*2+col)%3)*0.8f;
        // Solid perimeter walls with a real front entrance gap.
        const float wt=0.22f, doorHalf=1.0f;
        addObstacle(x-w+wt,z,wt,d+0.20f);
        addObstacle(x+w-wt,z,wt,d+0.20f);
        addObstacle(x,z-d+wt,w,wt);
        addObstacle(x-w*0.5f-doorHalf*0.5f,z+d-wt,
                    (w-doorHalf)*0.5f+0.08f,wt);
        addObstacle(x+w*0.5f+doorHalf*0.5f,z+d-wt,
                    (w-doorHalf)*0.5f+0.08f,wt);
    }


    // EXPANDED MEGA CITY: 6x6 additional solid building shells.
    // Front door gaps are kept clear; streets between blocks remain open.
    for(int row=0;row<6;row++) for(int col=0;col<6;col++) {
        float bx=230.0f+col*40.0f, bz=20.0f+row*40.0f;
        const float hw=7.0f, hd=7.0f, wt=0.24f, doorHalf=1.15f;
        addObstacle(bx-hw+wt,bz,wt,hd);
        addObstacle(bx+hw-wt,bz,wt,hd);
        addObstacle(bx,bz-hd+wt,hw,wt);
        addObstacle(bx-hw*0.5f-doorHalf*0.5f,bz+hd-wt,
                    (hw-doorHalf)*0.5f+0.08f,wt);
        addObstacle(bx+hw*0.5f+doorHalf*0.5f,bz+hd-wt,
                    (hw-doorHalf)*0.5f+0.08f,wt);
    }
    // Landmark collision footprints.
    addObstacle(350.0f,140.0f,13.0f,10.0f); // shopping mall
    addObstacle(470.0f,20.0f,10.0f,9.0f);   // hospital
    addObstacle(470.0f,220.0f,15.0f,13.0f); // stadium

    // Burj Khalifa-inspired tower: solid footprint includes the whole stepped base.
    addObstacle(137.0f,91.0f,6.5f,6.5f);

    // Charminar-inspired monument: central core and four solid minaret bases.
    addObstacle(83.0f,137.0f,3.55f,3.55f);
    for(int sx=-1;sx<=1;sx+=2) for(int sz=-1;sz<=1;sz+=2)
        addObstacle(83.0f+sx*4.1f,137.0f+sz*4.1f,0.72f,0.72f);

    // Cable bridge: deck edge rails and the main support pylon are solid.
    addObstacle(112.7f,166.0f,0.20f,16.0f);
    addObstacle(123.3f,166.0f,0.20f,16.0f);
    addObstacle(118.0f,166.0f,0.55f,0.55f);

    // Supermarket walls: leave a wide entrance in the front (z ~= 72).
    addObstacle(86.8f,78.0f,0.20f,6.0f);
    addObstacle(101.2f,78.0f,0.20f,6.0f);
    addObstacle(94.0f,84.0f,7.0f,0.20f);
    addObstacle(89.1f,71.85f,2.0f,0.16f);
    addObstacle(98.9f,71.85f,2.0f,0.16f);
    // Interior shelving is solid too, with walkable aisles between shelves.
    for(int shelf=0;shelf<3;shelf++)
        addObstacle(91.0f+shelf*3.0f,83.0f,0.42f,0.50f);

    // Street-light poles are solid so cars and pedestrians cannot pass through them.
    for(int k=0;k<10;k++)
        addObstacle(105.0f,24.0f+k*19.0f,0.18f,0.18f);

    // River channel is blocked except at the low bridge crossing.
    addObstacle(-155.0f,57.0f,6.5f,38.0f);
    addObstacle(-155.0f,143.0f,6.5f,31.0f);
    // Bridge side parapets and piers. Keep deck and both approach lanes clear.
    addObstacle(-155.0f,95.65f,15.0f,0.16f);
    addObstacle(-155.0f,104.35f,15.0f,0.16f);
    // Bridge piers sit below the deck, so they are visual supports only
    // and must not block vehicles travelling across the bridge.

    // Ramp edge guards only; the central 9.4-unit driving lane is open.
    addObstacle(-125.0f,35.0f,0.16f,17.2f);
    addObstacle(-115.0f,35.0f,0.16f,17.2f);

}


static const char* blockGlyph(char c) {
    switch(c) {
        case 'A': return "01110""10001""10001""11111""10001""10001""10001";
        case 'B': return "11110""10001""10001""11110""10001""10001""11110";
        case 'C': return "01111""10000""10000""10000""10000""10000""01111";
        case 'D': return "11110""10001""10001""10001""10001""10001""11110";
        case 'E': return "11111""10000""10000""11110""10000""10000""11111";
        case 'F': return "11111""10000""10000""11110""10000""10000""10000";
        case 'G': return "01111""10000""10000""10111""10001""10001""01111";
        case 'H': return "10001""10001""10001""11111""10001""10001""10001";
        case 'I': return "11111""00100""00100""00100""00100""00100""11111";
        case 'J': return "00111""00010""00010""00010""10010""10010""01100";
        case 'K': return "10001""10010""10100""11000""10100""10010""10001";
        case 'L': return "10000""10000""10000""10000""10000""10000""11111";
        case 'M': return "10001""11011""10101""10101""10001""10001""10001";
        case 'N': return "10001""11001""10101""10011""10001""10001""10001";
        case 'O': return "01110""10001""10001""10001""10001""10001""01110";
        case 'P': return "11110""10001""10001""11110""10000""10000""10000";
        case 'T': return "11111""00100""00100""00100""00100""00100""00100";
        case 'R': return "11110""10001""10001""11110""10100""10010""10001";
        case 'S': return "01111""10000""10000""01110""00001""00001""11110";
        case 'U': return "10001""10001""10001""10001""10001""10001""01110";
        case 'Y': return "10001""10001""01010""00100""00100""00100""00100";
        case 'Z': return "11111""00001""00010""00100""01000""10000""11111";
        default: return "00000""00000""00000""00000""00000""00000""00000";
    }
}
static void drawBlockText(const float* v,float cx,float cy,float frontZ,
                          const char* text,float scale) {
    int n=(int)std::strlen(text);
    float total=n*6.0f*scale;
    float start=cx-total*0.5f;
    for(int i=0;i<n;i++) {
        const char* bits=blockGlyph(text[i]);
        for(int r=0;r<7;r++) for(int c=0;c<5;c++) {
            if(bits[r*5+c]=='1') {
                float x=start+(i*6+c)*scale;
                float y=cy+(3-r)*scale;
                cube(v,x,y,frontZ,scale*0.46f,scale*0.46f,0.025f,
                     1.00f,0.72f,0.08f);
            }
        }
    }
}



static void drawExpandedMegaCity(const float* v) {
    const float y=-0.18f;
    // Main road connects the old city to the new district.
    cube(v,190.0f,y,112.0f,92.0f,0.10f,8.0f,0.11f,0.12f,0.14f);
    cube(v,190.0f,y+0.11f,108.5f,92.0f,0.018f,0.10f,0.94f,0.86f,0.62f);
    cube(v,190.0f,y+0.11f,115.5f,92.0f,0.018f,0.10f,0.94f,0.86f,0.62f);

    // Connected 6x6 road grid.
    for(int k=0;k<=6;k++) {
        float x=210.0f+k*40.0f;
        cube(v,x,y,120.0f,4.8f,0.10f,132.0f,0.14f,0.15f,0.17f);
        float z=0.0f+k*40.0f;
        cube(v,330.0f,y,z,130.0f,0.10f,4.8f,0.14f,0.15f,0.17f);
    }
    // Junction zebra crossings.
    for(int k=0;k<6;k++) {
        float z=20.0f+k*40.0f;
        for(int j=0;j<5;j++)
            cube(v,330.0f,y+0.12f,z-1.6f+j*0.8f,1.5f,0.02f,0.12f,0.92f,0.88f,0.74f);
    }

    // Entrance gate with a broad open vehicle lane.
    cube(v,178.0f,2.2f,112.0f,0.55f,2.2f,0.55f,0.60f,0.50f,0.36f);
    cube(v,202.0f,2.2f,112.0f,0.55f,2.2f,0.55f,0.60f,0.50f,0.36f);
    cube(v,178.0f,4.6f,112.0f,0.72f,0.18f,0.72f,0.15f,0.20f,0.24f);
    cube(v,202.0f,4.6f,112.0f,0.72f,0.18f,0.72f,0.15f,0.20f,0.24f);
    cube(v,190.0f,5.0f,112.0f,12.0f,0.16f,0.22f,0.15f,0.20f,0.24f);
    drawBlockText(v,190.0f,5.25f,111.70f,"MEGA CITY",0.27f);

    // Thirty-six mixed-use buildings, each with an open front doorway.
    for(int row=0;row<6;row++) for(int col=0;col<6;col++) {
        float x=230.0f+col*40.0f, z=20.0f+row*40.0f;
        const float hw=7.0f, hd=7.0f, wt=0.24f, doorHalf=1.15f;
        float h=13.0f+(float)((row*7+col*11)%8)*2.6f;
        float r=0.24f+0.035f*(col%4), g=0.30f+0.025f*(row%4), b=0.40f+0.035f*((row+col)%4);
        // Ground-floor interior floor and solid walls.
        cube(v,x,0.10f,z,hw-0.1f,0.10f,hd-0.1f,0.34f,0.34f,0.36f);
        cube(v,x-hw+wt,2.0f,z,wt,2.0f,hd,r*0.9f,g*0.9f,b*0.9f);
        cube(v,x+hw-wt,2.0f,z,wt,2.0f,hd,r*0.9f,g*0.9f,b*0.9f);
        cube(v,x,2.0f,z-hd+wt,hw,2.0f,wt,r*0.9f,g*0.9f,b*0.9f);
        cube(v,x-hw*0.5f-doorHalf*0.5f,2.0f,z+hd-wt,(hw-doorHalf)*0.5f,2.0f,wt,r*0.9f,g*0.9f,b*0.9f);
        cube(v,x+hw*0.5f+doorHalf*0.5f,2.0f,z+hd-wt,(hw-doorHalf)*0.5f,2.0f,wt,r*0.9f,g*0.9f,b*0.9f);
        // Visible shop counter and shelves inside.
        cube(v,x-3.7f,0.72f,z-2.8f,0.75f,0.72f,0.50f,0.40f,0.25f,0.13f);
        for(int q=0;q<3;q++)
            cube(v,x-4.1f+q*0.45f,1.45f,z-5.8f,0.16f,0.14f,0.16f,
                 0.82f-0.10f*q,0.36f+0.10f*q,0.18f);
        // Upper tower and blue glass window bands.
        cube(v,x,(h+2.0f)*0.5f,z,hw,h*0.5f-1.0f,hd,r,g,b);
        for(int floor=1;floor<(int)(h/2.8f);floor++) {
            float wy=2.0f+floor*2.5f;
            cube(v,x-hw-0.025f,wy,z,0.035f,0.52f,hd*0.70f,0.16f,0.64f,0.80f);
            cube(v,x+hw+0.025f,wy,z,0.035f,0.52f,hd*0.70f,0.16f,0.64f,0.80f);
            cube(v,x,wy,z-hd-0.025f,hw*0.70f,0.52f,0.035f,0.16f,0.64f,0.80f);
        }
        cube(v,x,h+0.20f,z,hw*0.88f,0.20f,hd*0.88f,0.12f,0.16f,0.21f);
    }

    // Shopping mall.
    cube(v,350.0f,3.0f,140.0f,13.0f,3.0f,10.0f,0.60f,0.64f,0.68f);
    cube(v,350.0f,3.1f,129.8f,9.0f,2.3f,0.10f,0.12f,0.38f,0.56f);
    drawBlockText(v,350.0f,6.2f,129.65f,"MALL",0.48f);
    // Hospital.
    cube(v,470.0f,4.0f,20.0f,10.0f,4.0f,9.0f,0.84f,0.86f,0.82f);
    cube(v,470.0f,4.0f,10.8f,7.0f,3.0f,0.10f,0.14f,0.42f,0.55f);
    drawBlockText(v,470.0f,8.2f,10.65f,"HOSPITAL",0.23f);
    // Stadium.
    cube(v,470.0f,2.0f,220.0f,15.0f,2.0f,13.0f,0.32f,0.36f,0.42f);
    cube(v,470.0f,4.2f,220.0f,14.0f,0.18f,12.0f,0.72f,0.20f,0.16f);
}

static void drawMegaCity(const float* v) {
    // Continuous asphalt network: two main avenues and a connected street grid.
    const float roadY=-0.20f;
    cube(v,112,roadY,112,5.5f,0.10f,105,0.12f,0.13f,0.15f);
    cube(v,112,roadY,112,105,0.10f,5.5f,0.12f,0.13f,0.15f);
    for(int i=0;i<4;i++) {
        float x=78.0f+i*23.0f;
        float z=66.0f+i*25.0f;
        cube(v,x,roadY,z,2.6f,0.09f,105,0.16f,0.17f,0.19f);
        cube(v,112,roadY,z,105,0.09f,2.6f,0.16f,0.17f,0.19f);
    }
    // Lane dividers and crosswalks.
    for(int k=0;k<28;k++) {
        float z=18.0f+k*7.0f;
        cube(v,112,roadY+0.105f,z,0.16f,0.018f,2.0f,0.94f,0.82f,0.30f);
        float x=18.0f+k*7.0f;
        cube(v,x,roadY+0.105f,112,2.0f,0.018f,0.16f,0.94f,0.82f,0.30f);
    }
    // City blocks with varied towers, glass facades, rooftop details and lit windows.
    for(int row=0;row<5;row++) for(int col=0;col<5;col++) {
        float x=70.0f+col*21.0f;
        float z=45.0f+row*25.0f;
        if((row==1&&col==1)||(row==2&&col==2)||(row==3&&col==3)) continue;
        float h=9.0f+(float)((row*7+col*11)%8)*2.2f;
        float w=5.0f+(float)((row+col)%3)*1.0f;
        float d=5.0f+(float)((row*2+col)%3)*0.8f;
        float r=0.22f+0.04f*(col%3), g=0.30f+0.035f*(row%3), b=0.40f+0.05f*((row+col)%3);
        // Accessible ground-floor shell: four solid walls, an open front
        // doorway, a walkable floor, and visible rooms/furniture inside.
        const float wallH=1.95f, wallT=0.16f, doorHalf=0.90f;
        cube(v,x,0.10f,z,w-0.10f,0.10f,d-0.10f,0.34f,0.34f,0.36f);
        cube(v,x-w+wallT,wallH,z,wallT,wallH,d, r*0.90f,g*0.90f,b*0.90f);
        cube(v,x+w-wallT,wallH,z,wallT,wallH,d, r*0.90f,g*0.90f,b*0.90f);
        cube(v,x,wallH,z-d+wallT,w,wallH,wallT,r*0.90f,g*0.90f,b*0.90f);
        cube(v,x-w*0.5f-doorHalf*0.5f,wallH,z+d-wallT,
             (w-doorHalf)*0.5f,wallH,wallT,r*0.90f,g*0.90f,b*0.90f);
        cube(v,x+w*0.5f+doorHalf*0.5f,wallH,z+d-wallT,
             (w-doorHalf)*0.5f,wallH,wallT,r*0.90f,g*0.90f,b*0.90f);
        // Interior divider creates a living/shop zone and a rear room.
        cube(v,x,1.45f,z,0.10f,1.45f,d*0.36f,0.62f,0.59f,0.52f);
        // Every building has usable furniture; alternating units are homes and shops.
        if((row+col)%3==0) {
            cube(v,x-w*0.48f,0.62f,z-d*0.48f,0.58f,0.62f,0.52f,0.28f,0.16f,0.08f);
            cube(v,x-w*0.48f,1.28f,z-d*0.48f,0.54f,0.08f,0.48f,0.78f,0.72f,0.62f);
            cube(v,x+w*0.45f,0.48f,z-d*0.40f,0.42f,0.48f,0.42f,0.30f,0.18f,0.09f);
        } else {
            cube(v,x-w*0.42f,0.78f,z-d*0.35f,0.62f,0.16f,0.72f,0.42f,0.28f,0.15f);
            cube(v,x+w*0.40f,0.60f,z-d*0.36f,0.44f,0.60f,0.40f,0.54f,0.34f,0.20f);
            for(int shelf=0;shelf<3;shelf++)
                cube(v,x-w*0.35f+shelf*0.42f,1.25f,z-d*0.50f,
                     0.16f,0.14f,0.16f,0.82f,0.42f,0.16f);
        }
        // Upper tower mass begins above the accessible ground storey.
        if(h>4.4f) {
            cube(v,x,(h+4.0f)*0.5f,z,w,h*0.5f-2.0f,d,r,g,b);
            cube(v,x,h+0.18f,z,w*0.92f,0.18f,d*0.92f,0.12f,0.16f,0.21f);
        }
        for(int floor=2;floor<h/2.5f;floor++) {
            float wy=floor*2.2f+0.8f;
            for(int side=-1;side<=1;side+=2) {
                cube(v,x+side*(w+0.025f),wy,z,0.035f,0.52f,d*0.70f,0.20f,0.70f,0.82f);
                cube(v,x,wy,z+side*(d+0.025f),w*0.70f,0.52f,0.035f,0.20f,0.67f,0.78f);
            }
        }
        cube(v,x,h+0.55f,z,0.10f,0.55f,0.10f,0.72f,0.76f,0.82f);
    }
    // Burj Khalifa-inspired supertall stepped spire landmark.
    float bx=137.0f,bz=91.0f;
    cube(v,bx,10.0f,bz,6.2f,10.0f,6.2f,0.48f,0.62f,0.72f);
    cube(v,bx,23.0f,bz,4.4f,13.0f,4.4f,0.52f,0.68f,0.78f);
    cube(v,bx,38.0f,bz,2.8f,15.0f,2.8f,0.58f,0.73f,0.82f);
    cube(v,bx,52.0f,bz,1.25f,14.0f,1.25f,0.62f,0.78f,0.86f);
    cube(v,bx,65.0f,bz,0.18f,13.0f,0.18f,0.75f,0.82f,0.88f);
    drawBlockText(v,bx,3.0f,bz-6.28f,"BURJ",0.38f);
    // Hyderabad Charminar-inspired four-arch monument.
    float hx=83.0f,hz=137.0f;
    cube(v,hx,3.8f,hz,5.8f,0.35f,5.8f,0.72f,0.68f,0.53f);
    cube(v,hx,4.0f,hz,3.4f,3.8f,3.4f,0.80f,0.75f,0.60f);
    for(int sx=-1;sx<=1;sx+=2) for(int sz=-1;sz<=1;sz+=2) {
        cube(v,hx+sx*4.1f,6.0f,hz+sz*4.1f,0.55f,6.0f,0.55f,0.78f,0.70f,0.54f);
        sphere(v,hx+sx*4.1f,12.2f,hz+sz*4.1f,0.85f,1.1f,0.85f,0.72f,0.66f,0.50f);
        cube(v,hx+sx*4.1f,14.0f,hz+sz*4.1f,0.22f,1.2f,0.22f,0.82f,0.75f,0.58f);
    }
    // Iconic cable-stayed bridge over the city canal.
    cube(v,118,0.0f,166,5.0f,0.28f,32.0f,0.36f,0.38f,0.40f);
    cube(v,118,9.0f,166,0.28f,9.0f,0.28f,0.78f,0.80f,0.82f);
    for(int k=0;k<12;k++) {
        float z=140.0f+k*4.7f;
        cube(v,118,4.5f,z,0.045f,4.5f,0.045f,0.84f,0.86f,0.88f);
        cube(v,114,4.5f,z,0.045f,4.5f,0.045f,0.84f,0.86f,0.88f);
        cube(v,122,4.5f,z,0.045f,4.5f,0.045f,0.84f,0.86f,0.88f);
    }
    // Supermarket shell, entrance and product shelves.
    cube(v,94,3.2f,78,7.0f,3.2f,6.0f,0.76f,0.78f,0.80f);
    cube(v,94,3.25f,71.85f,5.8f,2.5f,0.10f,0.14f,0.28f,0.42f);
    cube(v,94,6.55f,78,7.2f,0.18f,6.2f,0.08f,0.18f,0.25f);
    drawBlockText(v,94,6.0f,71.72f,"MARKET",0.32f);
    // CITY SUPERMARKET interior: two long stocked aisles, wall racks and checkout.
    for(int shelf=0;shelf<3;shelf++) {
        float x=89.8f+shelf*4.0f;
        cube(v,x,1.05f,78.0f,0.32f,1.05f,2.75f,0.34f,0.24f,0.15f);
        for(int level=0;level<4;level++) {
            cube(v,x,0.48f+level*0.48f,78.0f,0.29f,0.07f,2.62f,0.72f,0.66f,0.52f);
            for(int item=0;item<5;item++) {
                float iz=76.0f+item*0.95f;
                float rr=0.25f+0.12f*((item+shelf)%4);
                float gg=0.28f+0.13f*((item+level)%4);
                float bb=0.18f+0.16f*((shelf+level)%4);
                cube(v,x,0.70f+level*0.48f,iz,0.22f,0.17f,0.22f,rr,gg,bb);
            }
        }
    }
    // Refrigerated drinks and dairy display along the rear wall.
    cube(v,94.0f,1.25f,82.55f,5.8f,1.25f,0.32f,0.20f,0.34f,0.40f);
    for(int i=0;i<8;i++)
        cube(v,89.3f+i*1.32f,1.55f,82.15f,0.28f,0.45f,0.18f,
             0.18f+0.10f*(i%3),0.50f,0.72f-0.08f*(i%4));
    // Checkout counter, POS screen and card terminal near the front-right side.
    cube(v,98.2f,0.72f,74.3f,1.35f,0.72f,0.48f,0.30f,0.34f,0.38f);
    cube(v,98.2f,1.48f,74.3f,1.22f,0.08f,0.43f,0.82f,0.82f,0.78f);
    cube(v,98.2f,1.92f,74.05f,0.48f,0.35f,0.07f,0.025f,0.08f,0.12f);
    cube(v,98.2f,1.94f,73.96f,0.39f,0.25f,0.025f,0.08f,0.55f,0.72f);
    cube(v,97.25f,1.60f,74.3f,0.18f,0.06f,0.15f,0.08f,0.12f,0.15f);
    drawBlockText(v,94.0f,6.0f,71.72f,"MARKET",0.32f);
    drawBlockText(v,98.2f,2.38f,73.95f,"PAY",0.10f);
    // FAR DISTRICT: a wide river well away from the player's mansion.
    // The blue water channel runs north-south; green banks frame both sides.
    cube(v,-155.0f,-0.38f,97.0f,6.8f,0.18f,78.0f,0.08f,0.36f,0.62f);
    cube(v,-155.0f,-0.48f,97.0f,6.1f,0.10f,77.0f,0.05f,0.22f,0.48f);
    cube(v,-166.0f,-0.28f,97.0f,4.0f,0.16f,79.0f,0.24f,0.48f,0.20f);
    cube(v,-144.0f,-0.28f,97.0f,4.0f,0.16f,79.0f,0.24f,0.48f,0.20f);

    // Low, driveable bridge deck crossing the river. The approaches rise only 0.22 units.
    cube(v,-155.0f,0.02f,100.0f,15.0f,0.20f,4.8f,0.34f,0.37f,0.40f);
    cube(v,-155.0f,0.24f,100.0f,15.0f,0.035f,4.65f,0.18f,0.20f,0.22f);
    // Solid parapets are low enough not to obstruct the car.
    cube(v,-155.0f,0.62f,95.65f,15.0f,0.28f,0.16f,0.72f,0.75f,0.78f);
    cube(v,-155.0f,0.62f,104.35f,15.0f,0.28f,0.16f,0.72f,0.75f,0.78f);
    // Flat, low approach aprons connect both river banks to the deck.
    cube(v,-174.0f,-0.12f,100.0f,4.0f,0.12f,4.5f,0.28f,0.30f,0.32f);
    cube(v,-136.0f,-0.12f,100.0f,4.0f,0.12f,4.5f,0.28f,0.30f,0.32f);
    for(int k=0;k<7;k++) {
        float x=-167.0f+k*4.0f;
        cube(v,x,2.65f,95.45f,0.10f,0.82f,0.10f,0.24f,0.25f,0.27f);
        sphere(v,x,3.52f,95.45f,0.28f,0.22f,0.28f,1.0f,0.82f,0.48f);
    }
    // Bridge piers descend into the river.
    for(int k=0;k<5;k++) {
        float x=-167.0f+k*6.0f;
        cube(v,x,0.20f,100.0f,0.48f,1.15f,0.60f,0.48f,0.50f,0.52f);
    }

    // VIP MEGA RAMP: one continuous shallow slope, wide enough for the car.
    // The top rises smoothly by 4.2 units over 34 units of length.
    const float rampAngle=0.1226f;
    cubeSlopeX(v,-120.0f,2.10f,35.0f,5.0f,0.24f,17.2f,
               rampAngle,0.20f,0.22f,0.26f);
    cubeSlopeX(v,-120.0f,2.36f,35.0f,4.72f,0.045f,17.0f,
               rampAngle,0.72f,0.73f,0.76f);
    // Low safety edges follow the same slope; leave the full center lane open.
    cubeSlopeX(v,-124.82f,2.30f,35.0f,0.12f,0.14f,17.0f,
               rampAngle,0.95f,0.55f,0.12f);
    cubeSlopeX(v,-115.18f,2.30f,35.0f,0.12f,0.14f,17.0f,
               rampAngle,0.95f,0.55f,0.12f);
    // Smooth run-up and flat landing platform.
    cube(v,-120.0f,-0.18f,16.0f,5.0f,0.10f,3.0f,0.13f,0.14f,0.16f);
    cube(v,-120.0f,4.28f,53.0f,5.0f,0.18f,4.0f,0.16f,0.18f,0.21f);
    // Dedicated access road connects the stunt ramp to the remote west-side district.
    cube(v,-120.0f,-0.18f,79.0f,3.2f,0.10f,30.0f,0.13f,0.14f,0.16f);
    // Long boulevard from the open grounds near the mansion to the ramp district.
    cube(v,-60.0f,-0.18f,79.0f,60.0f,0.10f,3.2f,0.13f,0.14f,0.16f);
    for(int k=0;k<15;k++)
        cube(v,-4.0f-k*7.5f,-0.055f,79.0f,0.12f,0.018f,2.0f,0.94f,0.82f,0.30f);
    for(int k=0;k<8;k++)
        cube(v,-120.0f,-0.055f,54.0f+k*7.0f,0.12f,0.018f,2.0f,0.94f,0.82f,0.30f);

    // Street lights along the avenues.
    for(int k=0;k<10;k++) {
        float z=24.0f+k*19.0f;
        cube(v,105.0f,3.2f,z,0.12f,3.2f,0.12f,0.22f,0.23f,0.25f);
        cube(v,106.0f,6.25f,z,1.2f,0.10f,0.10f,0.92f,0.86f,0.60f);
    }
}

static void drawHouse(const float* v) {
    // Warm ceiling lamps and exterior wall sconces switch on with night mode.
    float lamp = nightMode ? 1.0f : 0.18f;
    sphere(v,-4.0f,3.02f,-5.0f,0.28f,0.12f,0.28f,1.0f,0.78f*lamp,0.34f*lamp);
    sphere(v, 4.0f,3.02f,-5.0f,0.28f,0.12f,0.28f,1.0f,0.78f*lamp,0.34f*lamp);
    sphere(v, 0.0f,3.02f, 4.0f,0.28f,0.12f,0.28f,1.0f,0.78f*lamp,0.34f*lamp);
    cube(v,-9.22f,2.0f,2.0f,0.08f,0.28f,0.18f,
         nightMode?1.0f:0.30f,nightMode?0.72f:0.28f,nightMode?0.28f:0.16f);
    cube(v, 9.22f,2.0f,2.0f,0.08f,0.28f,0.18f,
         nightMode?1.0f:0.30f,nightMode?0.72f:0.28f,nightMode?0.28f:0.16f);
    // Floor and walls, no opaque ceiling so the player can see the interior.
    cube(v,0,-0.18f,-2,9,0.18f,12,0.48f,0.38f,0.27f);
    cube(v,-9,1.7f,-2,0.18f,1.7f,12,0.73f,0.59f,0.43f);
    cube(v, 9,1.7f,-2,0.18f,1.7f,12,0.73f,0.59f,0.43f);
    cube(v,0,1.7f,-14,9,1.7f,0.18f,0.68f,0.53f,0.38f);
    cube(v,-2.25f,1.7f,10,6.75f,1.7f,0.18f,0.68f,0.53f,0.38f);
    cube(v,8.25f,1.7f,10,0.75f,1.7f,0.18f,0.68f,0.53f,0.38f);
    // Roof rim, not a ceiling.
    cube(v,0,3.45f,-2,9.25f,0.16f,12.25f,0.35f,0.13f,0.09f);
    // Main house identity sign: YASH SHUKLA HOUSE.
    // Sign backing is mounted on the OUTSIDE (+Z) face of the front wall.
    cube(v,0.0f,2.78f,10.30f,3.55f,0.44f,0.10f,0.055f,0.075f,0.12f);
    // Gold face sits in front of the backing, never behind the wall mesh.
    cube(v,0.0f,2.78f,10.425f,3.42f,0.34f,0.025f,0.95f,0.72f,0.16f);
    // High-contrast lettering faces the garden/driveway camera.
    drawBlockText(v,0.0f,2.78f,10.462f,"YASH SHUKLA",0.070f);
    // Main mansion gate: side posts only; keep the full center opening clear.
    cube(v,4.45f,2.55f,9.78f,0.14f,0.85f,0.18f,0.25f,0.10f,0.04f);
    cube(v,7.55f,2.55f,9.78f,0.14f,0.85f,0.18f,0.25f,0.10f,0.04f);
    cube(v,6.0f,3.25f,9.78f,1.7f,0.15f,0.18f,0.25f,0.10f,0.04f);
    // Two sliding iron gate leaves. They move outward when the nearby
    // wall button is activated.
    float gateSlide = mansionGateAmount * 1.48f;
    cube(v,5.27f-gateSlide,1.35f,9.78f,0.72f,1.18f,0.10f,
         0.12f,0.16f,0.18f);
    cube(v,6.73f+gateSlide,1.35f,9.78f,0.72f,1.18f,0.10f,
         0.12f,0.16f,0.18f);
    for(int i=0;i<5;i++) {
        float gx=4.72f+i*0.27f-gateSlide;
        cube(v,gx,1.35f,9.64f,0.035f,1.12f,0.035f,0.72f,0.76f,0.78f);
        float rx=7.28f-i*0.27f+gateSlide;
        cube(v,rx,1.35f,9.64f,0.035f,1.12f,0.035f,0.72f,0.76f,0.78f);
    }
    // Gate control panel beside the left pillar: green=open, red=closed.
    cube(v,4.05f,1.35f,9.55f,0.12f,0.24f,0.08f,0.12f,0.13f,0.14f);
    sphere(v,4.05f,1.43f,9.43f,0.075f,0.075f,0.055f,
           mansionGateOpen?0.10f:0.95f,mansionGateOpen?0.90f:0.08f,0.12f);
    // Delivery call panel moved outside the right side of the gate,
    // beyond the front-wall collision strip so the player can reach it.
    cube(v,9.15f,1.35f,12.15f,0.13f,0.30f,0.09f,0.035f,0.16f,0.08f);
    sphere(v,9.15f,1.38f,12.045f,0.09f,0.09f,0.055f,
           0.12f,deliveryActive?0.95f:0.72f,0.18f);
    drawBlockText(v,9.15f,1.74f,12.03f,"DELIVERY",0.055f);

    // Internal room dividers with broad passages.
    cube(v,-4,1.55f,-6.90f,0.14f,1.55f,1.10f,0.55f,0.40f,0.28f);
    cube(v,-4,1.55f,-1.90f,0.14f,1.55f,3.10f,0.55f,0.40f,0.28f);
    cube(v, 4,1.55f,-3,0.14f,1.55f,5.0f,0.55f,0.40f,0.28f);
    cube(v,0,1.55f,-7,3.8f,1.55f,0.14f,0.55f,0.40f,0.28f);

    // Bedroom: bed, pillow, wardrobe and bedside table.
    cube(v,-6,0.42f,-5,1.55f,0.42f,2.0f,0.34f,0.12f,0.10f);
    cube(v,-6,0.92f,-5,1.48f,0.12f,1.9f,0.82f,0.78f,0.68f);
    cube(v,-6.7f,1.08f,-6.25f,0.48f,0.12f,0.35f,0.95f,0.91f,0.82f);
    cube(v,-7.5f,1.25f,-10,0.7f,1.25f,0.6f,0.28f,0.12f,0.05f);
    cube(v,-3.0f,0.55f,-5,0.55f,0.55f,0.55f,0.38f,0.20f,0.09f);

    // Kitchen counters and fridge.
    cube(v,6,0.75f,-5,1.5f,0.75f,0.55f,0.43f,0.43f,0.40f);
    cube(v,7.5f,1.15f,-10,0.65f,1.15f,0.65f,0.72f,0.76f,0.78f);
    cube(v,6,1.55f,-5,1.5f,0.08f,0.55f,0.18f,0.18f,0.18f);

    // Dedicated children's study corner with a broad desk, two chairs,
    // books, pencils and a warm desk lamp.
    cube(v,0.0f,0.92f,-9.35f,2.15f,0.12f,0.82f,0.48f,0.27f,0.12f);
    for(int side=-1;side<=1;side+=2) {
        float sx=side*1.35f;
        cube(v,sx,0.48f,-8.05f,0.36f,0.48f,0.34f,0.28f,0.15f,0.07f);
        cube(v,sx,0.98f,-8.28f,0.38f,0.06f,0.08f,0.32f,0.18f,0.08f);
    }
    cube(v,-0.95f,1.08f,-9.35f,0.38f,0.045f,0.28f,0.12f,0.28f,0.72f);
    cube(v,0.05f,1.08f,-9.35f,0.38f,0.045f,0.28f,0.95f,0.78f,0.18f);
    cube(v,0.92f,1.08f,-9.35f,0.38f,0.045f,0.28f,0.16f,0.65f,0.28f);
    cube(v,1.72f,1.20f,-9.55f,0.07f,0.30f,0.07f,0.25f,0.22f,0.12f);
    sphere(v,1.72f,1.48f,-9.55f,0.18f,0.12f,0.18f,1.0f,0.82f,0.35f);

    // Dining table and chairs.
    cube(v,0,0.85f,2,2,0.12f,1,0.40f,0.20f,0.08f);
    cube(v,-1.7f,0.40f,2,0.12f,0.40f,0.12f,0.25f,0.12f,0.04f);
    cube(v, 1.7f,0.40f,2,0.12f,0.40f,0.12f,0.25f,0.12f,0.04f);
    cube(v,-2.8f,0.55f,2,0.48f,0.55f,0.48f,0.35f,0.16f,0.08f);
    cube(v, 2.8f,0.55f,2,0.48f,0.55f,0.48f,0.35f,0.16f,0.08f);

    // Bathroom sink and bathtub.
    cube(v,6,0.35f,-1,1.4f,0.35f,0.65f,0.78f,0.80f,0.77f);
    cube(v,6,0.18f,-10,1.5f,0.18f,0.75f,0.80f,0.83f,0.82f);

    // Upper landing / balcony. Draw this before the stairs so it cannot
    // cover the upper steps.
    cube(v,5.0f,2.48f,-1.0f,3.55f,0.12f,2.75f,
         0.42f,0.29f,0.18f);
    // Seven broad steps descend from the terrace edge toward the front.
    // Their top surfaces run from y=2.60 at z=1.00 down to y=0.50 at z=4.90.
    for(int i=0;i<7;i++) {
        float topY = 2.60f - i*0.35f;
        float stepZ = 1.00f + i*0.65f;
        cube(v,6.0f,topY-0.18f,stepZ,
             1.35f,0.18f,0.36f,0.42f,0.25f,0.13f);
    }
    // Balcony safety rails.
    cube(v,1.55f,3.05f,-1.0f,0.10f,0.45f,2.70f,
         0.25f,0.12f,0.06f);
    cube(v,8.45f,3.05f,-1.0f,0.10f,0.45f,2.70f,
         0.25f,0.12f,0.06f);
    // Back railing and front side railings. The central opening faces the stairs.
    cube(v,5.0f,3.05f,-3.48f,3.45f,0.45f,0.10f,
         0.25f,0.12f,0.06f);
    // Keep a wide, centered opening directly aligned with the staircase at x=6.
    // The previous opening was centered at x=5 and blocked the right side of the stairs.
    cube(v,3.00f,3.05f,1.48f,1.45f,0.45f,0.10f,
         0.25f,0.12f,0.06f);
    cube(v,7.95f,3.05f,1.48f,0.50f,0.45f,0.10f,
         0.25f,0.12f,0.06f);
}
static void drawAyushBike(const float* v) {
    if(!onAyushBike && !ayushBikeParked) return;
    float x=ayushBikeX, z=ayushBikeZ, a=ayushBikeYaw;

    // Transform every bike-local position with the same heading.
    // Local -Z is the front of the bike; the route heading points to college.
    auto bx = [&](float lx,float lz) {
        return x + cosf(a)*lx + sinf(a)*lz;
    };
    auto bz = [&](float lx,float lz) {
        return z - sinf(a)*lx + cosf(a)*lz;
    };

    // Wheels, frame, tank and seat all share the bike's longitudinal axis.
    sphereYaw(v,bx(0,-0.72f),0.40f,bz(0,-0.72f),0.34f,0.34f,0.15f,a,0.035f,0.04f,0.045f);
    sphereYaw(v,bx(0, 0.72f),0.40f,bz(0, 0.72f),0.34f,0.34f,0.15f,a,0.035f,0.04f,0.045f);
    cubeYaw(v,x,0.66f,z,0.12f,0.10f,0.80f,a,0.72f,0.72f,0.76f);
    cubeYaw(v,bx(0,-0.12f),0.88f,bz(0,-0.12f),0.28f,0.18f,0.38f,a,0.78f,0.08f,0.06f);
    cubeYaw(v,bx(0,0.34f),0.91f,bz(0,0.34f),0.24f,0.09f,0.40f,a,0.025f,0.025f,0.03f);
    cubeYaw(v,bx(0,-0.67f),0.94f,bz(0,-0.67f),0.40f,0.055f,0.08f,a,0.16f,0.16f,0.18f);
    sphereYaw(v,bx(0,-0.88f),0.90f,bz(0,-0.88f),0.13f,0.13f,0.09f,a,1.0f,0.86f,0.24f);

    if(ayushBikeParked && !onAyushBike) return;

    // Ayush sits in front, aligned along the bike (blue jacket).
    cubeYaw(v,bx(0,-0.18f),1.34f,bz(0,-0.18f),0.23f,0.34f,0.18f,a,0.12f,0.30f,0.76f);
    sphereYaw(v,bx(0,-0.25f),1.76f,bz(0,-0.25f),0.20f,0.22f,0.19f,a,0.91f,0.70f,0.55f);
    sphereYaw(v,bx(0,-0.27f),1.94f,bz(0,-0.27f),0.21f,0.10f,0.20f,a,0.055f,0.035f,0.025f);
    cubeYaw(v,bx(-0.28f,-0.52f),1.35f,bz(-0.28f,-0.52f),0.07f,0.10f,0.34f,a,0.91f,0.70f,0.55f);
    cubeYaw(v,bx( 0.28f,-0.52f),1.35f,bz( 0.28f,-0.52f),0.07f,0.10f,0.34f,a,0.91f,0.70f,0.55f);

    // Player sits directly behind Ayush on the pillion seat.
    cubeYaw(v,bx(0,0.42f),1.40f,bz(0,0.42f),0.23f,0.34f,0.20f,a,0.045f,0.18f,0.72f);
    sphereYaw(v,bx(0,0.40f),1.82f,bz(0,0.40f),0.20f,0.22f,0.19f,a,0.91f,0.70f,0.55f);
    sphereYaw(v,bx(0,0.39f),2.00f,bz(0,0.39f),0.21f,0.10f,0.20f,a,0.055f,0.035f,0.025f);
    cubeYaw(v,bx(-0.18f,0.78f),1.10f,bz(-0.18f,0.78f),0.10f,0.12f,0.30f,a,0.035f,0.045f,0.075f);
    cubeYaw(v,bx( 0.18f,0.78f),1.10f,bz( 0.18f,0.78f),0.10f,0.12f,0.30f,a,0.035f,0.045f,0.075f);
}

static void drawDeliveryRider(const float* v) {
    if(!deliveryActive) return;
    float x=deliveryX,z=deliveryZ,a=deliveryYaw;
    auto bx=[&](float lx,float lz){return x+cosf(a)*lx+sinf(a)*lz;};
    auto bz=[&](float lx,float lz){return z-sinf(a)*lx+cosf(a)*lz;};
    sphereYaw(v,bx(0,-0.62f),0.34f,bz(0,-0.62f),0.29f,0.29f,0.13f,a,0.025f,0.03f,0.035f);
    sphereYaw(v,bx(0,0.62f),0.34f,bz(0,0.62f),0.29f,0.29f,0.13f,a,0.025f,0.03f,0.035f);
    cubeYaw(v,x,0.60f,z,0.10f,0.09f,0.70f,a,0.16f,0.18f,0.20f);
    cubeYaw(v,bx(0,-0.12f),0.82f,bz(0,-0.12f),0.25f,0.16f,0.30f,a,0.12f,0.62f,0.24f);
    cubeYaw(v,bx(0,-0.10f),1.25f,bz(0,-0.10f),0.23f,0.30f,0.18f,a,0.08f,0.58f,0.22f);
    sphereYaw(v,bx(0,-0.12f),1.62f,bz(0,-0.12f),0.18f,0.19f,0.17f,a,0.88f,0.67f,0.52f);
    sphereYaw(v,bx(0,-0.14f),1.78f,bz(0,-0.14f),0.20f,0.10f,0.19f,a,0.08f,0.12f,0.16f);
    cubeYaw(v,bx(0,0.48f),1.18f,bz(0,0.48f),0.34f,0.31f,0.30f,a,0.04f,0.62f,0.22f);
}
static void drawAyushWalkingFriend(const float* v) {
    if(!ayushWalkingWithPlayer && !ayushReturningToCollege) return;
    float x=ayushFriendX, z=ayushFriendZ;
    float faceYaw=atan2f(playerX-x, playerZ-z);
    float bob=0.035f*sinf(ayushWalkPhase*2.0f)*ayushWalkBlend;
    float swing=0.48f*sinf(ayushWalkPhase)*ayushWalkBlend;
    // Ayush is intentionally a little taller than the player character.
    cubeYaw(v,x,1.13f+bob,z,0.34f,0.52f,0.22f,faceYaw,0.12f,0.30f,0.76f);
    sphere(v,x,1.77f+bob,z,0.22f,0.24f,0.20f,0.91f,0.70f,0.55f);
    sphere(v,x,1.97f+bob,z,0.23f,0.11f,0.21f,0.055f,0.035f,0.025f);
    cubeYaw(v,x-0.25f,1.05f+bob,z,0.10f,0.39f,0.12f,faceYaw+swing,0.10f,0.12f,0.18f);
    cubeYaw(v,x+0.25f,1.05f+bob,z,0.10f,0.39f,0.12f,faceYaw-swing,0.10f,0.12f,0.18f);
    cubeYaw(v,x-0.12f,0.30f,z,0.11f,0.30f,0.12f,faceYaw-swing*0.65f,0.035f,0.045f,0.075f);
    cubeYaw(v,x+0.12f,0.30f,z,0.11f,0.30f,0.12f,faceYaw+swing*0.65f,0.035f,0.045f,0.075f);
}

static void drawAyushMishraHouse(const float* v) {
    const float ox=25.0f;
    // Same exterior proportions, roof rim, front gate and warm stone palette
    // as Sugla House; this is a separate neighboring property.
    cube(v,ox,-0.18f,-2,9,0.18f,12,0.48f,0.38f,0.27f);
    cube(v,ox-9,1.7f,-2,0.18f,1.7f,12,0.73f,0.59f,0.43f);
    cube(v,ox+9,1.7f,-2,0.18f,1.7f,12,0.73f,0.59f,0.43f);
    // Rear wall split around a wide private bike exit.
    cube(v,ox-5.7f,1.7f,-14,3.3f,1.7f,0.18f,0.68f,0.53f,0.38f);
    cube(v,ox+5.7f,1.7f,-14,3.3f,1.7f,0.18f,0.68f,0.53f,0.38f);
    // Rear exit lintel and side posts.
    cube(v,ox,3.25f,-14,2.4f,0.15f,0.18f,0.35f,0.13f,0.09f);
    cube(v,ox-1.25f,1.45f,-14,0.14f,1.45f,0.18f,0.42f,0.27f,0.16f);
    cube(v,ox+1.25f,1.45f,-14,0.14f,1.45f,0.18f,0.42f,0.27f,0.16f);
    // Front boundary wall is split into two solid sections, leaving a
    // clear vehicle/person-sized opening aligned with the sliding gate.
    cube(v,ox-5.8f,1.7f,10,3.2f,1.7f,0.18f,0.68f,0.53f,0.38f);
    cube(v,ox+5.8f,1.7f,10,3.2f,1.7f,0.18f,0.68f,0.53f,0.38f);
    cube(v,ox,3.45f,-2,9.25f,0.16f,12.25f,0.35f,0.13f,0.09f);
    // Double-leaf metal gate slides apart after the visitor rings the bell.
    float gateSlide=2.85f*ayushGateAmount;
    cube(v,ox-1.75f,2.55f,11.15f,0.14f,0.95f,0.18f,0.20f,0.09f,0.035f);
    cube(v,ox+1.75f,2.55f,11.15f,0.14f,0.95f,0.18f,0.20f,0.09f,0.035f);
    cube(v,ox,3.55f,11.15f,1.9f,0.13f,0.18f,0.20f,0.09f,0.035f);
    for(int side=-1;side<=1;side+=2) {
        float panelX=ox+side*(0.82f+gateSlide);
        cube(v,panelX,1.30f,11.15f,0.78f,1.18f,0.10f,0.10f,0.16f,0.20f);
        cube(v,panelX,0.76f,11.22f,0.82f,0.055f,0.06f,0.72f,0.76f,0.80f);
        cube(v,panelX,1.82f,11.22f,0.82f,0.055f,0.06f,0.72f,0.76f,0.80f);
        for(int i=0;i<7;i++) {
            float bx=panelX-0.34f+i*0.113f;
            cube(v,bx,1.30f,11.22f,0.035f,1.02f,0.045f,
                 0.72f,0.76f,0.80f);
        }
    }
    // Gate center handle and decorative finials.
    sphere(v,ox,1.30f,11.28f,0.10f,0.10f,0.07f,0.88f,0.68f,0.22f);
    // Doorbell mounted on the right gate pillar, with a gold button.
    cube(v,ox+2.05f,1.55f,11.42f,0.22f,0.34f,0.08f,0.12f,0.12f,0.13f);
    sphere(v,ox+2.05f,1.57f,11.49f,0.085f,0.085f,0.045f,
           1.0f,0.72f,0.12f);

    // Matching front door, windows, porch and balcony details.
    cube(v,ox,1.15f,9.78f,0.72f,1.05f,0.08f,0.28f,0.12f,0.06f);
    cube(v,ox-5.6f,1.85f,10.13f,1.45f,0.85f,0.06f,0.22f,0.55f,0.72f);
    cube(v,ox+5.6f,1.85f,10.13f,1.45f,0.85f,0.06f,0.22f,0.55f,0.72f);
    cube(v,ox-5.6f,1.85f,10.20f,0.08f,0.90f,0.035f,0.82f,0.82f,0.78f);
    cube(v,ox+5.6f,1.85f,10.20f,0.08f,0.90f,0.035f,0.82f,0.82f,0.78f);
    // Name plate mounted on the outside (+Z) face, in front of the wall.
    // Wider gold panel fits the full AYUSH MISHRA HOUSE lettering.
    cube(v,ox,2.78f,10.30f,5.25f,0.46f,0.10f,0.055f,0.075f,0.12f);
    cube(v,ox,2.78f,10.425f,5.10f,0.36f,0.025f,0.95f,0.72f,0.16f);
    drawBlockText(v,ox,2.78f,10.462f,"AYUSH MISHRA HOUSE",0.045f);
    // Porch columns and small front garden boundary.
    for(int side=-1;side<=1;side+=2) {
        cube(v,ox+side*7.0f,1.35f,9.5f,0.18f,1.35f,0.18f,0.78f,0.72f,0.62f);
        cube(v,ox+side*7.0f,0.55f,13.0f,0.12f,0.55f,0.12f,0.28f,0.32f,0.25f);
    }
    // Living-room furniture visible through the open front.
    cube(v,ox-5.8f,0.42f,-5.0f,1.45f,0.42f,1.9f,0.34f,0.12f,0.10f);
    cube(v,ox-5.8f,0.92f,-5.0f,1.38f,0.12f,1.8f,0.82f,0.78f,0.68f);
    cube(v,ox+5.8f,0.75f,-5.0f,1.4f,0.75f,0.55f,0.43f,0.43f,0.40f);

    if(!onAyushBike) {
        // Ayush seated on a chair inside, playing a mobile battle-royale game.
        // Chair
        cube(v,ox+2.7f,0.72f,-4.4f,0.62f,0.12f,0.62f,0.28f,0.16f,0.09f);
        cube(v,ox+2.7f,1.18f,-4.72f,0.62f,0.52f,0.12f,0.30f,0.18f,0.10f);
        for(int sx=-1;sx<=1;sx+=2) for(int sz=-1;sz<=1;sz+=2)
            cube(v,ox+2.7f+sx*0.24f,0.34f,-4.4f+sz*0.24f,
                 0.07f,0.38f,0.07f,0.20f,0.12f,0.07f);
        // Seated body, neck and head
        cube(v,ox+2.7f,1.43f,-4.38f,0.38f,0.48f,0.25f,0.12f,0.30f,0.72f);
        sphere(v,ox+2.7f,1.92f,-4.38f,0.25f,0.27f,0.23f,0.91f,0.70f,0.55f);
        sphere(v,ox+2.7f,2.14f,-4.40f,0.26f,0.12f,0.24f,0.055f,0.035f,0.025f);
        // Bent legs resting forward from the chair
        cube(v,ox+2.48f,0.58f,-3.92f,0.13f,0.13f,0.48f,0.035f,0.045f,0.075f);
        cube(v,ox+2.92f,0.58f,-3.92f,0.13f,0.13f,0.48f,0.035f,0.045f,0.075f);
        // Arms bent toward a phone; phone has a bright game screen.
        cubeYaw(v,ox+2.43f,1.48f,-4.05f,0.10f,0.30f,0.10f,0.48f,0.91f,0.70f,0.55f);
        cubeYaw(v,ox+2.97f,1.48f,-4.05f,0.10f,0.30f,0.10f,-0.48f,0.91f,0.70f,0.55f);
        cube(v,ox+2.7f,1.38f,-3.98f,0.23f,0.16f,0.035f,0.025f,0.025f,0.03f);
        cube(v,ox+2.7f,1.39f,-3.955f,0.18f,0.11f,0.012f,0.10f,0.78f,0.30f);
    }

}

static void drawRooftopComplex(const float* v) {
    // Structural columns transfer the load of the rooftop floors to the mansion.
    for(int ix=0;ix<4;ix++) for(int iz=0;iz<4;iz++) {
        float x=-7.4f+ix*4.95f, z=-11.2f+iz*6.0f;
        // The east-inner column intersects the stairwell approach; omit it
        // on both commercial and rental levels to keep the stair lane open.
        bool stairSideColumn=(ix==3 && iz==1);
        if(!stairSideColumn) {
            cube(v,x,5.55f,z,0.22f,1.85f,0.22f,0.70f,0.69f,0.64f);
            if(rooftopLevel!=1)
                cube(v,x,9.30f,z,0.22f,1.85f,0.22f,0.70f,0.69f,0.64f);
        }
    }
    // First commercial floor slab is split around the stairwell opening.
    // The opening aligns with the east-side staircase (x=3.75..7.75,
    // z=-8.35..-1.35), so the slab cannot block the player's head/body.
    cube(v,-2.55f,3.72f,-2.0f,5.70f,0.22f,10.25f,0.48f,0.49f,0.47f);
    cube(v,8.00f,3.72f,-2.0f,0.20f,0.22f,10.25f,0.48f,0.49f,0.47f);
    cube(v,5.75f,3.72f,4.05f,2.25f,0.22f,2.85f,0.48f,0.49f,0.47f);
    cube(v,5.75f,3.72f,-10.85f,2.25f,0.22f,0.95f,0.48f,0.49f,0.47f);
    // The next-floor slab is hidden while viewing level 1 from inside.
    // This gives a clean open-top interior camera view of the supermarket/gym.
    if(rooftopLevel!=1)
        cube(v,0.0f,7.45f,-2.0f,8.25f,0.22f,10.25f,0.48f,0.49f,0.47f);
    // Outer walls with wide shopfront opening at the south/front side.
    cube(v,-8.05f,5.55f,-2.0f,0.20f,1.65f,10.0f,0.72f,0.73f,0.70f);
    cube(v, 8.05f,5.55f,-2.0f,0.20f,1.65f,10.0f,0.72f,0.73f,0.70f);
    cube(v,0.0f,5.55f,-12.0f,8.0f,1.65f,0.20f,0.72f,0.73f,0.70f);
    cube(v,-7.0f,5.55f,8.0f,1.0f,1.65f,0.20f,0.72f,0.73f,0.70f);
    cube(v, 7.0f,5.55f,8.0f,1.0f,1.65f,0.20f,0.72f,0.73f,0.70f);
    // Interior divider: supermarket left, gym right, central opening.
    cube(v,1.35f,5.55f,-2.0f,0.14f,1.55f,9.8f,0.78f,0.77f,0.70f);
    // Supermarket counters and long shelves.
    for(int row=0;row<3;row++) {
        float z=-8.0f+row*4.0f;
        cube(v,-3.4f,4.55f,z,0.55f,0.72f,1.35f,0.65f,0.43f,0.22f);
        cube(v,-3.4f,5.35f,z,0.58f,0.08f,1.38f,0.90f,0.72f,0.30f);
        for(int k=0;k<4;k++)
            cube(v,-3.4f,5.75f,z-0.9f+k*0.58f,0.38f,0.22f,0.16f,
                 0.22f+0.16f*(k%2),0.48f,0.28f+0.12f*(k%2));
    }
    cube(v,-6.3f,4.65f,5.2f,1.0f,0.65f,0.55f,0.30f,0.36f,0.42f); // checkout
    cube(v,-6.3f,5.38f,5.2f,0.78f,0.08f,0.48f,0.82f,0.86f,0.88f);
    cube(v,-6.3f,5.88f,5.05f,0.48f,0.38f,0.08f,0.035f,0.12f,0.18f);
    cube(v,-6.3f,5.90f,4.96f,0.39f,0.27f,0.025f,0.08f,0.48f,0.68f);
    cube(v,-5.55f,5.50f,5.18f,0.22f,0.06f,0.16f,0.10f,0.12f,0.14f);
    cube(v,-5.55f,5.58f,5.18f,0.15f,0.08f,0.10f,0.06f,0.68f,0.42f);
    const float productX[8]={-6.9f,-5.9f,-6.9f,-5.9f,-6.9f,-5.9f,-6.9f,-5.9f};
    const float productZ[8]={-8.0f,-8.0f,-4.0f,-4.0f,0.0f,0.0f,3.4f,3.4f};
    const float productR[8]={0.90f,0.72f,0.88f,0.96f,0.82f,0.92f,0.18f,0.25f};
    const float productG[8]={0.94f,0.48f,0.18f,0.76f,0.72f,0.22f,0.62f,0.78f};
    const float productB[8]={0.98f,0.20f,0.12f,0.12f,0.28f,0.10f,0.92f,0.35f};
    const char* productLabels[8]={"MILK","BREAD","APPLE","BANANA","RICE","CHIPS","JUICE","SOAP"};
    for(int p=0;p<8;p++) {
        cube(v,productX[p],5.15f,productZ[p],0.25f,0.34f,0.28f,productR[p],productG[p],productB[p]);
        cube(v,productX[p],5.52f,productZ[p],0.13f,0.04f,0.15f,0.96f,0.90f,0.66f);
        drawBlockText(v,productX[p],5.72f,productZ[p]+0.30f,productLabels[p],0.035f);
    }
    drawBlockText(v,-6.3f,6.18f,4.93f,"CHECKOUT",0.075f);
    // Gym: treadmills, benches and weights.
    for(int i=0;i<3;i++) {
        // Keep all gym equipment to the west so the east-side staircase
        // has a completely clear landing into the first-floor gym.
        float x=-3.0f+i*1.55f;
        cube(v,x,4.45f,-7.2f,0.48f,0.18f,1.25f,0.16f,0.18f,0.20f);
        cube(v,x,5.05f,-7.9f,0.08f,0.75f,0.08f,0.55f,0.58f,0.60f);
        cube(v,x,4.55f,-1.2f,0.55f,0.22f,1.35f,0.20f,0.22f,0.24f);
        sphere(v,x-0.55f,4.75f,-1.2f,0.22f,0.22f,0.22f,0.08f,0.08f,0.09f);
        sphere(v,x+0.55f,4.75f,-1.2f,0.22f,0.22f,0.22f,0.08f,0.08f,0.09f);
    }
    cube(v,4.7f,4.65f,4.5f,2.0f,0.65f,0.45f,0.28f,0.29f,0.31f); // gym bench
    // Hide the complete second-floor shell while the player is on level 1.
    if(rooftopLevel!=1) {
    // Second floor rental units: four rooms around a central corridor.
    cube(v,0.0f,9.28f,-2.0f,8.25f,0.22f,10.25f,0.54f,0.53f,0.49f);
    cube(v,-8.05f,10.95f,-2.0f,0.20f,1.55f,10.0f,0.80f,0.77f,0.69f);
    cube(v, 8.05f,10.95f,-2.0f,0.20f,1.55f,10.0f,0.80f,0.77f,0.69f);
    cube(v,0.0f,10.95f,-12.0f,8.0f,1.55f,0.20f,0.80f,0.77f,0.69f);
    cube(v,-7.0f,10.95f,8.0f,1.0f,1.55f,0.20f,0.80f,0.77f,0.69f);
    cube(v, 7.0f,10.95f,8.0f,1.0f,1.55f,0.20f,0.80f,0.77f,0.69f);
    // Cross walls split four rentable rooms; central passages remain open.
    cube(v,0.0f,10.95f,-2.0f,0.12f,1.50f,9.8f,0.72f,0.69f,0.62f);
    cube(v,-4.0f,10.95f,-2.0f,3.8f,1.50f,0.12f,0.72f,0.69f,0.62f);
    cube(v, 4.0f,10.95f,-2.0f,3.8f,1.50f,0.12f,0.72f,0.69f,0.62f);
    // Four individually lockable room doors.
    const float dx[4]={-4.0f,4.0f,-4.0f,4.0f};
    const float dz[4]={7.65f,7.65f,-11.65f,-11.65f};
    for(int i=0;i<4;i++) {
        float angle=rentalDoorOpen[i]?1.15f:0.0f;
        cubeYaw(v,dx[i],10.85f,dz[i],0.62f,1.05f,0.10f,angle,
                0.30f,0.14f,0.07f);
        sphere(v,dx[i]+0.42f,10.82f,dz[i]-0.08f,0.07f,0.07f,0.07f,
               rentalDoorUnlocked[i]?0.12f:0.88f,
               rentalDoorUnlocked[i]?0.78f:0.55f,0.12f);
    }
    // Furnish rental rooms with beds and wardrobes.
    for(int i=0;i<4;i++) {
        float x=(i%2==0?-5.8f:5.8f), z=(i<2?3.0f:-7.0f);
        cube(v,x,9.85f,z,1.25f,0.35f,1.9f,0.42f,0.22f,0.16f);
        cube(v,x,10.25f,z,1.18f,0.10f,1.82f,0.82f,0.79f,0.70f);
        cube(v,x+(i%2==0?1.3f:-1.3f),10.45f,z-1.0f,0.42f,0.75f,0.48f,
             0.32f,0.16f,0.08f);
    }
    // Top roof terrace and safety parapet.
    cube(v,0.0f,12.62f,-2.0f,8.30f,0.22f,10.30f,0.28f,0.32f,0.34f);
    cube(v,0.0f,13.15f,-12.1f,8.25f,0.30f,0.15f,0.26f,0.28f,0.30f);
    cube(v,-8.1f,13.15f,-2.0f,0.15f,0.30f,10.0f,0.26f,0.28f,0.30f);
    cube(v, 8.1f,13.15f,-2.0f,0.15f,0.30f,10.0f,0.26f,0.28f,0.30f);
    }

    // Redesigned wide staircase centered in the open floor cut-out.
    for(int i=0;i<9;i++) {
        float z=-1.95f-i*0.68f;
        float top=2.72f+i*0.16f;
        cube(v,5.75f,top-0.10f,z,1.55f,0.20f,0.52f,
             0.43f,0.35f,0.27f);
        cube(v,5.75f,top-0.015f,z,1.58f,0.03f,0.54f,
             0.58f,0.58f,0.56f);
    }
    // Full-width top landing bridges the last tread directly into level 1.
    // Landing top = 4.04, exactly the same as the commercial floor walk height.
    cube(v,5.75f,3.94f,-8.55f,1.72f,0.10f,1.42f,
         0.48f,0.49f,0.47f);
    cube(v,3.95f,3.45f,-4.75f,0.10f,0.65f,3.25f,0.24f,0.18f,0.12f);
    cube(v,7.55f,3.45f,-4.75f,0.10f,0.65f,3.25f,0.24f,0.18f,0.12f);
}

static void drawGarden(const float* v) {
    // Large bright lawn and a wide stone path leading away from the front door.
    cube(v,0,-0.55f,28,28,0.25f,38,0.16f,0.52f,0.13f);
    cube(v,0,-0.25f,23,2.35f,0.08f,15,0.68f,0.59f,0.43f);

    // Flower beds on both sides of the path.
    const float flowerX[] = {-5.5f,-3.8f,-6.8f,3.8f,5.5f,6.8f,
                             -5.5f,-3.8f,-6.8f,3.8f,5.5f,6.8f};
    const float flowerZ[] = {14,16,18,14,16,18,25,27,29,25,27,29};
    const float colors[6][3] = {
        {1.0f,0.12f,0.22f}, {1.0f,0.72f,0.08f},
        {0.75f,0.18f,0.95f}, {1.0f,0.42f,0.72f},
        {0.12f,0.65f,1.0f}, {1.0f,0.90f,0.20f}
    };

    for(int i=0;i<12;i++) {
        float x=flowerX[i], z=flowerZ[i];
        int c=i%6;
        // Stem and two leaves.
        cube(v,x,0.38f,z,0.045f,0.38f,0.045f,0.08f,0.38f,0.08f);
        sphere(v,x-0.12f,0.28f,z,0.15f,0.055f,0.08f,0.10f,0.55f,0.10f);
        sphere(v,x+0.12f,0.42f,z,0.15f,0.055f,0.08f,0.10f,0.55f,0.10f);
        // Five bright petals around a yellow centre.
        for(int petal=0;petal<5;petal++) {
            float a=2.0f*PI*petal/5.0f;
            sphere(v,x+cosf(a)*0.16f,0.78f+0.04f*sinf(a),z+sinf(a)*0.16f,
                   0.13f,0.12f,0.13f,
                   colors[c][0],colors[c][1],colors[c][2]);
        }
        sphere(v,x,0.80f,z,0.085f,0.085f,0.085f,1.0f,0.82f,0.08f);
    }

    // Trees stay beyond the path, with trunks and leafy crowns.
    const float tx[]={-13,-17,13,17,-20,20};
    const float tz[]={17,29,17,29,42,42};
    for(int i=0;i<6;i++) {
        cube(v,tx[i],1.45f,tz[i],0.28f,1.45f,0.28f,0.34f,0.17f,0.06f);
        sphere(v,tx[i],3.35f,tz[i],1.65f,1.8f,1.65f,0.07f,0.38f,0.07f);
        sphere(v,tx[i]-0.8f,3.0f,tz[i]+0.1f,0.85f,0.9f,0.85f,0.09f,0.44f,0.08f);
        sphere(v,tx[i]+0.8f,3.0f,tz[i]-0.1f,0.85f,0.9f,0.85f,0.08f,0.40f,0.06f);
        // Bright edible fruit hanging from the branches.
        for(int f=0;f<4;f++) {
            float a=(float)f*1.5707963f+0.35f;
            sphere(v,tx[i]+cosf(a)*1.12f,3.05f+(f%2)*0.42f,
                   tz[i]+sinf(a)*1.12f,0.22f,0.24f,0.22f,
                   (f%2)?0.95f:0.92f,(f%2)?0.18f:0.48f,0.06f);
        }
    }

    // Decorative garden pond: sandy bank, blue water and lily pads.
    sphere(v,10.0f,-0.30f,25.0f,4.2f,0.20f,3.0f,0.62f,0.48f,0.27f);
    sphere(v,10.0f,-0.12f,25.0f,3.75f,0.10f,2.62f,0.08f,0.48f,0.78f);
    sphere(v,10.0f,-0.035f,25.0f,3.35f,0.035f,2.25f,0.12f,0.66f,0.88f);
    // Subtle bright ripple streaks across the pond surface.
    for(int i=0;i<6;i++) {
        float rx=8.2f+(i%3)*1.35f;
        float rz=24.0f+(i/3)*1.15f;
        sphere(v,rx,0.012f,rz,0.48f,0.012f,0.045f,0.62f,0.86f,0.96f);
    }

    // Lily pads and flowers on the water.
    for(int i=0;i<5;i++) {
        float lx=8.0f+(float)i*0.9f;
        float lz=24.0f+(float)(i%2)*1.0f;
        sphere(v,lx,0.02f,lz,0.42f,0.035f,0.30f,0.10f,0.55f,0.20f);
        sphere(v,lx+0.10f,0.08f,lz,0.10f,0.08f,0.10f,1.0f,0.35f,0.60f);
    }

    // Reeds around the pond edge.
    for(int i=0;i<8;i++) {
        float rx=6.0f+(float)(i%4)*2.5f;
        float rz=21.5f+(float)(i/4)*7.0f;
        cube(v,rx,0.55f,rz,0.045f,0.55f,0.045f,0.12f,0.42f,0.12f);
        sphere(v,rx,1.12f,rz,0.09f,0.20f,0.09f,0.55f,0.35f,0.10f);
    }

    // Small garden bench.
    cube(v,0.0f,0.65f,33.0f,1.25f,0.12f,0.35f,0.42f,0.19f,0.08f);
    cube(v,0.0f,1.15f,33.3f,1.25f,0.42f,0.12f,0.42f,0.19f,0.08f);

    // Two comfortable garden armchairs for the player and his wife.
    for(int side=0;side<2;side++) {
        float cx=(side==0)?8.5f:12.2f;
        float cz=45.0f;
        float woodR=side==0?0.48f:0.34f;
        float woodG=side==0?0.22f:0.18f;
        float woodB=side==0?0.10f:0.08f;
        cube(v,cx,0.72f,cz,0.78f,0.13f,0.72f,woodR,woodG,woodB);
        cube(v,cx,1.22f,cz+0.48f,0.78f,0.58f,0.13f,woodR,woodG,woodB);
        for(int arm=-1;arm<=1;arm+=2) {
            cube(v,cx+arm*0.70f,0.98f,cz-0.02f,0.10f,0.30f,0.68f,woodR,woodG,woodB);
            cube(v,cx+arm*0.70f,0.42f,cz-0.25f,0.10f,0.42f,0.10f,woodR*0.75f,woodG*0.75f,woodB*0.75f);
            cube(v,cx+arm*0.70f,0.42f,cz+0.27f,0.10f,0.42f,0.10f,woodR*0.75f,woodG*0.75f,woodB*0.75f);
        }
        cube(v,cx,0.42f,cz-0.27f,0.68f,0.10f,0.10f,woodR*0.75f,woodG*0.75f,woodB*0.75f);
        cube(v,cx,0.42f,cz+0.27f,0.68f,0.10f,0.10f,woodR*0.75f,woodG*0.75f,woodB*0.75f);
        // Soft colourful seat cushion.
        cube(v,cx,0.82f,cz-0.02f,0.62f,0.045f,0.55f,
             side==0?0.12f:0.12f,side==0?0.48f:0.30f,side==0?0.78f:0.58f);
    }

    // Dense garden greenery around the family seating lawn: layered shrubs,
    // low flowering bushes and small ornamental trees, leaving clear walking gaps.
    const float bushX[10]={5.2f,7.0f,14.5f,16.5f,6.0f,15.8f,-16.0f,-19.0f,-8.0f,-4.8f};
    const float bushZ[10]={42.0f,50.0f,42.5f,48.0f,56.0f,56.0f,47.0f,53.0f,55.0f,49.0f};
    for(int i=0;i<10;i++) {
        float bx=bushX[i], bz=bushZ[i];
        sphere(v,bx,0.78f,bz,0.85f,0.78f,0.82f,0.06f,0.34f,0.08f);
        sphere(v,bx-0.48f,0.70f,bz+0.12f,0.58f,0.58f,0.62f,0.08f,0.42f,0.10f);
        sphere(v,bx+0.43f,0.68f,bz-0.12f,0.56f,0.55f,0.60f,0.05f,0.38f,0.07f);
        for(int f=0;f<3;f++) {
            float fx=bx+(f-1)*0.42f;
            sphere(v,fx,1.18f+(f%2)*0.12f,bz+0.20f,0.13f,0.12f,0.13f,
                   f==1?1.0f:0.95f,f==1?0.72f:0.25f,0.48f);
        }
    }
    // Two leafy ornamental trees enrich the lawn but stay clear of the chairs.
    const float gardenTreeX[2]={-22.0f,20.5f};
    const float gardenTreeZ[2]={51.0f,51.0f};
    for(int i=0;i<2;i++) {
        cube(v,gardenTreeX[i],1.55f,gardenTreeZ[i],0.25f,1.55f,0.25f,0.34f,0.18f,0.07f);
        sphere(v,gardenTreeX[i],3.55f,gardenTreeZ[i],1.55f,1.75f,1.55f,0.06f,0.39f,0.07f);
        sphere(v,gardenTreeX[i]-0.72f,3.25f,gardenTreeZ[i]+0.12f,0.82f,0.88f,0.82f,0.08f,0.46f,0.09f);
        sphere(v,gardenTreeX[i]+0.72f,3.22f,gardenTreeZ[i]-0.12f,0.82f,0.88f,0.82f,0.07f,0.42f,0.07f);
    }

    // Children's double swing: sturdy A-frame, crossbar, chains and two seats.
    const float swingX=-11.0f, swingZ=50.0f;
    for(int side=-1;side<=1;side+=2) {
        cubeYaw(v,swingX+side*2.8f,1.75f,swingZ,0.13f,1.78f,0.13f,
                side*0.30f,0.48f,0.29f,0.13f);
        cubeYaw(v,swingX+side*2.8f,1.75f,swingZ,0.13f,1.78f,0.13f,
                -side*0.30f,0.48f,0.29f,0.13f);
    }
    cube(v,swingX,3.35f,swingZ,3.05f,0.16f,0.16f,0.42f,0.24f,0.10f);
    // Children leave the swing when tapped; they now sit at their study desk.
    if(!childrenGoingStudy) {
    // Two little children gently swinging back and forth.
    for(int child=0;child<2;child++) {
        float phase=companionPhase*2.2f+child*1.65f;
        float sway=sinf(phase);
        float cx=swingX+(child==0?-0.82f:0.82f);
        float seatZ=swingZ+sway*0.62f;
        float seatY=1.55f+0.12f*cosf(phase);
        float lean=sway*0.18f;
        cubeYaw(v,cx,2.43f, swingZ,0.035f,0.88f,0.035f,lean,
                0.72f,0.72f,0.68f);
        cubeYaw(v,cx,2.43f, swingZ+0.04f,0.035f,0.88f,0.035f,lean,
                0.72f,0.72f,0.68f);
        cubeYaw(v,cx,seatY,seatZ,0.48f,0.09f,0.30f,lean,
                0.48f,0.22f,0.09f);
        // Small colourful child body, head, arms and dangling legs.
        cubeYaw(v,cx,seatY+0.48f,seatZ-0.02f,0.25f,0.34f,0.18f,
                lean,child==0?0.12f:0.92f,child==0?0.48f:0.28f,
                child==0?0.92f:0.18f);
        sphere(v,cx,seatY+0.91f,seatZ-0.04f,0.22f,0.23f,0.21f,
               0.91f,0.70f,0.55f);
        sphere(v,cx,seatY+1.04f,seatZ-0.08f,0.23f,0.11f,0.22f,
               child==0?0.12f:0.10f,0.07f,0.035f);
        cubeYaw(v,cx-0.32f,seatY+0.48f,seatZ-0.04f,0.07f,0.25f,0.07f,
                lean-0.25f,0.91f,0.70f,0.55f);
        cubeYaw(v,cx+0.32f,seatY+0.48f,seatZ-0.04f,0.07f,0.25f,0.07f,
                lean+0.25f,0.91f,0.70f,0.55f);
        cubeYaw(v,cx-0.13f,seatY-0.25f,seatZ-0.08f,0.075f,0.28f,0.075f,
                -sway*0.35f,0.12f,0.16f,0.25f);
        cubeYaw(v,cx+0.13f,seatY-0.25f,seatZ-0.08f,0.075f,0.28f,0.075f,
                sway*0.35f,0.12f,0.16f,0.25f);
    }
    } // children are no longer rendered on the swings after their study trip
}


static void drawRoadParkingMountains(const float* v) {
    // Long asphalt road running from the house garden toward the mountains.
    cube(v,0.0f,-0.28f,48.0f,5.2f,0.10f,43.0f,0.16f,0.17f,0.19f);
    // Road shoulders and dashed center line.
    cube(v,-5.45f,-0.22f,48.0f,0.22f,0.08f,43.0f,0.62f,0.58f,0.43f);
    cube(v, 5.45f,-0.22f,48.0f,0.22f,0.08f,43.0f,0.62f,0.58f,0.43f);
    for(int i=0;i<11;i++)
        cube(v,0.0f,-0.15f,12.0f+i*7.0f,0.10f,0.035f,2.0f,0.92f,0.86f,0.55f);

    // A marked parking lot beside the road, with individual parking bays.
    cube(v,10.5f,-0.26f,30.0f,8.2f,0.09f,10.0f,0.20f,0.21f,0.23f);
    for(int i=0;i<6;i++) {
        float x=4.0f+i*2.55f;
        cube(v,x,-0.13f,30.0f,0.035f,0.025f,4.3f,0.92f,0.90f,0.76f);
    }
    cube(v,10.5f,-0.13f,25.7f,8.0f,0.025f,0.035f,0.92f,0.90f,0.76f);

    // Natural rounded mountain range: overlapping broad foothills, taller
    // rounded peaks, and darker rocky shoulders create a layered silhouette.
    sphere(v,-29.0f,1.8f,65.0f,20.0f,7.8f,20.0f,0.25f,0.34f,0.22f);
    sphere(v,-31.0f,5.6f,66.0f,14.0f,8.8f,14.0f,0.29f,0.38f,0.25f);
    sphere(v,-23.0f,4.4f,73.0f,13.0f,7.0f,15.0f,0.31f,0.40f,0.27f);
    sphere(v, 29.0f,1.8f,67.0f,20.0f,7.8f,20.0f,0.24f,0.33f,0.21f);
    sphere(v, 31.0f,5.8f,68.0f,14.0f,9.0f,14.0f,0.28f,0.37f,0.24f);
    sphere(v, 23.0f,4.0f,76.0f,13.0f,7.2f,15.0f,0.30f,0.39f,0.26f);
    sphere(v, 0.0f,2.0f,84.0f,28.0f,10.0f,17.0f,0.30f,0.39f,0.26f);
    sphere(v,-5.0f,7.2f,84.0f,17.0f,10.0f,12.0f,0.34f,0.43f,0.29f);
    sphere(v, 8.0f,6.4f,88.0f,16.0f,9.0f,12.0f,0.32f,0.41f,0.27f);
    // Exposed rock bands on the mountain faces.
    sphere(v,-34.0f,4.2f,63.0f,9.5f,3.0f,11.0f,0.38f,0.36f,0.29f);
    sphere(v, 34.0f,4.0f,66.0f,9.5f,3.0f,11.0f,0.39f,0.37f,0.30f);
    sphere(v, 0.0f,7.0f,91.0f,12.0f,3.4f,7.0f,0.40f,0.38f,0.31f);

    // Winding paved road climbs the mountain in a series of banked segments.
    // Every segment follows the terrain height so cars can drive up the route.
    float prevX=mountainRoadX(47.0f), prevZ=47.0f;
    for(int s=1;s<=86;s++) {
        float z=47.0f+s*0.5f;
        float x=mountainRoadX(z);
        float dx=x-prevX, dz=z-prevZ;
        float len=sqrtf(dx*dx+dz*dz);
        float angle=atan2f(dx,dz);
        float h=hillHeightAt(x,z);
        cubeYaw(v,(x+prevX)*0.5f,h+0.12f,(z+prevZ)*0.5f,
                3.15f,0.12f,len*0.56f,angle,0.20f,0.21f,0.22f);
        // pale edge lines make the winding route readable from a distance
        cubeYaw(v,(x+prevX)*0.5f-1.48f*cosf(angle),h+0.20f,
                (z+prevZ)*0.5f+1.48f*sinf(angle),0.055f,0.035f,len*0.54f,
                angle,0.88f,0.84f,0.69f);
        cubeYaw(v,(x+prevX)*0.5f+1.48f*cosf(angle),h+0.20f,
                (z+prevZ)*0.5f-1.48f*sinf(angle),0.055f,0.035f,len*0.54f,
                angle,0.88f,0.84f,0.69f);
        prevX=x; prevZ=z;
    }

    // Pine trees: dark trunks and stacked tapered-looking green crowns.
    const float px[9]={-40,-39,-36,-31,-25,25,31,36,40};
    const float pz[9]={59,70,57,82,78,80,83,66,59};
    for(int i=0;i<9;i++) {
        float x=px[i], z=pz[i];
        cube(v,x,1.15f,z,0.28f,1.15f,0.28f,0.30f,0.16f,0.07f);
        sphere(v,x,2.55f,z,1.75f,2.1f,1.75f,0.035f,0.23f,0.10f);
        sphere(v,x,3.75f,z,1.35f,1.75f,1.35f,0.025f,0.29f,0.12f);
        sphere(v,x,4.75f,z,0.92f,1.25f,0.92f,0.04f,0.34f,0.14f);
    }
}

static bool inPond(float x,float z) {
    float dx=(x-10.0f)/3.35f;
    float dz=(z-25.0f)/2.25f;
    return dx*dx+dz*dz < 0.88f;
}

static float hillHeightAt(float x,float z) {
    auto mound = [&](float cx,float cz,float rx,float rz,float peak) {
        float dx=(x-cx)/rx, dz=(z-cz)/rz;
        float q=dx*dx+dz*dz;
        if(q>=1.0f) return 0.0f;
        return peak*sqrtf(1.0f-q);
    };
    float h=0.0f;
    h=std::max(h,mound(-29.0f,65.0f,20.0f,20.0f,8.8f));
    h=std::max(h,mound(-31.0f,66.0f,14.0f,14.0f,11.0f));
    h=std::max(h,mound(-23.0f,73.0f,13.0f,15.0f,8.0f));
    h=std::max(h,mound(29.0f,67.0f,20.0f,20.0f,8.8f));
    h=std::max(h,mound(31.0f,68.0f,14.0f,14.0f,11.0f));
    h=std::max(h,mound(23.0f,76.0f,13.0f,15.0f,8.0f));
    h=std::max(h,mound(0.0f,84.0f,28.0f,17.0f,10.0f));
    h=std::max(h,mound(-5.0f,84.0f,17.0f,12.0f,12.0f));
    h=std::max(h,mound(8.0f,88.0f,16.0f,12.0f,10.5f));
    return h;
}

static void drawGardenBuildingsAndCars(const float* v) {
    // Two small solid garden houses. Their collision footprints are registered
    // in setupCollision(), so the player cannot walk through the walls.
    const float houseXs[2] = {-20.0f, 20.0f};
    for(int i=0;i<2;i++) {
        float x=houseXs[i];
        // walls and flat roof
        cube(v,x,1.45f,3.0f,3.5f,1.45f,3.4f,
             i==0?0.78f:0.86f,i==0?0.67f:0.78f,0.52f);
        cube(v,x,3.05f,3.0f,3.8f,0.22f,3.7f,0.35f,0.16f,0.12f);
        // front door and windows
        cube(v,x,0.95f,6.43f,0.62f,0.95f,0.08f,0.28f,0.12f,0.06f);
        cube(v,x-2.0f,1.85f,6.48f,0.62f,0.48f,0.07f,0.22f,0.65f,0.86f);
        cube(v,x+2.0f,1.85f,6.48f,0.62f,0.48f,0.07f,0.22f,0.65f,0.86f);
        // simple porch
        cube(v,x,0.18f,7.0f,1.35f,0.12f,0.8f,0.55f,0.43f,0.30f);
    }

    // New two-car style garage beside the mansion driveway.
    // Front is intentionally open so the player can drive straight in/out.
    cube(v,-10.0f,3.55f,34.5f,4.20f,0.18f,5.50f,0.20f,0.22f,0.25f); // roof
    cube(v,-14.0f,1.72f,34.5f,0.18f,1.72f,5.50f,0.68f,0.69f,0.67f);
    cube(v,-6.0f,1.72f,34.5f,0.18f,1.72f,5.50f,0.68f,0.69f,0.67f);
    cube(v,-10.0f,1.72f,39.78f,4.20f,1.72f,0.18f,0.62f,0.63f,0.62f);
    cube(v,-10.0f,-0.22f,34.5f,4.00f,0.08f,5.30f,0.34f,0.35f,0.36f); // concrete floor
    // Wide open front entrance: only two side pillars, no gate or shutter.
    cube(v,-13.82f,1.72f,29.12f,0.18f,1.72f,0.18f,0.35f,0.36f,0.38f);
    cube(v,-6.18f,1.72f,29.12f,0.18f,1.72f,0.18f,0.35f,0.36f,0.38f);
    cube(v,-10.0f,3.25f,29.12f,3.80f,0.16f,0.18f,0.24f,0.25f,0.27f);
    // Segmented rolling shutter. The slats rise toward the roof when opened.
    float shutterLift = garageShutterAmount*2.55f;
    if(garageShutterAmount < 0.995f) {
        for(int s=0;s<17;s++) {
            float y=0.16f+s*0.19f+shutterLift;
            cube(v,-10.0f,y,29.12f,3.68f,0.075f,0.075f,
                 0.34f,0.38f,0.42f);
        }
    }
    // Garage shutter button on the outside of the left pillar.
    cube(v,-14.28f,1.30f,29.48f,0.09f,0.25f,0.10f,0.12f,0.13f,0.14f);
    sphere(v,-14.28f,1.38f,29.32f,0.075f,0.075f,0.055f,
           garageShutterOpen?0.10f:0.95f,garageShutterOpen?0.90f:0.08f,0.12f);
    sphere(v,-10.0f,3.12f,32.0f,0.22f,0.10f,0.22f,1.0f,0.88f,0.52f);

    // Three parked cars around the garden. Each has a solid collision box.
    const float carX[3] = {-10.0f,12.0f,0.0f};
    const float carZ[3] = {34.5f,9.0f,40.0f};
    const float carColors[3][3] = {
        {0.78f,0.08f,0.06f}, {0.08f,0.24f,0.82f}, {0.88f,0.68f,0.08f}
    };
    for(int i=0;i<3;i++) {
        float x=(i==0?carX0:carX[i]), z=(i==0?carZ0:carZ[i]);
        float a=(i==0?carYaw:0.0f);
        float r=carColors[i][0],g=carColors[i][1],b=carColors[i][2];
        float carVP[16];
        const float* drawVP=v;
        if(i==0 && std::fabs(carVisualHeight)>0.001f) {
            float tr[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,carVisualHeight,0,1};
            multiply(carVP,v,tr);
            drawVP=carVP;
        }
        // Main body stays at the car origin.
        cubeYaw(drawVP,x,0.62f,z,1.25f,0.38f,2.25f,a,r,g,b);

        // Convert every attached part from local car coordinates to world
        // coordinates. This keeps cabin, bumpers, wheels and lamps rigidly
        // attached to the same moving/rotating car origin.
        auto worldPoint = [&](float lx, float lz, float &wx, float &wz) {
            wx = x + cosf(a)*lx + sinf(a)*lz;
            wz = z - sinf(a)*lx + cosf(a)*lz;
        };

        float px,pz;
        // Open-top convertible cabin: no roof, no tall pillars, and no window panels.
        // Low sculpted door tops keep the car looking finished without blocking the view.
        for(int side=-1;side<=1;side+=2) {
            worldPoint(side*0.88f,-0.18f,px,pz);
            cubeYaw(drawVP,px,0.91f,pz,0.10f,0.16f,1.62f,a,0.10f,0.12f,0.15f);
            worldPoint(side*0.88f,-0.18f,px,pz);
            cubeYaw(drawVP,px,1.025f,pz,0.12f,0.035f,1.58f,a,0.62f,0.66f,0.70f);
        }
        // Short, low windshield only; it sits below eye level so the road remains clear.
        float gx,gz;
        worldPoint(0.0f,-0.91f,gx,gz);
        cubeYawAlpha(drawVP,gx,1.12f,gz,0.70f,0.12f,0.025f,a,0.70f,0.88f,0.96f,0.035f);

        // Visible interior: dashboard, instrument panel and a 3D steering wheel.
        // These are attached to the car, so they stay inside the cabin while driving.
        worldPoint(0.0f,-0.58f,px,pz);
        cubeYaw(drawVP,px,0.86f,pz,0.82f,0.075f,0.27f,a,0.055f,0.065f,0.075f);
        worldPoint(0.0f,-0.79f,px,pz);
        cubeYaw(drawVP,px,0.98f,pz,0.68f,0.09f,0.045f,a,0.10f,0.12f,0.14f);

        // Instrument cluster with a lit dial and a speed needle.
        worldPoint(-0.36f,-0.82f,px,pz);
        sphereYaw(drawVP,px,1.00f,pz,0.13f,0.07f,0.035f,a,0.015f,0.025f,0.035f);
        float dialNeedle=-2.35f+std::min(1.0f,carCurrentSpeed/180.0f)*4.70f;
        for(int tick=0;tick<9;tick++) {
            float ang=-2.35f+tick*0.5875f;
            float tx=px+cosf(a)*cosf(ang)*0.085f;
            float ty=1.00f+sinf(ang)*0.050f;
            float tz=pz-sinf(a)*cosf(ang)*0.085f;
            sphere(v,tx,ty,tz,0.012f,0.012f,0.012f,0.55f,0.82f,0.95f);
        }
        float nx=px+cosf(a)*cosf(dialNeedle)*0.075f;
        float ny=1.00f+sinf(dialNeedle)*0.045f;
        float nz=pz-sinf(a)*cosf(dialNeedle)*0.075f;
        sphere(v,nx,ny,nz,0.018f,0.018f,0.018f,1.0f,0.18f,0.10f);

        // Steering wheel ring in the driver's view; spokes rotate with steering.
        worldPoint(0.0f,-0.38f,px,pz);
        float wheelCX=px, wheelCZ=pz, wheelCY=1.12f;
        for(int seg=0;seg<12;seg++) {
            float ang=seg*0.523599f+steeringWheelAngle;
            float lx=cosf(ang)*0.255f;
            float ly=sinf(ang)*0.255f;
            float wx=x+cosf(a)*lx+sin(a)*(-0.38f);
            float wz=z-sinf(a)*lx+cosf(a)*(-0.38f);
            sphere(v,wx,wheelCY+ly,wz,0.045f,0.045f,0.045f,0.12f,0.13f,0.15f);
        }
        for(int spoke=0;spoke<3;spoke++) {
            float ang=steeringWheelAngle+spoke*2.0943951f;
            float lx=cosf(ang)*0.13f, ly=sinf(ang)*0.13f;
            float wx=x+cosf(a)*lx+sin(a)*(-0.38f);
            float wz=z-sinf(a)*lx+cosf(a)*(-0.38f);
            sphere(v,wx,wheelCY+ly,wz,0.055f,0.055f,0.055f,0.48f,0.51f,0.55f);
        }
        worldPoint(0.0f,-2.30f,px,pz);
        cubeYaw(drawVP,px,0.48f,pz,1.18f,0.14f,0.12f,a,0.18f,0.18f,0.18f);
        worldPoint(0.0f,2.30f,px,pz);
        cubeYaw(drawVP,px,0.48f,pz,1.18f,0.14f,0.12f,a,0.18f,0.18f,0.18f);

        // Four rubber tyres with visible hubs and rotating spoke markers.
        // The player's moving car spins; the other parked cars remain still.
        float spin=(i==0?carWheelSpin:0.0f);
        for(int side=-1;side<=1;side+=2) {
            for(int axle=-1;axle<=1;axle+=2) {
                float wx,wz;
                worldPoint(side*1.22f,axle*1.35f,wx,wz);
                sphereYaw(drawVP,wx,0.38f,wz,0.34f,0.34f,0.48f,a,0.045f,0.048f,0.055f);

                float faceX=wx+side*0.325f;
                sphereYaw(drawVP,faceX,0.38f,wz,0.035f,0.205f,0.205f,a,0.62f,0.66f,0.70f);
                sphereYaw(drawVP,faceX+side*0.012f,0.38f,wz,0.042f,0.075f,0.075f,a,0.16f,0.18f,0.20f);

                // Four bright lug/spoke dots orbit the hub as the wheel turns.
                for(int spoke=0;spoke<4;spoke++) {
                    float ang=spin+spoke*1.5707963f;
                    float yy=0.38f+cosf(ang)*0.145f;
                    float zz=wz+sinf(ang)*0.145f;
                    sphere(v,faceX+side*0.025f,yy,zz,0.045f,0.035f,0.035f,
                           0.90f,0.92f,0.96f);
                }
            }
        }

        // Front headlights and rear lamps use exactly the same local transform.
        float hx1,hz1,hx2,hz2,tx1,tz1,tx2,tz2;
        worldPoint(-0.72f,-2.28f,hx1,hz1);
        worldPoint( 0.72f,-2.28f,hx2,hz2);
        worldPoint(-0.72f, 2.28f,tx1,tz1);
        worldPoint( 0.72f, 2.28f,tx2,tz2);
        sphere(v,hx1,0.70f,hz1,0.22f,0.13f,0.08f,1.0f,0.92f,0.55f);
        sphere(v,hx2,0.70f,hz2,0.22f,0.13f,0.08f,1.0f,0.92f,0.55f);
        sphere(v,tx1,0.68f,tz1,0.18f,0.12f,0.08f,0.85f,0.03f,0.02f);
        sphere(v,tx2,0.68f,tz2,0.18f,0.12f,0.08f,0.85f,0.03f,0.02f);
    }
}





static void drawCollegeAccessRoad(const float* v) {
    // Long, widened, level ground corridor from the mansion road to the
    // GL Bajaj campus. The road now reaches the campus gate and parking apron.
    const float x1=-2.0f, z1=49.0f, x2=-145.0f, z2=-33.0f;
    float dx=x2-x1, dz=z2-z1;
    float len=sqrtf(dx*dx+dz*dz), angle=atan2f(dx,dz);
    float mx=(x1+x2)*0.5f, mz=(z1+z2)*0.5f;
    // Broad flat grass verge keeps the route connected to the world ground.
    cubeYaw(v,mx,-0.38f,mz,18.0f,0.12f,len*0.54f,
            angle,0.28f,0.40f,0.22f);
    // Extra-wide asphalt carriageway for cars, plus shoulder strips.
    cubeYaw(v,mx,-0.245f,mz,7.0f,0.12f,len*0.52f,
            angle,0.14f,0.15f,0.17f);
    cubeYaw(v,mx-7.8f*cosf(angle),-0.20f,mz+7.8f*sinf(angle),
            0.45f,0.07f,len*0.52f,angle,0.62f,0.58f,0.43f);
    cubeYaw(v,mx+7.8f*cosf(angle),-0.20f,mz-7.8f*sinf(angle),
            0.45f,0.07f,len*0.52f,angle,0.62f,0.58f,0.43f);
    // Dashed center line, placed as individual short road markings.
    for(int i=0;i<22;i++) {
        float t=(i+0.5f)/22.0f;
        cubeYaw(v,x1+dx*t,-0.105f,z1+dz*t,0.10f,0.025f,1.15f,
                angle,0.95f,0.86f,0.58f);
    }
    // Wide paved campus forecourt joins the road to the main entrance.
    cube(v,-145.0f,-0.24f,-31.8f,12.0f,0.12f,5.8f,
         0.16f,0.17f,0.19f);
}

static void drawAyushCollegeLane(const float* v) {
    // Private two-lane road starts directly behind Ayush's house and runs
    // straight to the GL BAZZAR college approach, without a winding detour.
    const float x1=25.0f, z1=-16.0f;
    const float x2=-145.0f, z2=-33.0f;
    const float dx=x2-x1, dz=z2-z1;
    const float len=sqrtf(dx*dx+dz*dz);
    const float angle=atan2f(dx,dz);
    const float mx=(x1+x2)*0.5f, mz=(z1+z2)*0.5f;
    cubeYaw(v,mx,-0.39f,mz,13.0f,0.12f,len*0.54f,
            angle,0.24f,0.36f,0.22f);
    cubeYaw(v,mx,-0.245f,mz,6.2f,0.12f,len*0.52f,
            angle,0.12f,0.13f,0.15f);
    // Pale shoulders and continuous edge guides.
    cubeYaw(v,mx-3.25f*cosf(angle),-0.19f,mz+3.25f*sinf(angle),
            0.22f,0.06f,len*0.52f,angle,0.78f,0.73f,0.58f);
    cubeYaw(v,mx+3.25f*cosf(angle),-0.19f,mz-3.25f*sinf(angle),
            0.22f,0.06f,len*0.52f,angle,0.78f,0.73f,0.58f);
    // Broken white center markings make the riding line easy to follow.
    int marks=(int)(len/5.0f);
    for(int i=0;i<marks;i++) {
        float t=(i+0.5f)/(float)marks;
        cubeYaw(v,x1+dx*t,-0.105f,z1+dz*t,
                0.09f,0.025f,1.45f,angle,0.94f,0.92f,0.82f);
    }
    // Rear gate apron links the back exit to the center of the lane.
    cubeYaw(v,x1,-0.22f,z1+1.4f,4.2f,0.10f,4.0f,
            angle,0.15f,0.16f,0.18f);
    // Campus arrival apron connects the lane to the main college gate.
    cubeYaw(v,x2,-0.22f,z2,9.0f,0.10f,6.0f,
            angle,0.15f,0.16f,0.18f);
}

static void drawAyushLaneTrees(const float* v) {
    // Trees and low shrubs follow both verges of the private college lane.
    const float x1=25.0f, z1=-16.0f, x2=-145.0f, z2=-33.0f;
    float dx=x2-x1, dz=z2-z1;
    float len=sqrtf(dx*dx+dz*dz);
    float nx=-dz/len, nz=dx/len;
    for(int i=1;i<=19;i++) {
        float t=i/20.0f;
        float cx=x1+dx*t, cz=z1+dz*t;
        for(int side=-1;side<=1;side+=2) {
            float tx=cx+nx*side*6.0f;
            float tz=cz+nz*side*6.0f;
            // Tree trunk and layered green canopy.
            cube(v,tx,1.25f,tz,0.20f,1.25f,0.20f,0.34f,0.20f,0.10f);
            sphere(v,tx,2.75f,tz,1.15f,1.45f,1.10f,
                   0.12f,0.34f,0.14f);
            sphere(v,tx-0.48f,2.48f,tz+0.18f,0.78f,0.92f,0.82f,
                   0.16f,0.42f,0.17f);
            // Small flowering shrubs at the foot of selected trees.
            if(i%2==0) {
                sphere(v,tx+1.1f,0.48f,tz+0.4f,0.62f,0.42f,0.58f,
                       0.18f,0.38f,0.16f);
                sphere(v,tx+1.12f,0.78f,tz+0.42f,0.12f,0.10f,0.12f,
                       0.95f,0.55f,0.68f);
            }
        }
    }
}


static void startAlienInvasion() {
    if(alienInvasionStarted) return;
    alienInvasionStarted=true; alienShipLanded=false;
    alienShipY=34.0f; alienShipDoor=0.0f;
    alienPrisonUnlocked=false; abductedNPCCount=0;
    for(NPC& n:npcs) { n.alienAbducting=false; n.alienCaptured=false; n.alienLift=0.0f; n.alienBeingLifted=false; }
    alienSoldiers.clear(); alienBullets.clear();
    for(int i=0;i<24;i++) {
        AlienSoldier a;
        float ang=(float)i*(2.0f*PI/24.0f);
        a.x=-145.0f+cosf(ang)*alienSpawnRadius;
        a.z=-43.5f+sinf(ang)*alienSpawnRadius;
        a.fireTimer=2.8f+(float)(i%7)*0.45f;
        alienSoldiers.push_back(a);
    }
}
static void updateAlienInvasion(float dt) {
    if(!alienInvasionStarted) return;
    if(!alienShipLanded) {
        alienShipY=std::max(3.8f,alienShipY-dt*3.4f);
        if(alienShipY<=3.8f) alienShipLanded=true;
    } else alienShipDoor=std::min(1.0f,alienShipDoor+dt*0.35f);
    for(AlienSoldier& a:alienSoldiers) if(a.alive) {
        a.hitFlash=std::max(0.0f,a.hitFlash-dt);
        float dx=playerX-a.x,dz=playerZ-a.z;
        float d=sqrtf(dx*dx+dz*dz);
        if(d>3.0f) {
            float step=std::min(4.6f*dt,d-1.7f);
            float nx=a.x+dx/(d>0.001f?d:1.0f)*step;
            float nz=a.z+dz/(d>0.001f?d:1.0f)*step;
            if(!collides(nx,a.z)) a.x=nx;
            if(!collides(a.x,nz)) a.z=nz;
        }
        a.yaw=atan2f(dx,dz);
        a.fireTimer-=dt;
        if(d<34.0f && a.fireTimer<=0.0f && !drivingCar) {
            a.fireTimer=2.8f+(float)(rand()%16)*0.12f;
            float inv=1.0f/(d>0.01f?d:1.0f);
            AlienBullet b; b.x=a.x; b.y=1.20f; b.z=a.z;
            b.vx=dx*inv*13.0f; b.vz=dz*inv*13.0f;
            b.vy=(playerHeight+0.9f-b.y)*inv*13.0f;
            alienBullets.push_back(b);
        }
    }
    // Residents form a defensive line, but their fire is deliberately weak.
    static float residentFireClock=0.0f;
    residentFireClock-=dt;
    int residentKills=0;
    for(size_t ni=0;ni<npcs.size();++ni) {
        NPC& n=npcs[ni];
        if(n.alienAbducting || n.alienCaptured) continue;
        float guardAngle=(float)ni*(2.0f*PI/10.0f);
        float guardX=cosf(guardAngle)*7.0f;
        float guardZ=6.0f+sinf(guardAngle)*7.0f;
        float rallyDX=guardX-n.x, rallyDZ=guardZ-n.z;
        float rallyD=sqrtf(rallyDX*rallyDX+rallyDZ*rallyDZ);
        if(rallyD>0.65f) {
            float rallyStep=std::min(1.25f*dt,rallyD);
            float nx=n.x+rallyDX/rallyD*rallyStep, nz=n.z+rallyDZ/rallyD*rallyStep;
            if(!collides(nx,n.z)) n.x=nx;
            if(!collides(n.x,nz)) n.z=nz;
        }
        float pdx=n.x-playerX, pdz=n.z-playerZ;
        if(pdx*pdx+pdz*pdz<225.0f) {
            AlienSoldier* target=nullptr; float best=22.0f;
            for(AlienSoldier& a:alienSoldiers) if(a.alive) {
                float dx=a.x-n.x,dz=a.z-n.z,dd=sqrtf(dx*dx+dz*dz);
                if(dd<best) {best=dd;target=&a;}
            }
            if(target && residentFireClock<=0.0f) {
                target->hp-=5; target->hitFlash=0.08f;
                if(target->hp<=0) { target->alive=false; residentKills++; }
                n.yaw=atan2f(target->x-n.x,target->z-n.z);
            }
        }
    }
    if(residentFireClock<=0.0f) residentFireClock=2.6f;

    // Up to three residents can be slowly abducted and carried to the school-side ship.
    const float shipX=-145.0f, shipZ=-43.5f;
    for(AlienSoldier& a:alienSoldiers) if(a.alive) {
        NPC* victim=nullptr; float nearest=2.0f;
        for(NPC& n:npcs) if(!n.alienAbducting && !n.alienCaptured) {
            float dx=n.x-a.x,dz=n.z-a.z,dd=sqrtf(dx*dx+dz*dz);
            if(dd<nearest) { nearest=dd; victim=&n; }
        }
        if(victim && nearest<1.7f) { victim->alienAbducting=true; abductedNPCCount++; }
    }
    for(NPC& n:npcs) if(n.alienAbducting && !n.alienCaptured) {
        if(!n.alienBeingLifted) {
            float dx=shipX-n.x,dz=shipZ-n.z,dd=sqrtf(dx*dx+dz*dz);
            if(dd>2.2f) {
                // The alien soldier drags the frightened resident quickly toward its ship.
                float step=std::min(5.2f*dt,dd-1.8f);
                n.x+=dx/dd*step; n.z+=dz/dd*step;
            } else {
                n.alienBeingLifted=true;
                n.alienLift=0.0f;
            }
        } else {
            // Tractor beam raises the resident visibly into the spaceship.
            n.alienLift=std::min(5.4f,n.alienLift+dt*2.15f);
            if(n.alienLift>=5.4f) n.alienCaptured=true;
        }
    }
    // The player's car knocks aliens flat and removes them immediately.
    if(drivingCar && carCurrentSpeed>8.0f) {
        for(AlienSoldier& a:alienSoldiers) if(a.alive) {
            float dx=a.x-carX0,dz=a.z-carZ0;
            if(dx*dx+dz*dz<7.0f) { a.alive=false; a.hp=0; }
        }
    }
    for(AlienBullet& b:alienBullets) {
        b.x+=b.vx*dt; b.y+=b.vy*dt; b.z+=b.vz*dt; b.life-=dt;
        float dx=b.x-playerX,dz=b.z-playerZ;
        if(dx*dx+dz*dz<0.72f && fabsf(b.y-(playerHeight+0.9f))<1.0f) {
            health=std::max(0.0f,health-2.5f); b.life=0.0f;
            if(health<=0.0f) gameOver=true;
        }
    }
    alienBullets.erase(std::remove_if(alienBullets.begin(),alienBullets.end(),
        [](const AlienBullet& b){return b.life<=0.0f;}),alienBullets.end());
}
static void drawAlienInvasion(const float* v) {
    if(alienInvasionStarted) {
        float sx=-145.0f, sz=-43.5f;
        sphere(v,sx,alienShipY,sz,8.0f,1.0f,6.0f,0.24f,0.28f,0.34f);
        sphere(v,sx,alienShipY+0.75f,sz,3.2f,1.15f,2.8f,0.12f,0.70f,0.88f);
        sphere(v,sx,alienShipY-0.30f,sz,7.6f,0.16f,5.8f,0.10f,0.85f,0.95f);
        if(alienShipLanded) {
            float beamPulse=0.55f+0.18f*sinf((float)SDL_GetTicks()*0.008f);
            // Wide cyan tractor beam directly under the UFO beside the school.
            cube(v,sx,alienShipY*0.50f,sz,5.2f,alienShipY*0.50f,4.2f,
                 0.08f,beamPulse,1.0f);
        }
        for(int leg=0;leg<4;leg++) {
            float a=leg*PI*0.5f;
            cube(v,sx+cosf(a)*5.5f,alienShipY*0.5f,sz+sinf(a)*4.2f,
                 0.16f,alienShipY*0.5f,0.16f,0.36f,0.40f,0.46f);
        }
        if(alienShipLanded) {
            cube(v,sx,1.85f,sz+5.8f+alienShipDoor*1.1f,2.0f,0.12f,2.0f,0.20f,0.78f,0.90f);
            // Visible holding cells beneath the landed spaceship.
            int cellCount=std::min((int)npcs.size(),12);
            for(int c=0;c<cellCount;c++) if(c<abductedNPCCount && !alienPrisonUnlocked) {
                float cx=sx-3.3f+(c%4)*2.2f;
                float cz=sz-2.0f+(c/4)*2.0f;
                cube(v,cx,0.95f,cz,0.72f,0.08f,0.72f,0.12f,0.16f,0.22f);
                for(int bar=0;bar<4;bar++) cube(v,cx-0.52f+bar*0.34f,1.45f,cz,0.035f,0.55f,0.035f,0.20f,0.82f,0.92f);
                cube(v,cx,2.0f,cz,0.62f,0.035f,0.035f,0.20f,0.82f,0.92f);
                sphere(v,cx,0.95f,cz,0.20f,0.32f,0.18f,0.20f,0.72f,0.48f);
            }
        }
    }
    for(const AlienSoldier& a:alienSoldiers) if(a.alive) {
        float c=a.hitFlash>0.0f?1.0f:0.40f;
        cubeYaw(v,a.x,1.05f,a.z,0.30f,0.46f,0.22f,a.yaw,c,0.72f,0.22f);
        sphere(v,a.x,1.68f,a.z,0.24f,0.25f,0.24f,0.35f,0.82f,0.48f);
        sphere(v,a.x-0.09f,1.72f,a.z-0.18f,0.045f,0.035f,0.035f,0.95f,0.10f,0.08f);
        sphere(v,a.x+0.09f,1.72f,a.z-0.18f,0.045f,0.035f,0.035f,0.95f,0.10f,0.08f);
        cubeYaw(v,a.x+0.28f,1.18f,a.z-0.30f,0.08f,0.08f,0.48f,a.yaw,0.10f,0.12f,0.13f);
    }
    for(const AlienBullet& b:alienBullets)
        sphere(v,b.x,b.y,b.z,0.10f,0.10f,0.10f,1.0f,0.18f,0.06f);
    if(!ak47Owned && !drivingCar) {
        // AK-47 displayed on the classroom wall beside the children's study desks.
        // Brightly lit wall-mounted rifle rack beside the house study desk.
        cube(v,akPickupX,1.65f,akPickupZ,0.12f,0.12f,0.88f,0.08f,0.09f,0.10f);
        cube(v,akPickupX,1.83f,akPickupZ-0.48f,0.18f,0.07f,0.28f,0.12f,0.13f,0.14f);
        cube(v,akPickupX,1.43f,akPickupZ+0.34f,0.10f,0.25f,0.18f,0.32f,0.18f,0.07f);
        cube(v,akPickupX+0.10f,1.65f,akPickupZ+0.05f,0.07f,0.07f,0.55f,0.72f,0.74f,0.76f);
        sphere(v,akPickupX,2.18f,akPickupZ,0.23f,0.23f,0.23f,1.0f,0.82f,0.08f);
    }
}
static void fireAK47() {
    if(onPlanetSurface) {
        if(fireCooldown>0.0f) return;
        fireCooldown=0.105f;
        float fx=-sinf(yaw), fz=-cosf(yaw);
        HomeworldGuard* target=nullptr;
        float best=58.0f;
        for(HomeworldGuard& g:homeworldGuards) if(g.alive) {
            float dx=g.x-playerX,dz=g.z-playerZ;
            float along=dx*fx+dz*fz;
            float side=fabsf(dx*fz-dz*fx);
            float dist=sqrtf(dx*dx+dz*dz);
            if(along>0.0f && side<1.9f && dist<best) { best=dist; target=&g; }
        }
        if(target) {
            target->hp-=34; target->flash=0.18f;
            if(target->hp<=0) { target->alive=false; SDL_Log("Alien guard eliminated."); }
        }
        return;
    }
    if(gameOver || !ak47Owned || fireCooldown>0.0f || drivingCar) return;
    ammo=999999; reserveAmmo=999999; fireCooldown=0.13f; gunSoundSamples=2205;
    float fx=-sinf(yaw), fz=-cosf(yaw);
    float best=85.0f; AlienSoldier* hit=nullptr;
    for(AlienSoldier& a:alienSoldiers) if(a.alive) {
        float dx=a.x-playerX,dz=a.z-playerZ;
        float along=dx*fx+dz*fz;
        if(along<0.0f || along>best) continue;
        float side=fabsf(dx*fz-dz*fx);
        if(side<(scopeMode?0.65f:1.45f)) {best=along;hit=&a;}
    }
    if(hit) {hit->hp-=scopeMode?100:55;hit->hitFlash=0.18f;if(hit->hp<=0)hit->alive=false;}
}
static void toggleScope() { if(ak47Owned) scopeMode=!scopeMode; }

static void drawGLBajajCollege(const float* v) {
    // Remote campus, deliberately far from the mansion: north-west district.
    // Same campus origin used by the original V143 college scene.
    const float cx=-145.0f, cz=-48.0f;
    // SOLID CAMPUS GATE: stone posts and metal gate wings; center passage stays open.
    cube(v,cx-9.0f,1.45f,cz+38.0f,0.42f,1.45f,0.42f,0.52f,0.39f,0.24f);
    cube(v,cx+9.0f,1.45f,cz+38.0f,0.42f,1.45f,0.42f,0.52f,0.39f,0.24f);
    for(int i=0;i<6;i++) {
        float gx=cx-7.0f+i*0.62f;
        cube(v,gx,1.0f,cz+38.0f,0.045f,1.0f,0.06f,0.12f,0.16f,0.19f);
        gx=cx+3.9f+i*0.62f;
        cube(v,gx,1.0f,cz+38.0f,0.045f,1.0f,0.06f,0.12f,0.16f,0.19f);
    }
    cube(v,cx-5.15f,1.0f,cz+38.0f,1.85f,0.07f,0.09f,0.12f,0.16f,0.19f);
    cube(v,cx+5.15f,1.0f,cz+38.0f,1.85f,0.07f,0.09f,0.12f,0.16f,0.19f);

    // INTERACTIVE CLASSROOM WING: roof omitted so the room and its occupants
    // remain visible from the player's camera. The floor and walls are solid.
    const float roomX=cx, roomZ=cz+20.5f; // (-145,-27.5)
    cube(v,roomX,-0.12f,roomZ,10.0f,0.12f,9.0f,0.72f,0.70f,0.62f);
    cube(v,roomX-10.0f,2.55f,roomZ,0.22f,2.55f,9.0f,0.78f,0.73f,0.62f);
    cube(v,roomX+10.0f,2.55f,roomZ,0.22f,2.55f,9.0f,0.78f,0.73f,0.62f);
    cube(v,roomX,2.55f,roomZ-9.0f,10.0f,2.55f,0.22f,0.78f,0.73f,0.62f);
    // Rear wall has a wide central entrance opening.
    cube(v,roomX-6.5f,2.55f,roomZ+9.0f,3.5f,2.55f,0.22f,0.78f,0.73f,0.62f);
    cube(v,roomX+6.5f,2.55f,roomZ+9.0f,3.5f,2.55f,0.22f,0.78f,0.73f,0.62f);
    // Double glass entrance gate, visibly folded open beside the doorway.
    cube(v,roomX-3.02f,1.35f,roomZ+8.82f,0.07f,1.35f,0.10f,0.16f,0.42f,0.55f);
    cube(v,roomX+3.02f,1.35f,roomZ+8.82f,0.07f,1.35f,0.10f,0.16f,0.42f,0.55f);
    cube(v,roomX-2.78f,1.35f,roomZ+8.72f,0.035f,0.08f,0.035f,0.86f,0.72f,0.35f);
    cube(v,roomX+2.78f,1.35f,roomZ+8.72f,0.035f,0.08f,0.035f,0.86f,0.72f,0.35f);
    // Teacher platform, desk and large writing board.
    cube(v,roomX,0.20f,roomZ-6.4f,4.2f,0.20f,1.0f,0.42f,0.39f,0.34f);
    cube(v,roomX,3.25f,roomZ-8.55f,4.1f,1.05f,0.12f,0.10f,0.24f,0.20f);
    cube(v,roomX,3.25f,roomZ-8.39f,3.75f,0.82f,0.035f,0.12f,0.34f,0.27f);
    cube(v,roomX,0.88f,roomZ-6.0f,1.55f,0.55f,0.55f,0.40f,0.22f,0.10f);
    // Teacher body, head, arms and pointer.
    cubeYaw(v,roomX,1.65f,roomZ-5.0f,0.34f,0.55f,0.23f,0.0f,0.18f,0.25f,0.68f);
    sphere(v,roomX,2.38f,roomZ-5.0f,0.27f,0.29f,0.25f,0.88f,0.68f,0.52f);
    cubeYaw(v,roomX-0.40f,1.75f,roomZ-5.1f,0.09f,0.38f,0.09f,-0.25f,0.88f,0.68f,0.52f);
    cubeYaw(v,roomX+0.40f,1.75f,roomZ-5.1f,0.09f,0.38f,0.09f,0.25f,0.88f,0.68f,0.52f);
    cubeYaw(v,roomX+0.62f,2.35f,roomZ-7.0f,0.035f,0.72f,0.035f,0.22f,0.72f,0.48f,0.18f);

    // Nine desks, books and chairs. Three rows of students face the teacher.
    for(int row=0;row<3;row++) for(int col=0;col<3;col++) {
        float sx=roomX-5.0f+col*5.0f;
        float sz=roomZ+5.0f-row*3.0f;
        cube(v,sx,0.92f,sz,0.88f,0.10f,0.52f,0.48f,0.27f,0.12f);
        cube(v,sx-0.68f,0.48f,sz,0.07f,0.48f,0.07f,0.24f,0.16f,0.08f);
        cube(v,sx+0.68f,0.48f,sz,0.07f,0.48f,0.07f,0.24f,0.16f,0.08f);
        cube(v,sx,0.97f,sz-0.10f,0.34f,0.035f,0.24f,0.90f,0.88f,0.72f);
        cube(v,sx,1.025f,sz-0.10f,0.30f,0.018f,0.21f,0.12f,0.25f,0.62f);
        cube(v,sx,1.045f,sz-0.10f,0.25f,0.008f,0.18f,0.92f,0.90f,0.78f);
        // Seated student: torso, head, arms and legs.
        float shirtR=((row+col)%3==0)?0.12f:(((row+col)%3==1)?0.72f:0.18f);
        float shirtG=((row+col)%3==0)?0.32f:(((row+col)%3==1)?0.18f:0.50f);
        float shirtB=((row+col)%3==0)?0.78f:(((row+col)%3==1)?0.16f:0.28f);
        cubeYaw(v,sx,1.28f,sz+0.82f,0.30f,0.42f,0.19f,0.0f,shirtR,shirtG,shirtB);
        sphere(v,sx,1.90f,sz+0.82f,0.23f,0.24f,0.21f,0.89f,0.69f,0.54f);
        cubeYaw(v,sx-0.32f,1.20f,sz+0.48f,0.07f,0.28f,0.07f,-0.45f,0.89f,0.69f,0.54f);
        cubeYaw(v,sx+0.32f,1.20f,sz+0.48f,0.07f,0.28f,0.07f,0.45f,0.89f,0.69f,0.54f);
        cubeYaw(v,sx-0.16f,0.43f,sz+1.02f,0.11f,0.34f,0.11f,0.0f,0.08f,0.10f,0.14f);
        cubeYaw(v,sx+0.16f,0.43f,sz+1.02f,0.11f,0.34f,0.11f,0.0f,0.08f,0.10f,0.14f);
    }
    // One student standing and reciting an answer to the teacher.
    cubeYaw(v,roomX+7.0f,1.05f,roomZ+1.0f,0.30f,0.48f,0.20f,0.0f,0.16f,0.55f,0.24f);
    sphere(v,roomX+7.0f,1.72f,roomZ+1.0f,0.23f,0.25f,0.21f,0.89f,0.69f,0.54f);
    cubeYaw(v,roomX+7.35f,1.48f,roomZ+0.85f,0.07f,0.35f,0.07f,-0.35f,0.89f,0.69f,0.54f);
    // Water station and a student walking toward it.
    cube(v,roomX+8.8f,1.05f,roomZ-4.0f,0.42f,1.05f,0.42f,0.12f,0.56f,0.72f);
    cube(v,roomX+8.8f,2.18f,roomZ-4.0f,0.25f,0.10f,0.25f,0.70f,0.82f,0.90f);
    cubeYaw(v,roomX+7.0f,0.92f,roomZ-3.6f,0.25f,0.40f,0.18f,0.0f,0.88f,0.42f,0.12f);
    sphere(v,roomX+7.0f,1.48f,roomZ-3.6f,0.22f,0.23f,0.20f,0.89f,0.69f,0.54f);
    float sip=0.5f+0.5f*sinf((float)SDL_GetTicks()*0.004f);
    cubeYaw(v,roomX+7.30f,1.20f+0.38f*sip,roomZ-3.58f,0.065f,0.27f,0.065f,-0.75f*sip,0.89f,0.69f,0.54f);
    cube(v,roomX+7.40f,1.47f+0.34f*sip,roomZ-3.50f,0.075f,0.10f,0.075f,0.65f,0.85f,0.95f);
    // Book shelf at the rear side of the classroom.
    cube(v,roomX-8.8f,1.35f,roomZ-4.0f,0.68f,1.35f,0.42f,0.34f,0.20f,0.10f);
    for(int i=0;i<6;i++) {
        float bx=roomX-9.28f+i*0.18f;
        cube(v,bx,1.45f,roomZ-3.55f,0.07f,0.78f,0.12f,
             0.10f+0.12f*(i%3),0.24f+0.08f*(i%2),0.62f);
    }

    // Campus grounds and front approach.
    cube(v,cx,-0.22f,cz,34.0f,0.18f,24.0f,0.34f,0.38f,0.32f);
    cube(v,cx,-0.10f,cz+15.0f,7.0f,0.10f,13.0f,0.18f,0.19f,0.21f);
    for(int i=0;i<7;i++) {
        cube(v,cx-14.0f+i*4.65f,0.12f,cz+10.2f,0.10f,0.035f,1.1f,
             0.92f,0.86f,0.66f);
    }
    // Main academic block: four storeys with warm stone and dark glass.
    cube(v,cx,5.7f,cz,25.0f,5.7f,7.0f,0.57f,0.48f,0.34f);
    cube(v,cx,11.55f,cz,25.8f,0.28f,7.6f,0.24f,0.20f,0.16f);
    // No oversized rooftop panel: keep the roofline clean.
    // Campus name is shown as gold lettering on the front facade below.
    // Side wings and strong vertical corner frames.
    cube(v,cx-12.7f,5.7f,cz,0.55f,5.85f,8.0f,0.42f,0.25f,0.13f);
    cube(v,cx+12.7f,5.7f,cz,0.55f,5.85f,8.0f,0.42f,0.25f,0.13f);
    // Central entrance tower with tall blue-glass facade and arch-like frame.
    // Open entrance portal with side piers and a high lintel.
    cube(v,cx-2.25f,7.0f,cz+3.42f,0.22f,7.0f,0.30f,0.76f,0.70f,0.58f);
    cube(v,cx+2.25f,7.0f,cz+3.42f,0.22f,7.0f,0.30f,0.76f,0.70f,0.58f);
    cube(v,cx,13.95f,cz+3.42f,2.5f,0.22f,0.30f,0.76f,0.70f,0.58f);
    // Glass door leaves are held open against the two sides.
    cube(v,cx-1.70f,1.55f,cz+3.30f,0.045f,1.55f,0.08f,0.18f,0.58f,0.72f);
    cube(v,cx+1.70f,1.55f,cz+3.30f,0.045f,1.55f,0.08f,0.18f,0.58f,0.72f);
    // Repeated classroom windows across four levels.
    for(int floor=0;floor<4;floor++) {
        float y=1.55f+floor*2.65f;
        for(int side=-1;side<=1;side+=2) for(int col=0;col<4;col++) {
            float x=cx+side*(5.0f+col*2.35f);
            cube(v,x,y,cz+3.62f,0.95f,0.82f,0.055f,0.16f,0.55f,0.69f);
            cube(v,x,y,cz+3.55f,0.08f,0.88f,0.06f,0.78f,0.78f,0.72f);
        }
    }
    // Side-facing windows add depth to the academic wings.
    for(int floor=0;floor<4;floor++) for(int side=-1;side<=1;side+=2) {
        cube(v,cx+side*12.99f,1.55f+floor*2.65f,cz-1.8f,
             0.05f,0.82f,1.05f,0.16f,0.55f,0.69f);
    }
    // Broad front steps and entrance doors.
    for(int i=0;i<5;i++)
        cube(v,cx,0.18f+i*0.12f,cz+5.0f+i*0.55f,4.5f,0.12f,0.58f,
             0.72f,0.69f,0.60f);
    cube(v,cx,1.45f,cz+3.90f,1.75f,1.45f,0.10f,0.18f,0.12f,0.09f);
    // Gold campus lettering directly on the front facade; no dark plaque.
    drawBlockText(v,cx,10.25f,cz+3.82f,"GL BAJAJ",0.085f);
    drawBlockText(v,cx,9.82f,cz+3.82f,"GROUP OF INSTITUTIONS",0.040f);
    // Boundary walls, gate pillars and a landscaped campus sign approach.
    // Campus boundary side walls removed around the arrival/parking area;
    // leave the approach open so the arriving bike and player are not blocked.
    // Front boundary wall removed completely so the bike can enter the campus freely.
    // Keep the two side boundary walls and entrance pillars.
    for(int side=-1;side<=1;side+=2) {
        cube(v,cx+side*3.0f,1.25f,cz+11.5f,0.55f,1.25f,0.55f,
             0.48f,0.32f,0.19f);
        sphere(v,cx+side*3.0f,2.05f,cz+11.5f,0.34f,0.34f,0.34f,
               0.78f,0.68f,0.48f);
    }
    // No duplicate floating entrance lettering; keep the frontage uncluttered.

    // Trees and flagpoles on the campus frontage.
    for(int i=0;i<4;i++) {
        float x=cx-13.0f+i*8.7f;
        cube(v,x,1.2f,cz+8.7f,0.16f,1.2f,0.16f,0.32f,0.20f,0.10f);
        sphere(v,x,2.45f,cz+8.7f,1.05f,1.15f,1.05f,0.08f,0.38f,0.12f);
    }
    cube(v,cx+15.0f,4.0f,cz+7.0f,0.07f,4.0f,0.07f,0.72f,0.72f,0.72f);
    cube(v,cx+15.45f,5.35f,cz+7.0f,0.9f,0.45f,0.035f,0.82f,0.12f,0.10f);
}

static void drawAirport(const float* v) {
    // Broad asphalt access road linking the mansion district to the airport.
    // Several overlapping segments form a gentle curve rather than a straight strip.
    const float roadX[6]={5.0f,12.0f,21.0f,30.0f,39.0f,47.0f};
    const float roadZ[6]={18.0f,34.0f,51.0f,68.0f,86.0f,103.0f};
    for(int i=0;i<5;i++) {
        float dx=roadX[i+1]-roadX[i], dz=roadZ[i+1]-roadZ[i];
        float len=sqrtf(dx*dx+dz*dz);
        float angle=atan2f(dx,dz);
        cubeYaw(v,(roadX[i]+roadX[i+1])*0.5f,-0.38f,
                (roadZ[i]+roadZ[i+1])*0.5f,4.2f,0.10f,len*0.56f,
                angle,0.20f,0.22f,0.24f);
        cubeYaw(v,(roadX[i]+roadX[i+1])*0.5f-1.9f*cosf(angle),-0.30f,
                (roadZ[i]+roadZ[i+1])*0.5f+1.9f*sinf(angle),
                0.06f,0.025f,len*0.53f,angle,0.90f,0.86f,0.68f);
        cubeYaw(v,(roadX[i]+roadX[i+1])*0.5f+1.9f*cosf(angle),-0.30f,
                (roadZ[i]+roadZ[i+1])*0.5f-1.9f*sinf(angle),
                0.06f,0.025f,len*0.53f,angle,0.90f,0.86f,0.68f);
    }
    // Airport apron and long runway, deliberately far from both houses.
    cube(v,47.0f,-0.42f,119.0f,42.0f,0.18f,78.0f,0.27f,0.29f,0.31f);
    cube(v,47.0f,-0.30f,119.0f,8.0f,0.05f,76.0f,0.12f,0.13f,0.14f);
    // Runway centerline dashes and edge markings.
    for(int i=0;i<18;i++) {
        float z=84.0f+i*4.0f;
        cube(v,47.0f,-0.25f,z,0.12f,0.025f,1.25f,0.92f,0.92f,0.86f);
    }
    cube(v,38.7f,-0.24f,119.0f,0.08f,0.025f,38.0f,0.95f,0.95f,0.88f);
    cube(v,55.3f,-0.24f,119.0f,0.08f,0.025f,38.0f,0.95f,0.95f,0.88f);
    // Taxiway leading to terminal.
    cube(v,63.0f,-0.32f,124.0f,14.0f,0.10f,5.0f,0.22f,0.24f,0.26f);
    // Modern terminal building and glass frontage.
    cube(v,73.0f,2.0f,124.0f,7.0f,2.0f,13.0f,0.72f,0.74f,0.76f);
    cube(v,65.8f,2.0f,124.0f,0.12f,1.35f,10.5f,0.12f,0.45f,0.62f);
    cube(v,73.0f,4.15f,124.0f,7.2f,0.18f,13.2f,0.20f,0.24f,0.29f);
    cube(v,73.0f,4.55f,124.0f,3.0f,0.16f,0.30f,0.12f,0.18f,0.22f);
    // Runway edge lights.
    for(int i=0;i<16;i++) for(int side=-1;side<=1;side+=2)
        sphere(v,47.0f+side*9.0f,0.05f,86.0f+i*4.4f,
               0.12f,0.12f,0.12f,0.95f,0.86f,0.45f);
    // Animated airport passengers walking around the terminal apron.
    for(int i=0;i<7;i++) {
        float phase=(float)SDL_GetTicks()*0.001f+i*1.7f;
        float x=68.0f+sinf(phase*0.55f+i)*2.6f;
        float z=116.0f+fmodf((phase*1.4f+i*5.0f),16.0f);
        sphere(v,x,1.55f,z,0.19f,0.22f,0.18f,0.82f,0.65f,0.48f);
        cube(v,x,1.02f,z,0.24f,0.42f,0.16f,
             0.18f+0.08f*(i%3),0.28f+0.06f*(i%2),0.62f);
        float leg=sinf(phase*5.0f)*0.16f;
        cubeYaw(v,x-0.08f,0.48f+leg,z,0.055f,0.28f,0.06f,leg,0.12f,0.12f,0.15f);
        cubeYaw(v,x+0.08f,0.48f-leg,z,0.055f,0.28f,0.06f,-leg,0.12f,0.12f,0.15f);
    }
    // Gate/boarding area markers.
    for(int i=0;i<4;i++) {
        cube(v,65.0f,0.75f,116.0f+i*5.0f,0.18f,0.75f,0.18f,0.25f,0.32f,0.38f);
        sphere(v,65.0f,1.55f,116.0f+i*5.0f,0.22f,0.22f,0.22f,0.12f,0.82f,0.32f);
    }
}
static void drawAirplane(const float* v) {
    float x=planeX,y=0.75f+planeAltitude,z=planeZ;
    float flap=0.10f*sinf(planeAnimTime*5.0f)*std::min(1.0f,planeSpeed/8.0f);
    float blink=(sinf(planeAnimTime*7.0f)>0.15f)?1.0f:0.12f;

    // Main fuselage, nose, tail and fixed wings.
    cubeYaw(v,x,y,z,0.52f,0.48f,3.9f,planeYaw,0.88f,0.90f,0.92f);
    sphereYaw(v,x,y,z-3.65f,0.48f,0.42f,0.65f,planeYaw,0.86f,0.88f,0.90f);
    cubeYaw(v,x,y+0.02f,z+3.35f,0.38f,0.34f,0.95f,planeYaw,0.82f,0.84f,0.87f);
    cubeYaw(v,x,y+0.02f,z+3.9f,1.25f,0.08f,0.52f,planeYaw,0.82f,0.84f,0.87f);
    cubeYaw(v,x,y,z,4.9f,0.12f,1.05f,planeYaw,0.78f,0.81f,0.84f);

    // Animated trailing-edge flaps and tail rudder (visible movement).
    cubeYaw(v,x-1.55f,y-0.015f,z+0.72f,0.78f,0.055f,0.42f,planeYaw+flap,0.64f,0.68f,0.72f);
    cubeYaw(v,x+1.55f,y-0.015f,z+0.72f,0.78f,0.055f,0.42f,planeYaw-flap,0.64f,0.68f,0.72f);
    cubeYaw(v,x,y+0.30f,z+3.62f,0.07f,0.38f,0.42f,planeYaw+0.06f*sinf(planeAnimTime*2.8f),0.72f,0.75f,0.78f);

    // Cockpit windows and cabin windows.
    cubeYaw(v,x,y+0.36f,z-2.35f,0.38f,0.18f,0.58f,planeYaw,0.12f,0.34f,0.48f);
    for(int i=0;i<8;i++) {
        float wz=z-1.35f+i*0.42f;
        sphereYaw(v,x-0.49f,y+0.12f,wz,0.045f,0.09f,0.08f,planeYaw,0.12f,0.42f,0.62f);
        sphereYaw(v,x+0.49f,y+0.12f,wz,0.045f,0.09f,0.08f,planeYaw,0.12f,0.42f,0.62f);
    }

    // Twin engines with animated intake glow and landing wheels.
    cubeYaw(v,x-1.45f,y-0.18f,z-0.15f,0.34f,0.30f,0.92f,planeYaw,0.28f,0.30f,0.33f);
    cubeYaw(v,x+1.45f,y-0.18f,z-0.15f,0.34f,0.30f,0.92f,planeYaw,0.28f,0.30f,0.33f);
    for(int side=-1;side<=1;side+=2) {
        sphereYaw(v,x+side*1.0f,y-0.42f,z-0.15f,0.16f,0.18f,0.16f,planeYaw,0.06f,0.06f,0.07f);
        sphereYaw(v,x+side*1.45f,y-0.18f,z-0.66f,0.16f,0.16f,0.08f,planeYaw,
                  0.95f,0.42f,0.08f);
    }

    // Navigation lights: red left, green right, blinking white tail beacon.
    sphereYaw(v,x-2.48f,y+0.04f,z,0.12f,0.09f,0.12f,planeYaw,blink,0.04f,0.03f);
    sphereYaw(v,x+2.48f,y+0.04f,z,0.12f,0.09f,0.12f,planeYaw,0.04f,blink,0.08f);
    sphereYaw(v,x,y+0.28f,z+4.18f,0.09f,0.08f,0.09f,planeYaw,blink,blink,blink);

    // Three seated passenger heads visible through the cabin windows.
    if(inAirplane) for(int i=0;i<planePassengers;i++) {
        float px=x+(i%2?0.18f:-0.18f), pz=z-0.55f+i*0.72f;
        sphereYaw(v,px,y+0.27f,pz,0.12f,0.13f,0.12f,planeYaw,0.80f,0.62f,0.46f);
    }
}

static void drawHorse(const float* v) {
    // The horse stays tied at its paddock until the player mounts it.
    float hx=ridingHorse?playerX:horseX;
    float hz=ridingHorse?playerZ:horseZ;
    float hy=ridingHorse?playerHeight:0.0f;
    float phase=horsePhase;
    float stride=ridingHorse?0.22f*sinf(phase*8.0f):0.025f*sinf(phase*2.0f);
    // Chestnut body, neck, head and muzzle.
    sphereYaw(v,hx,hy+1.22f,hz,0.62f,0.58f,1.02f,horseYaw,0.48f,0.20f,0.10f);
    cubeYaw(v,hx,hy+1.55f,hz-0.62f,0.40f,0.62f,0.40f,horseYaw,0.50f,0.22f,0.11f);
    sphereYaw(v,hx,hy+2.02f,hz-0.94f,0.34f,0.40f,0.36f,horseYaw,0.53f,0.24f,0.12f);
    sphereYaw(v,hx,hy+1.83f,hz-1.22f,0.27f,0.19f,0.25f,horseYaw,0.34f,0.13f,0.08f);
    // Ears, eyes, black mane and flowing tail.
    for(int side=-1;side<=1;side+=2) {
        sphereYaw(v,hx+side*0.20f,hy+2.39f,hz-0.91f,0.09f,0.25f,0.09f,horseYaw,0.28f,0.10f,0.06f);
        sphereYaw(v,hx+side*0.27f,hy+2.10f,hz-1.17f,0.045f,0.055f,0.045f,horseYaw,0.025f,0.02f,0.02f);
    }
    cubeYaw(v,hx,hy+2.02f,hz-0.55f,0.25f,0.22f,0.68f,horseYaw,0.12f,0.075f,0.055f);
    cubeYaw(v,hx,hy+1.48f,hz+1.05f,0.14f,0.16f,0.72f,horseYaw+0.12f*sinf(phase*2.0f),0.16f,0.075f,0.045f);
    // Saddle and bridle.
    cubeYaw(v,hx,hy+1.78f,hz+0.05f,0.48f,0.16f,0.64f,horseYaw,0.12f,0.055f,0.035f);
    cubeYaw(v,hx,hy+1.98f,hz-0.15f,0.52f,0.08f,0.46f,horseYaw,0.72f,0.12f,0.08f);
    // Four articulated legs with a gentle walking cycle.
    for(int i=0;i<4;i++) {
        float side=(i<2?-1.0f:1.0f);
        float fore=(i%2==0?-0.52f:0.52f);
        float sw=stride*sinf(phase*8.0f+i*1.57f);
        cubeYaw(v,hx+side*0.34f,hy+0.62f+sw,hz+fore,0.12f,0.62f,0.13f,
                horseYaw,0.28f,0.12f,0.065f);
        cubeYaw(v,hx+side*0.34f,hy+0.19f+sw,hz+fore,0.14f,0.18f,0.17f,
                horseYaw,0.10f,0.085f,0.07f);
    }
    if(ridingHorse) {
        sphereYaw(v,hx,hy+2.28f,hz+0.02f,0.24f,0.27f,0.22f,horseYaw,0.91f,0.70f,0.56f);
        sphereYaw(v,hx,hy+2.49f,hz+0.02f,0.25f,0.12f,0.23f,horseYaw,0.055f,0.035f,0.028f);
        cubeYaw(v,hx,hy+1.98f,hz+0.03f,0.36f,0.40f,0.22f,horseYaw,0.045f,0.16f,0.32f);
        cubeYaw(v,hx-0.27f,hy+1.90f,hz-0.20f,0.09f,0.34f,0.10f,horseYaw,0.91f,0.70f,0.56f);
        cubeYaw(v,hx+0.27f,hy+1.90f,hz-0.20f,0.09f,0.34f,0.10f,horseYaw,0.91f,0.70f,0.56f);
    }
    if(!ridingHorse) {
        // Tether post and visible rope beside the horse.
        cube(v,hx+1.05f,0.85f,hz+0.25f,0.12f,0.85f,0.12f,0.30f,0.20f,0.12f);
        cubeYaw(v,hx+0.50f,1.48f,hz+0.10f,0.56f,0.035f,0.035f,0.0f,0.72f,0.60f,0.30f);
    }
}

static void drawFemaleCompanion(const float* v) {
    if(drivingCar || inAirplane || ridingHorse) return;
    float bob=0.035f*sinf(companionPhase*2.0f);
    float swing=0.30f*sinf(companionPhase*7.0f)*walkBlend;
    float x=companionX,z=companionZ,y=0.0f;
    if(sleeping) {
        // Companion lies on her side beside the player, on the same mattress.
        cubeYaw(v,x,y+1.12f,z,0.24f,0.15f,0.72f,yaw,0.88f,0.16f,0.40f);
        sphere(v,x,y+1.17f,z-0.82f,0.21f,0.21f,0.21f,0.91f,0.70f,0.55f);
        sphere(v,x,y+1.33f,z-0.89f,0.23f,0.10f,0.23f,0.10f,0.055f,0.035f);
        cubeYaw(v,x-0.29f,y+1.10f,z-0.08f,0.08f,0.09f,0.48f,yaw,0.88f,0.16f,0.40f);
        cubeYaw(v,x+0.29f,y+1.10f,z-0.08f,0.08f,0.09f,0.48f,yaw,0.88f,0.16f,0.40f);
        cubeYaw(v,x-0.11f,y+1.08f,z+0.72f,0.10f,0.10f,0.55f,yaw,0.88f,0.16f,0.40f);
        cubeYaw(v,x+0.11f,y+1.08f,z+0.72f,0.10f,0.10f,0.55f,yaw,0.88f,0.16f,0.40f);
        return;
    }
    // A friendly, stylized young woman companion with long hair and a bright outfit.
    sphere(v,x,y+1.58f+bob,z,0.25f,0.29f,0.23f,0.91f,0.70f,0.55f);
    cubeYaw(v,x,y+1.08f+bob,z,0.38f,0.48f,0.25f,0.0f,0.88f,0.16f,0.42f);
    sphere(v,x,y+0.70f+bob,z+0.02f,0.30f,0.22f,0.28f,0.88f,0.16f,0.40f);
    // Hair cap and long side locks.
    sphere(v,x,y+1.78f+bob,z+0.015f,0.28f,0.20f,0.26f,0.10f,0.055f,0.035f);
    cube(v,x-0.23f,y+1.48f+bob,z,0.09f,0.42f,0.13f,0.10f,0.05f,0.03f);
    cube(v,x+0.23f,y+1.48f+bob,z,0.09f,0.42f,0.13f,0.10f,0.05f,0.03f);
    // Face details.
    sphere(v,x-0.095f,y+1.62f+bob,z-0.205f,0.035f,0.045f,0.025f,0.035f,0.025f,0.02f);
    sphere(v,x+0.095f,y+1.62f+bob,z-0.205f,0.035f,0.045f,0.025f,0.035f,0.025f,0.02f);
    sphere(v,x,y+1.52f+bob,z-0.23f,0.035f,0.045f,0.025f,0.92f,0.42f,0.38f);
    // When she returns with food, show a tray and a warm meal in her hands.
    if(foodReady) {
        cubeYaw(v,x,y+1.08f+bob,z-0.38f,0.34f,0.035f,0.25f,0.0f,0.34f,0.19f,0.09f);
        sphere(v,x-0.10f,y+1.17f+bob,z-0.38f,0.09f,0.07f,0.09f,0.92f,0.62f,0.18f);
        sphere(v,x+0.10f,y+1.17f+bob,z-0.38f,0.09f,0.07f,0.09f,0.92f,0.62f,0.18f);
    }
    // Arms and legs animate as she follows the player.
    cubeYaw(v,x-0.28f,y+1.08f+bob,z,0.09f,0.38f,0.10f,swing,0.91f,0.70f,0.55f);
    cubeYaw(v,x+0.28f,y+1.08f+bob,z,0.09f,0.38f,0.10f,-swing,0.91f,0.70f,0.55f);
    cubeYaw(v,x-0.12f,y+0.30f,z,0.11f,0.32f,0.12f,-swing,0.16f,0.12f,0.20f);
    cubeYaw(v,x+0.12f,y+0.30f,z,0.11f,0.32f,0.12f,swing,0.16f,0.12f,0.20f);
}

static void drawPlayerAvatar(const float* v) {
    if(drivingCar || inAirplane || ridingHorse || onAyushBike) return;
    float y=playerHeight + (rooftopLevel>0 ? 0.08f : 0.0f);
    if(sleeping) {
        cubeYaw(v,playerX,y+1.13f,playerZ,0.27f,0.16f,0.78f,yaw,0.045f,0.16f,0.32f);
        sphere(v,playerX,y+1.18f,playerZ-0.88f,0.22f,0.22f,0.22f,0.91f,0.70f,0.56f);
        sphere(v,playerX,y+1.34f,playerZ-0.94f,0.23f,0.09f,0.23f,0.055f,0.035f,0.028f);
        cubeYaw(v,playerX-0.34f,y+1.12f,playerZ-0.12f,0.09f,0.10f,0.50f,yaw,0.10f,0.25f,0.76f);
        cubeYaw(v,playerX+0.34f,y+1.12f,playerZ-0.12f,0.09f,0.10f,0.50f,yaw,0.10f,0.25f,0.76f);
        cubeYaw(v,playerX-0.13f,y+1.10f,playerZ+0.78f,0.11f,0.11f,0.62f,yaw,0.035f,0.045f,0.075f);
        cubeYaw(v,playerX+0.13f,y+1.10f,playerZ+0.78f,0.11f,0.11f,0.62f,yaw,0.035f,0.045f,0.075f);
        return;
    }
    if(sittingInGardenChair) {
        // Relaxed seated pose, aligned over the garden armchair seat.
        sphere(v,playerX,y+1.48f,playerZ,0.28f,0.31f,0.26f,0.91f,0.70f,0.56f);
        sphere(v,playerX,y+1.73f,playerZ+0.015f,0.29f,0.14f,0.27f,0.055f,0.035f,0.028f);
        cubeYaw(v,playerX,y+1.03f,playerZ,0.38f,0.43f,0.24f,yaw,0.045f,0.16f,0.32f);
        cubeYaw(v,playerX-0.43f,y+1.04f,playerZ,0.105f,0.34f,0.12f,yaw,0.10f,0.25f,0.76f);
        cubeYaw(v,playerX+0.43f,y+1.04f,playerZ,0.105f,0.34f,0.12f,yaw,0.10f,0.25f,0.76f);
        cubeYaw(v,playerX-0.17f,y+0.53f,playerZ-0.35f,0.13f,0.12f,0.43f,yaw,0.035f,0.045f,0.075f);
        cubeYaw(v,playerX+0.17f,y+0.53f,playerZ-0.35f,0.13f,0.12f,0.43f,yaw,0.035f,0.045f,0.075f);
        return;
    }
    float swing=sinf(walkPhase)*0.62f*walkBlend;
    float armSwing=-swing*0.82f;
    // Handsome hero-style face, styled dark hair, eyes and fitted jacket.
    sphere(v,playerX,y+1.62f,playerZ,0.28f,0.33f,0.26f,0.91f,0.70f,0.56f);
    sphere(v,playerX,y+1.88f,playerZ+0.015f,0.29f,0.15f,0.27f,0.055f,0.035f,0.028f);
    cubeYaw(v,playerX,y+1.91f,playerZ-0.10f,0.32f,0.12f,0.22f,yaw,0.045f,0.03f,0.025f);
    sphere(v,playerX-0.095f,y+1.66f,playerZ-0.235f,0.035f,0.045f,0.025f,0.025f,0.025f,0.03f);
    sphere(v,playerX+0.095f,y+1.66f,playerZ-0.235f,0.035f,0.045f,0.025f,0.025f,0.025f,0.03f);
    sphere(v,playerX,y+1.53f,playerZ-0.255f,0.045f,0.035f,0.025f,0.72f,0.34f,0.30f);
    cubeYaw(v,playerX,y+1.08f,playerZ,0.38f,0.49f,0.24f,yaw,
            0.045f,0.16f,0.32f);
    cubeYaw(v,playerX-0.43f,y+1.08f,playerZ,0.105f,0.39f,0.12f,
            yaw+armSwing,0.10f,0.25f,0.76f);
    cubeYaw(v,playerX+0.43f,y+1.08f,playerZ,0.105f,0.39f,0.12f,
            yaw-armSwing,0.10f,0.25f,0.76f);
    cubeYaw(v,playerX-0.14f,y+0.43f,playerZ,0.13f,0.41f,0.14f,
            yaw+swing,0.12f,0.12f,0.17f);
    cubeYaw(v,playerX+0.14f,y+0.43f,playerZ,0.13f,0.41f,0.14f,
            yaw-swing,0.12f,0.12f,0.17f);

    // Compact rifle held with BOTH hands, aimed along the player's facing direction.
    float fx=-sinf(yaw), fz=-cosf(yaw);
    float rx=cosf(yaw), rz=-sinf(yaw);
    float gunX=playerX+fx*0.72f, gunZ=playerZ+fz*0.72f;
    cubeYaw(v,gunX,y+1.30f,gunZ,0.105f,0.105f,0.78f,
            yaw,0.12f,0.13f,0.15f); // rifle body
    cubeYaw(v,gunX+fx*0.78f,y+1.30f,gunZ+fz*0.78f,0.045f,0.045f,0.42f,
            yaw,0.24f,0.25f,0.26f); // barrel
    cubeYaw(v,gunX-fx*0.30f,y+1.12f,gunZ-fz*0.30f,0.075f,0.16f,0.12f,
            yaw,0.20f,0.12f,0.07f); // grip
    sphere(v,playerX-rx*0.30f+fx*0.48f,y+1.25f,
           playerZ-rz*0.30f+fz*0.48f,
           0.13f,0.13f,0.13f,0.88f,0.66f,0.48f); // left hand
    sphere(v,playerX+rx*0.30f+fx*0.88f,y+1.25f,
           playerZ+rz*0.30f+fz*0.88f,
           0.13f,0.13f,0.13f,0.88f,0.66f,0.48f); // right hand
    if(parachuting) {
        float canopyY=y+5.1f;
        sphere(v,playerX,canopyY,playerZ,2.05f,0.48f,1.05f,0.92f,0.12f,0.12f);
        sphere(v,playerX-0.95f,canopyY-0.10f,playerZ,0.82f,0.34f,0.92f,0.98f,0.72f,0.12f);
        sphere(v,playerX+0.95f,canopyY-0.10f,playerZ,0.82f,0.34f,0.92f,0.10f,0.42f,0.92f);
        cubeYaw(v,playerX-0.72f,y+3.05f,playerZ,0.025f,2.0f,0.025f,-0.18f,0.92f,0.92f,0.90f);
        cubeYaw(v,playerX+0.72f,y+3.05f,playerZ,0.025f,2.0f,0.025f,0.18f,0.92f,0.92f,0.90f);
    }
}
static void drawAnimals(const float* v) {
    for(const Animal& a:animals) {
        float bob=0.05f*sinf(a.phase*5.0f);
        float bodyR=a.kind==0?0.48f:0.38f;
        // Grazing sheep / goat-like animals with four animated legs.
        sphere(v,a.x,0.78f+bob,a.z,bodyR,0.36f,0.68f,
               a.kind==0?0.88f:0.68f,a.kind==0?0.86f:0.42f,a.kind==0?0.80f:0.22f);
        sphere(v,a.x,1.12f+bob,a.z-0.48f,0.25f,0.25f,0.25f,
               a.kind==0?0.90f:0.72f,a.kind==0?0.86f:0.48f,a.kind==0?0.82f:0.26f);
        for(int leg=0;leg<4;leg++) {
            float lx=a.x+(leg<2?-0.25f:0.25f);
            float lz=a.z+(leg%2?0.34f:-0.34f);
            float swing=sinf(a.phase*7.0f+leg*1.7f)*0.10f;
            cubeYaw(v,lx,0.38f+swing,lz,0.075f,0.34f,0.075f,
                    a.dir,0.20f,0.16f,0.12f);
        }
        // Small muzzle and ears.
        sphere(v,a.x,1.03f+bob,a.z-0.70f,0.14f,0.11f,0.12f,0.35f,0.22f,0.16f);
        sphere(v,a.x-0.18f,1.28f+bob,a.z-0.52f,0.08f,0.16f,0.07f,0.65f,0.45f,0.35f);
        sphere(v,a.x+0.18f,1.28f+bob,a.z-0.52f,0.08f,0.16f,0.07f,0.65f,0.45f,0.35f);
    }
}
static void drawThrownFruits(const float* v) {
    for(const ThrownFruit& f:thrownFruits)
        sphere(v,f.x,0.35f,f.z,0.18f,0.18f,0.18f,0.95f,0.20f,0.06f);
}
static void drawSunAndSky(const float* v) {
    // Bright sun visible above the far garden.
    sphere(v,0,16,-35,2.0f,2.0f,2.0f,1.0f,0.86f,0.22f);
}
static void drawGhost(const float* v,float gx,float gz) {
    sphere(v,gx,1.28f,gz,0.48f,0.72f,0.32f,0.82f,0.84f,0.82f);
    sphere(v,gx,2.20f,gz,0.39f,0.43f,0.36f,0.90f,0.90f,0.85f);
    sphere(v,gx,2.55f,gz,0.41f,0.16f,0.38f,0.06f,0.05f,0.05f);
    sphere(v,gx-0.52f,1.48f,gz,0.15f,0.55f,0.15f,0.82f,0.84f,0.82f);
    sphere(v,gx+0.52f,1.48f,gz,0.15f,0.55f,0.15f,0.82f,0.84f,0.82f);
    sphere(v,gx-0.14f,2.25f,gz-0.32f,0.065f,0.085f,0.045f,0.85f,0.02f,0.02f);
    sphere(v,gx+0.14f,2.25f,gz-0.32f,0.065f,0.085f,0.045f,0.85f,0.02f,0.02f);
}
static void drawNightPhone(const float* v) {
    // A small outdoor phone stand near the front of YASH SHUKLA HOUSE.
    cube(v,-3.0f,0.72f,12.2f,0.34f,0.72f,0.25f,0.18f,0.19f,0.21f);
    cube(v,-3.0f,1.48f,12.2f,0.42f,0.40f,0.10f,0.07f,0.08f,0.10f);
    cube(v,-3.0f,1.49f,12.08f,0.31f,0.28f,0.025f,
         nightMode?0.10f:0.05f,nightMode?0.72f:0.28f,nightMode?0.95f:0.48f);
    drawBlockText(v,-3.0f,1.53f,12.045f,nightMode?"NIGHT":"64648",0.025f);
    // Screen glow and a physical call button.
    sphere(v,-3.0f,1.15f,12.02f,0.10f,0.07f,0.045f,
           nightMode?0.12f:0.16f,nightMode?0.95f:0.82f,0.35f);
    cube(v,-3.0f,0.10f,12.2f,0.72f,0.10f,0.48f,0.22f,0.22f,0.23f);
}

static void drawItems(const float* v) {
    for(const Item& it:items) if(it.active) {
        // Bright floating pickup marker above each object.
        cube(v,it.x,0.45f,it.z,0.22f,0.22f,0.22f,it.r,it.g,it.b);
        sphere(v,it.x,0.82f,it.z,0.10f,0.10f,0.10f,1.0f,0.95f,0.55f);
    }
}
static void drawHUDCircle(float x,float y,float radius,float r,float g,float b) {
    std::vector<float> verts;
    verts.push_back(x); verts.push_back(y); verts.push_back(0);
    for(int i=0;i<=32;i++) {
        float a=2*PI*i/32;
        verts.push_back(x+cosf(a)*radius);
        verts.push_back(y+sinf(a)*radius);
        verts.push_back(0);
    }
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    glUniformMatrix4fv(gMVP,1,GL_FALSE,identity);
    glUniform4f(gColor,r,g,b,0.82f);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glVertexAttribPointer(gPosition,3,GL_FLOAT,GL_FALSE,0,verts.data());
    glDrawArrays(GL_TRIANGLE_FAN,0,34);
}
static void drawHUDQuad(float cx,float cy,float hx,float hy,float r,float g,float b,float alpha) {
    const float verts[18]={
        cx-hx,cy-hy,0, cx+hx,cy-hy,0, cx+hx,cy+hy,0,
        cx-hx,cy-hy,0, cx+hx,cy+hy,0, cx-hx,cy+hy,0
    };
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    glUniformMatrix4fv(gMVP,1,GL_FALSE,identity);
    glUniform4f(gColor,r,g,b,alpha);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glVertexAttribPointer(gPosition,3,GL_FLOAT,GL_FALSE,0,verts);
    glDrawArrays(GL_TRIANGLES,0,6);
}
static void drawHUDLine(float x1,float y1,float x2,float y2,float r,float g,float b,float width=3.0f) {
    const float verts[6]={x1,y1,0,x2,y2,0};
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    glUniformMatrix4fv(gMVP,1,GL_FALSE,identity);
    glUniform4f(gColor,r,g,b,1.0f);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glVertexAttribPointer(gPosition,3,GL_FLOAT,GL_FALSE,0,verts);
    glLineWidth(width);
    glDrawArrays(GL_LINES,0,2);
}
static void drawHUDRing(float cx,float cy,float radius,float r,float g,float b,float width=5.0f) {
    std::vector<float> verts;
    for(int i=0;i<49;i++) {
        float a=2.0f*PI*(float)i/48.0f;
        verts.push_back(cx+cosf(a)*radius);
        verts.push_back(cy+sinf(a)*radius);
        verts.push_back(0.0f);
    }
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    glUniformMatrix4fv(gMVP,1,GL_FALSE,identity);
    glUniform4f(gColor,r,g,b,1.0f);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glVertexAttribPointer(gPosition,3,GL_FLOAT,GL_FALSE,0,verts.data());
    glLineWidth(width);
    glDrawArrays(GL_LINE_STRIP,0,49);
}
static void drawCarCockpitHUD() {
    // Compact dashboard overlay visible only while seated in the car.
    drawHUDQuad(0.0f,-0.48f,0.57f,0.22f,0.035f,0.045f,0.055f,0.88f);

    // Animated steering wheel; its spokes turn with left/right input.
    float cx=0.0f, cy=-0.48f, radius=0.145f;
    drawHUDRing(cx,cy,radius,0.10f,0.11f,0.12f,9.0f);
    float a=steeringWheelAngle;
    for(int k=0;k<3;k++) {
        float t=a+k*2.0943951f;
        drawHUDLine(cx,cy,cx+cosf(t)*radius*0.92f,cy+sinf(t)*radius*0.92f,
                    0.72f,0.74f,0.76f,4.0f);
    }
    // Hands follow the turning wheel.
    drawHUDLine(cx-0.17f,cy+0.16f,cx+cosf(a+2.45f)*radius*0.85f,
                cy+sinf(a+2.45f)*radius*0.85f,0.80f,0.57f,0.40f,5.0f);
    drawHUDLine(cx+0.17f,cy+0.16f,cx+cosf(a+0.70f)*radius*0.85f,
                cy+sinf(a+0.70f)*radius*0.85f,0.80f,0.57f,0.40f,5.0f);

    // Speedometer dial and moving needle.
    float sx=0.39f, sy=-0.43f;
    drawHUDRing(sx,sy,0.13f,0.72f,0.78f,0.82f,4.0f);
    float speedNorm=std::min(1.0f,carCurrentSpeed/180.0f);
    float needle=-2.35f+speedNorm*4.70f;
    drawHUDLine(sx,sy,sx+cosf(needle)*0.105f,sy+sinf(needle)*0.105f,
                1.0f,0.22f,0.12f,3.0f);
    drawHUDCircle(sx,sy,0.018f,0.92f,0.92f,0.92f);

    // Gear selector display: R / N / D shown as illuminated gear state.
    float gx=-0.39f, gy=-0.43f;
    drawHUDQuad(gx,gy,0.095f,0.095f,0.10f,0.13f,0.16f,1.0f);
    // Seven-segment style digit: D when moving forward, R in reverse, N when stopped.
    char gear=(smoothY>0.08f?'D':(smoothY<-0.08f?'R':'N'));
    // Clear letter-shaped gear display.
    if(gear=='D') {
        drawHUDLine(gx-0.035f,gy+0.05f,gx-0.035f,gy-0.05f,0.25f,0.95f,0.55f,4.0f);
        drawHUDLine(gx-0.035f,gy+0.05f,gx+0.015f,gy+0.05f,0.25f,0.95f,0.55f,4.0f);
        drawHUDLine(gx+0.015f,gy+0.05f,gx+0.04f,gy+0.025f,0.25f,0.95f,0.55f,4.0f);
        drawHUDLine(gx+0.04f,gy+0.025f,gx+0.04f,gy-0.025f,0.25f,0.95f,0.55f,4.0f);
        drawHUDLine(gx+0.04f,gy-0.025f,gx+0.015f,gy-0.05f,0.25f,0.95f,0.55f,4.0f);
        drawHUDLine(gx+0.015f,gy-0.05f,gx-0.035f,gy-0.05f,0.25f,0.95f,0.55f,4.0f);
    } else if(gear=='R') {
        drawHUDLine(gx-0.035f,gy+0.05f,gx-0.035f,gy-0.05f,1.0f,0.35f,0.25f,4.0f);
        drawHUDLine(gx-0.035f,gy+0.05f,gx+0.025f,gy+0.05f,1.0f,0.35f,0.25f,4.0f);
        drawHUDLine(gx+0.025f,gy+0.05f,gx+0.04f,gy+0.025f,1.0f,0.35f,0.25f,4.0f);
        drawHUDLine(gx+0.04f,gy+0.025f,gx+0.025f,gy,1.0f,0.35f,0.25f,4.0f);
        drawHUDLine(gx-0.035f,gy,gx+0.04f,gy-0.05f,1.0f,0.35f,0.25f,4.0f);
    } else {
        drawHUDLine(gx-0.04f,gy+0.05f,gx-0.04f,gy-0.05f,0.65f,0.82f,1.0f,4.0f);
        drawHUDLine(gx+0.04f,gy+0.05f,gx+0.04f,gy-0.05f,0.65f,0.82f,1.0f,4.0f);
        drawHUDLine(gx-0.04f,gy+0.05f,gx+0.04f,gy-0.05f,0.65f,0.82f,1.0f,4.0f);
    }

    // Three tiny digital speed digits beside the dial.
    auto digit=[&](int n,float x,float y) {
        static const int seg[10][7]={
            {1,1,1,1,1,1,0},{0,1,1,0,0,0,0},{1,1,0,1,1,0,1},
            {1,1,1,1,0,0,1},{0,1,1,0,0,1,1},{1,0,1,1,0,1,1},
            {1,0,1,1,1,1,1},{1,1,1,0,0,0,0},{1,1,1,1,1,1,1},
            {1,1,1,1,0,1,1}};
        float w=0.018f,h=0.025f;
        if(seg[n][0]) drawHUDLine(x-w,y+h,x+w,y+h,0.25f,0.95f,0.62f,2.0f);
        if(seg[n][1]) drawHUDLine(x+w,y+h,x+w,y,0.25f,0.95f,0.62f,2.0f);
        if(seg[n][2]) drawHUDLine(x+w,y,x+w,y-h,0.25f,0.95f,0.62f,2.0f);
        if(seg[n][3]) drawHUDLine(x-w,y-h,x+w,y-h,0.25f,0.95f,0.62f,2.0f);
        if(seg[n][4]) drawHUDLine(x-w,y,x-w,y-h,0.25f,0.95f,0.62f,2.0f);
        if(seg[n][5]) drawHUDLine(x-w,y+h,x-w,y,0.25f,0.95f,0.62f,2.0f);
        if(seg[n][6]) drawHUDLine(x-w,y,x+w,y,0.25f,0.95f,0.62f,2.0f);
    };
    int speedValue=(int)std::max(0.0f,std::min(199.0f,carCurrentSpeed));
    digit((speedValue/100)%10,0.32f,-0.23f);
    digit((speedValue/10)%10,0.37f,-0.23f);
    digit(speedValue%10,0.42f,-0.23f);

    // Fuel / engine status bar.
    drawHUDQuad(0.0f,-0.665f,0.22f,0.018f,0.16f,0.18f,0.20f,1.0f);
    drawHUDQuad(-0.22f+0.22f*std::min(1.0f,carCurrentSpeed/180.0f),-0.665f,
                0.22f*std::min(1.0f,carCurrentSpeed/180.0f),0.012f,
                0.20f,0.85f,0.42f,1.0f);
}

static void drawHUD() {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    drawHUDCircle(-0.72f,-0.65f,0.22f,0.18f,0.20f,0.22f);
    drawHUDCircle(-0.72f+joystickX*0.12f,-0.65f+joystickY*0.12f,0.09f,0.78f,0.78f,0.80f);
    drawHUDCircle(0.72f,-0.65f,0.17f,0.85f,0.08f,0.06f);
    drawHUDCircle(0.88f,0.18f,0.075f,0.95f,0.62f,0.16f); // slap
    drawHUDCircle(0.88f,-0.20f,0.075f,0.92f,0.28f,0.12f); // kick
    // Blue bottom-center action button: enter/exit car, board/start aircraft, or use nearby objects.
    drawHUDCircle(0.34f,-0.72f,0.105f,carHeadlightsOn?1.0f:0.35f,carHeadlightsOn?0.88f:0.42f,0.12f);
    float bedDx=playerX+6.0f, bedDz=playerZ+5.0f;
    bool nearBed=!drivingCar && !inAirplane && !ridingHorse &&
                 rooftopLevel==0 && !onUpperFloor && bedDx*bedDx+bedDz*bedDz<10.0f;
    drawHUDCircle(0.0f,-0.72f,0.14f,(drivingCar||inAirplane)?0.12f:0.08f,
                  (drivingCar||inAirplane)?0.78f:0.48f,(drivingCar||inAirplane)?0.30f:0.95f);
    drawHUDCircle(0,0,0.012f,1,1,1);
    // NPC COMMAND touch button: tap the upper-right panel on Android to type an order.
    // SDL_StartTextInput opens the Android soft keyboard when the SDL Android backend supports it.
    drawHUDQuad(0.73f,0.66f,0.245f,0.105f,
                npcCommandTyping?0.08f:0.025f,
                npcCommandTyping?0.38f:0.20f,
                npcCommandTyping?0.22f:0.12f,0.98f);
    drawHUDQuad(0.73f,0.66f,0.205f,0.012f,0.25f,0.95f,0.62f,1.0f);
    // Health bar at the top centre.
    drawHUDQuad(0.0f,0.91f,0.25f,0.035f,0.05f,0.05f,0.06f,0.92f);
    float hpRatio=std::max(0.0f,std::min(1.0f,health/100.0f));
    drawHUDQuad(-0.25f+0.25f*hpRatio,0.91f,0.25f*hpRatio,0.022f,
                hpRatio>0.55f?0.10f:0.95f,hpRatio>0.55f?0.85f:0.12f,0.12f,1.0f);
    if(!ak47Owned && !drivingCar && !inAirplane) {
        float gx=playerX-akPickupX,gz=playerZ-akPickupZ;
        if(gx*gx+gz*gz<20.0f) {
            drawHUDQuad(0.48f,-0.72f,0.18f,0.075f,0.10f,0.42f,0.20f,0.96f);
            drawHUDQuad(0.48f,-0.72f,0.12f,0.012f,0.55f,1.0f,0.62f,1.0f);
        }
    }
    if(ak47Owned) {
        drawHUDQuad(0.72f,-0.42f,0.19f,0.075f,0.025f,0.055f,0.06f,0.92f);
        drawHUDQuad(0.72f,-0.42f,0.17f,0.012f,0.18f,0.72f,0.38f,1.0f);
        drawHUDCircle(0.66f,0.18f,0.075f,0.12f,0.42f,0.78f);
        drawHUDCircle(0.86f,-0.48f,0.105f,0.88f,0.10f,0.07f);
        drawHUDRing(0.66f,0.18f,0.044f,0.80f,0.90f,1.0f,3.0f);
        drawHUDLine(0.62f,0.18f,0.70f,0.18f,0.90f,0.95f,1.0f,2.0f);
        drawHUDLine(0.66f,0.14f,0.66f,0.22f,0.90f,0.95f,1.0f,2.0f);
    }
    if(inRocket) {
        drawHUDQuad(0.0f,0.72f,0.34f,0.055f,0.015f,0.025f,0.09f,0.90f);
        float climb=std::max(0.0f,std::min(1.0f,rocketAltitude/900.0f));
        drawHUDQuad(-0.28f+0.28f*climb,0.72f,0.28f*climb,0.012f,
                    0.20f,0.85f,1.0f,1.0f);
        // Throttle meter and ignition/flight state indicator.
        drawHUDQuad(0.72f,0.62f,0.22f,0.035f,0.025f,0.035f,0.055f,0.90f);
        drawHUDQuad(0.61f+0.11f*rocketThrottle,0.62f,0.11f*rocketThrottle,0.014f,
                    1.0f,0.48f,0.08f,0.98f);
        drawHUDCircle(-0.78f,0.72f,0.018f,
                      rocketLaunched?0.10f:((rocketIgnitionArmed)?1.0f:0.35f),
                      rocketLaunched?0.95f:0.48f,0.20f);
    }
    if(scopeMode) {
        drawHUDRing(0.0f,0.0f,0.36f,0.12f,0.95f,0.32f,3.0f);
        drawHUDLine(-0.42f,0.0f,0.42f,0.0f,0.12f,0.95f,0.32f,2.0f);
        drawHUDLine(0.0f,-0.42f,0.0f,0.42f,0.12f,0.95f,0.32f,2.0f);
    }
    // Persistent bank balance panel in the top-right HUD corner.
    drawHUDQuad(0.73f,0.88f,0.245f,0.095f,0.025f,0.075f,0.105f,0.94f);
    drawHUDQuad(0.73f,0.945f,0.245f,0.030f,0.04f,0.34f,0.40f,0.98f);
    auto hudDigit=[&](int n,float x,float y) {
        static const int seg[10][7]={
            {1,1,1,1,1,1,0},{0,1,1,0,0,0,0},{1,1,0,1,1,0,1},
            {1,1,1,1,0,0,1},{0,1,1,0,0,1,1},{1,0,1,1,0,1,1},
            {1,0,1,1,1,1,1},{1,1,1,0,0,0,0},{1,1,1,1,1,1,1},
            {1,1,1,1,0,1,1}};
        float w=0.012f,h=0.014f;
        if(seg[n][0]) drawHUDLine(x-w,y+h,x+w,y+h,0.30f,0.96f,0.68f,2.0f);
        if(seg[n][1]) drawHUDLine(x+w,y+h,x+w,y,0.30f,0.96f,0.68f,2.0f);
        if(seg[n][2]) drawHUDLine(x+w,y,x+w,y-h,0.30f,0.96f,0.68f,2.0f);
        if(seg[n][3]) drawHUDLine(x-w,y-h,x+w,y-h,0.30f,0.96f,0.68f,2.0f);
        if(seg[n][4]) drawHUDLine(x-w,y,x-w,y-h,0.30f,0.96f,0.68f,2.0f);
        if(seg[n][5]) drawHUDLine(x-w,y+h,x-w,y,0.30f,0.96f,0.68f,2.0f);
        if(seg[n][6]) drawHUDLine(x-w,y,x+w,y,0.30f,0.96f,0.68f,2.0f);
    };
    int shownBalance=std::max(0,cashBalance), digits[6];
    for(int i=5;i>=0;i--){digits[i]=shownBalance%10;shownBalance/=10;}
    drawHUDCircle(0.535f,0.875f,0.012f,0.25f,0.92f,0.62f);
    for(int i=0;i<6;i++) hudDigit(digits[i],0.575f+i*0.055f,0.875f);
    if(supermarketCheckoutOpen) {
        drawHUDQuad(0.0f,0.48f,0.52f,0.36f,0.015f,0.055f,0.085f,0.95f);
        drawHUDQuad(0.0f,0.79f,0.52f,0.045f,0.04f,0.38f,0.48f,1.0f);
        for(int i=0;i<8;i++) {
            float x=(i%2==0?-0.25f:0.25f), y=0.62f-(i/2)*0.15f;
            drawHUDQuad(x,y,0.22f,0.055f,
                i==supermarketSelected?0.12f:0.08f,
                i==supermarketSelected?0.48f:0.16f,
                i==supermarketSelected?0.52f:0.22f,0.98f);
        }
    }
    // Minimal fist/boot glyphs inside the combat buttons.
    drawHUDLine(0.845f,0.18f,0.915f,0.18f,0.20f,0.10f,0.06f,5.0f);
    drawHUDLine(0.86f,0.145f,0.86f,0.215f,0.20f,0.10f,0.06f,4.0f);
    drawHUDLine(0.85f,-0.20f,0.92f,-0.20f,0.15f,0.06f,0.04f,6.0f);
    drawHUDLine(0.90f,-0.20f,0.93f,-0.16f,0.15f,0.06f,0.04f,4.0f);
    if(collegeBookOpen) {
        drawHUDQuad(0.0f,0.43f,0.48f,0.28f,0.02f,0.035f,0.055f,0.96f);
        drawHUDQuad(0.0f,0.66f,0.48f,0.045f,0.12f,0.34f,0.48f,1.0f);
        // Page lines: visible open-book content panel.
        for(int i=0;i<6;i++) {
            float yy=0.56f-i*0.075f;
            drawHUDLine(-0.36f,yy,0.34f,yy,0.86f,0.88f,0.82f,2.0f);
        }
    }
    if(drivingCar) drawCarCockpitHUD();
    if(gameOver) {
        drawHUDQuad(0.0f,0.0f,0.72f,0.42f,0.02f,0.015f,0.02f,0.96f);
        drawHUDQuad(0.0f,0.20f,0.52f,0.055f,0.62f,0.025f,0.025f,1.0f);
        drawHUDQuad(0.0f,-0.08f,0.38f,0.025f,0.85f,0.85f,0.85f,1.0f);
        drawHUDQuad(0.0f,-0.27f,0.20f,0.045f,0.20f,0.48f,0.22f,1.0f);
    }
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

static void engineAudioCallback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    Sint16* out = reinterpret_cast<Sint16*>(stream);
    int count = len / (int)sizeof(Sint16);
    for(int i=0;i<count;i++) {
        float sample=0.0f;
        if(gunSoundSamples>0) {
            gunSoundPhase+=6.28318530718f*(110.0f+fmodf((float)gunSoundSamples,1300.0f))/22050.0f;
            sample+=sinf(gunSoundPhase)*0.34f*(float)gunSoundSamples/2205.0f;
            --gunSoundSamples;
        }
        if(gEngineAudioOn) {
            // Procedural sporty inline-six / turbo-inspired engine sound.
            // RPM rises with accelerator input; it is synthesized, not a recording.
            const float rawThrottle = (float)gEngineThrottle;
            float throttle=std::max(0.0f,std::min(1.0f,rawThrottle));
            float startBoost=gEngineStartSamples>0?1.0f:0.0f;
            // Lower-pitched, bass-heavy Supra-inspired exhaust note.
            float rpmTone=58.0f + throttle*82.0f + startBoost*70.0f;
            gEnginePhase += 6.28318530718 * rpmTone / 22050.0;
            gTurboPhase += 6.28318530718 * (380.0f + throttle*900.0f) / 22050.0;
            if(gEnginePhase>6.28318530718) gEnginePhase-=6.28318530718;
            if(gTurboPhase>6.28318530718) gTurboPhase-=6.28318530718;

            float p=(float)gEnginePhase;
            float sixCylinder = sinf(p) * 0.68f
                              + sinf(p*2.0f+0.25f)*0.24f
                              + sinf(p*3.0f+0.50f)*0.12f
                              + sinf(p*4.0f)*0.05f;
            float exhaustBark = sinf(p*0.5f)*0.20f + sinf(p*2.0f)*0.10f;
            float turboWhistle = sinf((float)gTurboPhase) * (0.012f + throttle*0.06f);
            float ignition = startBoost ? sinf(p*5.0f)*0.12f : 0.0f;
            sample=(sixCylinder*0.60f+exhaustBark+turboWhistle+ignition)
                   *(0.18f+throttle*0.22f+(startBoost?0.10f:0.0f));
            if(gEngineStartSamples>0) --gEngineStartSamples;
        }
        out[i]=(Sint16)(std::max(-1.0f,std::min(1.0f,sample))*26000.0f);
    }
}

static void initEngineAudio() {
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq=22050;
    want.format=AUDIO_S16SYS;
    want.channels=1;
    want.samples=1024;
    want.callback=engineAudioCallback;
    gAudioDevice=SDL_OpenAudioDevice(nullptr,0,&want,&have,0);
    if(gAudioDevice) SDL_PauseAudioDevice(gAudioDevice,0);
}

static void toggleCar() {
    if(enterCarCooldown>0.0f) return;
    if(drivingCar) {
        drivingCar=false;
        carCurrentSpeed=0.0f;
        steeringWheelAngle=0.0f;
        gEngineAudioOn=0;
        gEngineThrottle=0.0f;
        gEngineStartSamples=0;
        playerX=carX0+2.0f;
        playerZ=carZ0;
        enterCarCooldown=0.6f;
        return;
    }
    float dx=playerX-carX0, dz=playerZ-carZ0;
    if(dx*dx+dz*dz < 12.0f) {
        drivingCar=true;
        carYaw=yaw;
        gEngineThrottle=0.0f;
        gEngineAudioOn=1;
        gEngineStartSamples=11025; // half-second engine ignition/rev sound
        enterCarCooldown=0.6f;
    }
}
static void tryPickup(); // Forward declaration: used by tryUseNearby() below.
static bool nearSupermarketCheckout() {
    if(drivingCar || inAirplane || ridingHorse || rooftopLevel!=0 || onUpperFloor) return false;
    // Checkout is inside the CITY MARKET at world position (98.2, 74.3).
    float dx=playerX-98.2f, dz=playerZ-74.3f;
    return dx*dx+dz*dz<8.0f;
}
static void buySupermarketProduct(int index) {
    if(index<0 || index>=8) return;
    int price=supermarketPrices[index];
    if(cashBalance<price) {
        SDL_Log("CARD DECLINED: Not enough balance for %s. Balance Rs.%d",supermarketNames[index],cashBalance);
        return;
    }
    cashBalance-=price;
    supermarketCart[index]++;
    supermarketLastBill=price;
    // Buying only adds products to the cart. The rider starts ONLY when
    // the separate DELIVERY button at the player's home is pressed.
    SDL_Log("ADDED TO DELIVERY CART: %s x1 = Rs.%d. Press the home DELIVERY button when ready. Balance Rs.%d",
            supermarketNames[index],price,cashBalance);
}

static void tryUseNearby() {
    if(onPlanetSurface) {
        // First action at the alien base: rescue the captured school children.
        if(homeworldMissionStarted && !homeworldPrisonOpen) {
            bool clear=true;
            for(const HomeworldGuard& g:homeworldGuards) if(g.alive) { clear=false; break; }
            float dx=playerX-(18.0f+rocketTravelX), dz=playerZ-(-28.0f+rocketTravelZ-12.0f);
            if(clear && dx*dx+dz*dz<100.0f) {
                homeworldPrisonOpen=true;
                rescuedStudents=2;
                SDL_Log("ALIEN PRISON OPENED: two school students rescued. Return to your rocket!");
                return;
            }
            SDL_Log("MISSION: defeat every alien guard, then reach the glowing prison entrance.");
            return;
        }
        if(homeworldPrisonOpen && studentsAboardRocket<rescuedStudents) {
            studentsAboardRocket=rescuedStudents;
            SDL_Log("Students safely boarded the rescue rocket. Return to Earth!");
            return;
        }
        onPlanetSurface=false;
        inRocket=true;
        rocketLanded=true;
        SDL_Log("Back inside the rocket cockpit. Rescued students are secured onboard.");
        return;
    }

    if(onPlanetSurface) {
        onPlanetSurface=false;
        inRocket=true;
        rocketLanded=true;
        SDL_Log("Back inside the rocket cockpit.");
        return;
    }
    if(inRocket) {
        if(currentPlanet>=0 && rocketAltitude>45.0f && rocketAltitude<70.0f) {
            inRocket=false; onPlanetSurface=true; rocketLanded=true;
            rocketLaunched=false; playerHeight=0.0f;
            if(currentPlanet==7 && !homeworldMissionStarted) {
                homeworldMissionStarted=true;
                homeworldPrisonOpen=false;
                homeworldGuards.clear();
                for(int g=0;g<7;g++) {
                    float a=(float)g*6.2831853f/7.0f;
                    HomeworldGuard guard;
                    guard.x=playerX+cosf(a)*(12.0f+(g%2)*4.0f);
                    guard.z=playerZ+sinf(a)*(12.0f+(g%2)*4.0f);
                    homeworldGuards.push_back(guard);
                }
                SDL_Log("NEBULON: Alien homeworld reached. Find the prison holding the kidnapped school children.");
            } else {
                SDL_Log("Landed on %s.",planetNames[std::max(0,currentPlanet)]);
            }
        } else if(rocketLanded) {
            rocketLanded=false; rocketLaunched=true;
            SDL_Log("Rocket takeoff from %s.",planetNames[std::max(0,currentPlanet)]);
        }
        return;
    }
    if(!inRocket && !drivingCar && !inAirplane && !ridingHorse) {
        float rx=playerX-18.0f, rz=playerZ+28.0f;
        if(rx*rx+rz*rz<4.0f) {
            inRocket=true; rocketLaunched=false; rocketLanded=false;
            rocketAltitude=0.0f; rocketTravelX=rocketTravelZ=0.0f;
            rocketVerticalSpeed=0.0f; rocketYaw=yaw; rocketPitch=0.0f; rocketRoll=0.0f;
            rocketThrottle=0.0f; rocketLaunchHold=0.0f; rocketShake=0.0f;
            rocketExhaustPhase=0.0f; rocketIgnitionArmed=false; rocketStageSeparated=false;
            currentPlanet=-1;
            // Clear any movement left over from the tap used to board.
            joystickX=joystickY=smoothX=smoothY=0.0f;
            SDL_Log("Entered rocket cockpit. Rocket is stationary. Push joystick UP to ignite and launch.");
            return;
        }
    }
    // Rescue mission: reach the ship entrance after clearing the aliens, then unlock the cells.
    if(alienInvasionStarted && alienShipLanded && !alienPrisonUnlocked) {
        float sx=playerX+146.0f, sz=playerZ+52.0f;
        bool allClear=true; for(const AlienSoldier& a:alienSoldiers) if(a.alive) { allClear=false; break; }
        if(sx*sx+sz*sz<100.0f && allClear) {
            alienPrisonUnlocked=true;
            for(NPC& n:npcs) if(n.alienCaptured || n.alienAbducting) {
                n.alienCaptured=false; n.alienAbducting=false;
                n.x=-146.0f+(float)((&n-&npcs[0])%3)*1.4f; n.z=-49.0f; n.task=0;
            }
            abductedNPCCount=0;
            SDL_Log("RESCUE COMPLETE: spaceship prison unlocked; residents are free!");
            return;
        }
    }
    if(!ak47Owned) {
        float wx=playerX-akPickupX,wz=playerZ-akPickupZ;
        if(wx*wx+wz*wz<20.0f) {
            ak47Owned=true; ammo=30; reserveAmmo=120;
            SDL_Log("AK-47 collected. Tap SCOPE to aim and FIRE to shoot.");
            return;
        }
    }

    // Use the cooler to drink water before checking nearby desks.
    if(playerX>-137.0f && playerX<-133.8f &&
       playerZ>-33.5f && playerZ<-28.0f) {
        health=std::min(100.0f,health+5.0f);
        SDL_Log("You drank water. Health +5.");
        return;
    }
    // Any classroom desk or shelf lets the player pick up/read a book.
    if(playerX>-155.8f && playerX<-134.2f &&
       playerZ>-37.0f && playerZ<-18.0f) {
        collegeBookOpen=!collegeBookOpen;
        if(collegeBookOpen) {
            collegeBookMessage="MATHS NOTES: A matrix is a rectangular array of numbers arranged in rows and columns. Example: A = [1 2; 3 4].";
            SDL_Log("BOOK PICKED UP. MATRICES: rows are horizontal; columns are vertical. Determinant = ad-bc.");
        } else SDL_Log("BOOK CLOSED AND PUT BACK.");
        return;
    }
    if(parachuting) {
        SDL_Log("Parachute is open. Steer with the joystick; you will land automatically.");
        return;
    }
    if(onAyushBike && ayushRideArrived) {
        onAyushBike=false;
        ayushBikeParked=true;
        ayushWalkingWithPlayer=true;
        ayushReturningToCollege=false;
        // Place Ayush and the player on opposite sides of the parked bike,
        // with enough clearance that their models do not overlap.
        float sideX=cosf(ayushBikeYaw);
        float sideZ=-sinf(ayushBikeYaw);
        playerX=ayushBikeX+2.6f*sideX;
        playerZ=ayushBikeZ+2.6f*sideZ;
        ayushFriendX=playerX+sinf(ayushBikeYaw)*2.2f;
        ayushFriendZ=playerZ+cosf(ayushBikeYaw)*2.2f;
        playerHeight=0.0f;
        yaw=ayushBikeYaw;
        SDL_Log("You got off the bike. Ayush will walk around the college with you. Press Action near Ayush when you want him to return and park the bike.");
        return;
    }
    if(ayushWalkingWithPlayer) {
        // The Action/E button is the explicit command to return to college.
        ayushWalkingWithPlayer=false;
        ayushReturningToCollege=true;
        ayushReturnProgress=0.0f;
        SDL_Log("Ayush is heading back to the college parking area and will park his bike there.");
        return;
    }
    if(onAyushBike) {
        SDL_Log("Ayush is driving you to college. Please wait until the bike stops.");
        return;
    }
    if(sittingInGardenChair) {
        sittingInGardenChair=false;
        gardenChairIndex=-1;
        playerZ-=0.9f;
        SDL_Log("You stood up from the garden chair.");
        return;
    }
    if(sleeping) {
        sleeping=false;
        // Wake up on the clear side of the doorway, beyond the bed and divider.
        playerX=-2.45f;
        playerZ=-5.0f;
        playerHeight=0.0f;
        companionX=-3.15f;
        companionZ=-5.0f;
        smoothX=smoothY=joystickX=joystickY=0.0f;
        SDL_Log("You woke up and got out of bed.");
        return;
    }
    if(!drivingCar && !inAirplane && !ridingHorse && rooftopLevel==0 && !onUpperFloor) {
        float bedDx=playerX+6.0f, bedDz=playerZ+5.0f;
        if(bedDx*bedDx+bedDz*bedDz<10.0f) {
            sleeping=true;
            companionCooking=false;
            companionReturningWithFood=false;
            foodReady=false;
            playerX=-6.0f;
            playerZ=-5.0f;
            companionX=-5.35f;
            companionZ=-5.0f;
            playerHeight=0.0f;
            smoothX=smoothY=joystickX=joystickY=0.0f;
            SDL_Log("You are sleeping. Press E or the action button to wake up.");
            return;
        }
    }
    if(!drivingCar && !inAirplane && !ridingHorse &&
       rooftopLevel==0 && !onUpperFloor) {
        const float chairX[2]={8.5f,12.2f};
        for(int i=0;i<2;i++) {
            float dx=playerX-chairX[i], dz=playerZ-45.0f;
            if(dx*dx+dz*dz<5.0f) {
                sittingInGardenChair=true;
                gardenChairIndex=i;
                playerX=chairX[i];
                playerZ=45.0f;
                yaw=0.0f;
                playerHeight=0.0f;
                smoothX=smoothY=joystickX=joystickY=0.0f;
                walkBlend=0.0f;
                SDL_Log("You sat in the garden chair. Press the action button again to stand up.");
                return;
            }
        }
    }
    // Open the physical checkout register when standing at the rooftop supermarket counter.
    if(nearSupermarketCheckout()) {
        supermarketCheckoutOpen=!supermarketCheckoutOpen;
        if(supermarketCheckoutOpen) {
            SDL_Log("CITY MARKET CHECKOUT OPEN - GAME CARD PAYMENT");
            for(int i=0;i<8;i++)
                SDL_Log("%d. %s - Rs.%d",i+1,supermarketNames[i],supermarketPrices[i]);
            SDL_Log("Tap a product row or press 1-8 to buy. Press E to close.");
        } else SDL_Log("CITY MARKET CHECKOUT CLOSED.");
        return;
    }
    // Load bags once. When they are already inside, do NOT consume the
    // Action press here: let it fall through to the car-entry interaction.
    if(!drivingCar && !inAirplane && groceryBags>0 && !groceriesInCar) {
        float cdx=playerX-carX0, cdz=playerZ-carZ0;
        if(cdx*cdx+cdz*cdz<25.0f) {
            groceriesInCar=true;
            SDL_Log("Groceries loaded into the car. Press Action again to enter and drive.");
            return;
        }
    }
    // Unload from the rear of the parked car, away from the driver's door.
    if(!drivingCar && !inAirplane && groceriesInCar) {
        float unloadX=carX0;
        float unloadZ=carZ0-3.8f;
        float ux=playerX-unloadX, uz=playerZ-unloadZ;
        if(ux*ux+uz*uz<2.8f) {
            groceriesInCar=false;
            SDL_Log("Groceries unloaded from the car.");
            return;
        }
    }
    if(ridingHorse) {
        ridingHorse=false;
        playerX=horseX+1.35f;
        playerZ=horseZ;
        SDL_Log("You got off the horse.");
        return;
    }
    if(!drivingCar && !inAirplane) {
        float horseDx=playerX-horseX, horseDz=playerZ-horseZ;
        if(horseDx*horseDx+horseDz*horseDz<8.0f) {
            ridingHorse=true;
            horseYaw=yaw;
            horsePhase=0.0f;
            playerX=horseX; playerZ=horseZ;
            SDL_Log("Mounted the horse! Use the joystick to ride; press E to get off.");
            return;
        }
        float phoneDx=playerX+3.0f, phoneDz=playerZ-12.2f;
        if(phoneDx*phoneDx+phoneDz*phoneDz<5.0f) {
            bool wasNight=nightMode;
            nightMode=!nightMode;
            if(!wasNight && nightMode) {
                startAlienInvasion();
                SDL_Log("NIGHT MODE: Alien spaceship landing at the SCHOOL!");
            } else if(wasNight && !nightMode) {
                alienInvasionStarted=false;
                alienShipLanded=false;
                alienSoldiers.clear();
                alienBullets.clear();
                alienPrisonUnlocked=false; abductedNPCCount=0;
                for(NPC& n:npcs) { n.alienAbducting=false; n.alienCaptured=false; n.alienLift=0.0f; n.alienBeingLifted=false; }
                SDL_Log("DAY MODE: Alien invasion ended.");
            }
            SDL_Log("Phone dialed 64648 (NIGHT). %s mode activated.",
                    nightMode?"Night":"Day");
            return;
        }
    }
    if(inAirplane) {
        if(planeFlying && planeAltitude>4.0f) {
            parachuting=true;
            inAirplane=false;
            planeEngineOn=false;
            gEngineAudioOn=0;
            playerX=planeX; playerZ=planeZ;
            playerHeight=planeAltitude; yaw=planeYaw;
            jumping=false; fallingFromRoof=false;
            SDL_Log("Parachute deployed! Use joystick to steer and glide. Landing is automatic.");
            return;
        }
        if(!planeEngineOn) {
            planeEngineOn=true;
            gEngineAudioOn=1; gEngineStartSamples=18000;
            SDL_Log("Engine started. Hold UP/FORWARD to accelerate; in the air pull DOWN/BACK to descend and land.");
        } else if(planeSpeed<0.8f && !planeFlying) {
            inAirplane=false; planeEngineOn=false; gEngineAudioOn=0;
            playerX=planeX+3.0f; playerZ=planeZ; playerHeight=0.0f;
            SDL_Log("Disembarked from aircraft.");
        } else {
            SDL_Log("Aircraft is moving. Slow down before exiting.");
        }
        return;
    }
    float pdx=playerX-planeX, pdz=playerZ-planeZ;
    if(!drivingCar && !planeFlying && pdx*pdx+pdz*pdz<30.0f) {
        inAirplane=true; planeEngineOn=false; planeSpeed=0.0f;
        playerX=planeX; playerZ=planeZ; yaw=planeYaw;
        SDL_Log("Boarded aircraft. E: engine, hold UP/FORWARD: accelerate, steer left/right, pull DOWN/BACK in air: descend and land on runway.");
        return;
    }
    // Elevator removed: use the physical staircase to reach level 1.
    // On rental floor, E near a room door unlocks it with the house key,
    // then toggles the door open/closed.
    if(rooftopLevel==2) {
        const float dx[4]={-4.0f,4.0f,-4.0f,4.0f};
        const float dz[4]={7.65f,7.65f,-11.65f,-11.65f};
        for(int i=0;i<4;i++) {
            float ax=playerX-dx[i], az=playerZ-dz[i];
            if(ax*ax+az*az<2.0f) {
                if(!rentalDoorUnlocked[i]) {
                    if(keysFound>0) rentalDoorUnlocked[i]=true;
                    else { SDL_Log("Room locked. Find the house key first."); return; }
                }
                rentalDoorOpen[i]=!rentalDoorOpen[i];
                SDL_Log("Rental room %d %s",i+1,rentalDoorOpen[i]?"opened":"closed");
                return;
            }
        }
    }
    if(deliveryBellRang && !drivingCar && !inAirplane && !ridingHorse) {
        float dx=playerX-4.0f, dz=playerZ-10.8f;
        if(dx*dx+dz*dz<20.0f) {
            deliveryActive=false; deliveryCompleted=true; deliveryBellRang=false;
            groceryBags+=std::max(1,deliveryOrderCount); deliveryOrderCount=0;
            SDL_Log("You received your grocery delivery. Bags are waiting at the door.");
            return;
        }
    }
    // Press the physical DELIVERY button at your own house to request the
    // items already placed in the supermarket cart.
    if(!drivingCar && !inAirplane && !ridingHorse && !deliveryActive) {
        // Keep this interaction well outside the gate-control radius and
        // at the same reachable spot as the visible delivery panel.
        float bdx=playerX-9.15f, bdz=playerZ-12.15f;
        if(bdx*bdx+bdz*bdz<16.0f) {
            int itemsInCart=0;
            for(int i=0;i<8;i++) itemsInCart+=supermarketCart[i];
            if(itemsInCart<=0) {
                SDL_Log("Your delivery cart is empty. Buy groceries at the city market first.");
            } else {
                deliveryActive=true; deliveryCompleted=false; deliveryBellRang=false;
                deliveryOrderCount=itemsInCart; deliveryPathIndex=0;
                deliveryX=deliveryPathX[0]; deliveryZ=deliveryPathZ[0];
                deliveryYaw=0.0f;
                deliveryBellTimer=0.0f;
                for(int i=0;i<8;i++) supermarketCart[i]=0;
                SDL_Log("Delivery requested from your home button. Rider is coming by bike!");
            }
            return;
        }
    }
    // Ring Ayush's doorbell from outside; the gate then slides open.
    // Call Ayush from inside his house: he brings his bike and takes you to college.
    if(!drivingCar && !inAirplane && !ridingHorse && !onAyushBike) {
        float friendDx=playerX-27.7f, friendDz=playerZ+4.4f;
        if(friendDx*friendDx+friendDz*friendDz<9.0f) {
            onAyushBike=true;
            ayushRideArrived=false;
            ayushWalkingWithPlayer=false;
            ayushReturningToCollege=false;
            ayushBikeParked=false;
            ayushRideProgress=0.0f;
            ayushBikeX=25.0f;
            ayushBikeZ=-16.0f;
            ayushBikeYaw=atan2f(170.0f,17.0f);
            ayushGateOpen=true;
            SDL_Log("Calling Ayush: 'Hello, college chale?' Ayush is bringing his bike. Press E after reaching college to get down.");
            return;
        }
    }
    if(!drivingCar && !inAirplane && !ridingHorse) {
        float bellDx=playerX-(25.0f+2.05f);
        float bellDz=playerZ-11.42f;
        if(bellDx*bellDx+bellDz*bellDz<16.0f) {
            ayushGateOpen=true;
            SDL_Log("Ding-dong! Ayush's gate is opening. Walk inside.");
            return;
        }
    }
    // These controls are deliberately placed beside the physical gates.
    float dgx=playerX-4.05f, dgz=playerZ-9.55f;
    if(dgx*dgx+dgz*dgz < 1.00f) {
        mansionGateOpen=!mansionGateOpen;
        SDL_Log("Mansion gate %s",mansionGateOpen?"opening":"closing");
        return;
    }
    float grx=playerX+14.28f, grz=playerZ-29.48f;
    if(grx*grx+grz*grz < 5.2f) {
        garageShutterOpen=!garageShutterOpen;
        SDL_Log("Garage shutter %s",garageShutterOpen?"opening":"closing");
        return;
    }
    if(drivingCar) { toggleCar(); return; }
    float dx=playerX-carX0, dz=playerZ-carZ0;
    if(dx*dx+dz*dz<36.0f) { toggleCar(); return; }
    tryPickup();
}

static void tryPickup() {
    for(Item& it:items) if(it.active) {
        float dx=it.x-playerX,dz=it.z-playerZ;
        if(dx*dx+dz*dz<2.4f) {
            if(it.name.find("Grocery:")==0) {
                if(cashBalance<25) { SDL_Log("Not enough cash for %s.",it.name.c_str()); return; }
                cashBalance-=25;
                groceryBags++;
                SDL_Log("Bought %s for Rs.25. Cash left: Rs.%d",it.name.c_str(),cashBalance);
            }
            it.active=false; inventoryCount++;
            if(it.name=="House key") keysFound++;
            else if(it.name=="Medkit") health=std::min(100.0f,health+35.0f);
            else if(it.name=="Ammo box") reserveAmmo+=24;
            else if(it.name=="Battery") gFlashlight=true;
            SDL_Log("Picked up: %s",it.name.c_str());
            return;
        }
    }
    // Pick ripe fruit from the nearest fruit tree.
    const float tx[]={-13,-17,13,17,-20,20};
    const float tz[]={17,29,17,29,42,42};
    for(int i=0;i<6;i++) {
        float dx=playerX-tx[i],dz=playerZ-tz[i];
        if(dx*dx+dz*dz<5.0f) {
            fruitCount++; score+=10;
            SDL_Log("Fruit collected! Score: %d",score);
            return;
        }
    }
    if(playerZ>8.8f && fabsf(playerX-6.0f)<1.5f) gDoorOpen=true;
}
static void startJump() {
    if(!drivingCar && !inAirplane && !ridingHorse && !sleeping &&
       !jumping && !fallingFromRoof && !swimming) {
        jumping=true;
        jumpBaseHeight=playerHeight;
        jumpVelocity=6.0f;
    }
}
static void throwFruit() {
    if(fruitCount<=0 || throwCooldown>0.0f || drivingCar) return;
    float fx=-sinf(yaw), fz=-cosf(yaw);
    thrownFruits.push_back({playerX+fx*0.8f,playerZ+fz*0.8f,fx*7.0f,fz*7.0f,1.8f});
    fruitCount--; throwCooldown=0.35f;
}
static void shoot() {
    if(ak47Owned) { fireAK47(); return; }
    if(fireCooldown>0) return;
    ammo=999999; reserveAmmo=999999; fireCooldown=0.30f;
}
static void reloadWeapon() {
    // Unlimited ammunition: reloading is unnecessary.
    ammo=999999;
    reserveAmmo=999999;
}
static void updateGame(float dt,float ghostX,float ghostZ,float& outGX,float& outGZ) {
    if(gameOver) { joystickX=joystickY=smoothX=smoothY=0.0f; return; }
    updateNPCs(dt);
    recordPlayerLearning(dt);
    updateAlienInvasion(dt);
    if(onPlanetSurface) {
        homeworldTime+=dt;
        float forward=-joystickY;
        float strafe=joystickX;
        float fx=-sinf(yaw), fz=-cosf(yaw);
        float rx=cosf(yaw), rz=-sinf(yaw);
        playerX+=(fx*forward+rx*strafe)*5.2f*dt;
        playerZ+=(fz*forward+rz*strafe)*5.2f*dt;
        playerX=std::max(-180.0f,std::min(180.0f,playerX));
        playerZ=std::max(-180.0f,std::min(180.0f,playerZ));
        for(HomeworldGuard& g:homeworldGuards) if(g.alive) {
            g.flash=std::max(0.0f,g.flash-dt);
            float dx=playerX-g.x,dz=playerZ-g.z;
            float d=sqrtf(dx*dx+dz*dz);
            if(d>2.8f && d>0.001f) {
                float step=std::min(1.35f*dt,d-2.2f);
                g.x+=dx/d*step; g.z+=dz/d*step;
            }
            g.yaw=atan2f(dx,dz);
        }
        return;
    }
    if(health<=0.0f) gameOver=true;
    // Children follow a clear, obstacle-avoiding route from the swing,
    // through the front entrance, around the kitchen and partition walls,
    // then to the two study chairs.
    if(childrenGoingStudy && !childrenAtStudyDesk) {
        const float pathX[7]={-11.0f,-7.0f,-1.0f,5.8f,7.8f,7.8f,3.0f};
        const float pathZ[7]={50.0f,43.0f,31.0f,16.0f,5.0f,-8.55f,-8.55f};
        childWalkPhase+=dt*9.0f;
        bool allArrived=true;
        for(int child=0;child<2;child++) {
            const float side=(child==0?-0.82f:0.82f);
            float targetX=0.0f, targetZ=0.0f;
            float remaining=3.0f*dt;
            int &segment=childPathSegment[child];
            segment=std::max(0,std::min(segment,6));
            while(remaining>0.0f && segment<6) {
                targetX=pathX[segment+1]+side;
                targetZ=pathZ[segment+1];
                float dx=targetX-childWorldX[child];
                float dz=targetZ-childWorldZ[child];
                float dist=sqrtf(dx*dx+dz*dz);
                if(dist<0.06f) { segment++; continue; }
                float step=std::min(remaining,dist);
                childWorldX[child]+=(dx/dist)*step;
                childWorldZ[child]+=(dz/dist)*step;
                remaining-=step;
                if(step>=dist-0.001f) segment++;
            }
            const float finalX=(child==0?-1.35f:1.35f);
            const float finalZ=-8.05f;
            if(segment>=6 && remaining>0.0f) {
                float dx=finalX-childWorldX[child], dz=finalZ-childWorldZ[child];
                float dist=sqrtf(dx*dx+dz*dz);
                if(dist>0.06f) {
                    float step=std::min(remaining,dist);
                    childWorldX[child]+=(dx/dist)*step;
                    childWorldZ[child]+=(dz/dist)*step;
                }
            }
            float dx=finalX-childWorldX[child], dz=finalZ-childWorldZ[child];
            if(sqrtf(dx*dx+dz*dz)>0.10f) allArrived=false;
            else {
                childWorldX[child]=finalX;
                childWorldZ[child]=finalZ;
            }
        }
        if(allArrived) {
            childrenAtStudyDesk=true;
            SDL_Log("Both children reached their study chairs.");
        }
    }

    smoothX+=(joystickX-smoothX)*std::min(1.0f,dt*10.0f);
    smoothY+=(joystickY-smoothY)*std::min(1.0f,dt*10.0f);
    swimming = !drivingCar && inPond(playerX,playerZ);

    enterCarCooldown=std::max(0.0f,enterCarCooldown-dt);
    throwCooldown=std::max(0.0f,throwCooldown-dt);
    const float gateRate=1.65f;
    ayushGateAmount += ((ayushGateOpen?1.0f:0.0f)-ayushGateAmount)
                       * std::min(1.0f,gateRate*dt);
    mansionGateAmount += ((mansionGateOpen?1.0f:0.0f)-mansionGateAmount)
                         * std::min(1.0f,gateRate*dt);
    garageShutterAmount += ((garageShutterOpen?1.0f:0.0f)-garageShutterAmount)
                           * std::min(1.0f,gateRate*dt);

    // Courier follows a small waypoint route and never advances through a collision.
    if(deliveryActive) {
        if(deliveryBellRang) {
            deliveryBellTimer+=dt;
            if(deliveryBellTimer>5.0f) {
                deliveryActive=false; deliveryCompleted=true;
                groceryBags+=std::max(1,deliveryOrderCount);
                deliveryOrderCount=0;
                SDL_Log("Delivery completed. Grocery bags are at your front door.");
            }
        } else {
            int idx=std::max(0,std::min(deliveryPathIndex,5));
            float dx=deliveryPathX[idx]-deliveryX, dz=deliveryPathZ[idx]-deliveryZ;
            float dist=sqrtf(dx*dx+dz*dz);
            if(dist<0.85f) {
                if(deliveryPathIndex<5) deliveryPathIndex++;
                else {
                    deliveryX=6.0f; deliveryZ=12.0f;
                    deliveryBellRang=true; deliveryBellTimer=0.0f;
                    SDL_Log("DING-DONG! Delivery rider reached your house and rang the doorbell.");
                }
            } else {
                float step=std::min(dist,4.5f*dt);
                // Local obstacle-avoidance steering: test many headings around
                // the direct route and choose the safest clear step that makes
                // the most progress. This lets the bike arc around walls,
                // buildings, people and animals instead of freezing at them.
                float baseYaw=atan2f(dx,dz);
                float bestScore=1.0e30f;
                float bestX=deliveryX, bestZ=deliveryZ, bestYaw=baseYaw;
                bool foundMove=false;
                const float angleChoices[25]={
                    0.0f, 0.18f,-0.18f, 0.36f,-0.36f, 0.56f,-0.56f,
                    0.78f,-0.78f, 1.02f,-1.02f, 1.28f,-1.28f,
                    1.57f,-1.57f, 1.90f,-1.90f, 2.25f,-2.25f,
                    2.62f,-2.62f, 2.95f,-2.95f, 3.14159f, -3.14159f
                };
                for(int k=0;k<25;k++) {
                    float a=baseYaw+angleChoices[k];
                    float tx=deliveryX+sinf(a)*step;
                    float tz=deliveryZ+cosf(a)*step;
                    if(deliveryBlocked(tx,tz)) continue;
                    // Prefer progress toward the active waypoint, but add a
                    // modest turn cost to avoid jittering left-right each frame.
                    float rx=deliveryPathX[idx]-tx, rz=deliveryPathZ[idx]-tz;
                    float remaining=sqrtf(rx*rx+rz*rz);
                    float turnCost=fabsf(angleChoices[k])*0.32f;
                    float score=remaining+turnCost;
                    if(score<bestScore) {
                        bestScore=score; bestX=tx; bestZ=tz; bestYaw=a;
                        foundMove=true;
                    }
                }
                if(foundMove) {
                    deliveryX=bestX; deliveryZ=bestZ; deliveryYaw=bestYaw;
                }
            }
        }
    }
    for(Animal& a:animals) {
        a.phase+=dt;
        a.x+=cosf(a.dir)*a.speed*dt;
        a.z+=sinf(a.dir)*a.speed*dt;
        if(a.x < -22 || a.x > 22 || a.z < 14 || a.z > 46 ||
           (int)(a.phase*2.0f)%13==0) a.dir+=1.7f*dt;
    }
    for(auto it=thrownFruits.begin();it!=thrownFruits.end();) {
        it->x+=it->vx*dt; it->z+=it->vz*dt; it->life-=dt;
        if(it->life<=0) it=thrownFruits.erase(it); else ++it;
    }
    // Update Ayush independently from player movement. These companion states
    // must never occupy an else-if branch that skips the player's walking code.
    if(ayushWalkingWithPlayer) {
        // Keep Ayush close behind the player, but let him catch up naturally
        // after turns or quick movement. Try direct movement first, then slide
        // along a wall so he does not get stuck trying to cross obstacles.
        float targetX=playerX+sinf(yaw)*1.75f;
        float targetZ=playerZ+cosf(yaw)*1.75f;
        float friendDX=targetX-ayushFriendX, friendDZ=targetZ-ayushFriendZ;
        float friendDist=sqrtf(friendDX*friendDX+friendDZ*friendDZ);
        float moveSpeed=8.5f;
        if(friendDist>0.55f) {
            float step=std::min(friendDist,moveSpeed*dt);
            float nx=ayushFriendX+(friendDX/friendDist)*step;
            float nz=ayushFriendZ+(friendDZ/friendDist)*step;
            if(!collides(nx,nz)) {
                ayushFriendX=nx;
                ayushFriendZ=nz;
            } else {
                if(!collides(nx,ayushFriendZ)) ayushFriendX=nx;
                if(!collides(ayushFriendX,nz)) ayushFriendZ=nz;
            }
        }
        float actualDX=targetX-ayushFriendX, actualDZ=targetZ-ayushFriendZ;
        float remaining=sqrtf(actualDX*actualDX+actualDZ*actualDZ);
        ayushWalkBlend+=(std::min(1.0f,remaining/0.8f)-ayushWalkBlend)
                        *std::min(1.0f,dt*7.0f);
        if(ayushWalkBlend>0.04f) ayushWalkPhase+=dt*8.5f;
    } else if(ayushReturningToCollege) {
        ayushWalkBlend=std::min(1.0f,ayushWalkBlend+dt*4.0f);
        ayushWalkPhase+=dt*8.5f;
        ayushReturnProgress=std::min(1.0f,ayushReturnProgress+dt/8.0f);
        float t=ayushReturnProgress;
        t=t*t*(3.0f-2.0f*t);
        static float returnStartX=0.0f, returnStartZ=0.0f;
        static bool returnStartSet=false;
        if(!returnStartSet) {
            returnStartX=ayushFriendX;
            returnStartZ=ayushFriendZ;
            returnStartSet=true;
        }
        ayushFriendX=returnStartX+(-145.0f-returnStartX)*t;
        ayushFriendZ=returnStartZ+(-30.0f-returnStartZ)*t;
        if(ayushReturnProgress>=1.0f) {
            ayushReturningToCollege=false;
            ayushWalkBlend=0.0f;
            returnStartSet=false;
            ayushBikeParked=true;
            ayushBikeX=-145.0f;
            ayushBikeZ=-33.0f;
            ayushBikeYaw=atan2f(170.0f,17.0f);
            SDL_Log("Ayush has returned to the college and parked his bike.");
        }
    }

    if(inRocket) {
        // Keep the Android joystick live while in the cockpit: never clear its
        // touch values here, otherwise the rocket cannot respond to steering.
        bool rocketThrust=(joystickY>0.10f);
        float throttleTarget=rocketThrust?std::max(0.0f,std::min(1.0f,joystickY)):0.0f;
        rocketThrottle += (throttleTarget-rocketThrottle)*std::min(1.0f,dt*(rocketThrust?0.72f:1.8f));
        rocketEngineGlow=std::max(0.0f,std::min(1.0f,rocketEngineGlow+(rocketThrottle>0.08f?dt*0.95f:-dt*0.55f)));
        rocketExhaustPhase += dt*(8.0f+rocketThrottle*34.0f);
        rocketYaw -= joystickX*0.72f*dt;
        rocketRoll += (-joystickX*0.55f-rocketRoll*1.8f)*dt;
        rocketPitch += ((rocketThrust?-0.10f:0.0f)-rocketPitch*0.75f)*dt;
        rocketPitch=std::max(-0.35f,std::min(0.35f,rocketPitch));
        if(!rocketLanded) {
            if(rocketThrottle>0.18f && !rocketLaunched) {
                rocketLaunchHold += dt;
                rocketIgnitionArmed=true;
                if(rocketLaunchHold>1.15f) {
                    rocketLaunched=true;
                    rocketStageSeparated=false;
                    rocketShake=0.9f;
                    SDL_Log("IGNITION CONFIRMED - HOLD-DOWN RELEASE - LIFTOFF");
                }
            } else if(!rocketLaunched) {
                rocketLaunchHold=0.0f;
                rocketIgnitionArmed=false;
            }
            if(rocketLaunched) {
                // Simplified flight model: thrust ramps smoothly, gravity always acts,
                // and atmospheric drag fades as altitude increases.
                float gravity=2.65f;
                float atmosphere=std::max(0.12f,1.0f-rocketAltitude/300.0f);
                float thrustAccel=rocketThrottle*10.8f;
                float drag=rocketVerticalSpeed*0.045f*atmosphere;
                rocketVerticalSpeed += (thrustAccel-gravity-drag)*dt;
                rocketVerticalSpeed=std::max(-4.5f,std::min(18.0f,rocketVerticalSpeed));
                rocketAltitude=std::max(0.0f,rocketAltitude+rocketVerticalSpeed*dt);
                rocketShake=std::max(0.0f,rocketShake-dt*0.48f);
                if(rocketAltitude>240.0f && !rocketStageSeparated) {
                    rocketStageSeparated=true;
                    SDL_Log("BOOSTER STAGE SEPARATED - UPPER STAGE ACTIVE");
                }
            } else {
                rocketVerticalSpeed=0.0f;
            }
            // Left/right steers sideways independently of vertical climb.
            rocketTravelX += (-sinf(rocketYaw)*joystickX*7.0f)*dt;
            rocketTravelZ += (-cosf(rocketYaw)*joystickX*7.0f)*dt;
            playerX=18.0f+rocketTravelX;
            playerZ=-28.0f+rocketTravelZ;
            if(rocketAltitude>45.0f) {
                float best=1.0e9f; int bestPlanet=-1;
                const float px[8]={-220,-130,0,150,360,610,900,1240};
                const float pz[8]={-520,-760,-1040,-1360,-1780,-2240,-2760,-3320};
                for(int k=0;k<8;k++) {
                    float dx=rocketTravelX-px[k], dz=rocketTravelZ-pz[k];
                    float d=dx*dx+dz*dz;
                    if(d<best) { best=d; bestPlanet=k; }
                }
                currentPlanet=bestPlanet;
            }
            if(rocketAltitude>900.0f) rocketAltitude=900.0f;
        } else {
            rocketVerticalSpeed=0.0f;
            rocketAltitude=0.0f;
        }
        smoothX=smoothY=0.0f;
        playerHeight=rocketAltitude;
    } else if(sittingInGardenChair) {
        smoothX=smoothY=0.0f;
        joystickX=joystickY=0.0f;
        walkBlend=0.0f;
    } else if(sleeping) {
        smoothX=smoothY=0.0f;
        walkBlend=0.0f;
    } else if(onAyushBike) {
        smoothX=smoothY=joystickX=joystickY=0.0f;
        walkBlend=0.0f;
        if(!ayushRideArrived) {
            // Smooth automatic ride from Ayush's house to the GL BAZZAR college gate.
            ayushRideProgress=std::min(1.0f,ayushRideProgress+dt/22.0f);
            float t=ayushRideProgress;
            t=t*t*(3.0f-2.0f*t);
            // Follow the exact centerline of the rear access lane.
            ayushBikeX=25.0f+(-145.0f-25.0f)*t;
            ayushBikeZ=-16.0f+(-33.0f+16.0f)*t;
            playerX=ayushBikeX;
            playerZ=ayushBikeZ;
            playerHeight=0.0f;
            ayushBikeYaw=atan2f(170.0f,17.0f);
            yaw=ayushBikeYaw;
            if(ayushRideProgress>=1.0f) {
                ayushRideArrived=true;
                SDL_Log("You have arrived at GL BAZZAR college with Ayush. Press E / Action to get off the bike.");
            }
        }
    } else if(parachuting) {
        yaw -= smoothX*0.95f*dt;
        float forward=smoothY;
        playerX += (-sinf(yaw)*forward*5.2f + cosf(yaw)*smoothX*2.0f)*dt;
        playerZ += (-cosf(yaw)*forward*5.2f - sinf(yaw)*smoothX*2.0f)*dt;
        playerHeight -= (2.0f-std::max(0.0f,forward)*0.65f)*dt;
        float groundY=hillHeightAt(playerX,playerZ);
        if(playerHeight<=groundY+0.03f) {
            playerHeight=groundY;
            parachuting=false;
            parachuteDrift=0.0f;
            planeFlying=false;
            planeEngineOn=false;
            planeSpeed=0.0f;
            SDL_Log("Safe parachute landing! You can move normally now.");
        }
        // The abandoned aircraft continues its forward flight in the background.
        planeAnimTime+=dt;
        planeX-=sinf(planeYaw)*planeSpeed*dt;
        planeZ-=cosf(planeYaw)*planeSpeed*dt;
    } else if(inAirplane) {
        planeAnimTime += dt;
        if(planeEngineOn) {
            float throttle=std::max(0.0f,smoothY);
            float descend=std::max(0.0f,-smoothY);
            planeSpeed += throttle*7.0f*dt;
            planeSpeed -= (throttle<0.05f?1.8f:0.20f)*dt;
            planeSpeed=std::max(0.0f,std::min(24.0f,planeSpeed));
            // Keep steering direction consistent with the car controls.
            planeYaw -= smoothX*(planeFlying?0.78f:0.42f)*dt;
            float nx=planeX-sinf(planeYaw)*planeSpeed*dt;
            float nz=planeZ-cosf(planeYaw)*planeSpeed*dt;

            // Take off only after reaching the end of the runway at sufficient speed.
            if(!planeFlying && planeSpeed>10.0f && (planeZ<88.0f || planeZ>151.0f)) {
                planeFlying=true;
                SDL_Log("Takeoff! Pull joystick DOWN/BACK to descend and land on the runway.");
            }
            planeX=nx; planeZ=nz;

            if(planeFlying) {
                // Forward/up input climbs gently; backward/down input descends.
                if(throttle>0.08f)
                    planeAltitude=std::min(38.0f,planeAltitude+throttle*3.2f*dt);
                if(descend>0.08f)
                    planeAltitude=std::max(0.0f,planeAltitude-descend*7.0f*dt);

                // Touch down only when low and over the airport runway.
                bool overRunway=(planeX>5.0f && planeX<89.0f &&
                                 planeZ>80.0f && planeZ<158.0f);
                if(overRunway && planeAltitude<=0.65f && planeSpeed>0.8f) {
                    planeAltitude=0.0f;
                    planeFlying=false;
                    SDL_Log("Landing complete! Release joystick to slow down, then press E to exit.");
                }
            }
            gEngineThrottle=throttle;
            gEngineAudioOn=1;
        } else {
            planeSpeed=std::max(0.0f,planeSpeed-3.0f*dt);
        }
        playerX=planeX; playerZ=planeZ; playerHeight=planeAltitude; yaw=planeYaw;
    } else if(ridingHorse) {
        horseYaw -= smoothX*1.25f*dt;
        float horseSpeed=5.8f;
        float nx=horseX-sinf(horseYaw)*smoothY*horseSpeed*dt;
        float nz=horseZ-cosf(horseYaw)*smoothY*horseSpeed*dt;
        if(!collides(nx,horseZ)) horseX=nx;
        if(!collides(horseX,nz)) horseZ=nz;
        horsePhase+=dt*(1.5f+std::fabs(smoothY)*7.0f);
        playerX=horseX; playerZ=horseZ; yaw=horseYaw;
        companionPhase+=dt*(1.0f+std::fabs(smoothY)*5.0f);
        {
            float tx=horseX+cosf(horseYaw)*2.0f, tz=horseZ-sinf(horseYaw)*2.0f;
            float f=std::min(1.0f,dt*2.0f);
            companionX+=(tx-companionX)*f;
            companionZ+=(tz-companionZ)*f;
        }
        playerHeight=0.0f;
        walkBlend=std::min(1.0f,std::fabs(smoothY));
    } else if(drivingCar) {
        carYaw -= smoothX*1.65f*dt;
        float throttle=smoothY;
        gEngineThrottle=std::fabs(throttle);
        float carSpeed=18.0f;
        carCurrentSpeed=std::fabs(throttle)*carSpeed*3.6f;
        steeringWheelAngle += (smoothX*0.95f-steeringWheelAngle)*std::min(1.0f,dt*7.0f);
        carWheelSpin += throttle*carSpeed*dt/0.34f;
        float nx=carX0-sinf(carYaw)*throttle*carSpeed*dt;
        float nz=carZ0-cosf(carYaw)*throttle*carSpeed*dt;
        // Keep the vehicle in the garden bounds and out of solid structures.
        if(!vehiclePathCollides(carX0,carZ0,nx,nz)) { carX0=nx; carZ0=nz; }
        else {
            // Collision response: keep the car at its last safe position.
            // Do not apply a positional nudge into the obstacle.
            carCurrentSpeed=0.0f;
            gEngineThrottle=0.0f;
        }
        playerX=carX0;
        playerZ=carZ0;
        yaw=carYaw;
        carVisualHeight=carRideHeightAt(carX0,carZ0);
        playerHeight=hillHeightAt(carX0,carZ0)+carVisualHeight;
        onUpperFloor=false;
    } else {
    float speed=swimming?1.65f:5.0f;
    float dx=(-sinf(yaw)*smoothY+cosf(yaw)*smoothX)*speed*dt;
    float dz=(-cosf(yaw)*smoothY-sinf(yaw)*smoothX)*speed*dt;
    float moveAmount=std::min(1.0f,sqrtf(smoothX*smoothX+smoothY*smoothY));
    walkBlend += (moveAmount-walkBlend)*std::min(1.0f,dt*9.0f);
    if(moveAmount>0.05f) walkPhase += dt*(swimming?4.0f:9.0f);
    else walkPhase += dt*1.2f;

    // Move in the world first. The stair exit corridor remains open to the
    // garden so the player can leave the last step instead of being trapped.
    float nx=playerX+dx, nz=playerZ+dz;
    // Alien soldiers are solid obstacles: the player cannot walk through them.
    for(const AlienSoldier& a:alienSoldiers) if(a.alive) {
        float ax=nx-a.x, az=nz-a.z;
        if(ax*ax+az*az<1.45f) { nx=playerX; nz=playerZ; break; }
    }
    bool hitX=collides(nx,playerZ);
    bool hitZ=collides(playerX,nz);
    if(!hitX) playerX=nx;
    // On impact, stay at the last valid position and slide along the wall.
    if(!hitZ) playerZ=nz;
    if(jumping && !parachuting) {
        playerHeight+=jumpVelocity*dt;
        jumpVelocity-=15.0f*dt;
        if(playerHeight<=jumpBaseHeight) {
            playerHeight=jumpBaseHeight;
            jumping=false;
            jumpVelocity=0.0f;
        }
    }
    if(fallingFromRoof) {
        playerHeight+=jumpVelocity*dt;
        jumpVelocity-=18.0f*dt;
        float groundY=hillHeightAt(playerX,playerZ);
        if(playerHeight<=groundY) {
            float drop=std::max(0.0f,roofFallStartHeight-groundY);
            playerHeight=groundY;
            fallingFromRoof=false;
            jumpVelocity=0.0f;
            float damage=std::max(0.0f,(drop-2.0f)*5.0f);
            health=std::max(1.0f,health-damage);
            if(damage>0.0f) SDL_Log("Hard landing! Fall damage: %.0f",damage);
        }
    }

    // Companion can take a cooking order, walk to the kitchen, prepare a meal,
    // then return to the player carrying it.
    companionPhase+=dt*(1.0f+walkBlend*6.0f);
    if(companionCooking) {
        const float kitchenX=6.0f, kitchenZ=-5.0f;
        float dx=kitchenX-companionX, dz=kitchenZ-companionZ;
        float dist=sqrtf(dx*dx+dz*dz);
        if(dist>0.30f) {
            float step=std::min(dist,2.5f*dt);
            companionX+=(dx/dist)*step;
            companionZ+=(dz/dist)*step;
        } else {
            cookingTimer+=dt;
            if(cookingTimer>=5.0f) {
                companionCooking=false;
                companionReturningWithFood=true;
            }
        }
    } else if(companionReturningWithFood) {
        float targetX=playerX+cosf(yaw)*1.25f;
        float targetZ=playerZ-sinf(yaw)*1.25f;
        float dx=targetX-companionX, dz=targetZ-companionZ;
        float dist=sqrtf(dx*dx+dz*dz);
        if(dist>0.65f) {
            float step=std::min(dist,2.8f*dt);
            companionX+=(dx/dist)*step;
            companionZ+=(dz/dist)*step;
        } else {
            companionReturningWithFood=false;
            foodReady=true;
            SDL_Log("Meal is ready! Press the action button near her to eat.");
        }
    } else {
        float targetX=playerX+cosf(yaw)*1.7f;
        float targetZ=playerZ-sinf(yaw)*1.7f;
        float follow=std::min(1.0f,dt*2.2f);
        companionX+=(targetX-companionX)*follow;
        companionZ+=(targetZ-companionZ)*follow;
    }

    // Leaving the first-floor rooftop edge starts a gravity-driven fall.
    if(rooftopLevel==1 && !fallingFromRoof &&
       (playerX < -8.15f || playerX > 8.15f ||
        playerZ < -12.15f || playerZ > 8.15f)) {
        roofFallStartHeight=playerHeight;
        fallingFromRoof=true;
        rooftopLevel=0;
        onUpperFloor=false;
        jumping=false;
        jumpVelocity=0.0f;
    }

    // Height follows the same open stair corridor in BOTH directions.
    // Keep the terrace level until the player reaches the stair approach;
    // then smoothly follow the stair slope down to the garden/ground level.
    const bool inStairLane = (playerX >= 4.15f && playerX <= 7.85f &&
                              playerZ > 1.20f && playerZ <= 5.35f);
    const bool onLanding = (playerX >= 1.15f && playerX <= 8.85f &&
                            playerZ >= -4.10f && playerZ <= 1.35f);
    if(inStairLane) {
        float t=(playerZ-1.00f)/3.90f;
        t=std::max(0.0f,std::min(1.0f,t));
        playerHeight=2.60f-t*2.10f;
    } else if(onLanding) {
        playerHeight=2.60f;
    } else if(!jumping && !fallingFromRoof && !parachuting) {
        playerHeight=0.0f;
    }
    onUpperFloor = (onLanding || (inStairLane && playerHeight > 1.85f));
    if(!onUpperFloor && !swimming && !fallingFromRoof && !parachuting) {
        playerHeight=std::max(playerHeight,hillHeightAt(playerX,playerZ));
    }
    if(swimming) playerHeight=0.12f+0.06f*sinf((float)SDL_GetTicks()*0.006f);

    // Apply the new stair ramp LAST so the old ground/terrace height logic
    // cannot reset the player's feet to ground level while climbing.
    const bool onStoreyStairs = (playerX>=3.85f && playerX<=7.65f &&
                                 playerZ<=-1.10f && playerZ>=-10.05f);
    if(onStoreyStairs && rooftopLevel==0) {
        // Reach the exact floor height at the final stair/landing junction.
        float t=(-playerZ-1.10f)/6.20f;
        t=std::max(0.0f,std::min(1.0f,t));
        playerHeight=2.60f+t*1.44f;
        onUpperFloor=true;
        if(t>=0.96f) {
            rooftopLevel=1;
            playerHeight=rooftopWalkHeight(1);
            onUpperFloor=false;
            SDL_Log("Reached supermarket and gym floor by stairs.");
        }
    } else if(onStoreyStairs && rooftopLevel==1) {
        float t=(-playerZ-1.10f)/6.20f;
        t=std::max(0.0f,std::min(1.0f,t));
        playerHeight=4.04f-t*1.44f;
        if(t<=0.04f) {
            rooftopLevel=0;
            playerHeight=2.60f;
            onUpperFloor=true;
            SDL_Log("Returned to main terrace by stairs.");
        }
    }
    if(rooftopLevel>0 && !onStoreyStairs && !jumping && !fallingFromRoof) {
        // Keep the storey slab height except while using stairs or airborne.
        playerHeight=rooftopWalkHeight(rooftopLevel);
    }
    }

    outGX=ghostX; outGZ=ghostZ;
    fireCooldown=std::max(0.0f,fireCooldown-dt);
}
static void handleTouch(const SDL_Event& e) {
    float x=e.tfinger.x,y=e.tfinger.y;
    if(e.type==SDL_FINGERDOWN) {
        if(supermarketCheckoutOpen && x>0.22f && x<0.78f && y>0.25f && y<0.79f) {
            int row=std::max(0,std::min(3,(int)((0.79f-y)/0.15f)));
            int col=(x>=0.50f)?1:0;
            supermarketSelected=row*2+col;
            buySupermarketProduct(supermarketSelected);
            return;
        }
        if(!drivingCar && !inAirplane && !ridingHorse && !sleeping &&
           childrenScreenHit(x,y)) {
            childrenGoingStudy=true;
            childrenAtStudyDesk=false;
            childWorldX[0]=-11.82f; childWorldX[1]=-10.18f;
            childWorldZ[0]=50.0f; childWorldZ[1]=50.0f;
            childPathSegment[0]=0; childPathSegment[1]=0;
            childrenStudyProgress=0.0f;
            SDL_Log("Children are walking home to their study desk.");
            return;
        }
        // Only a double-tap directly on the companion's visible on-screen body
        // issues the cooking order. Taps elsewhere are ignored.
        if(!drivingCar && !inAirplane && !ridingHorse && !sleeping &&
           companionScreenHit(x,y)) {
            Uint32 now=SDL_GetTicks();
            bool sameSpot=(fabsf(x-companionLastTapX)<0.10f &&
                           fabsf(y-companionLastTapY)<0.10f);
            if(companionLastTapMs!=0 && now-companionLastTapMs<450 && sameSpot) {
                if(foodReady) {
                    foodReady=false;
                    health=std::min(100.0f,health+30.0f);
                    SDL_Log("You ate the meal she brought. Health restored by 30.");
                } else if(!companionCooking && !companionReturningWithFood) {
                    companionCooking=true;
                    cookingTimer=0.0f;
                    SDL_Log("Double-tap on companion: she is going to the kitchen.");
                }
                companionLastTapMs=0;
                return;
            }
            companionLastTapMs=now;
            companionLastTapX=x;
            companionLastTapY=y;
        } else {
            companionLastTapMs=0;
        }
        if(!ak47Owned && !drivingCar && x>0.36f && x<0.60f && y>0.70f) {
            float gx=playerX-akPickupX,gz=playerZ-akPickupZ;
            if(gx*gx+gz*gz<20.0f) { tryUseNearby(); return; }
        }
        if(ak47Owned && x>0.57f && x<0.76f && y>0.32f && y<0.52f) { toggleScope(); return; }
        if(ak47Owned && x>0.77f && y>0.68f) { fireAK47(); return; }
        // Combat buttons: slap (upper) and kick (lower).
        if(x>0.81f && y>0.36f && y<0.48f) { attackNearestNPC(false); return; }
        if(x>0.81f && y>0.55f && y<0.67f) { attackNearestNPC(true); return; }
        // Upper-right NPC COMMAND panel. Tap it to open the phone keyboard.
        if(x>0.49f && y>0.045f && y<0.285f) {
            if(npcCommandTyping) {
                executeNPCCommand();
                npcCommandTyping=false;
                SDL_StopTextInput();
            } else {
                npcCommandTyping=true;
                npcCommandBuffer.clear();
                SDL_StartTextInput();
            }
            return;
        }
        if(drivingCar && x>0.60f && x<=0.76f && y>0.70f) {
            carHeadlightsOn=!carHeadlightsOn;
            SDL_Log("Car headlights %s",carHeadlightsOn?"ON":"OFF");
            return;
        }
        if(x>0.76f && y>0.70f) { shoot(); return; }
        if(x>0.60f && x<=0.76f && y>0.70f) { startJump(); return; }
        if(x>0.40f && x<0.60f && y>0.72f) {
            tryUseNearby();
            return;
        }
        if(x<0.40f) { leftFinger=e.tfinger.fingerId; joystickX=joystickY=0; }
        else { rightFinger=e.tfinger.fingerId; lastTouchX=x; lastTouchY=y; }
    } else if(e.type==SDL_FINGERMOTION) {
        if(e.tfinger.fingerId==leftFinger) {
            joystickX=std::max(-1.0f,std::min(1.0f,(x-0.14f)/0.14f));
            joystickY=std::max(-1.0f,std::min(1.0f,(0.825f-y)/0.14f));
        }
        if(e.tfinger.fingerId==rightFinger) {
            yaw-=(x-lastTouchX)*4.0f;
            pitch-=(y-lastTouchY)*3.0f;
            pitch=std::max(-1.0f,std::min(1.0f,pitch));
            lastTouchX=x; lastTouchY=y;
        }
    } else if(e.type==SDL_FINGERUP) {
        if(e.tfinger.fingerId==leftFinger) { leftFinger=-1; joystickX=joystickY=0; }
        if(e.tfinger.fingerId==rightFinger) rightFinger=-1;
    }
}

static void drawRocketPad(const float* vp) {
    // Launch pad placed well away from the house gate, at world position (18, -28), in front-right of the mansion.
    cube(vp,18.0f,0.10f,-28.0f,3.2f,0.18f,3.2f,0.24f,0.27f,0.30f);
    cube(vp,18.0f,0.25f,-28.0f,1.5f,0.08f,1.5f,0.42f,0.44f,0.48f);
    // Rocket body, nose, fins and engine bell.
    cube(vp,18.0f,2.0f,-28.0f,0.52f,1.45f,0.52f,0.88f,0.90f,0.94f);
    sphere(vp,18.0f,3.45f,-28.0f,0.52f,0.62f,0.52f,0.88f,0.18f,0.12f);
    cube(vp,17.38f,0.95f,-28.0f,0.22f,0.72f,0.20f,0.82f,0.12f,0.08f);
    cube(vp,18.62f,0.95f,-28.0f,0.22f,0.72f,0.20f,0.82f,0.12f,0.08f);
    sphere(vp,18.0f,0.48f,-28.0f,0.28f,0.35f,0.28f,1.0f,0.55f,0.08f);
    // Heavy launch deck, flame trench and reinforced pad markings.
    cube(vp,18.0f,0.03f,-31.0f,2.25f,0.10f,1.05f,0.10f,0.12f,0.15f);
    for(int side=-1;side<=1;side+=2) {
        cube(vp,18.0f+side*2.35f,2.8f,-28.0f,0.16f,2.8f,0.16f,0.38f,0.42f,0.46f);
        cube(vp,18.0f+side*2.35f,5.4f,-28.0f,0.14f,0.14f,0.14f,0.52f,0.56f,0.60f);
        cube(vp,18.0f+side*1.55f,4.8f,-28.0f,1.0f,0.12f,0.12f,0.50f,0.54f,0.58f);
    }
    cube(vp,18.0f,5.5f,-28.0f,4.9f,0.12f,0.14f,0.48f,0.52f,0.56f);
    // Retractable service arms reaching toward the rocket.
    cube(vp,16.2f,3.0f,-28.0f,1.45f,0.12f,0.12f,0.62f,0.66f,0.70f);
    cube(vp,19.8f,3.8f,-28.0f,1.45f,0.12f,0.12f,0.62f,0.66f,0.70f);
    // Tall cyan beacon and two glowing approach markers make the launch site easy to spot.
    cube(vp,18.0f,6.2f,-28.0f,0.10f,2.7f,0.10f,0.12f,0.78f,1.0f);
    sphere(vp,18.0f,9.0f,-28.0f,0.62f,0.62f,0.62f,0.10f,0.88f,1.0f);
    cube(vp,14.5f,0.7f,-28.0f,0.28f,0.7f,0.28f,0.10f,0.85f,1.0f);
    cube(vp,21.5f,0.7f,-28.0f,0.28f,0.7f,0.28f,0.10f,0.85f,1.0f);
}
static void drawRocketAndUniverse(const float* vp) {
    // At low altitude keep the original world visibly far below. It naturally
    // shrinks with perspective as the camera climbs; fade it out gradually.
    if(rocketAltitude<260.0f) {
        float worldFade=std::max(0.0f,1.0f-rocketAltitude/260.0f);
        cube(vp,18.0f,-0.75f,-28.0f,520.0f,0.35f,520.0f,
             0.08f*worldFade,0.30f*worldFade,0.12f*worldFade);
        cube(vp,0.0f,1.2f,-2.0f,18.0f,1.2f,24.0f,
             0.52f*worldFade,0.36f*worldFade,0.24f*worldFade);
        cube(vp,18.0f,0.8f,-28.0f,3.0f,0.8f,3.0f,
             0.24f*worldFade,0.28f*worldFade,0.32f*worldFade);
    }
    // Atmospheric layers: troposphere blue, ozone layer violet-blue, then dark space.
    if(rocketAltitude<260.0f) {
        float layer=std::max(0.0f,1.0f-rocketAltitude/260.0f);
        sphere(vp,18.0f,rocketAltitude-35.0f,-180.0f,150.0f,22.0f,150.0f,
               0.06f*layer,0.24f*layer,0.78f*layer);
        if(rocketAltitude>70.0f && rocketAltitude<230.0f) {
            float ozone=std::max(0.0f,1.0f-fabsf(rocketAltitude-145.0f)/85.0f);
            sphere(vp,18.0f,rocketAltitude-8.0f,-180.0f,165.0f,10.0f,165.0f,
                   0.18f*ozone,0.22f*ozone,0.82f*ozone);
        }
    }
    // Earth gradually appears below the rocket as a curved blue planet.
    if(rocketAltitude>35.0f) {
        float earthScale=std::min(1.0f,(rocketAltitude-35.0f)/120.0f);
        float earthRadius=210.0f;
        float earthY=-earthRadius-rocketAltitude*0.12f;
        sphere(vp,18.0f,earthY,-28.0f-260.0f,earthRadius,earthRadius,earthRadius,
               0.035f*earthScale,0.20f*earthScale,0.58f*earthScale);
        // Thin blue atmospheric halo rings.
        for(int k=0;k<3;k++) {
            float rr=earthRadius+8.0f+(float)k*7.0f;
            for(int j=0;j<36;j++) {
                float a=(float)j*6.2831853f/36.0f;
                sphere(vp,18.0f+cosf(a)*rr,earthY+2.0f,-288.0f+sinf(a)*rr,
                       3.2f,3.2f,3.2f,0.08f,0.42f,0.95f);
            }
        }
    }
    // Stars fade in as the rocket leaves the atmosphere.
    float starFade=std::max(0.0f,std::min(1.0f,(rocketAltitude-22.0f)/45.0f));
    for(int i=0;i<150;i++) {
        float x=playerX + (float)(((i*73)%211)-105)*2.0f;
        float y=rocketAltitude + (float)(((i*47)%137)-68)*1.7f;
        float z=playerZ-95.0f-(float)((i*29)%120);
        float r=(i%7==0)?0.10f:0.045f;
        sphere(vp,x,y,z,r*starFade,r*starFade,r*starFade,0.78f,0.84f,1.0f);
    }
    // Sun fades into the distance and disappears after leaving the atmosphere.
    if(rocketAltitude<55.0f) {
        float fade=1.0f-rocketAltitude/55.0f;
        sphere(vp,playerX+28.0f,rocketAltitude+36.0f,playerZ-190.0f,
               8.0f,8.0f,8.0f,1.0f,0.78f*fade+0.02f,0.18f*fade+0.01f);
    }
    // Eight major planets, arranged along a navigable route ahead of the rocket.
    const float px[8]={-220,-130,0,150,360,610,900,1240};
    const float py[8]={-55,70,0,-45,95,-35,80,-60};
    const float pz[8]={-520,-760,-1040,-1360,-1780,-2240,-2760,-3320};
    const float radius[8]={12.0f,20.0f,24.0f,16.0f,48.0f,42.0f,34.0f,32.0f};
    const float colors[8][3]={{0.58f,0.55f,0.50f},{0.92f,0.66f,0.28f},
      {0.12f,0.42f,0.95f},{0.86f,0.20f,0.12f},{0.82f,0.58f,0.34f},
      {0.88f,0.78f,0.48f},{0.35f,0.78f,0.82f},{0.42f,0.16f,0.62f}};
    for(int i=0;i<8;i++) {
        float x=18.0f+px[i];
        float y=rocketAltitude+py[i];
        float z=-28.0f+pz[i];
        sphere(vp,x,y,z,radius[i],radius[i],radius[i],colors[i][0],colors[i][1],colors[i][2]);
        if(i==5) {
            // Saturn's visible ring, built from short orbiting blocks.
            for(int k=0;k<24;k++) {
                float a=(float)k*6.2831853f/24.0f;
                cubeYaw(vp,x+cosf(a)*9.0f,y,z+sinf(a)*9.0f,1.0f,0.10f,0.45f,a,0.88f,0.76f,0.48f);
            }
        }
    }
    // A distant black hole with a glowing accretion ring.
    float bhx=playerX+185.0f, bhy=rocketAltitude+28.0f, bhz=playerZ-1080.0f;
    sphere(vp,bhx,bhy,bhz,13.0f,13.0f,13.0f,0.002f,0.002f,0.008f);
    for(int k=0;k<32;k++) {
        float a=(float)k*6.2831853f/32.0f;
        cubeYaw(vp,bhx+cosf(a)*19.0f,bhy+2.0f,bhz+sinf(a)*19.0f,
                2.2f,0.22f,0.65f,a,0.95f,0.36f,0.08f);
    }
    // Detailed reusable launch vehicle silhouette; camera remains behind the cockpit.
    float bodyY=rocketAltitude+1.35f;
    float bodyX=playerX+sinf(rocketYaw)*0.20f;
    float bodyZ=playerZ+cosf(rocketYaw)*0.20f;
    float sway=sinf(rocketExhaustPhase*0.31f)*rocketShake*0.10f;
    cubeSlopeX(vp,bodyX+sway,bodyY,bodyZ,0.92f,2.75f,0.92f,rocketPitch,0.78f,0.82f,0.88f);
    sphere(vp,bodyX+sway,rocketAltitude+3.15f,bodyZ,0.91f,0.72f,0.91f,0.90f,0.92f,0.95f);
    // Black thermal-protection nose cap and cockpit window.
    sphere(vp,bodyX+sway,rocketAltitude+3.66f,bodyZ,0.56f,0.30f,0.56f,0.12f,0.17f,0.22f);
    sphere(vp,bodyX+sway,rocketAltitude+2.60f,bodyZ-0.73f,0.31f,0.42f,0.12f,0.08f,0.42f,0.62f);
    // Orange interstage band and white tank markings.
    cube(vp,bodyX+sway,rocketAltitude+0.52f,bodyZ,0.98f,0.13f,0.98f,0.88f,0.36f,0.08f);
    for(int k=0;k<4;k++)
        cube(vp,bodyX+sway,rocketAltitude+1.05f+k*0.48f,bodyZ-0.935f,0.12f,0.045f,0.025f,0.18f,0.24f,0.30f);
    // Four aerodynamic grid fins around the lower stage.
    for(int k=0;k<4;k++) {
        float a=k*1.5707963f+rocketYaw;
        float fx=bodyX+cosf(a)*0.90f;
        float fz=bodyZ+sinf(a)*0.90f;
        cubeYaw(vp,fx,rocketAltitude+0.05f,fz,0.18f,0.72f,0.12f,a,0.72f,0.16f,0.10f);
    }
    // Two side boosters, with dark engine bells.
    for(int side=-1;side<=1;side+=2) {
        float bx=bodyX+side*0.78f;
        cubeSlopeX(vp,bx,rocketAltitude+0.35f,bodyZ,0.34f,2.05f,0.34f,rocketPitch,0.82f,0.84f,0.87f);
        sphere(vp,bx,rocketAltitude-0.72f,bodyZ,0.30f,0.30f,0.30f,0.16f,0.18f,0.20f);
    }
    // Main engine bell and three animated exhaust cores.
    sphere(vp,bodyX,rocketAltitude-0.88f,bodyZ,0.42f,0.34f,0.42f,0.13f,0.15f,0.18f);
    if(rocketEngineGlow>0.04f) {
        float pulse=0.82f+0.18f*sinf(rocketExhaustPhase);
        float plume=(0.8f+rocketThrottle*2.6f)*pulse*rocketEngineGlow;
        for(int side=-1;side<=1;side++) {
            float ex=bodyX+side*0.78f;
            float length=plume*(side==0?1.0f:0.78f);
            sphere(vp,ex,rocketAltitude-1.35f-length*0.55f,bodyZ,
                   0.20f+length*0.10f,length*0.72f,0.20f+length*0.10f,
                   1.0f,0.36f+0.30f*pulse,0.06f);
            sphere(vp,ex,rocketAltitude-1.55f-length*0.42f,bodyZ,
                   0.11f+length*0.055f,length*0.52f,0.11f+length*0.055f,
                   1.0f,0.82f,0.34f);
        }
        // Heat shimmer / glowing exhaust core.
        sphere(vp,bodyX,rocketAltitude-1.8f-plume*0.42f,bodyZ,
               0.22f,plume*0.48f,0.22f,0.78f,0.88f,1.0f);
    }
    // Stage-separation indicator: a visible amber band remains around the interstage.
    if(rocketStageSeparated)
        sphere(vp,bodyX,rocketAltitude+0.48f,bodyZ,1.02f,0.10f,1.02f,1.0f,0.55f,0.10f);
}

static void render(float ghostX,float ghostZ) {
    float p[16],v[16],vp[16];
    perspective(p,scopeMode?PI/8.5f:PI/3.0f,(float)gWidth/(float)gHeight,0.1f,scopeMode?1400.0f:900.0f);
    viewMatrix(v); multiply(vp,p,v);
    glClearColor(nightMode?0.012f:0.42f,nightMode?0.018f:0.72f,nightMode?0.075f:0.94f,1.0f); // day/night sky
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glUseProgram(gProgram);
    glUniform3f(gLight, -0.35f,nightMode?0.20f:0.85f,0.4f);
    if(inRocket) {
        glClearColor(0.0f,0.0f,0.012f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        drawRocketAndUniverse(vp);
        drawHUD();
        return;
    }
    if(onPlanetSurface) {
        const float planetColors[8][3]={{0.42f,0.40f,0.38f},{0.72f,0.48f,0.22f},
          {0.12f,0.38f,0.72f},{0.58f,0.16f,0.10f},{0.54f,0.36f,0.22f},
          {0.70f,0.58f,0.34f},{0.20f,0.55f,0.60f},{0.20f,0.10f,0.30f}};
        int pi=std::max(0,std::min(7,currentPlanet));
        glClearColor(0.0f,0.0f,0.012f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        cube(vp,playerX, -0.5f, playerZ, 800.0f,1.0f,800.0f,
             planetColors[pi][0],planetColors[pi][1],planetColors[pi][2]);
        // Nearby craters, rocks and the parked lander give the surface a sense of scale.
        for(int k=0;k<18;k++) {
            float ox=(float)((k*37)%31)-15.0f, oz=(float)((k*23)%37)-18.0f;
            sphere(vp,playerX+ox,0.0f,playerZ+oz,1.0f+(k%3),0.35f,1.2f,
                   planetColors[pi][0]*0.62f,planetColors[pi][1]*0.62f,planetColors[pi][2]*0.62f);
        }
        // Nebulon alien sky: ringed gas giant, violet moons and luminous horizon.
        sphere(vp,playerX+34.0f,28.0f,playerZ-95.0f,18.0f,18.0f,18.0f,0.34f,0.18f,0.48f);
        for(int r=0;r<28;r++) {
            float a=(float)r*6.2831853f/28.0f+homeworldTime*0.025f;
            sphere(vp,playerX+34.0f+cosf(a)*25.0f,28.0f+sinf(a)*3.0f,
                   playerZ-95.0f+sinf(a)*25.0f,2.0f,0.45f,1.2f,0.76f,0.48f,0.90f);
        }
        sphere(vp,playerX-30.0f,18.0f,playerZ-78.0f,7.0f,7.0f,7.0f,0.60f,0.44f,0.72f);
        // Jagged alien crystal formations and distant basalt spires.
        for(int k=0;k<16;k++) {
            float ox=(float)((k*31)%75)-37.0f, oz=-28.0f-(float)((k*19)%42);
            cubeYaw(vp,playerX+ox,1.8f,playerZ+oz,0.55f,2.2f+(k%4)*0.65f,0.55f,
                    (float)k*0.41f,0.18f,0.78f,0.72f);
            sphere(vp,playerX+ox,3.9f+(k%4)*0.55f,playerZ+oz,0.42f,0.75f,0.42f,
                   0.20f,0.95f,0.84f);
        }
        // Alien fortress and prison block containing the kidnapped school children.
        float prisonX=playerX, prisonZ=playerZ-15.0f;
        cube(vp,prisonX,2.2f,prisonZ,8.0f,2.2f,5.0f,0.16f,0.18f,0.26f);
        cube(vp,prisonX,4.5f,prisonZ,5.0f,0.18f,3.2f,0.24f,0.32f,0.44f);
        for(int c=0;c<2;c++) {
            float cx=prisonX+(c==0?-2.0f:2.0f);
            cube(vp,cx,1.6f,prisonZ-2.7f,1.35f,1.55f,0.12f,
                 homeworldPrisonOpen?0.12f:0.08f,homeworldPrisonOpen?0.75f:0.38f,0.92f);
            for(int b=0;b<5;b++)
                cube(vp,cx-0.62f+b*0.31f,1.65f,prisonZ-2.82f,
                     0.035f,1.45f,0.035f,0.18f,0.88f,0.98f);
            // Child silhouettes visible inside each cell until rescued.
            if(!homeworldPrisonOpen) {
                cube(vp,cx,1.25f,prisonZ-3.15f,0.26f,0.48f,0.18f,
                     c==0?0.16f:0.88f,c==0?0.55f:0.30f,c==0?0.92f:0.18f);
                sphere(vp,cx,1.72f,prisonZ-3.15f,0.17f,0.18f,0.16f,0.88f,0.68f,0.52f);
            }
        }
        // Alien guards are separate combatants; shoot with the existing AK-47.
        for(const HomeworldGuard& g:homeworldGuards) if(g.alive) {
            float flash=g.flash>0.0f?1.0f:0.34f;
            cubeYaw(vp,g.x,1.05f,g.z,0.38f,0.55f,0.30f,g.yaw,flash,0.28f,0.72f);
            sphere(vp,g.x,1.72f,g.z,0.29f,0.28f,0.29f,0.24f,0.88f,0.58f);
            sphere(vp,g.x-0.11f,1.77f,g.z-0.20f,0.055f,0.045f,0.045f,1.0f,0.08f,0.10f);
            sphere(vp,g.x+0.11f,1.77f,g.z-0.20f,0.055f,0.045f,0.045f,1.0f,0.08f,0.10f);
            cubeYaw(vp,g.x+0.36f,1.15f,g.z-0.32f,0.09f,0.09f,0.48f,g.yaw,0.10f,0.12f,0.16f);
        }
        // Glowing rescue beacon at the prison entrance.
        sphere(vp,prisonX,5.3f,prisonZ-3.8f,0.42f,0.42f,0.42f,
               homeworldPrisonOpen?0.10f:0.10f,homeworldPrisonOpen?1.0f:0.35f,1.0f);
        drawHUD();
        return;
    }
    // Large green world ground.
    cube(vp,150,-0.65f,130,700,0.35f,700,0.20f,0.52f,0.14f);
    drawGarden(vp);
    drawRocketPad(vp);
    if(childrenGoingStudy) {
        for(int child=0;child<2;child++) {
            float cx=childrenAtStudyDesk?(child==0?-1.35f:1.35f):childWorldX[child];
            float cz=childrenAtStudyDesk?-8.05f:childWorldZ[child];
            bool seated=childrenAtStudyDesk;
            float walk=sinf(childWalkPhase+child*1.2f)*0.22f;
            cubeYaw(vp,cx,seated?0.93f:0.88f,cz-0.08f,0.24f,0.34f,0.18f,
                    seated?0.10f:walk,child==0?0.12f:0.92f,
                    child==0?0.48f:0.28f,child==0?0.92f:0.18f);
            sphere(vp,cx,seated?1.38f:1.43f,cz-0.10f,0.21f,0.22f,0.20f,0.91f,0.70f,0.55f);
            sphere(vp,cx,seated?1.51f:1.56f,cz-0.12f,0.22f,0.10f,0.21f,0.10f,0.06f,0.035f);
            cubeYaw(vp,cx-0.25f,seated?1.05f:0.95f,cz-0.18f,0.07f,0.22f,0.07f,
                    seated?-0.20f:walk,0.91f,0.70f,0.55f);
            cubeYaw(vp,cx+0.25f,seated?1.05f:0.95f,cz-0.18f,0.07f,0.22f,0.07f,
                    seated?0.20f:-walk,0.91f,0.70f,0.55f);
            if(seated) cube(vp,cx,1.08f,-9.35f,0.28f,0.025f,0.20f,0.94f,0.92f,0.82f);
            else {
                cubeYaw(vp,cx-0.12f,0.35f+fabsf(walk)*0.25f,cz-0.06f,
                        0.075f,0.25f,0.075f,walk,0.12f,0.16f,0.25f);
                cubeYaw(vp,cx+0.12f,0.35f+fabsf(walk)*0.25f,cz+0.06f,
                        0.075f,0.25f,0.075f,-walk,0.12f,0.16f,0.25f);
            }
        }
    }
    drawRoadParkingMountains(vp);
    drawGardenBuildingsAndCars(vp);
    drawMegaCity(vp);
    drawExpandedMegaCity(vp);
    drawGLBajajCollege(vp);
    if(playerX>-155.0f && playerX<-135.0f && playerZ>-38.0f && playerZ<-17.0f) {
        if(collegeBookOpen) {
        }
    }
    drawCollegeAccessRoad(vp);
    drawAyushCollegeLane(vp);
    drawAyushLaneTrees(vp);
    drawAirport(vp);
    drawAirplane(vp);
    drawHouse(vp);
    drawNightPhone(vp);
    drawAyushMishraHouse(vp);
    drawAyushBike(vp);
    drawDeliveryRider(vp);
    drawAyushWalkingFriend(vp);
    drawRooftopComplex(vp);
    drawSunAndSky(vp);
    drawItems(vp);
    drawAnimals(vp);
    drawNPCs(vp);
    drawAlienInvasion(vp);
    // Keep command help out of the 3D world view; the touch command panel remains.
    drawHorse(vp);
    drawFemaleCompanion(vp);
    drawThrownFruits(vp);
    drawPlayerAvatar(vp);
    if(groceryBags>0 && !drivingCar && !groceriesInCar) {
        cube(vp,playerX+0.42f,playerHeight+0.72f,playerZ-0.35f,
             0.22f,0.25f,0.18f,0.78f,0.58f,0.30f);
    }
    if(groceryBags>0 && groceriesInCar) {
        for(int bag=0;bag<std::min(groceryBags,6);bag++)
            cube(vp,carX0-0.55f+(bag%3)*0.48f,0.72f+(bag/3)*0.28f,
                 carZ0+1.45f,0.20f,0.24f,0.18f,0.78f,0.58f,0.30f);
    }
    drawHUD();
}
int main(int argc,char** argv) {
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_AUDIO)!=0) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    gWindow=SDL_CreateWindow("HAUNTED HOUSE 3D V50",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        gWidth,gHeight,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_SHOWN);
    if(!gWindow) { SDL_Quit(); return 1; }
    gContext=SDL_GL_CreateContext(gWindow);
    if(!gContext) { SDL_DestroyWindow(gWindow); SDL_Quit(); return 1; }
    SDL_GL_SetSwapInterval(1);
    initEngineAudio();
    initNPCs();
    loadNPCMemory();
    initShaders(); initMeshes(); setupCollision();
    float ghostX=5.5f,ghostZ=-11.0f;
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL);
    Uint32 previous=SDL_GetTicks();
    while(gRunning) {
        Uint32 now=SDL_GetTicks();
        float dt=std::min(0.05f,(now-previous)/1000.0f); previous=now;
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_QUIT) gRunning=false;
            if(e.type==SDL_KEYDOWN) {
                if(npcCommandTyping) {
                    if(e.key.keysym.sym==SDLK_RETURN || e.key.keysym.sym==SDLK_KP_ENTER) {
                        executeNPCCommand(); npcCommandTyping=false; SDL_StopTextInput();
                    } else if(e.key.keysym.sym==SDLK_BACKSPACE && !npcCommandBuffer.empty()) {
                        npcCommandBuffer.pop_back();
                    } else if(e.key.keysym.sym==SDLK_ESCAPE) {
                        npcCommandTyping=false; npcCommandBuffer.clear(); SDL_StopTextInput();
                    }
                    continue;
                }
                switch(e.key.keysym.sym) {
                    case SDLK_ESCAPE: gRunning=false; break;
                    case SDLK_e: tryUseNearby(); break;
                    case SDLK_u: if(inRocket) { rocketLanded=!rocketLanded; } break;
                    case SDLK_t: npcCommandTyping=true; npcCommandBuffer.clear(); SDL_StartTextInput(); break;
                    case SDLK_1: if(supermarketCheckoutOpen) buySupermarketProduct(0); break;
                    case SDLK_2: if(supermarketCheckoutOpen) buySupermarketProduct(1); break;
                    case SDLK_3: if(supermarketCheckoutOpen) buySupermarketProduct(2); break;
                    case SDLK_4: if(supermarketCheckoutOpen) buySupermarketProduct(3); break;
                    case SDLK_5: if(supermarketCheckoutOpen) buySupermarketProduct(4); break;
                    case SDLK_6: if(supermarketCheckoutOpen) buySupermarketProduct(5); break;
                    case SDLK_7: if(supermarketCheckoutOpen) buySupermarketProduct(6); break;
                    case SDLK_8: if(supermarketCheckoutOpen) buySupermarketProduct(7); break;
                    case SDLK_f: gFlashlight=!gFlashlight; break;
                    case SDLK_l: if(drivingCar) carHeadlightsOn=!carHeadlightsOn; break;
                    case SDLK_SPACE: shoot(); break;
                    case SDLK_q: toggleScope(); break;
                    case SDLK_i: startAlienInvasion(); break;
                    case SDLK_j: startJump(); break;
                    case SDLK_g: throwFruit(); break;
                    case SDLK_r: reloadWeapon(); break;
                }
            }
            if(e.type==SDL_TEXTINPUT && npcCommandTyping) {
                if(npcCommandBuffer.size()<80) npcCommandBuffer+=e.text.text;
            }
            if(e.type==SDL_FINGERDOWN||e.type==SDL_FINGERMOTION||e.type==SDL_FINGERUP) handleTouch(e);
            if(e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_LEFT) {
                mouseLooking=true; SDL_SetRelativeMouseMode(SDL_TRUE);
            }
            if(e.type==SDL_MOUSEBUTTONUP) { mouseLooking=false; SDL_SetRelativeMouseMode(SDL_FALSE); }
            if(e.type==SDL_MOUSEMOTION && mouseLooking) {
                yaw-=e.motion.xrel*0.004f; pitch-=e.motion.yrel*0.004f;
                pitch=std::max(-1.0f,std::min(1.0f,pitch));
            }
        }
        const Uint8* k=SDL_GetKeyboardState(nullptr);
        if(leftFinger==-1) {
            joystickX=0; joystickY=0;
            if(k[SDL_SCANCODE_W]||k[SDL_SCANCODE_UP]) joystickY=1;
            if(k[SDL_SCANCODE_S]||k[SDL_SCANCODE_DOWN]) joystickY=-1;
            if(k[SDL_SCANCODE_A]||k[SDL_SCANCODE_LEFT]) joystickX=-1;
            if(k[SDL_SCANCODE_D]||k[SDL_SCANCODE_RIGHT]) joystickX=1;
        }
        updateGame(dt,ghostX,ghostZ,ghostX,ghostZ);
        static float titleTimer=0.0f; titleTimer+=dt;
        if(titleTimer>0.5f) {
            char title[160];
            std::snprintf(title,sizeof(title),"HAUNTED HOUSE 3D | NPC ORDER: TAP TOP-RIGHT | Score: %d",score);
            SDL_SetWindowTitle(gWindow,title); titleTimer=0.0f;
        }
        SDL_GL_GetDrawableSize(gWindow,&gWidth,&gHeight);
        if(gWidth<1) gWidth=1; if(gHeight<1) gHeight=1;
        glViewport(0,0,gWidth,gHeight);
        render(ghostX,ghostZ);
        SDL_GL_SwapWindow(gWindow);
    }
    saveNPCMemory();
    glDeleteBuffers(1,&gCubeVBO); glDeleteBuffers(1,&gSphereVBO);
    glDeleteProgram(gProgram);
    SDL_GL_DeleteContext(gContext); SDL_DestroyWindow(gWindow); SDL_Quit();
    return 0;
}
