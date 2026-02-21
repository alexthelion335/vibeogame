#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

struct Potato {
    Vector3 pos{};
    Vector3 vel{};
    float life = 0.0f;
    float radius = 0.22f;
};

struct EnemyPotato {
    Vector3 pos{};
    Vector3 vel{};
    float life = 0.0f;
    float radius = 0.18f;
};

struct Chicken {
    Vector3 pos{};
    float speed = 2.0f;
    float hp = 20.0f;
    float radius = 0.55f;
    float wobble = 0.0f;
    float facingYaw = 0.0f;
    bool isBrown = false;
    float shootCooldown = 0.0f;
};

struct Cloud {
    Vector3 pos{};
    float scale = 1.0f;
    float speed = 1.0f;
};

enum class DeathCause {
    None,
    Peck,
    Potato
};

static float RandRange(float a, float b) {
    return a + (b - a) * (static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f);
}

static Vector3 ForwardFromCamera(const Camera3D& cam) {
    Vector3 fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    return fwd;
}

static Color ScaleColor(Color c, float factor) {
    factor = Clamp(factor, 0.0f, 2.0f);
    Color out{};
    out.r = static_cast<unsigned char>(Clamp(c.r * factor, 0.0f, 255.0f));
    out.g = static_cast<unsigned char>(Clamp(c.g * factor, 0.0f, 255.0f));
    out.b = static_cast<unsigned char>(Clamp(c.b * factor, 0.0f, 255.0f));
    out.a = c.a;
    return out;
}

static float ToonShade(float value) {
    value = Clamp(value, 0.0f, 1.0f);
    if (value < 0.20f) return 0.22f;
    if (value < 0.45f) return 0.45f;
    if (value < 0.70f) return 0.68f;
    if (value < 0.90f) return 0.86f;
    return 1.0f;
}

