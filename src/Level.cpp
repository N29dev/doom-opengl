#include "Level.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <queue>
#include <limits>

namespace {
    bool near(float x1, float z1, float x2, float z2, float distance) {
        const float dx = x1 - x2;
        const float dz = z1 - z2;
        return dx * dx + dz * dz <= distance * distance;
    }
}

bool Level::load(const std::string& filePath, std::string& error) {
    std::ifstream file(filePath);
    if (!file) {
        error = "Cannot open level: " + filePath;
        return false;
    }

    nlohmann::json data;
    try {
        file >> data;
        if (!data.is_object() || !data.contains("tiles") || !data["tiles"].is_array()) {
            error = "Level JSON must contain a 'tiles' array of strings.";
            return false;
        }

        const auto& rows = data["tiles"];
        if (rows.size() < 3 || rows.size() > 128 || !rows[0].is_string()) {
            error = "Level must be 3..128 rows of equal-length strings.";
            return false;
        }

        const int newWidth = static_cast<int>(rows[0].get<std::string>().size());
        if (newWidth < 3 || newWidth > 128) {
            error = "Level width must be between 3 and 128 tiles.";
            return false;
        }

        std::vector<std::string> tiles;
        int spawnCount = 0;
        int exitCount = 0;
        GridPosition newSpawn{};

        for (int row = 0; row < static_cast<int>(rows.size()); ++row) {
            if (!rows[row].is_string()) {
                error = "Every entry in 'tiles' must be a string.";
                return false;
            }
            std::string line = rows[row].get<std::string>();
            if (static_cast<int>(line.size()) != newWidth) {
                error = "All level rows must have the same width.";
                return false;
            }
            for (int column = 0; column < newWidth; ++column) {
                const char tile = line[column];
                if (std::string("#.PDMHSFBX~").find(tile) == std::string::npos) {
                    error = "Unknown tile character at row " + std::to_string(row);
                    return false;
                }
                if (tile == 'P') {
                    ++spawnCount;
                    newSpawn = {column, row};
                }
                if (tile == 'X') ++exitCount;
            }
            tiles.push_back(std::move(line));
        }

        if (spawnCount != 1 || exitCount != 1) {
            error = "A level needs exactly one P spawn and one X exit.";
            return false;
        }

        // Optional geometry extends the original format without changing flat maps.
        const float newCeiling = data.value("ceilingHeight", 4.5f);
        if (!std::isfinite(newCeiling) || newCeiling < 4.5f || newCeiling > 32.0f) {
            error = "ceilingHeight must be between 4.5 and 32."; return false;
        }
        std::vector<Platform> newPlatforms;
        const int mapHeight = static_cast<int>(tiles.size());
        std::vector<bool> occupied(static_cast<std::size_t>(newWidth) * mapHeight, false);
        if (data.contains("platforms")) {
            const auto& items = data["platforms"];
            if (!items.is_array() || items.size() > 4096) {
                error = "platforms must be an array of at most 4096 boxes."; return false;
            }
            for (const auto& item : items) {
                if (!item.is_object() || !item.contains("column") || !item.contains("row") ||
                    !item["column"].is_number_integer() || !item["row"].is_number_integer()) {
                    error = "Each platform needs integer column and row."; return false;
                }
                Platform p{item.at("column").get<int>(), item.at("row").get<int>(),
                           item.at("bottom").get<float>(), item.at("top").get<float>()};
                if (p.column < 0 || p.column >= newWidth || p.row < 0 || p.row >= mapHeight ||
                    !std::isfinite(p.bottom) || !std::isfinite(p.top) || p.bottom < 0.0f ||
                    p.top - p.bottom < 0.099f || p.top > newCeiling - 1.6f) {
                    error = "Invalid platform bounds or insufficient ceiling clearance."; return false;
                }
                const int index = p.row * newWidth + p.column;
                const char tile = tiles[p.row][p.column];
                if (occupied[index] || tile == '#' || tile == 'D') {
                    error = "Only one platform per tile; walls and doors cannot contain platforms."; return false;
                }
                const float requiredClearance = tile == 'B' ? 3.1f : (tile == 'F' ? 3.8f : 1.6f);
                if (p.top + requiredClearance > newCeiling) {
                    error = "Platform entity has insufficient ceiling clearance."; return false;
                }
                occupied[index] = true;
                newPlatforms.push_back(p);
            }
        }

        // Level validation is shared by the game and the command-line tool.
        // Doors can be opened during gameplay, so only walls block reachability.
        const int newHeight = static_cast<int>(tiles.size());
        std::vector<bool> visited(static_cast<std::size_t>(newWidth) * newHeight, false);
        std::queue<GridPosition> frontier;
        visited[newSpawn.row * newWidth + newSpawn.column] = true;
        frontier.push(newSpawn);
        bool reachable = false;

        constexpr int dx[] = {1, -1, 0, 0};
        constexpr int dz[] = {0, 0, 1, -1};
        while (!frontier.empty()) {
            const GridPosition cell = frontier.front();
            frontier.pop();
            if (tiles[cell.row][cell.column] == 'X') {
                reachable = true;
                break;
            }
            for (int direction = 0; direction < 4; ++direction) {
                const int nextColumn = cell.column + dx[direction];
                const int nextRow = cell.row + dz[direction];
                if (nextColumn < 0 || nextColumn >= newWidth ||
                    nextRow < 0 || nextRow >= newHeight) continue;
                const int nextIndex = nextRow * newWidth + nextColumn;
                if (visited[nextIndex] || tiles[nextRow][nextColumn] == '#') continue;
                visited[nextIndex] = true;
                frontier.push({nextColumn, nextRow});
            }
        }

        if (!reachable) {
            error = "Exit X cannot be reached from spawn P (walls block the path).";
            return false;
        }

        hasLava_ = std::any_of(tiles.begin(), tiles.end(), [](const std::string& row) {
            return row.find('~') != std::string::npos;
        });
        platforms_ = std::move(newPlatforms);
        ceilingHeight_ = newCeiling;
        tiles_ = std::move(tiles);
        spawn_ = newSpawn;
        openDoors_.assign(width() * height(), false);
        collectedHealth_.assign(width() * height(), false);
        collectedShotgun_.assign(width() * height(), false);
        error.clear();
        return true;
    } catch (const nlohmann::json::exception& e) {
        error = std::string("Invalid level JSON: ") + e.what();
        return false;
    }
}

