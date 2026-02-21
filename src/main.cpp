#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// Platform-specific networking
#ifdef _WIN32
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOUSER
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define close closesocket
#else
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

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

    NetChickenSnapshot chickens[MAX_SYNC_CHICKENS]{};
    NetPotatoSnapshot potatoes[MAX_SYNC_POTATOES]{};
    NetPotatoSnapshot enemyPotatoes[MAX_SYNC_ENEMY_POTATOES]{};
};

static NetVec3 ToNetVec3(Vector3 v) {
    return {v.x, v.y, v.z};
}

static Vector3 FromNetVec3(NetVec3 v) {
    return {v.x, v.y, v.z};
}

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

static void DrawCenteredTextLine(const char* text, int centerX, int y, int fontSize, Color color) {
    DrawText(text, centerX - MeasureText(text, fontSize) / 2, y, fontSize, color);
}

static void DrawMenuCard(Rectangle rect,
                         const char* title,
                         const char* subtitle,
                         bool selected,
                         bool hovered,
                         Color accent) {
    Color base = Fade({18, 29, 42, 255}, (selected || hovered) ? 0.93f : 0.82f);
    Color glow = Fade(accent, selected ? 0.32f : (hovered ? 0.22f : 0.08f));
    DrawRectangleRounded(rect, 0.18f, 8, base);
    DrawRectangleRounded(rect, 0.18f, 8, glow);
    DrawRectangleRoundedLines(rect, 0.18f, 8, selected ? accent : Fade(RAYWHITE, 0.40f));

    const int titleSize = 33;
    const int subtitleSize = 20;
    DrawText(title, static_cast<int>(rect.x) + 24, static_cast<int>(rect.y) + 12, titleSize, RAYWHITE);
    DrawText(subtitle, static_cast<int>(rect.x) + 24, static_cast<int>(rect.y) + 50, subtitleSize,
             selected ? Fade(accent, 0.95f) : Fade(RAYWHITE, 0.82f));
}

int main() {
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 1;
    }
#endif

    const int initialScreenWidth = 1400;
    const int initialScreenHeight = 900;
    int screenWidth = initialScreenWidth;
    int screenHeight = initialScreenHeight;

    InitWindow(screenWidth, screenHeight, "Chicken Potato FPS");
    SetTargetFPS(120);
    SetExitKey(KEY_NULL);

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
    float coopHealth = 100.0f;
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
    GameMode gameMode = GameMode::SinglePlayer;
    ScreenState screenState = ScreenState::Intro;
    int introSelection = 0;
    int onlineSetupSelection = 0;
    bool onlineIpFocused = true;
    Vector3 coopPos = {2.5f, playerHeight, 8.0f};
    float coopYaw = PI;
    float coopPitch = 0.0f;
    float coopShootCooldown = 0.0f;

    NetRole netRole = NetRole::None;
#ifdef _WIN32
    SOCKET netSocket = INVALID_SOCKET;
#else
    int netSocket = -1;
#endif
    sockaddr_in netPeerAddr{};
    bool netHasPeer = false;
    std::string netJoinAddress = "127.0.0.1";
    std::string netStatus = "";
    uint32_t netSequence = 1;
    float netSendCooldown = 0.0f;
    float netSnapshotCooldown = 0.0f;
    Vector3 onlineHostPos = {0.0f, playerHeight, 6.0f};
    float onlineHostYaw = PI;
    bool netDisconnected = false;
    double netLastReceiveTime = 0.0;

    struct RemoteInputState {
        bool forward = false;
        bool backward = false;
        bool left = false;
        bool right = false;
        bool sprint = false;
        bool jump = false;
        bool shoot = false;
        float yaw = PI;
        float pitch = 0.0f;
    } remoteInput;

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
            camera.position.x + std::cos(ang) * dist,
            0.6f,
            camera.position.z + std::sin(ang) * dist
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
        coopHealth = 100.0f;
        coopPos = {2.5f, playerHeight, 8.0f};
        coopYaw = PI;
        coopPitch = 0.0f;
        coopShootCooldown = 0.0f;
        remoteInput = {};
        remoteInput.yaw = PI;
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

    auto closeNetwork = [&]() {
        if (netHasPeer) {
            NetHeader disc{};
            disc.magic = NET_MAGIC;
            disc.type = NET_DISCONNECT;
#ifdef _WIN32
            if (netSocket != INVALID_SOCKET) {
                for (int i = 0; i < 3; ++i)
                    sendto(netSocket, reinterpret_cast<const char*>(&disc), sizeof(disc), 0,
                           reinterpret_cast<const sockaddr*>(&netPeerAddr), sizeof(netPeerAddr));
            }
#else
            if (netSocket >= 0) {
                for (int i = 0; i < 3; ++i)
                    sendto(netSocket, &disc, sizeof(disc), 0,
                           reinterpret_cast<const sockaddr*>(&netPeerAddr), sizeof(netPeerAddr));
            }
#endif
        }
#ifdef _WIN32
        if (netSocket != INVALID_SOCKET) {
            closesocket(netSocket);
            netSocket = INVALID_SOCKET;
        }
#else
        if (netSocket >= 0) {
            close(netSocket);
            netSocket = -1;
        }
#endif
        netRole = NetRole::None;
        netHasPeer = false;
        netDisconnected = false;
        netLastReceiveTime = 0.0;
        netStatus.clear();
    };

