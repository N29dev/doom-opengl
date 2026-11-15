#pragma once
// Included inside main.cpp's renderer namespace, after draw_model_box.
// All models are original procedural cube assemblies, using the shared VAO.
// +Z is forward for world models, -Z for the camera-space weapon.

Mat4 translate_model(Vec3 p) {
    return model_matrix(p, {1.0f, 1.0f, 1.0f});
}

Mat4 rotate_model_x(float angle) {
    Mat4 m = identity();
    const float c = std::cos(angle), s = std::sin(angle);
    m.v[5] = m.v[10] = c;
    m.v[6] = s;
    m.v[9] = -s;
    return m;
}

Mat4 rotate_model_y(float angle) {
    Mat4 m = identity();
    const float c = std::cos(angle), s = std::sin(angle);
    m.v[0] = m.v[10] = c;
    m.v[2] = -s;
    m.v[8] = s;
    return m;
}

Mat4 rotate_model_z(float angle) {
    Mat4 m = identity();
    const float c = std::cos(angle), s = std::sin(angle);
    m.v[0] = m.v[5] = c;
    m.v[1] = s;
    m.v[4] = -s;
    return m;
}

struct ModelPainter {
    GLuint program, vao;
    Mat4 root;
    int material = 1;

    void box(Vec3 p, Vec3 size, Vec3 color, int overrideMaterial = -1) const {
        draw_model_box(program, vao, root * model_matrix(p, size), color,
                       overrideMaterial < 0 ? material : overrideMaterial);
    }

    ModelPainter joint(Vec3 p, const Mat4& rotation) const {
        return {program, vao, root * translate_model(p) * rotation, material};
    }
};

void draw_demon_model(GLuint program, GLuint vao, Vec3 p, float yaw,
                      float health, float phase, float walking) {
    ModelPainter m{program, vao, translate_model(p) * rotate_model_y(yaw)};
    const Vec3 skin{0.52f + 0.24f * health, 0.10f, 0.07f};
    const Vec3 dark{0.28f, 0.055f, 0.045f};
    const Vec3 bone{0.79f, 0.70f, 0.48f};
    const Vec3 eye{1.0f, 0.71f, 0.08f};
    const float stride = std::sin(phase) * walking * 0.55f;

    m.box({0, 0.10f, 0}, {0.48f, 0.62f, 0.43f}, skin);
    m.box({0, -0.24f, 0}, {0.43f, 0.21f, 0.39f}, dark);
    m.box({0, 0.19f, 0.235f}, {0.30f, 0.25f, 0.07f}, dark);
    m.box({0, 0.50f, 0.015f}, {0.45f, 0.36f, 0.42f}, skin);
    m.box({0, 0.38f, 0.23f}, {0.30f, 0.13f, 0.14f}, dark);
    m.box({0, 0.38f, 0.307f}, {0.19f, 0.055f, 0.02f}, {0.08f, 0.025f, 0.02f});
    for (int side : {-1, 1}) {
        const float sign = static_cast<float>(side);
        m.box({sign * 0.115f, 0.55f, 0.231f}, {0.12f, 0.055f, 0.03f}, eye, 3);
        m.joint({sign * 0.165f, 0.67f, 0}, rotate_model_z(-sign * 0.27f))
            .box({0, 0.09f, 0}, {0.10f, 0.21f, 0.12f}, bone);

        const auto arm = m.joint({sign * 0.325f, 0.31f, 0}, rotate_model_x(sign * stride));
        arm.box({0, -0.19f, 0}, {0.15f, 0.41f, 0.20f}, skin);
        arm.box({0, -0.43f, 0.04f}, {0.17f, 0.17f, 0.23f}, dark);
        const auto leg = m.joint({sign * 0.13f, -0.30f, 0}, rotate_model_x(-sign * stride));
        leg.box({0, -0.23f, 0}, {0.20f, 0.45f, 0.23f}, dark);
        leg.box({0, -0.48f, 0.075f}, {0.23f, 0.18f, 0.36f}, skin);
    }
}