int main() {
    const int initialScreenWidth = 1400;
    const int initialScreenHeight = 900;
    int screenWidth = initialScreenWidth;
    int screenHeight = initialScreenHeight;

    InitWindow(screenWidth, screenHeight, "Chicken Potato FPS");
    SetTargetFPS(120);
    SetExitKey(KEY_NULL);
    DisableCursor();

    Camera3D camera{};
    camera.position = {0.0f, 1.8f, 6.0f};
    camera.target = {0.0f, 1.8f, 5.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 80.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    float yaw = PI;
    float pitch = 0.0f;

    constexpr float playerHeight = 1.8f;
    constexpr float gravity = 26.0f;
    float verticalVelocity = 0.0f;
    bool grounded = true;

    float playerHealth = 100.0f;
    int score = 0;
    int wave = 1;
    bool dead = false;
    bool paused = false;
    bool shouldExit = false;
    bool fullscreen = true;
    int windowedWidth = screenWidth;
    int windowedHeight = screenHeight;

    const std::vector<Vector2> resolutionOptions = {
        {640, 360},
        {800, 600},
        {960, 540},
        {1024, 576},
        {1152, 648},
        {1280, 720},
        {1366, 768},
        {1600, 900},
        {1920, 1080},
        {2560, 1440}
    };
    int currentResolutionIndex = 7;
    int pauseSelection = 0;

    const int monitor = GetCurrentMonitor();
    const int monitorWidth = GetMonitorWidth(monitor);
    const int monitorHeight = GetMonitorHeight(monitor);

    int bestResolutionIndex = 0;
    int bestResolutionDistance = 1 << 30;
    for (int i = 0; i < static_cast<int>(resolutionOptions.size()); ++i) {
        int dw = static_cast<int>(resolutionOptions[i].x) - monitorWidth;
        int dh = static_cast<int>(resolutionOptions[i].y) - monitorHeight;
        int dist = std::abs(dw) + std::abs(dh);
        if (dist < bestResolutionDistance) {
            bestResolutionDistance = dist;
            bestResolutionIndex = i;
        }
    }
    currentResolutionIndex = bestResolutionIndex;

    SetWindowSize(monitorWidth, monitorHeight);
    ToggleFullscreen();

    std::vector<Potato> potatoes;
    std::vector<EnemyPotato> enemyPotatoes;
    std::vector<Chicken> chickens;
    std::vector<Cloud> clouds;

    float shootCooldown = 0.0f;
    float damageTick = 0.0f;
    float waveSpawnCooldown = 0.0f;
    float waveTitleTimer = 0.0f;
    constexpr float waveTitleDuration = 1.8f;
    int waveTotalToSpawn = 0;
    int waveSpawned = 0;
    int waveBrownToSpawn = 0;
    int waveBrownSpawned = 0;
    DeathCause deathCause = DeathCause::None;

    for (int i = 0; i < 20; ++i) {
        Cloud cloud{};
        cloud.pos = {
            RandRange(-120.0f, 120.0f),
            RandRange(22.0f, 38.0f),
            RandRange(-120.0f, 120.0f)
        };
        cloud.scale = RandRange(1.1f, 2.8f);
        cloud.speed = RandRange(0.6f, 2.0f);
        clouds.push_back(cloud);
    }

    auto spawnChicken = [&](float minDist, float maxDist) {
        float ang = RandRange(0.0f, 2.0f * PI);
        float dist = RandRange(minDist, maxDist);

        int remaining = std::max(1, waveTotalToSpawn - waveSpawned);
        int brownRemaining = std::max(0, waveBrownToSpawn - waveBrownSpawned);
        bool makeBrown = (brownRemaining > 0) && (GetRandomValue(1, remaining) <= brownRemaining);

        Chicken c{};
        c.pos = {
            camera.position.x + std::cosf(ang) * dist,
            0.6f,
            camera.position.z + std::sinf(ang) * dist
        };
        c.speed = RandRange(1.8f, 3.0f) + wave * 0.18f;
        c.hp = 20.0f + wave * 4.0f;
        c.wobble = RandRange(0.0f, 10.0f);
        c.facingYaw = ang + PI;
        c.isBrown = makeBrown;
        c.shootCooldown = RandRange(0.8f, 2.0f);
        chickens.push_back(c);

        if (makeBrown) {
            waveBrownSpawned++;
        }
        waveSpawned++;
    };

    auto startWave = [&](int newWave) {
        wave = newWave;
        waveTotalToSpawn = 8 + (wave - 1) * 4;
        waveBrownToSpawn = std::max(1, static_cast<int>(std::round(waveTotalToSpawn * 0.05f)));
        waveSpawned = 0;
        waveBrownSpawned = 0;
        waveSpawnCooldown = 0.0f;
        waveTitleTimer = waveTitleDuration;
    };

    auto resetGame = [&]() {
        camera.position = {0.0f, 1.8f, 6.0f};
        yaw = PI;
        pitch = 0.0f;
        verticalVelocity = 0.0f;
        grounded = true;
        playerHealth = 100.0f;
        score = 0;
        dead = false;
        deathCause = DeathCause::None;
        potatoes.clear();
        enemyPotatoes.clear();
        chickens.clear();
        shootCooldown = 0.0f;
        damageTick = 0.0f;
        startWave(1);
    };

    auto applyResolutionIndex = [&](int newIndex) {
        currentResolutionIndex = (newIndex + static_cast<int>(resolutionOptions.size())) % static_cast<int>(resolutionOptions.size());
        const int targetW = static_cast<int>(resolutionOptions[currentResolutionIndex].x);
        const int targetH = static_cast<int>(resolutionOptions[currentResolutionIndex].y);
        if (!fullscreen) {
            SetWindowSize(targetW, targetH);
        } else {
            ToggleFullscreen();
            SetWindowSize(targetW, targetH);
            ToggleFullscreen();
        }
    };

    auto toggleFullscreenWindowed = [&]() {
        if (!fullscreen) {
            windowedWidth = GetScreenWidth();
            windowedHeight = GetScreenHeight();
            SetWindowSize(static_cast<int>(resolutionOptions[currentResolutionIndex].x),
                          static_cast<int>(resolutionOptions[currentResolutionIndex].y));
            ToggleFullscreen();
            fullscreen = true;
        } else {
            ToggleFullscreen();
            fullscreen = false;
            SetWindowSize(windowedWidth, windowedHeight);
        }
    };

    startWave(1);

    while (!shouldExit) {
        if (WindowShouldClose()) {
            shouldExit = true;
            continue;
        }

        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        const float dt = GetFrameTime();

        const int panelW = 560;
        const int panelH = 320;
        const int panelX = screenWidth / 2 - panelW / 2;
        const int panelY = screenHeight / 2 - panelH / 2;

        Rectangle resumeRect = {static_cast<float>(panelX + 62), static_cast<float>(panelY + 96), 420.0f, 40.0f};
        Rectangle resolutionRect = {static_cast<float>(panelX + 62), static_cast<float>(panelY + 146), 420.0f, 40.0f};
        Rectangle displayModeRect = {static_cast<float>(panelX + 62), static_cast<float>(panelY + 196), 420.0f, 40.0f};
        Rectangle quitRect = {static_cast<float>(panelX + 62), static_cast<float>(panelY + 246), 420.0f, 40.0f};

        Rectangle resLeftRect = {static_cast<float>(panelX + 440), static_cast<float>(panelY + 148), 26.0f, 32.0f};
        Rectangle resRightRect = {static_cast<float>(panelX + 474), static_cast<float>(panelY + 148), 26.0f, 32.0f};

        if (!dead && IsKeyPressed(KEY_ESCAPE)) {
            paused = !paused;
            pauseSelection = 0;
            if (paused) {
                EnableCursor();
            } else {
                DisableCursor();
            }
        }

        if (!paused && waveTitleTimer > 0.0f) {
            waveTitleTimer = std::max(0.0f, waveTitleTimer - dt);
        }

        for (auto& cloud : clouds) {
            cloud.pos.x += cloud.speed * dt;
            if (cloud.pos.x > 140.0f) cloud.pos.x = -140.0f;
        }

        if (paused && !dead) {
            if (IsKeyPressed(KEY_UP)) {
                pauseSelection = (pauseSelection + 3) % 4;
            }
            if (IsKeyPressed(KEY_DOWN)) {
                pauseSelection = (pauseSelection + 1) % 4;
            }

            if (pauseSelection == 1) {
                if (IsKeyPressed(KEY_LEFT)) {
                    applyResolutionIndex(currentResolutionIndex - 1);
                }
                if (IsKeyPressed(KEY_RIGHT)) {
                    applyResolutionIndex(currentResolutionIndex + 1);
                }
            }

            Vector2 mousePos = GetMousePosition();
            if (CheckCollisionPointRec(mousePos, resumeRect)) pauseSelection = 0;
            else if (CheckCollisionPointRec(mousePos, resolutionRect)) pauseSelection = 1;
            else if (CheckCollisionPointRec(mousePos, displayModeRect)) pauseSelection = 2;
            else if (CheckCollisionPointRec(mousePos, quitRect)) pauseSelection = 3;

            auto triggerPauseAction = [&](int action) {
                if (action == 0) {
                    paused = false;
                    DisableCursor();
                } else if (action == 1) {
                    applyResolutionIndex(currentResolutionIndex + 1);
                } else if (action == 2) {
                    toggleFullscreenWindowed();
                } else if (action == 3) {
                    shouldExit = true;
                }
            };

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (CheckCollisionPointRec(mousePos, resLeftRect)) {
                    applyResolutionIndex(currentResolutionIndex - 1);
                } else if (CheckCollisionPointRec(mousePos, resRightRect)) {
                    applyResolutionIndex(currentResolutionIndex + 1);
                } else if (CheckCollisionPointRec(mousePos, resumeRect)) {
                    triggerPauseAction(0);
                } else if (CheckCollisionPointRec(mousePos, resolutionRect)) {
                    triggerPauseAction(1);
                } else if (CheckCollisionPointRec(mousePos, displayModeRect)) {
                    triggerPauseAction(2);
                } else if (CheckCollisionPointRec(mousePos, quitRect)) {
                    triggerPauseAction(3);
                }
            }

            if (IsKeyPressed(KEY_ENTER)) {
                triggerPauseAction(pauseSelection);
            }
        }

        if (dead) {
            if (IsKeyPressed(KEY_R)) {
                resetGame();
            }
        } else if (!paused) {
            Vector2 mouse = GetMouseDelta();
            const float mouseSensitivity = 0.0026f;
            yaw -= mouse.x * mouseSensitivity;
            pitch -= mouse.y * mouseSensitivity;
            pitch = Clamp(pitch, -1.4f, 1.4f);

            Vector3 forward = {
                std::cosf(pitch) * std::sinf(yaw),
                std::sinf(pitch),
                std::cosf(pitch) * std::cosf(yaw)
            };
            Vector3 flatForward = Vector3Normalize({forward.x, 0.0f, forward.z});
            Vector3 right = Vector3Normalize(Vector3CrossProduct(flatForward, {0.0f, 1.0f, 0.0f}));

            float moveSpeed = IsKeyDown(KEY_LEFT_SHIFT) ? 10.5f : 7.0f;
            Vector3 movement = {0.0f, 0.0f, 0.0f};
            if (IsKeyDown(KEY_W)) movement = Vector3Add(movement, flatForward);
            if (IsKeyDown(KEY_S)) movement = Vector3Subtract(movement, flatForward);
            if (IsKeyDown(KEY_D)) movement = Vector3Add(movement, right);
            if (IsKeyDown(KEY_A)) movement = Vector3Subtract(movement, right);

            if (Vector3Length(movement) > 0.001f) {
                movement = Vector3Scale(Vector3Normalize(movement), moveSpeed * dt);
                camera.position = Vector3Add(camera.position, movement);
            }

            if (grounded && IsKeyPressed(KEY_SPACE)) {
                verticalVelocity = 9.5f;
                grounded = false;
            }

            verticalVelocity -= gravity * dt;
            camera.position.y += verticalVelocity * dt;
            if (camera.position.y <= playerHeight) {
                camera.position.y = playerHeight;
                verticalVelocity = 0.0f;
                grounded = true;
            }

            camera.target = Vector3Add(camera.position, forward);

            shootCooldown -= dt;
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && shootCooldown <= 0.0f) {
                shootCooldown = 0.12f;

                Vector3 muzzle = Vector3Add(camera.position, Vector3Scale(forward, 1.1f));
                muzzle.y -= 0.15f;

                Potato p{};
                p.pos = muzzle;
                p.vel = Vector3Scale(forward, 34.0f);
                p.life = 3.0f;
                potatoes.push_back(p);
            }

            for (auto& p : potatoes) {
                p.life -= dt;
                p.vel.y -= 6.0f * dt;
                p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
            }
            potatoes.erase(
                std::remove_if(potatoes.begin(), potatoes.end(), [](const Potato& p) {
                    return p.life <= 0.0f || p.pos.y < -2.0f;
                }),
                potatoes.end()
            );

            waveSpawnCooldown -= dt;
            if (waveSpawned < waveTotalToSpawn && waveSpawnCooldown <= 0.0f) {
                int remaining = waveTotalToSpawn - waveSpawned;
                int batchSize = std::max(2, std::min(5, 2 + wave / 2));
                int spawnNow = std::min(remaining, batchSize);
                for (int i = 0; i < spawnNow; ++i) {
                    spawnChicken(18.0f, 42.0f);
                }
                waveSpawnCooldown = 1.1f;
            }

            for (auto& c : chickens) {
                c.wobble += dt * 7.0f;
                Vector3 toPlayer = Vector3Subtract(camera.position, c.pos);
                toPlayer.y = 0.0f;
                float d = Vector3Length(toPlayer);
                if (d > 0.01f) {
                    Vector3 dir = Vector3Scale(toPlayer, 1.0f / d);
                    c.facingYaw = std::atan2f(dir.x, dir.z);
                    c.pos = Vector3Add(c.pos, Vector3Scale(dir, c.speed * dt));

                    if (c.isBrown) {
                        c.shootCooldown -= dt;
                        if (c.shootCooldown <= 0.0f && d < 65.0f) {
                            Vector3 muzzle = c.pos;
                            muzzle.y = 0.95f;
                            Vector3 aimTarget = camera.position;
                            aimTarget.y = camera.position.y - 0.45f;

                            Vector3 shotDir = Vector3Normalize(Vector3Subtract(aimTarget, muzzle));
                            EnemyPotato ep{};
                            ep.pos = muzzle;
                            ep.vel = Vector3Scale(shotDir, 20.0f + wave * 0.12f);
                            ep.life = 5.5f;
                            enemyPotatoes.push_back(ep);

                            c.shootCooldown = RandRange(1.2f, 2.2f);
                        }
                    }
                }
            }

            for (auto& ep : enemyPotatoes) {
                ep.life -= dt;
                ep.vel.y -= 5.0f * dt;
                ep.pos = Vector3Add(ep.pos, Vector3Scale(ep.vel, dt));
                if (Vector3Distance(ep.pos, camera.position) < 0.55f) {
                    ep.life = 0.0f;
                    float prevHealth = playerHealth;
                    playerHealth -= 11.0f;
                    if (prevHealth > 0.0f && playerHealth <= 0.0f) {
                        deathCause = DeathCause::Potato;
                    }
                }
            }
            enemyPotatoes.erase(
                std::remove_if(enemyPotatoes.begin(), enemyPotatoes.end(), [](const EnemyPotato& ep) {
                    return ep.life <= 0.0f || ep.pos.y < -2.0f;
                }),
                enemyPotatoes.end()
            );

            for (auto& c : chickens) {
                for (auto& p : potatoes) {
                    if (p.life <= 0.0f) continue;
                    float hitDist = c.radius + p.radius;
                    if (Vector3Distance(c.pos, p.pos) < hitDist) {
                        p.life = 0.0f;
                        c.hp -= 22.0f;
                    }
                }
            }

            int before = static_cast<int>(chickens.size());
            chickens.erase(
                std::remove_if(chickens.begin(), chickens.end(), [](const Chicken& c) {
                    return c.hp <= 0.0f;
                }),
                chickens.end()
            );
            score += (before - static_cast<int>(chickens.size())) * 10;

            if (chickens.empty() && waveSpawned >= waveTotalToSpawn) {
                startWave(wave + 1);
            }

            damageTick += dt;
            if (damageTick >= 0.2f) {
                damageTick = 0.0f;
                for (const auto& c : chickens) {
                    if (c.isBrown) continue;
                    Vector3 toPlayer = Vector3Subtract(camera.position, c.pos);
                    toPlayer.y = 0.0f;
                    float d = Vector3Length(toPlayer);
                    if (d < 1.35f) {
                        float prevHealth = playerHealth;
                        playerHealth -= 4.0f;
                        if (prevHealth > 0.0f && playerHealth <= 0.0f) {
                            deathCause = DeathCause::Peck;
                        }
                    }
                }
            }

            if (playerHealth <= 0.0f) {
                playerHealth = 0.0f;
                dead = true;
                EnableCursor();
            }
        }

        if (!dead && !paused && IsCursorHidden() == false) {
            DisableCursor();
        }

        BeginDrawing();
        ClearBackground({120, 190, 250, 255});

        BeginMode3D(camera);

        const Vector3 lightDir = Vector3Normalize({-0.55f, -1.0f, -0.35f});
        const Vector3 sunDir = Vector3Negate(lightDir);
        const Vector3 sunPos = Vector3Add(camera.position, Vector3Scale(sunDir, 145.0f));
        const float safeSunY = std::max(0.15f, sunDir.y);

        auto ProjectToGroundFromSun = [&](Vector3 p) {
            float t = p.y / safeSunY;
            Vector3 q = Vector3Subtract(p, Vector3Scale(sunDir, t));
            q.y = 0.03f;
            return q;
        };

        auto DrawProjectedShadowCapsule = [&](Vector3 a, Vector3 b, float ra, float rb, Color shadowColor) {
            Vector3 pa = ProjectToGroundFromSun(a);
            Vector3 pb = ProjectToGroundFromSun(b);
            float d = Vector3Distance(pa, pb);
            int steps = std::max(1, static_cast<int>(d / 0.28f));
            for (int i = 0; i <= steps; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(steps);
                Vector3 p = Vector3Lerp(pa, pb, t);
                float r = ra + (rb - ra) * t;
                DrawCylinder(p, r, r, 0.045f, 16, shadowColor);
            }
        };

        float sunlightLinear = 0.30f + 0.70f * Clamp(sunDir.y, 0.0f, 1.0f);
        float sunlight = ToonShade(sunlightLinear);

        DrawSphere(sunPos, 4.0f, {255, 236, 150, 255});
        DrawSphere(sunPos, 5.8f, Fade({255, 220, 120, 255}, 0.24f));

        for (const auto& cloud : clouds) {
            Vector3 c = {
                camera.position.x + cloud.pos.x,
                cloud.pos.y,
                camera.position.z + cloud.pos.z
            };
            Color cloudBright = ScaleColor(RAYWHITE, 0.85f + 0.25f * sunlight);
            cloudBright.a = 210;
            DrawSphere(c, 1.5f * cloud.scale, cloudBright);
            DrawSphere({c.x + 1.4f * cloud.scale, c.y + 0.3f * cloud.scale, c.z}, 1.2f * cloud.scale, cloudBright);
            DrawSphere({c.x - 1.3f * cloud.scale, c.y + 0.2f * cloud.scale, c.z + 0.4f * cloud.scale}, 1.1f * cloud.scale, cloudBright);
            DrawSphere({c.x + 0.2f * cloud.scale, c.y + 0.4f * cloud.scale, c.z - 0.8f * cloud.scale}, 1.15f * cloud.scale, cloudBright);
        }

        DrawPlane({0.0f, 0.0f, 0.0f}, {300.0f, 300.0f}, ScaleColor(DARKGREEN, 0.65f + 0.35f * sunlight));

        for (int x = -6; x <= 6; ++x) {
            Vector3 treeA = {x * 12.0f, 2.0f, -50.0f};
            Vector3 treeB = {x * 12.0f, 2.0f, 50.0f};
            Vector3 shadowA = ProjectToGroundFromSun(treeA);
            Vector3 shadowB = ProjectToGroundFromSun(treeB);
            DrawCylinder(shadowA, 0.55f, 0.55f, 0.05f, 18, Fade(BLACK, 0.25f));
            DrawProjectedShadowCapsule({x * 12.0f, 4.0f, -50.0f}, {x * 12.0f, 4.0f, -50.0f}, 1.65f, 1.65f, Fade(BLACK, 0.20f));
            DrawCube({x * 12.0f, 2.0f, -50.0f}, 1.2f, 4.0f, 1.2f, ScaleColor(BROWN, 0.7f + 0.35f * sunlight));
            DrawCube({x * 12.0f, 4.0f, -50.0f}, 4.0f, 1.2f, 4.0f, ScaleColor(DARKGREEN, 0.75f + 0.30f * sunlight));
            DrawCylinder(shadowB, 0.55f, 0.55f, 0.05f, 18, Fade(BLACK, 0.25f));
            DrawProjectedShadowCapsule({x * 12.0f, 4.0f, 50.0f}, {x * 12.0f, 4.0f, 50.0f}, 1.65f, 1.65f, Fade(BLACK, 0.20f));
            DrawCube({x * 12.0f, 2.0f, 50.0f}, 1.2f, 4.0f, 1.2f, ScaleColor(BROWN, 0.7f + 0.35f * sunlight));
            DrawCube({x * 12.0f, 4.0f, 50.0f}, 4.0f, 1.2f, 4.0f, ScaleColor(DARKGREEN, 0.75f + 0.30f * sunlight));
        }
        for (int z = -4; z <= 4; ++z) {
            Vector3 treeA = {-70.0f, 2.0f, z * 12.0f};
            Vector3 treeB = {70.0f, 2.0f, z * 12.0f};
            Vector3 shadowA = ProjectToGroundFromSun(treeA);
            Vector3 shadowB = ProjectToGroundFromSun(treeB);
            DrawCylinder(shadowA, 0.55f, 0.55f, 0.05f, 18, Fade(BLACK, 0.25f));
            DrawProjectedShadowCapsule({-70.0f, 4.0f, z * 12.0f}, {-70.0f, 4.0f, z * 12.0f}, 1.65f, 1.65f, Fade(BLACK, 0.20f));
            DrawCube({-70.0f, 2.0f, z * 12.0f}, 1.2f, 4.0f, 1.2f, ScaleColor(BROWN, 0.7f + 0.35f * sunlight));
            DrawCube({-70.0f, 4.0f, z * 12.0f}, 4.0f, 1.2f, 4.0f, ScaleColor(DARKGREEN, 0.75f + 0.30f * sunlight));
            DrawCylinder(shadowB, 0.55f, 0.55f, 0.05f, 18, Fade(BLACK, 0.25f));
            DrawProjectedShadowCapsule({70.0f, 4.0f, z * 12.0f}, {70.0f, 4.0f, z * 12.0f}, 1.65f, 1.65f, Fade(BLACK, 0.20f));
            DrawCube({70.0f, 2.0f, z * 12.0f}, 1.2f, 4.0f, 1.2f, ScaleColor(BROWN, 0.7f + 0.35f * sunlight));
            DrawCube({70.0f, 4.0f, z * 12.0f}, 4.0f, 1.2f, 4.0f, ScaleColor(DARKGREEN, 0.75f + 0.30f * sunlight));
        }

        for (const auto& c : chickens) {
            Vector3 bodyPos = c.pos;
            bodyPos.y = 0.65f + std::sinf(c.wobble) * 0.06f;
            Vector3 forwardDir = {std::sinf(c.facingYaw), 0.0f, std::cosf(c.facingYaw)};
            Vector3 rightDir = {forwardDir.z, 0.0f, -forwardDir.x};

            // More accurate dynamic planar shadows (projected from sun direction)
            DrawProjectedShadowCapsule(
                Vector3Add(bodyPos, Vector3Scale(forwardDir, -0.22f)),
                Vector3Add(bodyPos, Vector3Scale(forwardDir, 0.34f)),
                0.34f,
                0.30f,
                Fade(BLACK, 0.35f)
            );
            DrawProjectedShadowCapsule(
                Vector3Add(bodyPos, {0.0f, 0.28f, 0.0f}),
                Vector3Add(bodyPos, Vector3Scale(forwardDir, 0.48f)),
                0.18f,
                0.20f,
                Fade(BLACK, 0.30f)
            );
            DrawProjectedShadowCapsule(
                Vector3Add(bodyPos, Vector3Add(Vector3Scale(rightDir, 0.13f), {0.0f, -0.22f, 0.08f})),
                Vector3Add(bodyPos, Vector3Add(Vector3Scale(rightDir, 0.13f), {0.0f, -0.58f, 0.18f})),
                0.07f,
                0.06f,
                Fade(BLACK, 0.22f)
            );
            DrawProjectedShadowCapsule(
                Vector3Add(bodyPos, Vector3Add(Vector3Scale(rightDir, -0.13f), {0.0f, -0.22f, 0.08f})),
                Vector3Add(bodyPos, Vector3Add(Vector3Scale(rightDir, -0.13f), {0.0f, -0.58f, 0.18f})),
                0.07f,
                0.06f,
                Fade(BLACK, 0.22f)
            );

            // Keep lighting stable (no wobble-driven glow/pulsing)
            Vector3 pseudoNormal = Vector3Normalize(Vector3Add(forwardDir, {0.0f, 0.82f, 0.0f}));
            float bodyLight = ToonShade(0.25f + 0.75f * Clamp(Vector3DotProduct(pseudoNormal, sunDir), 0.0f, 1.0f));

            Vector3 headNormal = Vector3Normalize(Vector3Add(forwardDir, {0.0f, 0.9f, 0.0f}));
            Vector3 wingNormal = Vector3Normalize(Vector3Add(rightDir, {0.0f, 0.45f, 0.0f}));
            Vector3 feetNormal = Vector3Normalize({0.0f, 0.70f, 0.25f});
            Vector3 combNormal = Vector3Normalize(Vector3Add(forwardDir, {0.0f, 0.85f, 0.0f}));
            Vector3 beakNormal = Vector3Normalize(Vector3Add(forwardDir, {0.0f, 0.35f, 0.0f}));

            float headLight = ToonShade(0.25f + 0.75f * Clamp(Vector3DotProduct(headNormal, sunDir), 0.0f, 1.0f));
            float wingLight = ToonShade(0.25f + 0.75f * Clamp(Vector3DotProduct(wingNormal, sunDir), 0.0f, 1.0f));
            float feetLight = ToonShade(0.25f + 0.75f * Clamp(Vector3DotProduct(feetNormal, sunDir), 0.0f, 1.0f));
            float combLight = ToonShade(0.25f + 0.75f * Clamp(Vector3DotProduct(combNormal, sunDir), 0.0f, 1.0f));
            float beakLight = ToonShade(0.25f + 0.75f * Clamp(Vector3DotProduct(beakNormal, sunDir), 0.0f, 1.0f));

            auto LocalToChicken = [&](float right, float up, float forward) {
                Vector3 p = bodyPos;
                p = Vector3Add(p, Vector3Scale(rightDir, right));
                p = Vector3Add(p, {0.0f, up, 0.0f});
                p = Vector3Add(p, Vector3Scale(forwardDir, forward));
                return p;
            };

            float step = std::sinf(c.wobble * 1.8f);
            float footLiftL = std::max(0.0f, step) * 0.06f;
            float footLiftR = std::max(0.0f, -step) * 0.06f;

            Color baseFeatherColor = c.isBrown ? BROWN : WHITE;
            Color accentFeatherColor = c.isBrown ? DARKBROWN : RAYWHITE;

            Color bodyColor = ScaleColor(baseFeatherColor, 1.05f * bodyLight);
            Color chestColor = ScaleColor(accentFeatherColor, 0.95f * bodyLight);
            Color wingColor = ScaleColor(accentFeatherColor, 0.92f * wingLight);
            Color wingRibColor = ScaleColor(baseFeatherColor, 0.78f * wingLight);
            Color headColor = ScaleColor(baseFeatherColor, 1.08f * headLight);
            Vector3 beakBase = LocalToChicken(0.0f, 0.33f, 0.60f);
            Vector3 beakTip = LocalToChicken(0.0f, 0.30f, 0.77f);

            DrawSphere(LocalToChicken(0.0f, 0.00f, -0.02f), 0.47f, bodyColor);
            DrawSphere(LocalToChicken(0.0f, 0.05f, 0.22f), 0.38f, chestColor);

            DrawSphere(LocalToChicken(0.34f, 0.04f, 0.00f), 0.19f, wingColor);
            DrawSphere(LocalToChicken(-0.34f, 0.04f, 0.00f), 0.19f, wingColor);
            DrawCube(LocalToChicken(0.34f, 0.01f, 0.04f), 0.08f, 0.18f, 0.30f, wingRibColor);
            DrawCube(LocalToChicken(-0.34f, 0.01f, 0.04f), 0.08f, 0.18f, 0.30f, wingRibColor);

            Vector3 head = LocalToChicken(0.0f, 0.33f, 0.42f);
            DrawSphere(head, 0.24f, headColor);
            DrawSphere(LocalToChicken(0.00f, 0.57f, 0.40f), 0.10f, ScaleColor(RED, 0.95f * combLight));
            DrawSphere(LocalToChicken(0.07f, 0.41f, 0.59f), 0.038f, RAYWHITE);
            DrawSphere(LocalToChicken(-0.07f, 0.41f, 0.59f), 0.038f, RAYWHITE);
            DrawSphere(LocalToChicken(0.07f, 0.405f, 0.615f), 0.016f, BLACK);
            DrawSphere(LocalToChicken(-0.07f, 0.405f, 0.615f), 0.016f, BLACK);

            DrawCylinderEx(beakBase, beakTip, 0.05f, 0.01f, 10, ScaleColor(ORANGE, 0.9f + 0.2f * beakLight));

            Vector3 leftHip = LocalToChicken(0.12f, -0.24f, 0.10f);
            Vector3 rightHip = LocalToChicken(-0.12f, -0.24f, 0.10f);
            Vector3 leftFoot = LocalToChicken(0.12f, -0.58f + footLiftL, 0.13f);
            Vector3 rightFoot = LocalToChicken(-0.12f, -0.58f + footLiftR, 0.13f);
            Color legColor = ScaleColor(ORANGE, 0.75f + 0.45f * feetLight);

            DrawCylinderEx(leftHip, leftFoot, 0.022f, 0.018f, 8, legColor);
            DrawCylinderEx(rightHip, rightFoot, 0.022f, 0.018f, 8, legColor);

            DrawCylinderEx(leftFoot, LocalToChicken(0.12f, -0.60f + footLiftL, 0.24f), 0.012f, 0.008f, 8, legColor);
            DrawCylinderEx(leftFoot, LocalToChicken(0.17f, -0.60f + footLiftL, 0.21f), 0.010f, 0.007f, 8, legColor);
            DrawCylinderEx(leftFoot, LocalToChicken(0.07f, -0.60f + footLiftL, 0.21f), 0.010f, 0.007f, 8, legColor);

            DrawCylinderEx(rightFoot, LocalToChicken(-0.12f, -0.60f + footLiftR, 0.24f), 0.012f, 0.008f, 8, legColor);
            DrawCylinderEx(rightFoot, LocalToChicken(-0.07f, -0.60f + footLiftR, 0.21f), 0.010f, 0.007f, 8, legColor);
            DrawCylinderEx(rightFoot, LocalToChicken(-0.17f, -0.60f + footLiftR, 0.21f), 0.010f, 0.007f, 8, legColor);

            if (c.isBrown) {
                Vector3 cannonBase = LocalToChicken(0.24f, 0.02f, 0.14f);
                Vector3 cannonTip = LocalToChicken(0.24f, 0.02f, 0.58f);
                DrawCylinderEx(cannonBase, cannonTip, 0.055f, 0.045f, 10, ScaleColor(DARKGRAY, 0.9f + 0.3f * bodyLight));
            }

            float hpRatio = c.hp / (20.0f + wave * 4.0f);
            hpRatio = Clamp(hpRatio, 0.0f, 1.0f);
            DrawCube({bodyPos.x, bodyPos.y + 1.08f, bodyPos.z}, 0.8f * hpRatio, 0.05f, 0.05f, RED);
        }

        for (const auto& p : potatoes) {
            float pa = Clamp(0.42f - p.pos.y * 0.025f, 0.12f, 0.42f);
            DrawProjectedShadowCapsule(p.pos, p.pos, p.radius * 0.82f, p.radius * 0.82f, Fade(BLACK, pa));
            DrawSphere(p.pos, p.radius, {176, 122, 58, 255});
        }

        for (const auto& ep : enemyPotatoes) {
            float pa = Clamp(0.42f - ep.pos.y * 0.025f, 0.12f, 0.42f);
            DrawProjectedShadowCapsule(ep.pos, ep.pos, ep.radius * 0.80f, ep.radius * 0.80f, Fade(BLACK, pa));
            DrawSphere(ep.pos, ep.radius, {118, 76, 38, 255});
        }

        Vector3 camForward = ForwardFromCamera(camera);
        Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, camera.up));
        Vector3 camUp = Vector3Normalize(Vector3CrossProduct(camRight, camForward));

        auto ToCameraLocal = [&](float right, float up, float forward) {
            Vector3 p = camera.position;
            p = Vector3Add(p, Vector3Scale(camRight, right));
            p = Vector3Add(p, Vector3Scale(camUp, up));
            p = Vector3Add(p, Vector3Scale(camForward, forward));
            return p;
        };

        Vector3 gunStockTop = ToCameraLocal(0.26f, -0.28f, 0.70f);
        Vector3 gunStockBottom = ToCameraLocal(0.26f, -0.38f, 0.70f);
        Vector3 barrelStart = ToCameraLocal(0.26f, -0.24f, 0.95f);
        Vector3 barrelEnd = ToCameraLocal(0.26f, -0.24f, 1.46f);

        DrawCylinderEx(gunStockTop, gunStockBottom, 0.09f, 0.13f, 14, DARKBROWN);
        DrawCylinderEx(barrelStart, barrelEnd, 0.06f, 0.06f, 14, ScaleColor(GRAY, 0.75f + 0.4f * sunlight));

        EndMode3D();

        DrawRectangle(20, 20, 340, 115, Fade(BLACK, 0.45f));
        DrawText(TextFormat("HP: %i", static_cast<int>(playerHealth)), 36, 35, 30, playerHealth > 30.0f ? GREEN : RED);
        DrawText(TextFormat("Score: %i", score), 36, 67, 26, YELLOW);
        DrawText(TextFormat("Wave: %i", wave), 36, 96, 26, SKYBLUE);

        if (!paused) {
            DrawLine(screenWidth / 2 - 10, screenHeight / 2, screenWidth / 2 + 10, screenHeight / 2, WHITE);
            DrawLine(screenWidth / 2, screenHeight / 2 - 10, screenWidth / 2, screenHeight / 2 + 10, WHITE);
        }

        DrawText("Potato Cannon vs Chicken Horde", 20, screenHeight - 40, 24, BLACK);

        if (!dead && waveTitleTimer > 0.0f) {
            float t = waveTitleTimer / waveTitleDuration;
            float alpha = (t > 0.75f) ? (1.0f - (t - 0.75f) / 0.25f) : (t / 0.75f);
            alpha = Clamp(alpha, 0.0f, 1.0f);

            const int bannerW = 420;
            const int bannerH = 72;
            const int bannerX = screenWidth / 2 - bannerW / 2;
            const int bannerY = screenHeight / 3 - bannerH / 2;
            DrawRectangle(bannerX, bannerY, bannerW, bannerH, Fade(BLACK, 0.45f * alpha));
            DrawText(TextFormat("WAVE %i", wave), screenWidth / 2 - 90, bannerY + 20, 36, Fade(YELLOW, alpha));
        }

        if (dead) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.6f));
            const char* deathText = "YOU DIED";
            if (deathCause == DeathCause::Peck) deathText = "YOU WERE PECKED TO DEATH";
            else if (deathCause == DeathCause::Potato) deathText = "YOU WERE POTATO-BLASTED";

            const int deathFontSize = 54;
            const int deathTextWidth = MeasureText(deathText, deathFontSize);
            DrawText(deathText, (screenWidth - deathTextWidth) / 2, screenHeight / 2 - 80, deathFontSize, RED);
            DrawText(TextFormat("Final Score: %i", score), screenWidth / 2 - 130, screenHeight / 2 + 4, 30, RAYWHITE);
            DrawText("Press R to restart", screenWidth / 2 - 130, screenHeight / 2 + 42, 26, YELLOW);
        }

        if (paused && !dead) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.55f));
            DrawRectangle(panelX, panelY, panelW, panelH, Fade(DARKGRAY, 0.90f));
            DrawRectangleLinesEx({static_cast<float>(panelX), static_cast<float>(panelY), static_cast<float>(panelW), static_cast<float>(panelH)}, 3, YELLOW);

            DrawText("PAUSED", panelX + 210, panelY + 22, 44, RAYWHITE);

            Color resumeColor = (pauseSelection == 0) ? YELLOW : RAYWHITE;
            Color resColor = (pauseSelection == 1) ? YELLOW : RAYWHITE;
            Color displayModeColor = (pauseSelection == 2) ? YELLOW : RAYWHITE;
            Color quitColor = (pauseSelection == 3) ? YELLOW : RAYWHITE;

            DrawText("Resume", panelX + 70, panelY + 100, 34, resumeColor);
            DrawText(TextFormat("Resolution: %ix%i", static_cast<int>(resolutionOptions[currentResolutionIndex].x), static_cast<int>(resolutionOptions[currentResolutionIndex].y)), panelX + 70, panelY + 152, 30, resColor);
            DrawText("<", panelX + 444, panelY + 152, 30, resColor);
            DrawText(">", panelX + 478, panelY + 152, 30, resColor);
            DrawText(TextFormat("Display Mode: %s", fullscreen ? "Fullscreen" : "Windowed"), panelX + 70, panelY + 202, 30, displayModeColor);
            DrawText("Quit Game", panelX + 70, panelY + 252, 34, quitColor);

            DrawText("Mouse: Click items   Resolution arrows work with mouse", panelX + 70, panelY + 286, 18, LIGHTGRAY);
            DrawText("UP/DOWN + ENTER still work   ESC: Resume", panelX + 70, panelY + 304, 18, LIGHTGRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
