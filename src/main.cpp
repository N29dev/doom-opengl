#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include "Level.hpp"

namespace {
    struct Vec3 {
        float x, y, z;
    };

    Vec3 operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    Vec3 operator-(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    Vec3 operator*(Vec3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }

    float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

    Vec3 cross(Vec3 a, Vec3 b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    Vec3 normalize(Vec3 a) {
        float length = std::sqrt(dot(a, a));
        return length > 0.0f ? a * (1.0f / length) : Vec3{ 0, 0, 0 };
    }

    struct Mat4 {
        float v[16]{};
    };

    Mat4 identity() {
        Mat4 result{};
        result.v[0] = result.v[5] = result.v[10] = result.v[15] = 1.0f;
        return result;
    }

    Mat4 operator*(const Mat4& a, const Mat4& b) {
        Mat4 result{};
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                for (int k = 0; k < 4; ++k) {
                    result.v[column * 4 + row] +=
                        a.v[k * 4 + row] * b.v[column * 4 + k];
                }
            }
        }
        return result;
    }

    Mat4 model_matrix(Vec3 position, Vec3 size) {
        Mat4 result = identity();
        result.v[0] = size.x;
        result.v[5] = size.y;
        result.v[10] = size.z;
        result.v[12] = position.x;
        result.v[13] = position.y;
        result.v[14] = position.z;
        return result;
    }

