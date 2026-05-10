#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

// ============================================================================
// ENUMS & CONSTANTS
// ============================================================================

enum class WeaponType {
    PotatoCannon = 0,
    PotatoShotgun = 1,
    PotatoGrenade = 2
};

enum class DeathCause {
    None,
    Peck,
    Potato
};

enum class GameMode {
    SinglePlayer,
    CoOp,
    Online
};

enum class ScreenState {
    Intro,
    OnlineSetup,
    Playing
};

enum class NetRole {
    None,
    Host,
    Client
};

constexpr uint32_t NET_MAGIC = 0x43504653;  // CPFS
constexpr uint8_t NET_HELLO = 1;
constexpr uint8_t NET_INPUT = 2;
constexpr uint8_t NET_SNAPSHOT = 3;
constexpr uint8_t NET_DISCONNECT = 4;
constexpr int NET_PORT = 42069;
constexpr int MAX_SYNC_CHICKENS = 72;
constexpr int MAX_SYNC_POTATOES = 96;
constexpr int MAX_SYNC_ENEMY_POTATOES = 72;
constexpr int MAX_INVENTORY_SLOTS = 3;
constexpr int MAX_HIGHSCORES = 8;

// ============================================================================
// MATH STRUCTURES (Engine-Agnostic)
// ============================================================================

#ifndef RAYLIB_H
// Use our own Vector3/Vector2 if Raylib isn't included
struct Vector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    
    Vector3() = default;
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }
    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }
    float dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    Vector3 cross(const Vector3& other) const {
        return Vector3(y * other.z - z * other.y,
                       z * other.x - x * other.z,
                       x * other.y - y * other.x);
    }
    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }
    Vector3 normalized() const {
        float len = length();
        if (len > 0.0f) return *this * (1.0f / len);
        return *this;
    }
};

struct Vector2 {
    float x = 0.0f, y = 0.0f;
    
    Vector2() = default;
    Vector2(float x_, float y_) : x(x_), y(y_) {}
    
    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }
    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }
    Vector2 operator*(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }
    float length() const {
        return std::sqrt(x * x + y * y);
    }
    Vector2 normalized() const {
        float len = length();
        if (len > 0.0f) return *this * (1.0f / len);
        return *this;
    }
};
#else
// Use Raylib's Vector3/Vector2 if already included
// (Note: Raylib's versions don't have all the methods, but we'll use them as-is)
#endif

// ============================================================================
// GAME ENTITIES
// ============================================================================

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

struct ConfettiParticle {
    Vector3 pos{};
    Vector3 vel{};
    float life = 0.0f;
    struct Color {
        uint8_t r, g, b, a;
    } color{};
};

struct MedPack {
    Vector3 pos{};
    bool active = true;
    float bobTimer = 0.0f;
};

struct GrenadeProjectile {
    Vector3 pos{};
    Vector3 vel{};
    float fuseTimer = 0.0f;
    float blastRadius = 6.0f;
    bool exploded = false;
};

struct InventorySlot {
    WeaponType weapon = WeaponType::PotatoCannon;
    bool owned = false;
};

struct HighscoreEntry {
    std::string initials = "---";
    int score = 0;
};

// ============================================================================
// NETWORKING STRUCTURES
// ============================================================================

struct NetVec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct NetHeader {
    uint32_t magic = NET_MAGIC;
    uint8_t type = 0;
    uint8_t reservedA = 0;
    uint16_t reservedB = 0;
};

struct NetHelloPacket {
    NetHeader h{};
};

struct NetInputPacket {
    NetHeader h{};
    uint32_t sequence = 0;
    uint8_t moveMask = 0;
    uint8_t pad[3]{};
    float yaw = 0.0f;
    float pitch = 0.0f;
};

struct NetChickenSnapshot {
    NetVec3 pos{};
    float hp = 0.0f;
    float wobble = 0.0f;
    float facingYaw = 0.0f;
    uint8_t isBrown = 0;
    uint8_t pad[3]{};
};

struct NetPotatoSnapshot {
    NetVec3 pos{};
    NetVec3 vel{};
    float life = 0.0f;
    float radius = 0.0f;
};

struct NetSnapshotPacket {
    NetHeader h{};
    uint32_t sequence = 0;
    uint8_t hasClient = 0;
    uint8_t dead = 0;
    uint8_t deathCause = 0;
    uint8_t pad0 = 0;

    float playerHealth = 0.0f;
    float coopHealth = 0.0f;
    int32_t score = 0;
    int32_t wave = 0;

    NetVec3 playerPos{};
    float playerYaw = 0.0f;
    float playerPitch = 0.0f;

    NetVec3 coopPos{};
    float coopYaw = 0.0f;

    uint16_t chickenCount = 0;
    uint16_t potatoCount = 0;
    uint16_t enemyPotatoCount = 0;
    uint16_t pad1 = 0;
    int32_t waveTotalToSpawn = 0;
    int32_t waveSpawned = 0;

