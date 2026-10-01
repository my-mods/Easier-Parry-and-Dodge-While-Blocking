#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace EasierParry {
struct Settings {int riposteDirection{1};bool debugLogging{};};
// Match the native attack-to-block conversion for cardinal hit directions.
// Non-directional attacks retain vanilla selection; never call its RNG fallback.
inline uint8_t parriedBlock(uint8_t attack) {
    return attack==1?8:attack==2?4:attack==3?1:attack==4?2:0;
}
// Indices follow the player's visible directions: top, bottom, left, right.
inline int riposteSide(uint8_t block,int mode) {
    int side=block==1?0:block==2?1:block==4?2:block==8?3:-1;
    return side<0||mode<1||mode>2?-1:mode==1?(side^1):side;
}
}