void draw_heavy_model(GLuint program, GLuint vao, Vec3 p, float yaw,
                      float health, float phase, float walking) {
    ModelPainter m{program, vao, translate_model(p) * rotate_model_y(yaw)};
    const Vec3 armor{0.08f, 0.25f + 0.12f * health, 0.62f + 0.2f * health};
    const Vec3 steel{0.22f, 0.34f, 0.48f};
    const Vec3 dark{0.045f, 0.07f, 0.14f};
    const Vec3 glow{0.16f, 0.85f, 1.0f};
    const float stride = std::sin(phase) * walking * 0.38f;

    m.box({0, 0.25f, 0}, {1.02f, 1.13f, 0.82f}, armor);
    m.box({0, -0.36f, 0}, {0.82f, 0.28f, 0.72f}, dark);
    m.box({0, 1.02f, 0}, {0.68f, 0.61f, 0.60f}, steel);
    m.box({0, 1.05f, 0.31f}, {0.58f, 0.22f, 0.08f}, dark);
    m.box({0, 1.07f, 0.36f}, {0.46f, 0.085f, 0.025f}, glow, 3);
    m.box({0, 0.33f, 0.43f}, {0.46f, 0.50f, 0.12f}, dark);
    m.box({0, 0.33f, 0.50f}, {0.23f, 0.27f, 0.035f}, glow, 3);

    for (int side : {-1, 1}) {
        const float sign = static_cast<float>(side);
        const auto arm = m.joint({sign * 0.66f, 0.63f, 0}, rotate_model_x(sign * stride));
        arm.box({0, -0.06f, 0}, {0.43f, 0.50f, 0.71f}, steel);
        arm.box({0, -0.55f, 0}, {0.33f, 0.66f, 0.43f}, armor);
        arm.box({0, -0.91f, 0.045f}, {0.43f, 0.42f, 0.57f}, dark);
        const auto leg = m.joint({sign * 0.28f, -0.50f, 0}, rotate_model_x(-sign * stride));
        leg.box({0, -0.37f, 0}, {0.42f, 0.68f, 0.49f}, armor);
        leg.box({0, -0.68f, 0.09f}, {0.49f, 0.25f, 0.67f}, steel);
        leg.box({0, -0.88f, 0.16f}, {0.51f, 0.20f, 0.74f}, dark);
    }
}

void draw_flying_model(GLuint program, GLuint vao, Vec3 p, float yaw,
                       float health, float time) {
    ModelPainter m{program, vao, translate_model(p) * rotate_model_y(yaw)};
    const Vec3 purple{0.39f + 0.27f * health, 0.08f, 0.61f};
    const Vec3 wing{0.32f, 0.035f, 0.43f};
    const Vec3 dark{0.16f, 0.025f, 0.24f};
    const float flap = std::sin(time * 7.5f + p.x) * 0.55f;
    m.box({0, 0, 0}, {0.88f, 0.70f, 0.75f}, purple);
    m.box({0, 0.04f, 0.31f}, {0.62f, 0.46f, 0.31f}, dark);
    m.box({0, 0.085f, 0.48f}, {0.38f, 0.24f, 0.05f}, {1.0f, 0.45f, 0.12f}, 3);
    m.box({0, 0.085f, 0.514f}, {0.105f, 0.19f, 0.02f}, dark);
    m.box({0, -0.20f, 0.43f}, {0.32f, 0.10f, 0.10f}, {0.07f, 0.025f, 0.10f});
    for (int side : {-1, 1}) {
        const float sign = static_cast<float>(side);
        m.box({sign * 0.11f, -0.26f, 0.47f}, {0.075f, 0.14f, 0.075f}, {0.86f, 0.80f, 0.69f});
        m.joint({sign * 0.29f, 0.29f, 0}, rotate_model_z(-sign * 0.40f))
            .box({0, 0.11f, 0}, {0.13f, 0.23f, 0.16f}, dark);
        const auto w = m.joint({sign * 0.42f, 0.06f, -0.04f}, rotate_model_z(sign * flap));
        w.box({sign * 0.28f, 0, 0}, {0.57f, 0.12f, 0.54f}, wing);
        w.box({sign * 0.59f, 0, -0.17f}, {0.29f, 0.10f, 0.36f}, wing);
        w.box({sign * 0.17f, 0.045f, 0.20f}, {0.35f, 0.09f, 0.10f}, purple);
    }
}

