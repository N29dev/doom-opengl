#pragma once

#include <string>
#include <vector>

// Small, renderer-independent grid format. Each tile is 2x2 world units.
// '#' wall, '.' floor, 'P' player, 'D' door, 'M' enemy,
// 'H' health pickup, 'S' shotgun pickup, 'F' flying enemy,
// 'B' big blue enemy, 'X' exit.
struct GridPosition {
    int column = 0;
    int row = 0;
};

// Platforms occupy a 2 x 2 tile footprint, with independent bottom and top heights.
struct Platform {
    int column = 0;
    int row = 0;
    float bottom = 0.0f;
    float top = 1.0f;
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
    bool isShotgunCollected(int column, int row) const;
    bool isSolid(int column, int row) const;
    bool circleBlocked(float worldX, float worldZ, float radius) const;
    bool bodyBlocked(float x, float z, float radius, float feet, float head) const;
    bool landingHeight(float x, float z, float radius, float from, float to, float& height) const;
    bool ceilingLimit(float x, float z, float radius, float fromHead, float toHead, float& height) const;
    float surfaceAt(int column, int row) const;
    float ceilingHeight() const { return ceilingHeight_; }
    bool hasLava() const { return hasLava_; }
    bool hasPlatforming() const { return hasLava_ || !platforms_.empty(); }
    bool hasFloor(int column, int row) const;
    const std::vector<Platform>& platforms() const { return platforms_; }
    float spawnFloor() const { return surfaceAt(spawn_.column, spawn_.row); }
    bool toggleNearbyDoor(float worldX, float worldZ);
    bool collectNearbyHealth(float worldX, float worldZ, float feet = 0.0f);
    bool collectNearbyShotgun(float worldX, float worldZ, float feet = 0.0f);
    bool reachedExit(float worldX, float worldZ, float feet = 0.0f) const;

    std::vector<GridPosition> positions(char symbol) const;

private:
    int index(int column, int row) const { return row * width() + column; }

    std::vector<std::string> tiles_;
    std::vector<bool> openDoors_;
    std::vector<bool> collectedHealth_;
    std::vector<bool> collectedShotgun_;
    GridPosition spawn_;
    std::vector<Platform> platforms_;
    std::vector<float> surfaceHeights_;
    float ceilingHeight_ = 4.5f;
    bool hasLava_ = false;
    bool overlapsTile(float x, float z, float radius, int column, int row) const;
    bool nearSurface(float feet, int column, int row) const;
};