    Mat4 perspective(float fov, float aspect, float nearPlane, float farPlane) {
        float t = std::tan(fov / 2.0f);
        Mat4 result{};
        result.v[0] = 1.0f / (aspect * t);
        result.v[5] = 1.0f / t;
        result.v[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result.v[11] = -1.0f;
        result.v[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        return result;
    }

    Mat4 view_matrix(Vec3 eye, Vec3 target) {
        Vec3 forward = normalize(target - eye);
        Vec3 right = normalize(cross(forward, { 0, 1, 0 }));
        Vec3 up = cross(right, forward);

        Mat4 result = identity();
        result.v[0] = right.x;
        result.v[4] = right.y;
        result.v[8] = right.z;
        result.v[1] = up.x;
        result.v[5] = up.y;
        result.v[9] = up.z;
        result.v[2] = -forward.x;
        result.v[6] = -forward.y;
        result.v[10] = -forward.z;
        result.v[12] = -dot(right, eye);
        result.v[13] = -dot(up, eye);
        result.v[14] = dot(forward, eye);
        return result;
    }

    struct Camera {
        Vec3 position{ 0, 1.4f, 6.0f };
        float yaw = -90.0f;
        float pitch = 0.0f;
        float verticalVelocity = 0.0f;
        bool onGround = true;

        Vec3 front() const {
            const float radians = 3.14159265f / 180.0f;
            float y = yaw * radians;
            float p = pitch * radians;
            return normalize({
                std::cos(y) * std::cos(p),
                std::sin(p),
                std::sin(y) * std::cos(p)
                });
        }
    } camera;

    bool firstMouse = true;
    bool flashlightEnabled = false;
    bool flashlightKeyHeld = false;
    double mouseX = 640.0;
    double mouseY = 360.0;
    bool previousSpacePressed = false;
    bool previousFirePressed = false;
    int playerHealth = 100;
    float damageCooldown = 0.0f;
    float shotFlashTimer = 0.0f;
    constexpr float weaponRange = 24.0f;
    Vec3 rayEnd{};

    struct Enemy {
        Vec3 position;
        Vec3 size;
        int health;
    };

    Level level;
    Level initialLevel;
    std::vector<Enemy> enemies;
    bool gameWon = false;
    bool interactKeyHeld = false;
    bool restartKeyHeld = false;

    void reset_game() {
        level = initialLevel; // Copy the immutable template, including closed doors and uncollected pickups.
        enemies.clear();
        for (const GridPosition pos : level.positions('M')) {
            enemies.push_back({ {level.worldX(pos.column), 0.9f, level.worldZ(pos.row)},
                                {0.8f, 1.8f, 0.8f}, 100 });
        }
        camera.position = {level.spawnX(), 1.4f, level.spawnZ()};
        camera.yaw = 0.0f;
        camera.pitch = 0.0f;
        camera.verticalVelocity = 0.0f;
        camera.onGround = true;
        playerHealth = 100;
        damageCooldown = 0.0f;
        shotFlashTimer = 0.0f;
        gameWon = false;
        previousSpacePressed = false;
        previousFirePressed = false;
        firstMouse = true;
    }

    void fire_weapon();

    void mouse_callback(GLFWwindow*, double x, double y) {
        if (firstMouse) {
            mouseX = x;
            mouseY = y;
            firstMouse = false;
            return;
        }

        camera.yaw += static_cast<float>(x - mouseX) * 0.1f;
        camera.pitch = std::clamp(
            camera.pitch + static_cast<float>(mouseY - y) * 0.1f,
            -88.0f, 88.0f
        );

        mouseX = x;
        mouseY = y;
    }

    void input(GLFWwindow* window, float deltaTime) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        const bool restartPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (restartPressed && !restartKeyHeld) {
            reset_game();
            glfwSetWindowTitle(window, "DOOM OpenGL | E: door | F: light | R: restart");
            std::cout << "Level restarted.\\n";
        }
        restartKeyHeld = restartPressed;
        if (playerHealth <= 0 || gameWon) return;

        const bool fPressed = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
        if (fPressed && !flashlightKeyHeld) flashlightEnabled = !flashlightEnabled;
        flashlightKeyHeld = fPressed;

        const bool ePressed = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
        if (ePressed && !interactKeyHeld && level.toggleNearbyDoor(camera.position.x, camera.position.z))
            std::cout << "Door toggled.\\n";
        interactKeyHeld = ePressed;

        const Vec3 front = camera.front();
        const Vec3 forward = normalize({front.x, 0.0f, front.z});
        const Vec3 right = normalize(cross(forward, {0.0f, 1.0f, 0.0f}));
        Vec3 movement{0.0f, 0.0f, 0.0f};

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement = movement + forward;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement = movement - forward;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement = movement - right;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement = movement + right;

        // Separate X/Z collision allows the player to slide along a wall.
        if (dot(movement, movement) > 0.0f) {
            movement = normalize(movement) * (4.5f * deltaTime);
            constexpr float playerRadius = 0.33f;
            if (!level.circleBlocked(camera.position.x + movement.x, camera.position.z, playerRadius))
                camera.position.x += movement.x;
            if (!level.circleBlocked(camera.position.x, camera.position.z + movement.z, playerRadius))
                camera.position.z += movement.z;
        }

        const bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spacePressed && !previousSpacePressed && camera.onGround) {
            camera.verticalVelocity = 6.5f;
            camera.onGround = false;
        }
        previousSpacePressed = spacePressed;

        const bool firePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (firePressed && !previousFirePressed) fire_weapon();
        previousFirePressed = firePressed;

        camera.position.y += camera.verticalVelocity * deltaTime;
        camera.verticalVelocity -= 18.0f * deltaTime;
        if (camera.position.y <= 1.4f) {
            camera.position.y = 1.4f;
            camera.verticalVelocity = 0.0f;
            camera.onGround = true;
        }

        damageCooldown = std::max(0.0f, damageCooldown - deltaTime);
        shotFlashTimer = std::max(0.0f, shotFlashTimer - deltaTime);
        if (playerHealth < 100 && level.collectNearbyHealth(camera.position.x, camera.position.z)) {
            playerHealth = std::min(100, playerHealth + 35);
            std::cout << "Picked up health. Current health: " << playerHealth << '\\n';
        }

        // Prototype enemy movement: move directly toward the player if nearby.
        // No pathfinding, animations, or advanced behavior: left for teammates.
        for (Enemy& enemy : enemies) {
            if (enemy.health <= 0) continue;
            const float dx = camera.position.x - enemy.position.x;
            const float dz = camera.position.z - enemy.position.z;
            const float distance = std::sqrt(dx * dx + dz * dz);
            if (distance > 1.2f && distance < 9.0f) {
                const float stepX = dx / distance * (0.8f * deltaTime);
                const float stepZ = dz / distance * (0.8f * deltaTime);
                if (!level.circleBlocked(enemy.position.x + stepX, enemy.position.z, 0.42f))
                    enemy.position.x += stepX;
                if (!level.circleBlocked(enemy.position.x, enemy.position.z + stepZ, 0.42f))
                    enemy.position.z += stepZ;
            }
            const float closeX = camera.position.x - enemy.position.x;
            const float closeZ = camera.position.z - enemy.position.z;
            if (damageCooldown <= 0.0f && closeX * closeX + closeZ * closeZ < 1.2f * 1.2f) {
                playerHealth = std::max(0, playerHealth - 10);
                damageCooldown = 1.0f;
                if (playerHealth == 0) {
                    std::cout << "You died. Press R to restart.\\n";
                    glfwSetWindowTitle(window, "YOU DIED - press R to restart");
                }
            }
        }

        if (level.reachedExit(camera.position.x, camera.position.z)) {
            gameWon = true;
            std::cout << "Test level complete! Press R to restart.\\n";
            glfwSetWindowTitle(window, "TEST COMPLETE - press R to restart");
        }
    }

