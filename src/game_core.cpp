#include "game_core.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

// Global platform bridge instance (set by main.cpp)
IPlatformBridge* g_platform_bridge = nullptr;

namespace game_core {

// Constants from original
constexpr float PLAYER_HEIGHT = 1.8f;
constexpr float GRAVITY = 26.0f;
constexpr float WAVE_TITLE_DURATION = 1.8f;
constexpr float SHIELD_DURATION = 10.0f;
constexpr float SHOP_DURATION = 10.0f;
constexpr float PI = 3.14159265359f;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

float random_range(float a, float b) {
    if (!g_platform_bridge) return a;
    return g_platform_bridge->get_random(a, b);
}

Vector3 net_vec3_to_vector3(const NetVec3& v) {
    return Vector3(v.x, v.y, v.z);
}

NetVec3 vector3_to_net_vec3(const Vector3& v) {
    return NetVec3{v.x, v.y, v.z};
}

static float clamp(float val, float min_val, float max_val) {
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

static float distance(const Vector3& a, const Vector3& b) {
    Vector3 diff = a - b;
    return diff.length();
}

// ============================================================================
// INITIALIZATION & RESET
// ============================================================================

void initialize(GameState& state) {
    // Generate initial clouds
    for (int i = 0; i < 20; ++i) {
        Cloud cloud{};
        cloud.pos = Vector3(
            random_range(-120.0f, 120.0f),
            random_range(22.0f, 38.0f),
            random_range(-120.0f, 120.0f)
        );
        cloud.scale = random_range(1.1f, 2.8f);
        cloud.speed = random_range(0.6f, 2.0f);
        state.clouds.push_back(cloud);
    }
    
    // Initialize inventory
    state.inventory[0] = {WeaponType::PotatoCannon, true};
    state.inventory[1] = {WeaponType::PotatoShotgun, false};
    state.inventory[2] = {WeaponType::PotatoGrenade, false};
    
    // Load highscores
    if (g_platform_bridge) {
        std::string user_path = g_platform_bridge->get_user_data_path();
        load_highscores(state, user_path + "/highscores.txt");
    }
    
    // Start first wave
    start_wave(state, 1);
}

void reset_game(GameState& state) {
    state.camera_position = Vector3(0.0f, 1.8f, 6.0f);
    state.yaw = PI;
    state.pitch = 0.0f;
    state.vertical_velocity = 0.0f;
    state.grounded = true;
    state.player_health = 100.0f;
    state.coop_health = 100.0f;
    state.coop_pos = Vector3(2.5f, PLAYER_HEIGHT, 8.0f);
    state.coop_yaw = PI;
    state.coop_pitch = 0.0f;
    state.coop_vertical_velocity = 0.0f;
    state.coop_grounded = true;
    state.coop_shoot_cooldown = 0.0f;
    
    state.dead = false;
    state.death_cause = DeathCause::None;
    state.score = 0;
    state.paused = false;
    state.damage_flash_timer = 0.0f;
    state.knockback_vel = Vector3(0.0f, 0.0f, 0.0f);
    state.prev_health = 100.0f;
    state.audio_events.clear();
    
    state.potatoes.clear();
    state.enemy_potatoes.clear();
    state.chickens.clear();
    state.confetti.clear();
    state.med_packs.clear();
    state.grenades.clear();
    
    state.shoot_cooldown = 0.0f;
    state.damage_tick = 0.0f;
    state.wave_spawn_cooldown = 0.0f;
    state.wave_title_timer = 0.0f;
    
    state.current_weapon_slot = 0;
    state.fire_rate_upgrade_count = 0;
    state.has_shield_charge = false;
    state.shield_active = false;
    state.shield_timer = 0.0f;
    state.owns_scythe = false;
    state.scythe_cooldown = 0.0f;
    state.scythe_swing_visual_timer = 0.0f;
    
    state.inventory[0] = {WeaponType::PotatoCannon, true};
    state.inventory[1] = {WeaponType::PotatoShotgun, false};
    state.inventory[2] = {WeaponType::PotatoGrenade, false};
    
    state.highscore_submitted_this_run = false;
    state.entering_initials = false;
    state.pending_initials = "AAA";
    state.viewing_highscores = false;
    
    state.shop_open = false;
    state.shop_timer = 0.0f;
    state.inventory_open = false;
    state.inventory_swap_from = -1;
    state.pause_selection = 0;
    
    state.chicken_nuggets = 0;
    state.client_last_wave = 1;
    state.client_target_pos = Vector3(2.5f, PLAYER_HEIGHT, 8.0f);
    state.client_host_target_pos = Vector3(0.0f, PLAYER_HEIGHT, 6.0f);
    state.client_host_target_yaw = PI;
    state.remote_input = RemoteInputState{};
    state.remote_input.yaw = PI;
    
    start_wave(state, 1);
}

void start_wave(GameState& state, int wave_num) {
    state.wave = wave_num;
    state.wave_total_to_spawn = 8 + (wave_num - 1) * 4;
    state.wave_brown_to_spawn = std::max(1, static_cast<int>(
        std::round(state.wave_total_to_spawn * 0.05f)));
    state.wave_spawned = 0;
    state.wave_brown_spawned = 0;
    state.wave_spawn_cooldown = 0.0f;
    state.wave_title_timer = WAVE_TITLE_DURATION;
}

// ============================================================================
// WAVE SPAWNING
// ============================================================================

static void spawn_chicken(GameState& state, float min_dist, float max_dist) {
    float ang = random_range(0.0f, 2.0f * PI);
    float dist = random_range(min_dist, max_dist);
    
    int remaining = std::max(1, state.wave_total_to_spawn - state.wave_spawned);
    int brown_remaining = std::max(0, state.wave_brown_to_spawn - state.wave_brown_spawned);
    
    bool make_brown = false;
    if (brown_remaining > 0 && g_platform_bridge) {
        int rand_val = g_platform_bridge->get_random_int(1, remaining);
        make_brown = (rand_val <= brown_remaining);
    }
    
    Chicken c{};
    c.pos = Vector3(
        state.camera_position.x + std::cos(ang) * dist,
        0.6f,
        state.camera_position.z + std::sin(ang) * dist
    );
    c.speed = random_range(1.8f, 3.0f) + state.wave * 0.18f;
    c.hp = 20.0f + state.wave * 4.0f;
    c.wobble = random_range(0.0f, 10.0f);
    c.facingYaw = ang + PI;
    c.isBrown = make_brown;
    c.shootCooldown = random_range(0.8f, 2.0f);
    
    state.chickens.push_back(c);
    
    if (make_brown) {
        state.wave_brown_spawned++;
    }
    state.wave_spawned++;
}

// ============================================================================
// HIGHSCORES
// ============================================================================

void load_highscores(GameState& state, const std::string& file_path) {
    // Initialize with defaults
    for (auto& entry : state.highscores) {
        entry = {"---", 0};
    }
    
    if (!g_platform_bridge) return;
    
    std::string content = g_platform_bridge->read_file(file_path);
    if (content.empty()) return;
    
    std::istringstream iss(content);
    std::string line;
    int idx = 0;
    
    while (idx < MAX_HIGHSCORES && std::getline(iss, line)) {
        if (line.empty()) continue;
        
        std::istringstream line_stream(line);
        std::string initials;
        int score = 0;
        
        if (!(line_stream >> initials >> score)) {
            continue;
        }
        
        if (initials.empty()) {
            initials = "---";
        }
        
        // Uppercase and pad/truncate
        for (char& ch : initials) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        if (initials.size() > 3) initials = initials.substr(0, 3);
        while (initials.size() < 3) initials.push_back('-');
        
        state.highscores[idx++] = {initials, std::max(0, score)};
    }
    
    // Sort by score descending
    std::sort(state.highscores.begin(), state.highscores.end(),
        [](const HighscoreEntry& a, const HighscoreEntry& b) {
            return a.score > b.score;
        });
}

void save_highscores(const GameState& state, const std::string& file_path) {
    if (!g_platform_bridge) return;
    
    std::string content;
    for (const auto& entry : state.highscores) {
        content += entry.initials + " " + std::to_string(entry.score) + "\n";
    }
    
    g_platform_bridge->write_file(file_path, content);
}

bool score_qualifies(int score, const GameState& state) {
    return score > 0 && score > state.highscores.back().score;
}

void submit_highscore(GameState& state, const std::string& initials) {
    std::string fixed = initials;
    
    // Validate and uppercase
    for (char& ch : fixed) {
        if (!std::isalpha(static_cast<unsigned char>(ch))) {
            ch = 'A';
        }
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    
    // Pad/truncate to 3 chars
    while (fixed.size() < 3) fixed.push_back('A');
    if (fixed.size() > 3) fixed = fixed.substr(0, 3);
    
    // Insert and sort
    state.highscores.back() = {fixed, state.score};
    std::sort(state.highscores.begin(), state.highscores.end(),
        [](const HighscoreEntry& a, const HighscoreEntry& b) {
            return a.score > b.score;
        });
    
    // Save
    if (g_platform_bridge) {
        std::string user_path = g_platform_bridge->get_user_data_path();
        save_highscores(state, user_path + "/highscores.txt");
    }
    
    state.highscore_submitted_this_run = true;
    state.entering_initials = false;
}

// ============================================================================
// NETWORKING (STUBS for now; actual socket logic stays in main.cpp)
// ============================================================================

bool setup_host(GameState& state, int port) {
    state.net_role = NetRole::Host;
    state.net_has_peer = false;
    state.net_status = "Waiting for client...";
    return true;  // Actual socket creation in main.cpp
}

bool setup_client(GameState& state, const std::string& host, int port) {
    state.net_role = NetRole::Client;
    state.net_has_peer = false;
    state.net_status = "Connecting to host...";
    return true;  // Actual socket creation in main.cpp
}

void close_network(GameState& state) {
    state.net_role = NetRole::None;
    state.net_has_peer = false;
    state.net_disconnected = false;
    state.net_last_receive_time = 0.0;
    state.net_status.clear();
}

// ============================================================================
// MAIN GAME UPDATE (Non-rendering gameplay logic)
// ============================================================================

void update(GameState& state, const InputState& input, float delta) {
    // Clear audio events from last frame
    state.audio_events.clear();
    
    // Update clouds
    constexpr float CLOUD_WRAP_DIST = 140.0f;
    for (auto& cloud : state.clouds) {
        cloud.pos.x += cloud.speed * delta;
        if (cloud.pos.x > CLOUD_WRAP_DIST) {
            cloud.pos.x = -CLOUD_WRAP_DIST;
        }
    }
    
    // Update wave title timer
    if (!state.paused && state.wave_title_timer > 0.0f) {
        state.wave_title_timer = std::max(0.0f, state.wave_title_timer - delta);
    }
    
    // Gameplay update (when not paused and not dead, or during online client mode)
    bool online_client = (state.game_mode == GameMode::Online && state.net_role == NetRole::Client);
    
    if (!state.dead || online_client) {
        // === PLAYER MOVEMENT ===
        if (state.player_health > 0.0f) {
            Vector3 forward{
                std::cos(state.pitch) * std::sin(state.yaw),
                std::sin(state.pitch),
                std::cos(state.pitch) * std::cos(state.yaw)
            };
            Vector3 flat_forward = Vector3(forward.x, 0.0f, forward.z).normalized();
            Vector3 right = flat_forward.cross(Vector3(0.0f, 1.0f, 0.0f)).normalized();
            
            Vector3 movement{0.0f, 0.0f, 0.0f};
            float move_speed = 5.0f;
            float sprint_multiplier = 1.5f;
            
            if (input.move_forward) movement = movement + flat_forward * move_speed;
            if (input.move_backward) movement = movement - flat_forward * move_speed;
            if (input.move_left) movement = movement - right * move_speed;
            if (input.move_right) movement = movement + right * move_speed;
            
            if (input.sprint) {
                movement = movement * sprint_multiplier;
            }
            
            state.camera_position = state.camera_position + movement * delta;
            
            // Gravity & jump
            state.vertical_velocity -= GRAVITY * delta;
            state.camera_position.y += state.vertical_velocity * delta;
            
            if (state.camera_position.y <= PLAYER_HEIGHT) {
                state.camera_position.y = PLAYER_HEIGHT;
                state.vertical_velocity = 0.0f;
                state.grounded = true;
                
                if (input.jump) {
                    state.vertical_velocity = 10.0f;
                    state.grounded = false;
                }
            } else {
                state.grounded = false;
            }
            
            // Camera rotation (from mouse or touch)
            float look_sensitivity = 0.003f;
            if (input.mouse_delta_x != 0.0f || input.mouse_delta_y != 0.0f) {
                state.yaw += input.mouse_delta_x * look_sensitivity;
                state.pitch += input.mouse_delta_y * look_sensitivity;
                state.pitch = clamp(state.pitch, -1.5708f, 1.5708f);  // ±π/2
            }
            
            // Touch camera control
            if (input.touch_camera_delta_x != 0.0f || input.touch_camera_delta_y != 0.0f) {
                float touch_sensitivity = 0.002f;
                state.yaw += input.touch_camera_delta_x * touch_sensitivity;
                state.pitch += input.touch_camera_delta_y * touch_sensitivity;
                state.pitch = clamp(state.pitch, -1.5708f, 1.5708f);
            }
            
            // === WEAPON FIRING ===
            bool shoot_pressed = input.shoot || input.touch_fire_tap || input.touch_fire_hold;
            
            if (shoot_pressed && state.shoot_cooldown <= 0.0f) {
                WeaponType current_weapon = state.inventory[state.current_weapon_slot].weapon;
                float fire_rate = 0.15f * (1.0f - state.fire_rate_upgrade_count * 0.1f);
                
                if (current_weapon == WeaponType::PotatoCannon) {
                    // Single potato
                    Potato p{};
                    Vector3 fwd{
                        std::cos(state.yaw),
                        0.0f,
                        std::sin(state.yaw)
                    };
                    if (Vector3LengthSqr(fwd) < 0.0001f) {
                        fwd = {0.0f, 0.0f, -1.0f};
                    } else {
                        fwd = Vector3Normalize(fwd);
                    }
                    p.pos = state.camera_position + fwd * 1.1f;
                    p.vel = fwd * 25.0f;
                    p.life = 5.0f;
                    state.potatoes.push_back(p);
                    
                    state.shoot_cooldown = fire_rate;
                    state.audio_events.push_back("shoot_cannon");
                } else if (current_weapon == WeaponType::PotatoShotgun) {
                    // Shotgun spread
                    Vector3 fwd{
                        std::cos(state.yaw),
                        0.0f,
                        std::sin(state.yaw)
                    };
                    if (Vector3LengthSqr(fwd) < 0.0001f) {
                        fwd = {0.0f, 0.0f, -1.0f};
                    } else {
                        fwd = Vector3Normalize(fwd);
                    }
                    Vector3 right_dir = Vector3(0.0f, 1.0f, 0.0f).cross(fwd).normalized();
                    Vector3 up_dir = fwd.cross(right_dir).normalized();
                    
                    for (int i = 0; i < 4; ++i) {
                        Potato p{};
                        p.pos = state.camera_position + fwd * 1.1f;
                        
                        float angle = (i - 1.5f) * 0.2f;
                        Vector3 spread_dir = (fwd + right_dir * angle).normalized();
                        p.vel = spread_dir * 25.0f;
                        p.life = 5.0f;
                        state.potatoes.push_back(p);
                    }
                    
                    state.shoot_cooldown = fire_rate * 1.5f;
                    state.audio_events.push_back("shoot_shotgun");
                } else if (current_weapon == WeaponType::PotatoGrenade) {
                    // Grenade
                    GrenadeProjectile g{};
                    Vector3 fwd{
                        std::cos(state.yaw),
                        0.0f,
                        std::sin(state.yaw)
                    };
                    if (Vector3LengthSqr(fwd) < 0.0001f) {
                        fwd = {0.0f, 0.0f, -1.0f};
                    } else {
                        fwd = Vector3Normalize(fwd);
                    }
                    g.pos = state.camera_position + fwd * 1.1f;
                    g.vel = fwd * 15.0f;
                    g.fuseTimer = 2.0f;
                    state.grenades.push_back(g);
                    
                    state.shoot_cooldown = fire_rate * 2.0f;
                    state.audio_events.push_back("shoot_grenade");
                }
            }
            
            state.shoot_cooldown = std::max(0.0f, state.shoot_cooldown - delta);
        }
        
        // === ENTITY UPDATES ===
        
        // Update potatoes
        for (auto it = state.potatoes.begin(); it != state.potatoes.end();) {
            it->life -= delta;
            it->pos = it->pos + it->vel * delta;
            
            // Simple gravity
            it->vel.y -= GRAVITY * delta;
            
            // Collision with chickens
            for (auto& chicken : state.chickens) {
                float dist = distance(it->pos, chicken.pos);
                if (dist < it->radius + chicken.radius) {
                    it->life = 0.0f;  // Mark for deletion
                    
                    // Deal damage
                    chicken.hp -= 10.0f;
                    state.score += 5;
                    state.audio_events.push_back("hit");
                    
                    if (chicken.hp <= 0.0f) {
                        state.score += 10;
                        state.chicken_nuggets += 1;
                        // Chicken marked for deletion below
                    }
                    break;
                }
            }
            
            if (it->life <= 0.0f) {
                it = state.potatoes.erase(it);
            } else {
                ++it;
            }
        }
        
        // Update enemy potatoes (from chickens)
        for (auto it = state.enemy_potatoes.begin(); it != state.enemy_potatoes.end();) {
            it->life -= delta;
            it->pos = it->pos + it->vel * delta;
            it->vel.y -= GRAVITY * delta;
            
            // Collision with player
            float dist_to_player = distance(it->pos, state.camera_position);
            if (dist_to_player < it->radius + 0.5f) {
                it->life = 0.0f;
                state.player_health -= 5.0f;
                state.damage_flash_timer = 0.3f;
                state.audio_events.push_back("hit_player");
            }
            
            // Collision with co-op player
            if (state.game_mode == GameMode::CoOp) {
                float dist_to_coop = distance(it->pos, state.coop_pos);
                if (dist_to_coop < it->radius + 0.5f) {
                    it->life = 0.0f;
                    state.coop_health -= 5.0f;
                    state.audio_events.push_back("hit_coop");
                }
            }
            
            if (it->life <= 0.0f) {
                it = state.enemy_potatoes.erase(it);
            } else {
                ++it;
            }
        }
        
        // Update grenades
        for (auto it = state.grenades.begin(); it != state.grenades.end();) {
            it->fuseTimer -= delta;
            it->pos = it->pos + it->vel * delta;
            it->vel.y -= GRAVITY * delta;
            
            if (it->fuseTimer <= 0.0f && !it->exploded) {
                it->exploded = true;
                state.audio_events.push_back("explosion");
                
                // Deal damage to all chickens in blast radius
                for (auto& chicken : state.chickens) {
                    float dist = distance(it->pos, chicken.pos);
                    if (dist < it->blastRadius) {
                        chicken.hp -= 30.0f;
                        state.score += 8;
                        if (chicken.hp <= 0.0f) {
                            state.score += 15;
                            state.chicken_nuggets += 2;
                        }
                    }
                }
                
                // Damage to player if in radius
                float dist_player = distance(it->pos, state.camera_position);
                if (dist_player < it->blastRadius) {
                    state.player_health -= 10.0f;
                    state.damage_flash_timer = 0.3f;
                }
            }
            
            if (it->fuseTimer < -1.0f) {
                it = state.grenades.erase(it);
            } else {
                ++it;
            }
        }
        
        // Remove dead chickens
        for (auto it = state.chickens.begin(); it != state.chickens.end();) {
            if (it->hp <= 0.0f) {
                it = state.chickens.erase(it);
            } else {
                ++it;
            }
        }
        
        // === CHICKEN AI ===
        for (auto& chicken : state.chickens) {
            // Target closest player
            Vector3 target_pos = state.camera_position;
            float closest_dist = distance(chicken.pos, target_pos);
            
            if (state.game_mode == GameMode::CoOp) {
                float dist_to_coop = distance(chicken.pos, state.coop_pos);
                if (dist_to_coop < closest_dist) {
                    target_pos = state.coop_pos;
                    closest_dist = dist_to_coop;
                }
            }
            
            // Move toward target
            if (closest_dist > 0.1f) {
                Vector3 to_target = target_pos - chicken.pos;
                Vector3 dir = to_target.normalized();
                float speed_this_frame = chicken.speed;
                chicken.pos = chicken.pos + dir * speed_this_frame * delta;
                
                // Update facing
                chicken.facingYaw = std::atan2(dir.x, dir.z);
            }
            
            // Melee attack
            if (closest_dist < 1.5f) {
                chicken.shootCooldown -= delta;
                if (chicken.shootCooldown <= 0.0f) {
                    EnemyPotato ep{};
                    ep.pos = chicken.pos;
                    ep.vel = (target_pos - chicken.pos).normalized() * 15.0f;
                    ep.life = 5.0f;
                    state.enemy_potatoes.push_back(ep);
                    
                    chicken.shootCooldown = random_range(1.0f, 2.0f);
                    state.audio_events.push_back("chicken_attack");
                }
            }
            
            // Wobble animation
            chicken.wobble += 2.0f * delta;
        }
        
        // === WAVE SPAWNING ===
        if (state.wave_spawn_cooldown <= 0.0f && state.wave_spawned < state.wave_total_to_spawn) {
            spawn_chicken(state, 8.0f, 15.0f);
            state.wave_spawn_cooldown = 0.5f;
        }
        state.wave_spawn_cooldown -= delta;
        
        // Check wave completion
        if (state.wave_spawned >= state.wave_total_to_spawn && state.chickens.empty()) {
            start_wave(state, state.wave + 1);
            state.shop_open = true;
            state.shop_timer = SHOP_DURATION;
        }
        
        // Damage feedback
        state.damage_flash_timer = std::max(0.0f, state.damage_flash_timer - delta);
        
        // Check death
        if (state.player_health <= 0.0f) {
            state.dead = true;
            state.death_cause = DeathCause::Peck;  // Simplified
            state.audio_events.push_back("death");
        }
    }
}

// ============================================================================
// RENDER SNAPSHOT (What to render)
// ============================================================================

RenderSnapshot get_render_snapshot(const GameState& state) {
    RenderSnapshot snap{};
    
    snap.player_pos = state.camera_position;
    snap.player_yaw = state.yaw;
    snap.player_pitch = state.pitch;
    
    snap.player_health = state.player_health;
    snap.coop_health = state.coop_health;
    snap.score = state.score;
    snap.wave = state.wave;
    snap.dead = state.dead;
    snap.death_cause = state.death_cause;
    
    snap.chickens_remaining = static_cast<int>(state.chickens.size()) +
        std::max(0, state.wave_total_to_spawn - state.wave_spawned);
    snap.wave_total_to_spawn = state.wave_total_to_spawn;
    snap.wave_spawned = state.wave_spawned;
    snap.wave_title_timer = state.wave_title_timer;
    
    snap.chickens = state.chickens;
    snap.potatoes = state.potatoes;
    snap.enemy_potatoes = state.enemy_potatoes;
    
    snap.chicken_nuggets = state.chicken_nuggets;
    snap.current_weapon_slot = state.current_weapon_slot;
    for (int i = 0; i < MAX_INVENTORY_SLOTS; ++i) {
        snap.inventory[i] = state.inventory[i];
    }
    snap.shop_open = state.shop_open;
    snap.inventory_open = state.inventory_open;
    snap.paused = state.paused;
    
    snap.shield_active = state.shield_active;
    snap.shield_timer = state.shield_timer;
    snap.owns_scythe = state.owns_scythe;
    
    snap.damage_flash_timer = state.damage_flash_timer;
    snap.net_status = state.net_status;
    snap.net_role = state.net_role;
    
    snap.entering_initials = state.entering_initials;
    snap.pending_initials = state.pending_initials;
    snap.pending_score = state.score;
    snap.highscores = state.highscores;
    
    snap.audio_events = state.audio_events;
    
    return snap;
}

}  // namespace game_core
