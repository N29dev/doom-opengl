#include "Combat.hpp"
#include <iostream>
int main() {
    if (combat::rayCount(1)!=1 || combat::rayCount(2)!=5 ||
        combat::hitDamage(1,true,0,24)!=25 || combat::hitDamage(1,false,0,24)!=40 ||
        combat::hitDamage(2,true,0,14)!=55 || combat::hitDamage(2,true,14,14)!=12 ||
        combat::hitDamage(2,true,2,14)<=combat::hitDamage(2,true,10,14)) {
        std::cerr << "Weapon damage regression\n"; return 1;
    }
    return 0;
}