    bool ray_intersects_box(Vec3 origin, Vec3 direction, Vec3 center, Vec3 size, float& distance) {
        const float origins[] = { origin.x, origin.y, origin.z };
        const float directions[] = { direction.x, direction.y, direction.z };
        const float halfSizes[] = { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
        const float centers[] = { center.x, center.y, center.z };
        float nearDistance = 0.0f;
        float farDistance = weaponRange;

        for (int axis = 0; axis < 3; ++axis) {
            float minimum = centers[axis] - halfSizes[axis];
            float maximum = centers[axis] + halfSizes[axis];
            if (std::abs(directions[axis]) < 0.0001f) {
                if (origins[axis] < minimum || origins[axis] > maximum) return false;
                continue;
            }

            float first = (minimum - origins[axis]) / directions[axis];
            float second = (maximum - origins[axis]) / directions[axis];
            if (first > second) std::swap(first, second);
            nearDistance = std::max(nearDistance, first);
            farDistance = std::min(farDistance, second);
            if (nearDistance > farDistance) return false;
        }

        distance = nearDistance;
        return farDistance >= 0.0f && nearDistance <= weaponRange;
    }

    bool ray_intersects_enemy(Vec3 origin, Vec3 direction, const Enemy& enemy, float& distance) {
        return ray_intersects_box(origin, direction, enemy.position, enemy.size, distance);
    }

    void fire_weapon() {
        const Vec3 direction = camera.front();
        rayEnd = camera.position + direction * weaponRange;
        shotFlashTimer = 0.12f;
        float nearestHit = weaponRange;
        int hitEnemy = -1;

        // Find the nearest solid wall or closed door before looking for enemies.
        // This prevents shooting through level geometry.
        for (int row = 0; row < level.height(); ++row) {
            for (int column = 0; column < level.width(); ++column) {
                if (!level.isSolid(column, row)) continue;
                float distance = 0.0f;
                if (ray_intersects_box(camera.position, direction,
                        {level.worldX(column), 2.25f, level.worldZ(row)},
                        {Level::kTileSize, 4.5f, Level::kTileSize}, distance)
                        && distance < nearestHit) {
                    nearestHit = distance;
                    rayEnd = camera.position + direction * distance;
                }
            }
        }

        for (std::size_t index = 0; index < enemies.size(); ++index) {
            if (enemies[index].health <= 0) continue;

            float hitDistance = 0.0f;
            if (ray_intersects_enemy(camera.position, direction, enemies[index], hitDistance) &&
                hitDistance < nearestHit) {
                nearestHit = hitDistance;
                hitEnemy = static_cast<int>(index);
            }
        }

        if (hitEnemy >= 0) {
            rayEnd = camera.position + direction * nearestHit;
            enemies[hitEnemy].health = std::max(0, enemies[hitEnemy].health - 34);
            if (enemies[hitEnemy].health == 0)
                std::cout << "Enemy eliminated.\\n";
        }
    }

    GLuint compile_shader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (success != GL_TRUE) {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            std::string log(static_cast<size_t>(std::max(length, 1)), '\0');
            glGetShaderInfoLog(shader, length, nullptr, log.data());
            std::cerr << "Shader compilation error:\n" << log << '\n';
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    GLuint create_program() {
        const char* vs = R"(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 world_position;
out vec3 world_normal;

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);
    world_position = worldPos.xyz;
    world_normal = normalize(mat3(transpose(inverse(model))) * normal);
    gl_Position = projection * view * worldPos;
}
)";