char Level::tileAt(int column, int row) const {
    if (column < 0 || column >= width() || row < 0 || row >= height()) return '#';
    return tiles_[row][column];
}

float Level::worldX(int column) const {
    return (static_cast<float>(column) + 0.5f - width() * 0.5f) * kTileSize;
}

float Level::worldZ(int row) const {
    return (static_cast<float>(row) + 0.5f - height() * 0.5f) * kTileSize;
}

bool Level::isDoorOpen(int column, int row) const {
    return tileAt(column, row) == 'D' && openDoors_[index(column, row)];
}

bool Level::isHealthCollected(int column, int row) const {
    return tileAt(column, row) == 'H' && collectedHealth_[index(column, row)];
}

bool Level::isShotgunCollected(int column, int row) const {
    return tileAt(column, row) == 'S' && collectedShotgun_[index(column, row)];
}

bool Level::isSolid(int column, int row) const {
    const char tile = tileAt(column, row);
    return tile == '#' || (tile == 'D' && !isDoorOpen(column, row));
}

bool Level::collectNearbyShotgun(float x, float z, float feet) {
    for (int row = 0; row < height(); ++row) {
        for (int column = 0; column < width(); ++column) {
            if (tileAt(column, row) != 'S' || isShotgunCollected(column, row)) continue;
            if (near(x, z, worldX(column), worldZ(row), 1.0f) && nearSurface(feet, column, row)) {
                collectedShotgun_[index(column, row)] = true;
                return true;
            }
        }
    }
    return false;
}

bool Level::circleBlocked(float x, float z, float radius) const {
    // Collision against nearby wall tile AABBs; outside the map is solid.
    const auto cellX = [&](float v) {
        return static_cast<int>(std::floor(v / kTileSize + width() * 0.5f));
    };
    const auto cellZ = [&](float v) {
        return static_cast<int>(std::floor(v / kTileSize + height() * 0.5f));
    };

    for (int row = cellZ(z - radius); row <= cellZ(z + radius); ++row) {
        for (int column = cellX(x - radius); column <= cellX(x + radius); ++column) {
            if (!isSolid(column, row)) continue;
            const float half = kTileSize * 0.5f;
            const float nearestX = std::clamp(x, worldX(column) - half, worldX(column) + half);
            const float nearestZ = std::clamp(z, worldZ(row) - half, worldZ(row) + half);
            const float dx = x - nearestX;
            const float dz = z - nearestZ;
            if (dx * dx + dz * dz < radius * radius) return true;
        }
    }
    return false;
}

bool Level::hasFloor(int column, int row) const {
    return column >= 0 && column < width() && row >= 0 && row < height() &&
           tileAt(column, row) != '~';
}

