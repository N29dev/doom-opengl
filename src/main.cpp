#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <string>

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

    std::array<Enemy, 3> enemies{{
        { { 0.0f, 0.9f, -3.5f }, { 0.8f, 1.8f, 0.8f }, 100 },
        { { -2.8f, 0.9f, -6.5f }, { 0.8f, 1.8f, 0.8f }, 100 },
        { { 2.8f, 0.9f, -7.0f }, { 0.8f, 1.8f, 0.8f }, 100 }
    }};

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
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        Vec3 front = camera.front();
        Vec3 forward = normalize({ front.x, 0.0f, front.z });
        Vec3 right = normalize(cross(forward, { 0.0f, 1.0f, 0.0f }));
        float speed = 4.5f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.position = camera.position + forward * speed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.position = camera.position - forward * speed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.position = camera.position - right * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.position = camera.position + right * speed;

        bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spacePressed && !previousSpacePressed && camera.onGround) {
            camera.verticalVelocity = 6.5f;
            camera.onGround = false;
        }
        previousSpacePressed = spacePressed;

        bool firePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (firePressed && !previousFirePressed) {
            fire_weapon();
        }
        previousFirePressed = firePressed;

        camera.position.y += camera.verticalVelocity * deltaTime;
        camera.verticalVelocity -= 18.0f * deltaTime;
        if (camera.position.y <= 1.4f) {
            camera.position.y = 1.4f;
            camera.verticalVelocity = 0.0f;
            camera.onGround = true;
        }

        camera.position.x = std::clamp(camera.position.x, -8.0f, 8.0f);
        camera.position.z = std::clamp(camera.position.z, -8.0f, 8.0f);
        damageCooldown = std::max(0.0f, damageCooldown - deltaTime);
        shotFlashTimer = std::max(0.0f, shotFlashTimer - deltaTime);

        if (damageCooldown <= 0.0f) {
            for (const Enemy& enemy : enemies) {
                if (enemy.health <= 0) continue;

                float dx = camera.position.x - enemy.position.x;
                float dz = camera.position.z - enemy.position.z;
                if (dx * dx + dz * dz < 1.2f * 1.2f) {
                    playerHealth = std::max(0, playerHealth - 10);
                    damageCooldown = 1.0f;
                    break;
                }
            }
        }
    }

    bool ray_intersects_enemy(Vec3 origin, Vec3 direction, const Enemy& enemy, float& distance) {
        const float origins[] = { origin.x, origin.y, origin.z };
        const float directions[] = { direction.x, direction.y, direction.z };
        const float halfSizes[] = { enemy.size.x * 0.5f, enemy.size.y * 0.5f, enemy.size.z * 0.5f };
        const float centers[] = { enemy.position.x, enemy.position.y, enemy.position.z };
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

    void fire_weapon() {
        const Vec3 direction = camera.front();
        rayEnd = camera.position + direction * weaponRange;
        shotFlashTimer = 0.12f;
        float nearestHit = weaponRange;
        int hitEnemy = -1;

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

    color = vec4(ambient + direct, 1.0);
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

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        1280, 720, "Doom OpenGL - WASD + Mouse", nullptr, nullptr
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

        glClearColor(0.025f, 0.018f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_box(program, vao, { 0, -0.25f, 0 }, { 18, 0.5f, 18 }, { 0.17f, 0.09f, 0.08f });
        draw_box(program, vao, { 0, 5.0f, 0 }, { 18, 0.5f, 18 }, { 0.1f, 0.07f, 0.12f });
        draw_box(program, vao, { 0, 2.4f, -9 }, { 18, 5, 0.5f }, { 0.34f, 0.08f, 0.06f });
        draw_box(program, vao, { -9, 2.4f, 0 }, { 0.5f, 5, 18 }, { 0.26f, 0.07f, 0.08f });
        draw_box(program, vao, { 9, 2.4f, 0 }, { 0.5f, 5, 18 }, { 0.26f, 0.07f, 0.08f });

        draw_box(program, vao, { -4.5f, 1.2f, -2.5f }, { 1.4f, 2.4f, 1.4f }, { 0.45f, 0.18f, 0.07f });
        draw_box(program, vao, { 4.5f, 1.2f, -5.5f }, { 1.4f, 2.4f, 1.4f }, { 0.4f, 0.12f, 0.05f });
        draw_box(program, vao, { -4.5f, 1.2f, -7.0f }, { 1.4f, 2.4f, 1.4f }, { 0.5f, 0.2f, 0.06f });
        draw_box(program, vao, { 4.5f, 1.2f, -1.0f }, { 1.4f, 2.4f, 1.4f }, { 0.32f, 0.13f, 0.06f });

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