        const char* fs = R"(#version 330 core
in vec3 world_position;
in vec3 world_normal;

uniform vec3 object_color;
uniform vec3 light_position;
uniform vec3 view_position;
uniform vec3 flashlight_direction;
uniform bool flashlight_enabled;

out vec4 color;

void main()
{
    vec3 n = normalize(world_normal);
    vec3 baseColor = object_color;

    // Procedural brick pattern on vertical surfaces.
    if (abs(n.y) < 0.5)
    {
        vec2 uv;

        if (abs(n.x) > 0.5)
            uv = world_position.zy;
        else
            uv = world_position.xy;

        vec2 brick = uv * vec2(1.4, 1.0);
        brick.x += floor(brick.y) * 0.5;

        vec2 cell = fract(brick);

        // Dark mortar lines; brighter brick faces.
        float mortar =
            step(0.06, cell.x) * step(0.08, cell.y);

        float brickShade = mix(0.35, 1.15, mortar);
        baseColor *= brickShade;
    }
    else
    {
        // Tile pattern on floor and ceiling.
        vec2 tile = fract(world_position.xz * 1.2);

        float tileFace =
            step(0.04, tile.x) * step(0.04, tile.y);

        baseColor *= mix(0.55, 1.0, tileFace);
    }

    // Diffuse lighting with distance attenuation.
    vec3 toLight = light_position - world_position;
    float distanceToLight = length(toLight);
    vec3 lightDirection = normalize(toLight);

    float diffuse = max(dot(n, lightDirection), 0.0);

    float attenuation =
        1.0 / (1.0 + 0.045 * distanceToLight
                   + 0.0075 * distanceToLight * distanceToLight);

    vec3 ambient = baseColor * 0.22;
    vec3 direct = baseColor * diffuse * attenuation * 1.5;

    // Player flashlight: a soft-edged spotlight pointed along the camera.
    vec3 flashlight = vec3(0.0);
    if (flashlight_enabled)
    {
        vec3 fromEye = world_position - view_position;
        float eyeDistance = length(fromEye);
        vec3 rayDirection = normalize(fromEye);
        float cone = smoothstep(0.90, 0.97,
                                dot(rayDirection, flashlight_direction));
        float facing = max(dot(n, -rayDirection), 0.0);
        float falloff = 1.0 / (1.0 + 0.12 * eyeDistance
                                   + 0.04 * eyeDistance * eyeDistance);
        flashlight = baseColor * vec3(1.0, 0.93, 0.78)
                     * cone * facing * falloff * 2.3;
    }

    color = vec4(ambient + direct + flashlight, 1.0);
}
)";

        GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, vs);
        GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fs);

        if (vertexShader == 0 || fragmentShader == 0) {
            if (vertexShader) glDeleteShader(vertexShader);
            if (fragmentShader) glDeleteShader(fragmentShader);
            return 0;
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        if (success != GL_TRUE) {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            std::string log(static_cast<size_t>(std::max(length, 1)), '\0');
            glGetProgramInfoLog(program, length, nullptr, log.data());
            std::cerr << "Shader linking error:\n" << log << '\n';
            glDeleteProgram(program);
            program = 0;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return program;
    }

    void draw_box(GLuint program, GLuint vao, Vec3 position, Vec3 size, Vec3 objectColor) {
        Mat4 model = model_matrix(position, size);

        glUniformMatrix4fv(
            glGetUniformLocation(program, "model"), 1, GL_FALSE, model.v
        );
        glUniform3f(
            glGetUniformLocation(program, "object_color"),
            objectColor.x, objectColor.y, objectColor.z
        );

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    GLuint create_hud_program() {
        const char* vertexSource = R"(#version 330 core
layout(location = 0) in vec2 position;
uniform vec2 offset;
uniform vec2 size;
void main() { gl_Position = vec4(position * size + offset, 0.0, 1.0); }
)";
        const char* fragmentSource = R"(#version 330 core
uniform vec3 rectangle_color;
out vec4 color;
void main() { color = vec4(rectangle_color, 1.0); }
)";

        GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, vertexSource);
        GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fragmentSource);
        if (!vertexShader || !fragmentShader) {
            if (vertexShader) glDeleteShader(vertexShader);
            if (fragmentShader) glDeleteShader(fragmentShader);
            return 0;
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            glDeleteProgram(program);
            return 0;
        }
        return program;
    }

    struct HudRenderer {
        GLuint program = 0;
        GLuint vao = 0;
        GLuint vbo = 0;

        bool initialize() {
            program = create_hud_program();
            if (!program) return false;

            constexpr float vertices[] = {
                -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
                -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
            };
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
            glEnableVertexAttribArray(0);
            return true;
        }

        void rectangle(float x, float y, float width, float height, Vec3 color) const {
            glUseProgram(program);
            glUniform2f(glGetUniformLocation(program, "offset"), x, y);
            glUniform2f(glGetUniformLocation(program, "size"), width, height);
            glUniform3f(glGetUniformLocation(program, "rectangle_color"), color.x, color.y, color.z);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        void destroy() {
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
            glDeleteProgram(program);
        }
    };

    GLuint create_ray_program() {
        const char* vertexSource = R"(#version 330 core
layout(location = 0) in vec3 position;
uniform mat4 view;
uniform mat4 projection;
void main() { gl_Position = projection * view * vec4(position, 1.0); }
)";
        const char* fragmentSource = R"(#version 330 core
uniform vec3 ray_color;
out vec4 color;
void main() { color = vec4(ray_color, 1.0); }
)";

        GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, vertexSource);
        GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fragmentSource);
        if (!vertexShader || !fragmentShader) {
            if (vertexShader) glDeleteShader(vertexShader);
            if (fragmentShader) glDeleteShader(fragmentShader);
            return 0;
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            glDeleteProgram(program);
            return 0;
        }
        return program;
    }

    struct RayRenderer {
        GLuint program = 0;
        GLuint vao = 0;
        GLuint vbo = 0;

        bool initialize() {
            program = create_ray_program();
            if (!program) return false;

            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
            glEnableVertexAttribArray(0);
            return true;
        }

        void draw(Mat4 view, Mat4 projection) const {
            if (shotFlashTimer <= 0.0f) return;
            const float vertices[] = {
                camera.position.x, camera.position.y, camera.position.z,
                rayEnd.x, rayEnd.y, rayEnd.z
            };
            glUseProgram(program);
            glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, view.v);
            glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection.v);
            glUniform3f(glGetUniformLocation(program, "ray_color"), 1.0f, 0.28f, 0.04f);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindVertexArray(vao);
            glDrawArrays(GL_LINES, 0, 2);
        }

        void destroy() {
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
            glDeleteProgram(program);
        }
    };

    void draw_hud(const HudRenderer& hud) {
        glDisable(GL_DEPTH_TEST);
        // Simple visual status strips; future UI and text are left for the team.
        if (gameWon) hud.rectangle(0.0f, 0.2f, 1.25f, 0.10f, {0.05f, 0.7f, 0.3f});
        if (playerHealth == 0) hud.rectangle(0.0f, 0.2f, 1.25f, 0.10f, {0.8f, 0.04f, 0.04f});

        constexpr float barX = -0.69f;
        constexpr float barY = -0.84f;
        constexpr float barWidth = 0.48f;
        constexpr float barHeight = 0.09f;
        const float healthRatio = std::clamp(playerHealth / 100.0f, 0.0f, 1.0f);

        hud.rectangle(barX, barY, barWidth, barHeight, { 0.12f, 0.03f, 0.03f });
        if (healthRatio > 0.0f) {
            hud.rectangle(
                barX - barWidth * (1.0f - healthRatio) * 0.5f,
                barY, barWidth * healthRatio, barHeight * 0.55f,
                healthRatio > 0.3f ? Vec3{ 0.18f, 0.85f, 0.18f } : Vec3{ 0.95f, 0.12f, 0.08f }
            );
        }

        hud.rectangle(0.0f, 0.0f, 0.035f, 0.008f, { 1.0f, 0.86f, 0.62f });
        hud.rectangle(0.0f, 0.0f, 0.008f, 0.035f, { 1.0f, 0.86f, 0.62f });

        const Vec3 weaponColor{ 0.19f, 0.18f, 0.2f };
        hud.rectangle(0.17f, -0.77f, 0.16f, 0.2f, weaponColor);
        hud.rectangle(0.19f, -0.9f, 0.1f, 0.16f, { 0.12f, 0.11f, 0.13f });
        hud.rectangle(0.16f, -0.61f, 0.08f, 0.16f, { 0.34f, 0.32f, 0.33f });
        if (shotFlashTimer > 0.0f) {
            hud.rectangle(0.16f, -0.48f, 0.12f, 0.1f, { 1.0f, 0.72f, 0.12f });
        }

        glEnable(GL_DEPTH_TEST);
    }
}