void draw_medkit_model(GLuint program, GLuint vao, Vec3 p, float time) {
    p.y += std::sin(time * 2.5f) * 0.07f;
    ModelPainter m{program, vao, translate_model(p) * rotate_model_y(time * 0.75f)};
    const Vec3 ivory{0.87f, 0.86f, 0.75f};
    const Vec3 red{0.88f, 0.055f, 0.06f};
    m.box({0, 0, 0}, {0.62f, 0.40f, 0.30f}, ivory);
    m.box({0, -0.14f, 0}, {0.64f, 0.09f, 0.32f}, {0.35f, 0.39f, 0.38f});
    m.box({0, 0.27f, 0}, {0.26f, 0.06f, 0.09f}, {0.31f, 0.34f, 0.32f});
    for (int side : {-1, 1}) {
        const float sign = static_cast<float>(side);
        m.box({sign * 0.10f, 0.22f, 0}, {0.06f, 0.10f, 0.09f}, {0.31f, 0.34f, 0.32f});
        m.box({0, 0.035f, sign * 0.16f}, {0.25f, 0.075f, 0.025f}, red, 3);
        m.box({0, 0.035f, sign * 0.165f}, {0.075f, 0.25f, 0.025f}, red, 3);
    }
}

// Weapon parts use -Z as muzzle direction. Reused in the pickup and viewmodel.
void shotgun_parts(const ModelPainter& m) {
    const Vec3 steel{0.37f, 0.40f, 0.43f};
    const Vec3 dark{0.10f, 0.12f, 0.15f};
    const Vec3 wood{0.48f, 0.23f, 0.10f};
    m.box({0, 0, -0.09f}, {0.19f, 0.15f, 0.34f}, steel);
    m.box({0, -0.045f, 0.25f}, {0.16f, 0.15f, 0.40f}, wood);
    m.box({0, -0.07f, 0.45f}, {0.18f, 0.20f, 0.10f}, dark);
    m.box({0, -0.085f, -0.34f}, {0.19f, 0.12f, 0.33f}, wood);
    for (int side : {-1, 1}) {
        const float x = static_cast<float>(side) * 0.052f;
        m.box({x, 0.031f, -0.49f}, {0.095f, 0.105f, 0.64f}, steel);
        m.box({x, 0.031f, -0.814f}, {0.062f, 0.068f, 0.015f}, dark);
    }
    m.box({0, 0.105f, -0.73f}, {0.028f, 0.045f, 0.045f}, dark);
}

void draw_shotgun_pickup(GLuint program, GLuint vao, Vec3 p, float time) {
    p.y += std::sin(time * 2.5f) * 0.08f;
    ModelPainter m{program, vao, translate_model(p) * rotate_model_y(time * 0.8f)};
    shotgun_parts(m);
}

void draw_door_model(GLuint program, GLuint vao, Vec3 p) {
    ModelPainter m{program, vao, translate_model(p)};
    const Vec3 metal{0.39f, 0.32f, 0.19f};
    const Vec3 panel{0.22f, 0.23f, 0.25f};
    const Vec3 frame{0.10f, 0.12f, 0.15f};
    m.box({0, 0, 0}, {2.0f, 4.5f, 2.0f}, metal);
    // Cover each possible approach because doors are grid tiles, not oriented meshes.
    for (int face = 0; face < 4; ++face) {
        auto side = m.joint({0, 0, 0}, rotate_model_y(face * 1.57079633f));
        side.box({0, 0, 1.012f}, {1.56f, 3.50f, 0.026f}, panel);
        for (int i : {-1, 1}) {
            const float sign = static_cast<float>(i);
            side.box({sign * 0.87f, 0, 1.026f}, {0.15f, 4.20f, 0.055f}, frame);
            side.box({0, sign * 1.50f, 1.034f}, {1.50f, 0.14f, 0.06f}, metal);
            side.box({sign * 0.23f, 0.03f, 1.045f}, {0.18f, 0.42f, 0.08f}, frame);
        }
        side.box({0, 1.08f, 1.045f}, {0.58f, 0.15f, 0.045f}, {0.94f, 0.55f, 0.08f}, 3);
        for (int i = -2; i <= 2; ++i) {
            side.box({i * 0.25f, -1.13f, 1.045f}, {0.20f, 0.10f, 0.04f},
                     (i % 2 == 0) ? Vec3{0.90f, 0.55f, 0.08f} : frame);
        }
    }
}