    NetChickenSnapshot chickens[MAX_SYNC_CHICKENS]{};
    NetPotatoSnapshot potatoes[MAX_SYNC_POTATOES]{};
    NetPotatoSnapshot enemyPotatoes[MAX_SYNC_ENEMY_POTATOES]{};
};

// ============================================================================
// INPUT STATE (For Godot/Raylib to pass to C++ core)
// ============================================================================

struct InputState {
    // Keyboard
    bool move_forward = false;
    bool move_backward = false;
    bool move_left = false;
    bool move_right = false;
    bool sprint = false;
    bool jump = false;
    bool shoot = false;
    
    // Mouse
    float mouse_delta_x = 0.0f;
    float mouse_delta_y = 0.0f;
    
    // Touch (camera look)
    float touch_camera_delta_x = 0.0f;
    float touch_camera_delta_y = 0.0f;
    
    // Touch fire (tap/hold)
    bool touch_fire_tap = false;
    bool touch_fire_hold = false;
    
    // Pause/UI
    bool pause = false;
    
    // Weapon/inventory/shop
    bool toggle_inventory = false;
    bool toggle_shop = false;
    bool weapon_switch_next = false;
    bool weapon_switch_prev = false;
    bool activate_shield = false;
    bool activate_scythe = false;
    
    // Co-op player 2 (local)
    struct CoOpInput {
        bool move_forward = false;
        bool move_backward = false;
        bool move_left = false;
        bool move_right = false;
        bool sprint = false;
        bool jump = false;
        bool shoot = false;
    } coop;
    
    // UI navigation
    bool ui_up = false;
    bool ui_down = false;
    bool ui_left = false;
    bool ui_right = false;
    bool ui_select = false;
    bool ui_back = false;
};

// ============================================================================
// RENDER STATE (For C++ core to return to Godot/Raylib)
// ============================================================================

struct RenderSnapshot {
    // Player camera
    Vector3 player_pos{};
    float player_yaw = 0.0f;
    float player_pitch = 0.0f;
    
    // Game state
    float player_health = 100.0f;
    float coop_health = 100.0f;
    int score = 0;
    int wave = 1;
    bool dead = false;
    DeathCause death_cause = DeathCause::None;
    
    // Wave state
    int chickens_remaining = 0;
    int wave_total_to_spawn = 0;
    int wave_spawned = 0;
    float wave_title_timer = 0.0f;
    
    // Entities
    std::vector<Chicken> chickens;
    std::vector<Potato> potatoes;
    std::vector<EnemyPotato> enemy_potatoes;
    
    // UI/HUD
    int chicken_nuggets = 0;
    int current_weapon_slot = 0;
    InventorySlot inventory[MAX_INVENTORY_SLOTS];
    bool shop_open = false;
    bool inventory_open = false;
    bool paused = false;
    
    // Shield/Scythe
    bool shield_active = false;
    float shield_timer = 0.0f;
    bool owns_scythe = false;
    
    // Damage feedback
    float damage_flash_timer = 0.0f;
    
    // Network status
    std::string net_status;
    NetRole net_role = NetRole::None;
    
    // Highscore entry
    bool entering_initials = false;
    std::string pending_initials;
    int pending_score = 0;
    
    // Highscores
    std::array<HighscoreEntry, MAX_HIGHSCORES> highscores{};
    
    // Audio events (queued each frame for Godot/Raylib to play)
    std::vector<std::string> audio_events; // "shoot", "hit", "death", etc.
};

// ============================================================================
// GAME STATE (Core simulation state, independent of rendering)
// ============================================================================

struct RemoteInputState {
    bool forward = false;
    bool backward = false;
    bool left = false;
    bool right = false;
    bool sprint = false;
    bool jump = false;
    bool shoot = false;
    float yaw = 0.0f;
    float pitch = 0.0f;
};

class GameState {
public:
    // Gameplay loop control
    GameMode game_mode = GameMode::SinglePlayer;
    ScreenState screen_state = ScreenState::Intro;
    bool should_exit = false;
    
    // Player state
    Vector3 camera_position{0.0f, 1.8f, 6.0f};
    float yaw = 3.14159f;  // PI
    float pitch = 0.0f;
    float player_health = 100.0f;
    float vertical_velocity = 0.0f;
    bool grounded = true;
    bool dead = false;
    DeathCause death_cause = DeathCause::None;
    
    // Co-op player 2
    Vector3 coop_pos{2.5f, 1.8f, 8.0f};
    float coop_yaw = 3.14159f;
    float coop_pitch = 0.0f;
    float coop_health = 100.0f;
    float coop_vertical_velocity = 0.0f;
    bool coop_grounded = true;
    