int main(int argc, char** argv) {
    const std::string levelFile = argc > 1 ? argv[1] : "assets/levels/test_arena.json";
    std::string loadError;
    if (!initialLevel.load(levelFile, loadError)) {
        std::cerr << loadError << '\\n';
        return 1;
    }
    reset_game();
    std::cout << "Loaded " << levelFile << " | W/A/S/D: move, Space: jump, "
              << "mouse: aim, left-click: shoot, E: door, F: light, R: restart.\\n";

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        1280, 720, "DOOM OpenGL | E: door | F: light | R: restart", nullptr, nullptr
    );

    if (!window) {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const Vec3 corners[] = {
        {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}
    };

    const Vec3 normals[] = {
        {-1, 0, 0}, {1, 0, 0}, {0, -1, 0},
        {0, 1, 0}, {0, 0, -1}, {0, 0, 1}
    };

    const int faces[6][6] = {
        {0, 4, 7, 7, 3, 0},
        {1, 2, 6, 6, 5, 1},
        {0, 1, 5, 5, 4, 0},
        {3, 7, 6, 6, 2, 3},
        {0, 3, 2, 2, 1, 0},
        {4, 5, 6, 6, 7, 4}
    };

    float vertices[216];
    int offset = 0;

    for (int face = 0; face < 6; ++face) {
        for (int i = 0; i < 6; ++i) {
            Vec3 p = corners[faces[face][i]];
            Vec3 n = normals[face];

            vertices[offset++] = p.x;
            vertices[offset++] = p.y;
            vertices[offset++] = p.z;
            vertices[offset++] = n.x;
            vertices[offset++] = n.y;
            vertices[offset++] = n.z;
        }
    }

    GLuint vao = 0;
    GLuint vbo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    GLuint program = create_program();
    if (!program) {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glUseProgram(program);
    glUniform3f(glGetUniformLocation(program, "light_position"), 0.0f, 3.5f, 1.0f);

    HudRenderer hud;
    if (!hud.initialize()) {
        glDeleteProgram(program);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    RayRenderer rayRenderer;
    if (!rayRenderer.initialize()) {
        hud.destroy();
        glDeleteProgram(program);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    constexpr int pixelWidth = 320;
    constexpr int pixelHeight = 180;

    GLuint pixelFbo = 0;
    GLuint pixelColor = 0;
    GLuint pixelDepth = 0;

    glGenFramebuffers(1, &pixelFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, pixelFbo);

    glGenTextures(1, &pixelColor);
    glBindTexture(GL_TEXTURE_2D, pixelColor);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB8, pixelWidth, pixelHeight,
        0, GL_RGB, GL_UNSIGNED_BYTE, nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pixelColor, 0
    );

    glGenRenderbuffers(1, &pixelDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, pixelDepth);
    glRenderbufferStorage(
        GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, pixelWidth, pixelHeight
    );
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER, pixelDepth
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Pixel framebuffer is incomplete.\n";
        glDeleteRenderbuffers(1, &pixelDepth);
        glDeleteTextures(1, &pixelColor);
        glDeleteFramebuffers(1, &pixelFbo);
        rayRenderer.destroy();
        hud.destroy();
        glDeleteProgram(program);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    double previous = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float deltaTime = std::min(static_cast<float>(now - previous), 0.1f);
        previous = now;

        input(window, deltaTime);

        int width = 1;
        int height = 1;
        glfwGetFramebufferSize(window, &width, &height);

        glBindFramebuffer(GL_FRAMEBUFFER, pixelFbo);
        glViewport(0, 0, pixelWidth, pixelHeight);
        glEnable(GL_DEPTH_TEST);

        Mat4 view = view_matrix(camera.position, camera.position + camera.front());
        Mat4 projection = perspective(
            70.0f * 3.14159265f / 180.0f,
            static_cast<float>(pixelWidth) / pixelHeight,
            0.1f, 100.0f
        );

        glUseProgram(program);
        glUniformMatrix4fv(
            glGetUniformLocation(program, "view"), 1, GL_FALSE, view.v
        );
        glUniformMatrix4fv(
            glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection.v
        );
        glUniform3f(
            glGetUniformLocation(program, "view_position"),
            camera.position.x, camera.position.y, camera.position.z
        );
        Vec3 flashlightDirection = camera.front();
        glUniform3f(
            glGetUniformLocation(program, "flashlight_direction"),
            flashlightDirection.x, flashlightDirection.y, flashlightDirection.z
        );
        glUniform1i(
            glGetUniformLocation(program, "flashlight_enabled"),
            flashlightEnabled ? GL_TRUE : GL_FALSE
        );

        glClearColor(0.025f, 0.018f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Level-driven geometry. This is deliberately simple: one tile = one box.
        // Teammates can replace the placeholder geometry and materials later.
        const float floorWidth = level.width() * Level::kTileSize;
        const float floorDepth = level.height() * Level::kTileSize;
        draw_box(program, vao, {0.0f, -0.25f, 0.0f},
                 {floorWidth, 0.5f, floorDepth}, {0.17f, 0.09f, 0.08f});
        draw_box(program, vao, {0.0f, 4.75f, 0.0f},
                 {floorWidth, 0.5f, floorDepth}, {0.1f, 0.07f, 0.12f});

        for (int row = 0; row < level.height(); ++row) {
            for (int column = 0; column < level.width(); ++column) {
                const char tile = level.tileAt(column, row);
                const float x = level.worldX(column);
                const float z = level.worldZ(row);
                if (level.isSolid(column, row)) {
                    const Vec3 wallColor = tile == 'D'
                        ? Vec3{0.48f, 0.35f, 0.09f}
                        : Vec3{0.30f, 0.08f, 0.09f};
                    draw_box(program, vao, {x, 2.25f, z},
                             {Level::kTileSize, 4.5f, Level::kTileSize}, wallColor);
                }
                if (tile == 'H' && !level.isHealthCollected(column, row)) {
                    draw_box(program, vao, {x, 0.5f, z},
                             {0.6f, 0.6f, 0.6f}, {0.08f, 0.7f, 0.30f});
                }
                if (tile == 'X') {
                    draw_box(program, vao, {x, 0.04f, z},
                             {1.4f, 0.08f, 1.4f}, {0.09f, 0.65f, 0.5f});
                }
            }
        }

        for (const Enemy& enemy : enemies) {
            if (enemy.health <= 0) continue;
            float healthRatio = enemy.health / 100.0f;
            draw_box(
                program, vao, enemy.position, enemy.size,
                { 0.35f + 0.45f * healthRatio, 0.04f, 0.03f }
            );
        }

        rayRenderer.draw(view, projection);
        draw_hud(hud);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, pixelFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(
            0, 0, pixelWidth, pixelHeight,
            0, 0, width, height,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteRenderbuffers(1, &pixelDepth);
    glDeleteTextures(1, &pixelColor);
    glDeleteFramebuffers(1, &pixelFbo);
    rayRenderer.destroy();
    hud.destroy();
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