#ifdef _WIN32
    auto createUdpSocket = [&]() -> SOCKET {
        SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s == INVALID_SOCKET) return INVALID_SOCKET;
        u_long mode = 1;
        ioctlsocket(s, FIONBIO, &mode);
        return s;
    };
#else
    auto createUdpSocket = [&]() -> int {
        int s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0) return -1;
        int flags = fcntl(s, F_GETFL, 0);
        if (flags >= 0) {
            fcntl(s, F_SETFL, flags | O_NONBLOCK);
        }
        return s;
    };
#endif

    auto setupOnlineHost = [&]() -> bool {
        closeNetwork();
        netSocket = createUdpSocket();
#ifdef _WIN32
        if (netSocket == INVALID_SOCKET) {
#else
        if (netSocket < 0) {
#endif
            netStatus = "Failed to create socket";
            return false;
        }

        sockaddr_in bindAddr{};
        bindAddr.sin_family = AF_INET;
        bindAddr.sin_port = htons(NET_PORT);
        bindAddr.sin_addr.s_addr = INADDR_ANY;
        if (bind(netSocket, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr)) != 0) {
            netStatus = "Could not bind port 42069";
            closeNetwork();
            return false;
        }

        netRole = NetRole::Host;
        netHasPeer = false;
        netStatus = "Hosting on UDP port 42069 (waiting for player)";
        return true;
    };

    auto setupOnlineClient = [&]() -> bool {
        closeNetwork();
        netSocket = createUdpSocket();
#ifdef _WIN32
        if (netSocket == INVALID_SOCKET) {
#else
        if (netSocket < 0) {
#endif
            netStatus = "Failed to create socket";
            return false;
        }

        sockaddr_in server{};
        server.sin_family = AF_INET;
        server.sin_port = htons(NET_PORT);
        if (inet_pton(AF_INET, netJoinAddress.c_str(), &server.sin_addr) != 1) {
            netStatus = "Invalid host IP";
            closeNetwork();
            return false;
        }

        netPeerAddr = server;
        netRole = NetRole::Client;
        netHasPeer = true;
        netStatus = "Connecting to host...";
        return true;
    };

    auto sendHello = [&]() {
#ifdef _WIN32
        if (netSocket == INVALID_SOCKET || !netHasPeer) return;
#else
        if (netSocket < 0 || !netHasPeer) return;
#endif
        NetHelloPacket hello{};
        hello.h.type = NET_HELLO;
#ifdef _WIN32
        sendto(netSocket, reinterpret_cast<const char*>(&hello), sizeof(hello), 0,
               reinterpret_cast<const sockaddr*>(&netPeerAddr), sizeof(netPeerAddr));
#else
        sendto(netSocket, &hello, sizeof(hello), 0,
               reinterpret_cast<const sockaddr*>(&netPeerAddr), sizeof(netPeerAddr));
#endif
    };

    auto recvNetworkPackets = [&]() {
#ifdef _WIN32
        if (netSocket == INVALID_SOCKET) return;
#else
        if (netSocket < 0) return;
#endif

        uint8_t buffer[32768];
        while (true) {
            sockaddr_in from{};
            socklen_t fromLen = sizeof(from);
#ifdef _WIN32
            int bytes = recvfrom(netSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0,
                                 reinterpret_cast<sockaddr*>(&from), &fromLen);
#else
            ssize_t bytes = recvfrom(netSocket, buffer, sizeof(buffer), 0,
                                     reinterpret_cast<sockaddr*>(&from), &fromLen);
#endif
            if (bytes <= 0) break;
            if (bytes < static_cast<int>(sizeof(NetHeader))) continue;

            const NetHeader* hdr = reinterpret_cast<const NetHeader*>(buffer);
            if (hdr->magic != NET_MAGIC) continue;

            if (netRole == NetRole::Host) {
                if (hdr->type == NET_HELLO) {
                    netPeerAddr = from;
                    netHasPeer = true;
                    netStatus = "Client connected";
                } else if (hdr->type == NET_INPUT && bytes >= static_cast<ssize_t>(sizeof(NetInputPacket))) {
                    if (!netHasPeer) {
                        netPeerAddr = from;
                        netHasPeer = true;
                    }
                    const NetInputPacket* in = reinterpret_cast<const NetInputPacket*>(buffer);
                    uint8_t mask = in->moveMask;
                    remoteInput.forward = (mask & (1 << 0)) != 0;
                    remoteInput.backward = (mask & (1 << 1)) != 0;
                    remoteInput.left = (mask & (1 << 2)) != 0;
                    remoteInput.right = (mask & (1 << 3)) != 0;
                    remoteInput.sprint = (mask & (1 << 4)) != 0;
                    remoteInput.jump = (mask & (1 << 5)) != 0;
                    remoteInput.shoot = (mask & (1 << 6)) != 0;
                    remoteInput.yaw = in->yaw;
                    remoteInput.pitch = in->pitch;
                }
            } else if (netRole == NetRole::Client) {
                if (hdr->type == NET_DISCONNECT) {
                    netDisconnected = true;
                    netStatus = "Host disconnected";
                } else if (hdr->type == NET_SNAPSHOT && bytes >= static_cast<ssize_t>(sizeof(NetSnapshotPacket))) {
                    const NetSnapshotPacket* snap = reinterpret_cast<const NetSnapshotPacket*>(buffer);

                    playerHealth = snap->playerHealth;
                    coopHealth = snap->coopHealth;
                    score = snap->score;
                    wave = snap->wave;
                    dead = snap->dead != 0;
                    deathCause = static_cast<DeathCause>(snap->deathCause);

                    onlineHostPos = FromNetVec3(snap->playerPos);
                    onlineHostYaw = snap->playerYaw;

                    camera.position = FromNetVec3(snap->coopPos);
                    camera.position.y = playerHeight;
                    coopPos = FromNetVec3(snap->coopPos);
                    coopYaw = snap->coopYaw;

                    chickens.clear();
                    chickens.reserve(snap->chickenCount);
                    for (int i = 0; i < snap->chickenCount; ++i) {
                        Chicken c{};
                        c.pos = FromNetVec3(snap->chickens[i].pos);
                        c.hp = snap->chickens[i].hp;
                        c.wobble = snap->chickens[i].wobble;
                        c.facingYaw = snap->chickens[i].facingYaw;
                        c.isBrown = snap->chickens[i].isBrown != 0;
                        c.radius = 0.55f;
                        chickens.push_back(c);
                    }

                    potatoes.clear();
                    potatoes.reserve(snap->potatoCount);
                    for (int i = 0; i < snap->potatoCount; ++i) {
                        Potato p{};
                        p.pos = FromNetVec3(snap->potatoes[i].pos);
                        p.vel = FromNetVec3(snap->potatoes[i].vel);
                        p.life = snap->potatoes[i].life;
                        p.radius = snap->potatoes[i].radius;
                        potatoes.push_back(p);
                    }

                    enemyPotatoes.clear();
                    enemyPotatoes.reserve(snap->enemyPotatoCount);
                    for (int i = 0; i < snap->enemyPotatoCount; ++i) {
                        EnemyPotato ep{};
                        ep.pos = FromNetVec3(snap->enemyPotatoes[i].pos);
                        ep.vel = FromNetVec3(snap->enemyPotatoes[i].vel);
                        ep.life = snap->enemyPotatoes[i].life;
                        ep.radius = snap->enemyPotatoes[i].radius;
                        enemyPotatoes.push_back(ep);
                    }

                    netLastReceiveTime = GetTime();
                    netStatus = (snap->hasClient != 0) ? "Connected" : "Waiting for host snapshot";
                }
            }
        }
    };

    auto sendInputToHost = [&](uint8_t moveMask) {
#ifdef _WIN32
        if (netRole != NetRole::Client || netSocket == INVALID_SOCKET || !netHasPeer) return;
#else
        if (netRole != NetRole::Client || netSocket < 0 || !netHasPeer) return;
#endif

        NetInputPacket input{};
        input.h.type = NET_INPUT;
        input.sequence = netSequence++;
        input.moveMask = moveMask;
        input.yaw = yaw;
        input.pitch = pitch;

#ifdef _WIN32
        sendto(netSocket, reinterpret_cast<const char*>(&input), sizeof(input), 0,
               reinterpret_cast<const sockaddr*>(&netPeerAddr), sizeof(netPeerAddr));
#else
        sendto(netSocket, &input, sizeof(input), 0,
               reinterpret_cast<const sockaddr*>(&netPeerAddr), sizeof(netPeerAddr));
#endif
    };

    auto sendSnapshotToClient = [&]() {
#ifdef _WIN32
        if (netRole != NetRole::Host || netSocket == INVALID_SOCKET || !netHasPeer) return;
#else
        if (netRole != NetRole::Host || netSocket < 0 || !netHasPeer) return;
#endif

        NetSnapshotPacket snap{};
        snap.h.type = NET_SNAPSHOT;
        snap.sequence = netSequence++;
        snap.hasClient = netHasPeer ? 1 : 0;
        snap.dead = dead ? 1 : 0;
        snap.deathCause = static_cast<uint8_t>(deathCause);
        snap.playerHealth = playerHealth;
        snap.coopHealth = coopHealth;
        snap.score = score;
        snap.wave = wave;
        snap.playerPos = ToNetVec3(camera.position);
        snap.playerYaw = yaw;
        snap.playerPitch = pitch;
        snap.coopPos = ToNetVec3(coopPos);
        snap.coopYaw = coopYaw;

        snap.chickenCount = static_cast<uint16_t>(std::min(static_cast<int>(chickens.size()), MAX_SYNC_CHICKENS));
        snap.potatoCount = static_cast<uint16_t>(std::min(static_cast<int>(potatoes.size()), MAX_SYNC_POTATOES));
        snap.enemyPotatoCount = static_cast<uint16_t>(std::min(static_cast<int>(enemyPotatoes.size()), MAX_SYNC_ENEMY_POTATOES));

        for (int i = 0; i < snap.chickenCount; ++i) {
            snap.chickens[i].pos = ToNetVec3(chickens[i].pos);
            snap.chickens[i].hp = chickens[i].hp;
            snap.chickens[i].wobble = chickens[i].wobble;
            snap.chickens[i].facingYaw = chickens[i].facingYaw;
            snap.chickens[i].isBrown = chickens[i].isBrown ? 1 : 0;
        }
        for (int i = 0; i < snap.potatoCount; ++i) {
            snap.potatoes[i].pos = ToNetVec3(potatoes[i].pos);
            snap.potatoes[i].vel = ToNetVec3(potatoes[i].vel);
            snap.potatoes[i].life = potatoes[i].life;
            snap.potatoes[i].radius = potatoes[i].radius;
        }
        for (int i = 0; i < snap.enemyPotatoCount; ++i) {
            snap.enemyPotatoes[i].pos = ToNetVec3(enemyPotatoes[i].pos);
            snap.enemyPotatoes[i].vel = ToNetVec3(enemyPotatoes[i].vel);
            snap.enemyPotatoes[i].life = enemyPotatoes[i].life;
            snap.enemyPotatoes[i].radius = enemyPotatoes[i].radius;
        }

#ifdef _WIN32
        sendto(netSocket, reinterpret_cast<const char*>(&snap), sizeof(snap), 0,
               reinterpret_cast<const sockaddr*>(&netPeerAddr), sizeof(netPeerAddr));
#else
        sendto(netSocket, &snap, sizeof(snap), 0,
               reinterpret_cast<const sockaddr*>(&netPeerAddr), sizeof(netPeerAddr));
#endif
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

        recvNetworkPackets();
        netSendCooldown = std::max(0.0f, netSendCooldown - dt);
        netSnapshotCooldown = std::max(0.0f, netSnapshotCooldown - dt);
        if (gameMode == GameMode::Online && netRole == NetRole::Client && netHasPeer && netSendCooldown <= 0.0f) {
            sendHello();
            netSendCooldown = 0.5f;
        }

        if (gameMode == GameMode::Online && netRole == NetRole::Client && !netDisconnected) {
            if (netLastReceiveTime > 0.0 && (GetTime() - netLastReceiveTime) > 5.0) {
                netDisconnected = true;
                netStatus = "Connection to host lost";
            }
        }

        if (netDisconnected) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                netDisconnected = false;
                closeNetwork();
                screenState = ScreenState::Intro;
                EnableCursor();
                ShowCursor();
                continue;
            }
            BeginDrawing();
            ClearBackground({20, 20, 30, 255});
            const char* discText = "HOST DISCONNECTED";
            int discFontSize = 48;
            DrawText(discText, (screenWidth - MeasureText(discText, discFontSize)) / 2, screenHeight / 2 - 40, discFontSize, RED);
            const char* escText = "Press ESC to return to menu";
            DrawText(escText, (screenWidth - MeasureText(escText, 26)) / 2, screenHeight / 2 + 30, 26, YELLOW);
            EndDrawing();
            continue;
        }

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

        if (screenState == ScreenState::Intro) {
            if (IsCursorHidden()) {
                EnableCursor();
                ShowCursor();
            }

            const int introW = std::min(960, std::max(620, screenWidth - 180));
            const int introH = 520;
            const int introX = screenWidth / 2 - introW / 2;
            const int introY = screenHeight / 2 - introH / 2;
            const int centerX = introX + introW / 2;

            Rectangle singleRect = {static_cast<float>(introX + 48), static_cast<float>(introY + 150), static_cast<float>(introW - 96), 90.0f};
            Rectangle coopRect = {static_cast<float>(introX + 48), static_cast<float>(introY + 256), static_cast<float>(introW - 96), 90.0f};
            Rectangle onlineRect = {static_cast<float>(introX + 48), static_cast<float>(introY + 362), static_cast<float>(introW - 96), 90.0f};

            if (IsKeyPressed(KEY_ESCAPE)) {
                shouldExit = true;
                continue;
            }

            Vector2 mousePos = GetMousePosition();
            const bool singleHover = CheckCollisionPointRec(mousePos, singleRect);
            const bool coopHover = CheckCollisionPointRec(mousePos, coopRect);
            const bool onlineHover = CheckCollisionPointRec(mousePos, onlineRect);

            if (singleHover) introSelection = 0;
            else if (coopHover) introSelection = 1;
            else if (onlineHover) introSelection = 2;

            GameMode selectedMode = GameMode::SinglePlayer;
            bool startSelectedMode = false;
            bool openOnlineSetup = false;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (singleHover) {
                    introSelection = 0;
                    selectedMode = GameMode::SinglePlayer;
                    startSelectedMode = true;
                } else if (coopHover) {
                    introSelection = 1;
                    selectedMode = GameMode::CoOp;
                    startSelectedMode = true;
                } else if (onlineHover) {
                    introSelection = 2;
                    openOnlineSetup = true;
                }
            }

            if (openOnlineSetup) {
                screenState = ScreenState::OnlineSetup;
                onlineSetupSelection = 0;
                onlineIpFocused = true;
                continue;
            }

            if (startSelectedMode) {
                closeNetwork();
                gameMode = selectedMode;
                screenState = ScreenState::Playing;
                pauseSelection = 0;
                resetGame();
                DisableCursor();
                continue;
            }

            BeginDrawing();
            DrawRectangleGradientV(0, 0, screenWidth, screenHeight, {94, 170, 240, 255}, {48, 92, 148, 255});
            DrawRectangleRounded({static_cast<float>(introX), static_cast<float>(introY), static_cast<float>(introW), static_cast<float>(introH)}, 0.04f, 12, Fade({8, 14, 22, 255}, 0.86f));
            DrawRectangleRoundedLines({static_cast<float>(introX), static_cast<float>(introY), static_cast<float>(introW), static_cast<float>(introH)}, 0.04f, 12, Fade(SKYBLUE, 0.8f));

            DrawCenteredTextLine("CHICKEN POTATO FPS", centerX, introY + 28, 54, RAYWHITE);
            DrawCenteredTextLine("Pick a mode to deploy your potato cannon", centerX, introY + 92, 25, Fade(RAYWHITE, 0.92f));

            DrawMenuCard(singleRect, "Singleplayer", "Solo survival against endless chickens", introSelection == 0, singleHover, {255, 224, 85, 255});
            DrawMenuCard(coopRect, "Local Co-op", "Player 2: IJKL + Right Shift + Right Ctrl", introSelection == 1, coopHover, {118, 215, 255, 255});
            DrawMenuCard(onlineRect, "Online Multiplayer", "Host or join over LAN/Internet (UDP 42069)", introSelection == 2, onlineHover, {173, 135, 255, 255});

            DrawCenteredTextLine("Mouse: hover + click button   |   Esc: Quit", centerX, introY + introH - 34, 20, Fade(RAYWHITE, 0.85f));
            EndDrawing();
            continue;
        }

        if (screenState == ScreenState::OnlineSetup) {
            if (IsCursorHidden()) {
                EnableCursor();
                ShowCursor();
            }

            const int setupW = std::min(1040, std::max(700, screenWidth - 180));
            const int setupH = 540;
            const int setupX = screenWidth / 2 - setupW / 2;
            const int setupY = screenHeight / 2 - setupH / 2;
            const int centerX = setupX + setupW / 2;

            float cardW = (setupW - 170.0f) / 2.0f;
            Rectangle hostRect = {static_cast<float>(setupX + 56), static_cast<float>(setupY + 166), cardW, 112.0f};
            Rectangle joinRect = {hostRect.x + cardW + 58.0f, static_cast<float>(setupY + 166), cardW, 112.0f};
            Rectangle ipRect = {static_cast<float>(setupX + 56), static_cast<float>(setupY + 312), static_cast<float>(setupW - 112), 58.0f};
            Rectangle backRect = {static_cast<float>(setupX + setupW / 2 - 120), static_cast<float>(setupY + 438), 240.0f, 56.0f};

            Vector2 mousePos = GetMousePosition();
            bool hostHover = CheckCollisionPointRec(mousePos, hostRect);
            bool joinHover = CheckCollisionPointRec(mousePos, joinRect);
            bool backHover = CheckCollisionPointRec(mousePos, backRect);
            bool ipHover = CheckCollisionPointRec(mousePos, ipRect);

            if (hostHover) onlineSetupSelection = 0;
            else if (joinHover) onlineSetupSelection = 1;
            else if (backHover) onlineSetupSelection = 2;

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                onlineIpFocused = ipHover;
            }

            if (IsKeyPressed(KEY_TAB)) {
                onlineIpFocused = !onlineIpFocused;
            }

            if (onlineIpFocused) {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= '0' && key <= '9') || key == '.') {
                        if (netJoinAddress.size() < 31) {
                            netJoinAddress.push_back(static_cast<char>(key));
                        }
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && !netJoinAddress.empty()) {
                    netJoinAddress.pop_back();
                }
            }

            if (!onlineIpFocused) {
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                    onlineSetupSelection = (onlineSetupSelection + 3 - 1) % 3;
                }
                if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                    onlineSetupSelection = (onlineSetupSelection + 1) % 3;
                }
            }

            auto startOnlineMode = [&](NetRole role) {
                bool ok = (role == NetRole::Host) ? setupOnlineHost() : setupOnlineClient();
                if (!ok) return;

                gameMode = GameMode::Online;
                screenState = ScreenState::Playing;
                pauseSelection = 0;
                resetGame();
                DisableCursor();
            };

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (hostHover) {
                    startOnlineMode(NetRole::Host);
                } else if (joinHover) {
                    startOnlineMode(NetRole::Client);
                } else if (backHover) {
                    screenState = ScreenState::Intro;
                    netStatus.clear();
                }
            }

            if (!onlineIpFocused && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))) {
                if (onlineSetupSelection == 0) {
                    startOnlineMode(NetRole::Host);
                } else if (onlineSetupSelection == 1) {
                    startOnlineMode(NetRole::Client);
                } else {
                    screenState = ScreenState::Intro;
                    netStatus.clear();
                }
            }

            if (IsKeyPressed(KEY_ESCAPE)) {
                screenState = ScreenState::Intro;
                netStatus.clear();
            }

            BeginDrawing();
            DrawRectangleGradientV(0, 0, screenWidth, screenHeight, {94, 170, 240, 255}, {48, 92, 148, 255});
            DrawRectangleRounded({static_cast<float>(setupX), static_cast<float>(setupY), static_cast<float>(setupW), static_cast<float>(setupH)}, 0.04f, 12, Fade({8, 14, 22, 255}, 0.88f));
            DrawRectangleRoundedLines({static_cast<float>(setupX), static_cast<float>(setupY), static_cast<float>(setupW), static_cast<float>(setupH)}, 0.04f, 12, Fade({130, 190, 255, 255}, 0.8f));

            DrawCenteredTextLine("ONLINE MULTIPLAYER", centerX, setupY + 24, 52, RAYWHITE);
            DrawCenteredTextLine("Choose host/join, then launch", centerX, setupY + 92, 24, Fade(RAYWHITE, 0.92f));

            DrawMenuCard(hostRect, "Host Game", "Create lobby and wait for player", onlineSetupSelection == 0, hostHover, {255, 224, 85, 255});
            DrawMenuCard(joinRect, "Join Game", "Connect to host IPv4 address", onlineSetupSelection == 1, joinHover, {173, 135, 255, 255});

            DrawText("Host IP for joining:", setupX + 62, setupY + 284, 24, Fade(RAYWHITE, 0.90f));
            DrawRectangleRounded(ipRect, 0.12f, 8, Fade({16, 44, 86, 255}, 0.86f));
            DrawRectangleRoundedLines(ipRect, 0.12f, 8, onlineIpFocused ? YELLOW : Fade(RAYWHITE, 0.5f));
            DrawText(netJoinAddress.c_str(), static_cast<int>(ipRect.x) + 18, static_cast<int>(ipRect.y) + 15, 30, RAYWHITE);
            DrawText("Tip: click the field or press Tab to focus/unfocus", setupX + 62, setupY + 380, 20, Fade(RAYWHITE, 0.72f));

            DrawRectangleRounded(backRect, 0.24f, 8, (backHover || onlineSetupSelection == 2) ? Fade({220, 90, 90, 255}, 0.40f) : Fade({220, 90, 90, 255}, 0.22f));
            DrawRectangleRoundedLines(backRect, 0.24f, 8, (onlineSetupSelection == 2) ? ORANGE : Fade(RAYWHITE, 0.5f));
            DrawCenteredTextLine("Back", static_cast<int>(backRect.x + backRect.width / 2), static_cast<int>(backRect.y) + 14, 30, RAYWHITE);

            DrawCenteredTextLine("Enter: select highlighted card   |   Esc: back", centerX, setupY + setupH - 34, 20, Fade(RAYWHITE, 0.84f));
            if (!netStatus.empty()) {
                DrawCenteredTextLine(netStatus.c_str(), centerX, setupY + 126, 24, ORANGE);
            }
            EndDrawing();
            continue;
        }

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
            bool onlineClient = (gameMode == GameMode::Online && netRole == NetRole::Client);

            Vector2 mouse = GetMouseDelta();
            const float mouseSensitivity = 0.0026f;
            yaw -= mouse.x * mouseSensitivity;
            pitch -= mouse.y * mouseSensitivity;
            pitch = Clamp(pitch, -1.4f, 1.4f);

            Vector3 forward = {
                std::cos(pitch) * std::sin(yaw),
                std::sin(pitch),
                std::cos(pitch) * std::cos(yaw)
            };
            Vector3 flatForward = Vector3Normalize({forward.x, 0.0f, forward.z});
            Vector3 right = Vector3Normalize(Vector3CrossProduct(flatForward, {0.0f, 1.0f, 0.0f}));

            if (playerHealth > 0.0f || onlineClient) {
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
            }

            camera.target = Vector3Add(camera.position, forward);

            if (onlineClient) {
                uint8_t moveMask = 0;
                if (coopHealth > 0.0f) {
                    if (IsKeyDown(KEY_W)) moveMask |= (1 << 0);
                    if (IsKeyDown(KEY_S)) moveMask |= (1 << 1);
                    if (IsKeyDown(KEY_A)) moveMask |= (1 << 2);
                    if (IsKeyDown(KEY_D)) moveMask |= (1 << 3);
                    if (IsKeyDown(KEY_LEFT_SHIFT)) moveMask |= (1 << 4);
                    if (IsKeyDown(KEY_SPACE)) moveMask |= (1 << 5);
                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) moveMask |= (1 << 6);
                }

                sendInputToHost(moveMask);
                camera.position = coopPos;
                camera.position.y = playerHeight;
                Vector3 onlineForward = {
                    std::cos(pitch) * std::sin(yaw),
                    std::sin(pitch),
                    std::cos(pitch) * std::cos(yaw)
                };
                camera.target = Vector3Add(camera.position, onlineForward);
            } else {
                if (playerHealth > 0.0f && grounded && IsKeyPressed(KEY_SPACE)) {
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
                if (playerHealth > 0.0f && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && shootCooldown <= 0.0f) {
                    shootCooldown = 0.12f;

                    Vector3 muzzle = Vector3Add(camera.position, Vector3Scale(forward, 1.1f));
                    muzzle.y -= 0.15f;

                    Potato p{};
                    p.pos = muzzle;
                    p.vel = Vector3Scale(forward, 34.0f);
                    p.life = 3.0f;
                    potatoes.push_back(p);
                }
            }

            if (!onlineClient && (gameMode == GameMode::CoOp || gameMode == GameMode::Online) && coopHealth > 0.0f) {
                Vector3 coopMove = {0.0f, 0.0f, 0.0f};
                bool coopShootDown = false;

                if (gameMode == GameMode::CoOp) {
                    if (IsKeyDown(KEY_I)) coopMove.z -= 1.0f;
                    if (IsKeyDown(KEY_K)) coopMove.z += 1.0f;
                    if (IsKeyDown(KEY_J)) coopMove.x -= 1.0f;
                    if (IsKeyDown(KEY_L)) coopMove.x += 1.0f;
                    coopShootDown = IsKeyDown(KEY_RIGHT_SHIFT);
                } else {
                    Vector3 remoteFwd = {std::sin(remoteInput.yaw), 0.0f, std::cos(remoteInput.yaw)};
                    Vector3 remoteRight = Vector3Normalize(Vector3CrossProduct(remoteFwd, {0.0f, 1.0f, 0.0f}));
                    if (remoteInput.forward) coopMove = Vector3Add(coopMove, remoteFwd);
                    if (remoteInput.backward) coopMove = Vector3Subtract(coopMove, remoteFwd);
                    if (remoteInput.right) coopMove = Vector3Add(coopMove, remoteRight);
                    if (remoteInput.left) coopMove = Vector3Subtract(coopMove, remoteRight);
                    coopYaw = remoteInput.yaw;
                    coopPitch = remoteInput.pitch;
                    coopShootDown = remoteInput.shoot;
                }

                float coopSpeed = ((gameMode == GameMode::CoOp && IsKeyDown(KEY_RIGHT_CONTROL)) ||
                                   (gameMode == GameMode::Online && remoteInput.sprint))
                                      ? 8.8f
                                      : 6.0f;
                if (Vector3Length(coopMove) > 0.001f) {
                    coopMove.y = 0.0f;
                    Vector3 coopDir = Vector3Normalize(coopMove);
                    if (gameMode == GameMode::CoOp) {
                        coopYaw = std::atan2(coopDir.x, coopDir.z);
                    }
                    coopPos = Vector3Add(coopPos, Vector3Scale(coopDir, coopSpeed * dt));
                }
                coopPos.y = playerHeight;

                coopShootCooldown -= dt;
                if (coopShootDown && coopShootCooldown <= 0.0f) {
                    coopShootCooldown = 0.16f;
                    Vector3 coopForward = {
                        std::cos(coopPitch) * std::sin(coopYaw),
                        std::sin(coopPitch),
                        std::cos(coopPitch) * std::cos(coopYaw)
                    };
                    coopForward = Vector3Normalize(coopForward);

                    Potato p{};
                    p.pos = Vector3Add(coopPos, Vector3Scale(coopForward, 0.9f));
                    p.pos.y -= 0.20f;
                    p.vel = Vector3Scale(coopForward, 31.0f);
                    p.life = 3.0f;
                    potatoes.push_back(p);
                }
            }

            if (!onlineClient) {
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
                Vector3 p1Target = camera.position;
                p1Target.y = playerHeight;
                Vector3 p2Target = coopPos;

                bool p1Alive = playerHealth > 0.0f;
                bool p2Alive = ((gameMode == GameMode::CoOp || gameMode == GameMode::Online) && coopHealth > 0.0f);

                Vector3 targetPos = p1Target;
                if (p2Alive && !p1Alive) {
                    targetPos = p2Target;
                } else if (p1Alive && p2Alive) {
                    float d1 = Vector3Distance(c.pos, p1Target);
                    float d2 = Vector3Distance(c.pos, p2Target);
                    targetPos = (d2 < d1) ? p2Target : p1Target;
                }

                Vector3 toPlayer = Vector3Subtract(targetPos, c.pos);
                toPlayer.y = 0.0f;
                float d = Vector3Length(toPlayer);
                if (d > 0.01f) {
                    Vector3 dir = Vector3Scale(toPlayer, 1.0f / d);
                    c.facingYaw = std::atan2(dir.x, dir.z);
                    c.pos = Vector3Add(c.pos, Vector3Scale(dir, c.speed * dt));

                    if (c.isBrown) {
                        c.shootCooldown -= dt;
                        if (c.shootCooldown <= 0.0f && d < 65.0f) {
                            Vector3 muzzle = c.pos;
                            muzzle.y = 0.95f;
                            Vector3 aimTarget = targetPos;
                            aimTarget.y -= 0.45f;

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
                if (playerHealth > 0.0f && Vector3Distance(ep.pos, camera.position) < 0.55f) {
                    ep.life = 0.0f;
                    float prevHealth = playerHealth;
                    playerHealth -= 11.0f;
                    if (prevHealth > 0.0f && playerHealth <= 0.0f) {
                        deathCause = DeathCause::Potato;
                    }
                } else if ((gameMode == GameMode::CoOp || gameMode == GameMode::Online) && coopHealth > 0.0f && Vector3Distance(ep.pos, coopPos) < 0.62f) {
                    ep.life = 0.0f;
                    coopHealth -= 11.0f;
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
                if (gameMode != GameMode::SinglePlayer) {
                    if (playerHealth <= 0.0f) playerHealth = 100.0f;
                    if (coopHealth <= 0.0f) coopHealth = 100.0f;
                }
                startWave(wave + 1);
            }

            damageTick += dt;
            if (damageTick >= 0.2f) {
                damageTick = 0.0f;
                for (const auto& c : chickens) {
                    if (c.isBrown) continue;
                    Vector3 toP1 = Vector3Subtract(camera.position, c.pos);
                    toP1.y = 0.0f;
                    if (playerHealth > 0.0f && Vector3Length(toP1) < 1.35f) {
                        float prevHealth = playerHealth;
                        playerHealth -= 4.0f;
                        if (prevHealth > 0.0f && playerHealth <= 0.0f) {
                            deathCause = DeathCause::Peck;
                        }
                    }
                    if ((gameMode == GameMode::CoOp || gameMode == GameMode::Online) && coopHealth > 0.0f) {
                        Vector3 toP2 = Vector3Subtract(coopPos, c.pos);
                        toP2.y = 0.0f;
                        if (Vector3Length(toP2) < 1.35f) {
                            coopHealth -= 4.0f;
                        }
                    }
                }
            }

            playerHealth = std::max(0.0f, playerHealth);
            coopHealth = std::max(0.0f, coopHealth);

            if (gameMode == GameMode::SinglePlayer) {
                if (playerHealth <= 0.0f) {
                    dead = true;
                    EnableCursor();
                }
            } else {
                if (playerHealth <= 0.0f && coopHealth <= 0.0f) {
                    dead = true;
                    EnableCursor();
                }
            }
            }
        }

        if (!dead && !paused && IsCursorHidden() == false) {
            DisableCursor();
        }

        if (gameMode == GameMode::Online && netRole == NetRole::Host && netSnapshotCooldown <= 0.0f) {
            sendSnapshotToClient();
            netSnapshotCooldown = 1.0f / 30.0f;
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
            bodyPos.y = 0.65f + std::sin(c.wobble) * 0.06f;
            Vector3 forwardDir = {std::sin(c.facingYaw), 0.0f, std::cos(c.facingYaw)};
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

            float step = std::sin(c.wobble * 1.8f);
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

        auto DrawHumanoid = [&](Vector3 pos, float facingYaw, float hp, Color aliveColor, Color gunColor) {
            Vector3 body = {pos.x, 1.05f, pos.z};
            Vector3 head = {pos.x, 1.62f, pos.z};
            Vector3 fwd = {std::sin(facingYaw), 0.0f, std::cos(facingYaw)};
            Vector3 right = {fwd.z, 0.0f, -fwd.x};

            DrawProjectedShadowCapsule({pos.x, 1.0f, pos.z}, {pos.x, 1.6f, pos.z}, 0.22f, 0.18f, Fade(BLACK, 0.30f));

            Color bodyColor = (hp > 0.0f) ? aliveColor : GRAY;
            DrawCylinder(body, 0.20f, 0.24f, 0.80f, 16, bodyColor);
            DrawSphere(head, 0.17f, ScaleColor(bodyColor, 1.08f));
            DrawCylinderEx(
                Vector3Add(body, Vector3Add(Vector3Scale(right, 0.13f), {0.0f, 0.10f, 0.0f})),
                Vector3Add(body, Vector3Add(Vector3Scale(right, 0.13f), Vector3Scale(fwd, 0.52f))),
                0.05f, 0.04f, 10, gunColor
            );
        };

        if (gameMode == GameMode::CoOp || (gameMode == GameMode::Online && netRole == NetRole::Host)) {
            DrawHumanoid(coopPos, coopYaw, coopHealth, SKYBLUE, DARKBLUE);
        }
        if (gameMode == GameMode::Online && netRole == NetRole::Client) {
            DrawHumanoid(onlineHostPos, onlineHostYaw, playerHealth, ORANGE, BROWN);
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

        float myHealth = (netRole == NetRole::Client) ? coopHealth : playerHealth;
        float partnerHealth = (netRole == NetRole::Client) ? playerHealth : coopHealth;

        DrawRectangle(20, 20, 340, 115, Fade(BLACK, 0.45f));
        DrawText(TextFormat("HP: %i", static_cast<int>(myHealth)), 36, 35, 30, myHealth > 30.0f ? GREEN : RED);
        DrawText(TextFormat("Score: %i", score), 36, 67, 26, YELLOW);
        DrawText(TextFormat("Wave: %i", wave), 36, 96, 26, SKYBLUE);
        if (gameMode == GameMode::CoOp || gameMode == GameMode::Online) {
            DrawRectangle(20, 145, 340, 42, Fade(BLACK, 0.45f));
            const char* coopLabel = (netRole == NetRole::Client) ? "HOST HP" : ((gameMode == GameMode::Online) ? "ONLINE HP" : "CO-OP HP");
            DrawText(TextFormat("%s: %i", coopLabel, static_cast<int>(partnerHealth)), 36, 154, 28, partnerHealth > 30.0f ? SKYBLUE : ORANGE);
        }

        if (gameMode == GameMode::Online) {
            DrawRectangle(20, 194, 520, 36, Fade(BLACK, 0.45f));
            DrawText(TextFormat("Online: %s", netStatus.empty() ? "Starting..." : netStatus.c_str()), 36, 202, 22, YELLOW);
        }

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

        if (!dead && gameMode != GameMode::SinglePlayer) {
            bool iAmDead = (netRole == NetRole::Client) ? (coopHealth <= 0.0f) : (playerHealth <= 0.0f);
            if (iAmDead) {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
                const char* waitText = "YOU DIED";
                int waitFontSize = 48;
                DrawText(waitText, (screenWidth - MeasureText(waitText, waitFontSize)) / 2, screenHeight / 2 - 60, waitFontSize, RED);
                const char* reviveText = "Partner still fighting! Revive next wave.";
                DrawText(reviveText, (screenWidth - MeasureText(reviveText, 24)) / 2, screenHeight / 2 + 10, 24, YELLOW);
            }
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

    closeNetwork();
    CloseWindow();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