void draw_projectile_model(GLuint program, GLuint vao, Vec3 p, float time) {
    ModelPainter m{program, vao, translate_model(p) * rotate_model_y(time * 7.0f) * rotate_model_z(time * 5.0f)};
    m.box({0, 0, 0}, {0.24f, 0.24f, 0.24f}, {1.0f, 0.24f, 0.91f}, 3);
    m.box({0, 0, 0}, {0.36f, 0.10f, 0.10f}, {0.71f, 0.07f, 0.95f}, 3);
    m.box({0, 0, 0}, {0.10f, 0.36f, 0.10f}, {0.71f, 0.07f, 0.95f}, 3);
    m.box({0, 0, 0}, {0.10f, 0.10f, 0.36f}, {0.71f, 0.07f, 0.95f}, 3);
}

void draw_view_weapon(GLuint program, GLuint vao, bool shotgun, float phase,
                      float bob, float flash, float reloadRatio) {
    const float kick = std::clamp(flash / (shotgun ? 0.18f : 0.12f), 0.0f, 1.0f);
    const float reload = std::sin(std::clamp(reloadRatio, 0.0f, 1.0f) * 3.14159265f);
    const Vec3 origin{(shotgun ? 0.10f : 0.16f) + std::sin(phase) * 0.018f * bob,
                      -0.23f + std::cos(phase * 2.0f) * 0.012f * bob - reload * 0.095f,
                      -0.56f + kick * 0.08f};
    ModelPainter m{program, vao, translate_model(origin) * rotate_model_x(kick * 0.16f - reload * 0.52f) *
                   rotate_model_z(reload * 0.16f), 2};
    const Vec3 dark{0.14f, 0.16f, 0.19f};
    const Vec3 metal{0.43f, 0.46f, 0.50f};
    const Vec3 glove{0.25f, 0.22f, 0.17f};
    if (shotgun) {
        shotgun_parts(m);
        m.box({0.09f, -0.13f, 0.12f}, {0.13f, 0.18f, 0.28f}, glove);
        m.box({-0.13f, -0.14f, -0.32f}, {0.16f, 0.15f, 0.27f}, glove);
    } else {
        m.box({0, 0, -0.04f}, {0.12f, 0.13f, 0.32f}, dark);
        m.box({0, 0.071f, -0.075f}, {0.15f, 0.105f, 0.37f}, metal);
        m.box({0, 0.060f, -0.267f}, {0.09f, 0.07f, 0.014f}, dark);
        m.box({0, -0.12f, 0.066f}, {0.10f, 0.24f, 0.15f}, dark);
        m.box({0, 0.139f, -0.22f}, {0.030f, 0.028f, 0.035f}, {0.72f, 0.78f, 0.72f});
        m.box({0, 0.139f, 0.071f}, {0.08f, 0.026f, 0.035f}, dark);
        m.box({0.025f, -0.17f, 0.10f}, {0.15f, 0.19f, 0.19f}, glove);
        m.box({0.057f, -0.22f, 0.20f}, {0.15f, 0.15f, 0.25f}, glove);
    }
    if (flash > 0.0f) {
        const float z = shotgun ? -0.89f : -0.32f;
        const float y = shotgun ? 0.031f : 0.065f;
        const float radius = (shotgun ? 0.21f : 0.14f) * (0.5f + kick * 0.5f);
        // Bright stylized flash, rendered in the weapon's camera-space material.
        m.box({0, y, z}, {radius, radius * 0.40f, radius}, {1.0f, 0.46f, 0.05f});
        m.box({0, y, z}, {radius * 0.36f, radius, radius * 0.68f}, {1.0f, 0.90f, 0.32f});
    }
}
