#include "Level.hpp"
#include <cmath>
#include <iostream>
#include <string>
int main(int argc, char** argv) {
    if (argc != 2) return 1;
    Level l; std::string error;
    if (!l.load(argv[1],error)) { std::cerr << error << '\n'; return 1; }
    int failed = 0;
    auto check = [&](bool ok,const char* what) { if (!ok) { std::cerr << what << '\n'; ++failed; } };
    float y = 0;
    const float x = l.worldX(6), z = l.worldZ(3);
    check(l.hasLava() && !l.hasFloor(6,3),"lava floor");
    check(l.landingHeight(x,z,0.33f,3,0,y) && std::abs(y-1.4f)<0.001f,"landing on elevated box");
    check(!l.landingHeight(l.worldX(5),z,0.33f,0,-1,y),"gap has no ground");
    check(l.bodyBlocked(x,z,0.33f,0.7f,2.3f),"side collision");
    check(!l.bodyBlocked(x,z,0.33f,1.4f,3.0f),"standing on top");
    check(l.ceilingLimit(x,z,0.33f,0.9f,1.3f,y) && std::abs(y-1.1f)<0.001f,"underside collision");
    check(l.ceilingLimit(x,z,0.33f,9.9f,10.1f,y) && std::abs(y-10)<0.001f,"room ceiling");
    check(!l.reachedExit(l.worldX(15),z,0),"cannot exit below raised pad");
    check(l.reachedExit(l.worldX(15),z,4.5f),"raised exit");
    check(l.landingHeight(l.spawnX(),l.spawnZ(),0.33f,0,-0.1f,y) && std::abs(y)<0.001f,"ordinary floor");
    if (failed) return 1;
    std::cout << "Platform collision tests passed\n";
    return 0;
}