bool Level::overlapsTile(float x, float z, float radius, int column, int row) const {
    const float half = kTileSize * 0.5f;
    const float dx = x - std::clamp(x, worldX(column) - half, worldX(column) + half);
    const float dz = z - std::clamp(z, worldZ(row) - half, worldZ(row) + half);
    return dx * dx + dz * dz < radius * radius;
}

float Level::surfaceAt(int column, int row) const {
    for (const auto& p : platforms_)
        if (p.column == column && p.row == row) return p.top;
    return 0.0f;
}

bool Level::nearSurface(float feet, int column, int row) const {
    return !hasPlatforming() || std::abs(feet - surfaceAt(column, row)) <= 0.5f;
}

bool Level::bodyBlocked(float x, float z, float radius, float feet, float head) const {
    if (circleBlocked(x, z, radius)) return true;
    constexpr float eps = 0.001f;
    for (const auto& p : platforms_) {
        if (feet < p.top - eps && head > p.bottom + eps &&
            overlapsTile(x, z, radius, p.column, p.row)) return true;
    }
    return false;
}

bool Level::landingHeight(float x, float z, float radius, float from, float to, float& height) const {
    constexpr float eps = 0.002f;
    bool hit = false;
    auto consider = [&](float top) {
        if (top <= from + eps && top >= to - eps && (!hit || top > height)) {
            height = top; hit = true;
        }
    };
    if (from >= -eps && to <= eps) {
        const int x0 = static_cast<int>(std::floor((x - radius) / kTileSize + width() * 0.5f));
        const int x1 = static_cast<int>(std::floor((x + radius) / kTileSize + width() * 0.5f));
        const int z0 = static_cast<int>(std::floor((z - radius) / kTileSize + this->height() * 0.5f));
        const int z1 = static_cast<int>(std::floor((z + radius) / kTileSize + this->height() * 0.5f));
        for (int row = z0; row <= z1; ++row)
            for (int col = x0; col <= x1; ++col)
                if (hasFloor(col,row) && overlapsTile(x,z,radius,col,row)) consider(0.0f);
    }
    for (const auto& p : platforms_)
        if (overlapsTile(x,z,radius,p.column,p.row)) consider(p.top);
    return hit;
}

bool Level::ceilingLimit(float x, float z, float radius, float fromHead, float toHead, float& height) const {
    constexpr float eps = 0.002f;
    bool hit = false;
    auto consider = [&](float bottom) {
        if (fromHead <= bottom + eps && toHead >= bottom - eps && (!hit || bottom < height)) {
            height = bottom; hit = true;
        }
    };
    consider(ceilingHeight_);
    for (const auto& p : platforms_)
        if (overlapsTile(x,z,radius,p.column,p.row)) consider(p.bottom);
    return hit;
}

bool Level::toggleNearbyDoor(float x, float z) {
    int nearestColumn = -1;
    int nearestRow = -1;
    float nearestDistance2 = 2.6f * 2.6f;

    for (int row = 0; row < height(); ++row) {
        for (int column = 0; column < width(); ++column) {
            if (tileAt(column, row) != 'D') continue;
            const float dx = x - worldX(column);
            const float dz = z - worldZ(row);
            const float distance2 = dx * dx + dz * dz;
            if (distance2 >= nearestDistance2) continue;
            if (isDoorOpen(column, row) && distance2 < 1.3f * 1.3f) continue;
            nearestDistance2 = distance2;
            nearestColumn = column;
            nearestRow = row;
        }
    }

    if (nearestColumn < 0) return false;
    const int doorIndex = index(nearestColumn, nearestRow);
    openDoors_[doorIndex] = !openDoors_[doorIndex];
    return true;
}

bool Level::collectNearbyHealth(float x, float z, float feet) {
    for (int row = 0; row < height(); ++row) {
        for (int column = 0; column < width(); ++column) {
            if (tileAt(column, row) != 'H' || isHealthCollected(column, row)) continue;
            if (near(x, z, worldX(column), worldZ(row), 1.0f) && nearSurface(feet, column, row)) {
                collectedHealth_[index(column, row)] = true;
                return true;
            }
        }
    }
    return false;
}

bool Level::reachedExit(float x, float z, float feet) const {
    for (auto position : positions('X')) {
        if (near(x, z, worldX(position.column), worldZ(position.row), 0.95f) &&
            nearSurface(feet, position.column, position.row))
            return true;
    }
    return false;
}

std::vector<GridPosition> Level::positions(char symbol) const {
    std::vector<GridPosition> found;
    for (int row = 0; row < height(); ++row)
        for (int column = 0; column < width(); ++column)
            if (tileAt(column, row) == symbol)
                found.push_back({column, row});
    return found;
}
