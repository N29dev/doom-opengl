#pragma once

#include <string>
#include <vector>

// Small, renderer-independent grid format. Each tile is 2x2 world units.
// '#' wall, '.' floor, 'P' player, 'D' door, 'M' enemy,
// 'H' health pickup, 'X' exit.
struct GridPosition {
    int column = 0;
    int row = 0;
};

class Level {
public:
    static constexpr float kTileSize = 2.0f;

    bool load(const std::string& filePath, std::string& error);

    int width() const { return static_cast<int>(tiles_.empty() ? 0 : tiles_.front().size()); }
    int height() const { return static_cast<int>(tiles_.size()); }
    char tileAt(int column, int row) const;

    float worldX(int column) const;
    float worldZ(int row) const;
    float spawnX() const { return worldX(spawn_.column); }
    float spawnZ() const { return worldZ(spawn_.row); }

    bool isDoorOpen(int column, int row) const;
    bool isHealthCollected(int column, int row) const;
    bool isSolid(int column, int row) const;
    bool circleBlocked(float worldX, float worldZ, float radius) const;
    bool toggleNearbyDoor(float worldX, float worldZ);
    bool collectNearbyHealth(float worldX, float worldZ);
    bool reachedExit(float worldX, float worldZ) const;

    std::vector<GridPosition> positions(char symbol) const;

private:
    int index(int column, int row) const { return row * width() + column; }

    std::vector<std::string> tiles_;
    std::vector<bool> openDoors_;
    std::vector<bool> collectedHealth_;
    GridPosition spawn_;
};