    // Gameplay entities
    std::vector<Potato> potatoes;
    std::vector<EnemyPotato> enemy_potatoes;
    std::vector<Chicken> chickens;
    std::vector<Cloud> clouds;
    std::vector<ConfettiParticle> confetti;
    std::vector<MedPack> med_packs;
    std::vector<GrenadeProjectile> grenades;
    
    // Cooldowns & timers
    float shoot_cooldown = 0.0f;
    float damage_tick = 0.0f;
    float wave_spawn_cooldown = 0.0f;
    float wave_title_timer = 0.0f;
    float damage_flash_timer = 0.0f;
    float shop_timer = 0.0f;
    float scythe_cooldown = 0.0f;
    float scythe_swing_visual_timer = 0.0f;
    float coop_shoot_cooldown = 0.0f;
    
    // Wave state
    int score = 0;
    int wave = 1;
    int wave_total_to_spawn = 0;
    int wave_spawned = 0;
    int wave_brown_to_spawn = 0;
    int wave_brown_spawned = 0;
    
    // Inventory & upgrades
    InventorySlot inventory[MAX_INVENTORY_SLOTS];
    int current_weapon_slot = 0;
    int fire_rate_upgrade_count = 0;
    bool has_shield_charge = false;
    bool shield_active = false;
    float shield_timer = 0.0f;
    bool owns_scythe = false;
    int chicken_nuggets = 0;
    
    // UI state
    bool paused = false;
    bool shop_open = false;
    bool inventory_open = false;
    int inventory_swap_from = -1;
    int pause_selection = 0;
    
    // Highscores
    std::array<HighscoreEntry, MAX_HIGHSCORES> highscores{};
    bool highscore_submitted_this_run = false;
    bool entering_initials = false;
    std::string pending_initials = "AAA";
    bool viewing_highscores = false;
    
    // Knockback/damage feedback
    Vector3 knockback_vel{};
    float prev_health = 100.0f;
    
    // Networking
    NetRole net_role = NetRole::None;
    bool net_has_peer = false;
    std::string net_join_address = "127.0.0.1";
    std::string net_status;
    uint32_t net_sequence = 1;
    float net_send_cooldown = 0.0f;
    float net_snapshot_cooldown = 0.0f;
    bool net_disconnected = false;
    double net_last_receive_time = 0.0;
    Vector3 client_target_pos{2.5f, 1.8f, 8.0f};
    Vector3 client_host_target_pos{0.0f, 1.8f, 6.0f};
    float client_host_target_yaw = 3.14159f;
    int client_last_wave = 1;
    RemoteInputState remote_input{};
    
    // Online/co-op interpolation targets (for client)
    Vector3 online_host_pos{0.0f, 1.8f, 6.0f};
    float online_host_yaw = 3.14159f;
    
    // Audio events queue (populated each frame)
    std::vector<std::string> audio_events;
};

// ============================================================================
// GAME CORE FUNCTIONS (Engine-Agnostic)
// ============================================================================

namespace game_core {

// Initialize game state
void initialize(GameState& state);

// Main update loop (called every frame with delta and input)
void update(GameState& state, const InputState& input, float delta);

// Reset game for new session
void reset_game(GameState& state);

// Start a new wave
void start_wave(GameState& state, int wave_num);

// Get snapshot for rendering
RenderSnapshot get_render_snapshot(const GameState& state);

// Network functions
bool setup_host(GameState& state, int port);
bool setup_client(GameState& state, const std::string& host, int port);
void close_network(GameState& state);

// Highscores
void load_highscores(GameState& state, const std::string& file_path);
void save_highscores(const GameState& state, const std::string& file_path);
bool score_qualifies(int score, const GameState& state);
void submit_highscore(GameState& state, const std::string& initials);

// Utility functions
float random_range(float a, float b);
Vector3 net_vec3_to_vector3(const NetVec3& v);
NetVec3 vector3_to_net_vec3(const Vector3& v);

}  // namespace game_core

// ============================================================================
// PLATFORM BRIDGE (For Raylib/Godot to implement)
// ============================================================================

class IPlatformBridge {
public:
    virtual ~IPlatformBridge() = default;
    
    // File I/O
    virtual std::string read_file(const std::string& path) = 0;
    virtual bool write_file(const std::string& path, const std::string& content) = 0;
    virtual bool file_exists(const std::string& path) = 0;
    virtual std::string get_user_data_path() = 0;
    
    // Random
    virtual float get_random(float min_val, float max_val) = 0;
    virtual int get_random_int(int min_val, int max_val) = 0;
    
    // Time
    virtual double get_time() = 0;
    
    // Networking (optional; can stay in main.cpp for now)
    virtual int create_udp_socket() = 0;
    virtual void close_udp_socket(int socket) = 0;
};

// Global bridge (set by main.cpp)
extern IPlatformBridge* g_platform_bridge;

// Initialize Raylib platform bridge (call from main.cpp on startup)
void init_raylib_bridge();
