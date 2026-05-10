#include <raylib.h>
#include "game_core.h"
#include <cstdlib>

// ============================================================================
// RAYLIB PLATFORM BRIDGE IMPLEMENTATION
// ============================================================================

class RaylibPlatformBridge : public IPlatformBridge {
public:
    std::string read_file(const std::string& path) override {
        if (!FileExists(path.c_str())) {
            return "";
        }
        
        unsigned char* data = LoadFileData(path.c_str(), nullptr);
        if (!data) return "";
        
        std::string result(reinterpret_cast<const char*>(data));
        UnloadFileData(data);
        return result;
    }
    
    bool write_file(const std::string& path, const std::string& content) override {
        // SaveFileText takes non-const char* but doesn't modify them
        // We need to cast away const, which is safe here
        return SaveFileText(const_cast<char*>(path.c_str()), 
                           const_cast<char*>(content.c_str()));
    }
    
    bool file_exists(const std::string& path) override {
        return FileExists(path.c_str());
    }
    
    std::string get_user_data_path() override {
        // Use current working directory for now (Raylib doesn't have a built-in user path)
        // In a real app, you might use platform-specific APIs
        return ".";
    }
    
    float get_random(float min_val, float max_val) override {
        return min_val + (max_val - min_val) * (static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f);
    }
    
    int get_random_int(int min_val, int max_val) override {
        return GetRandomValue(min_val, max_val);
    }
    
    double get_time() override {
        return GetTime();
    }
    
    int create_udp_socket() override {
        // Networking is handled separately; this is a stub
        return -1;
    }
    
    void close_udp_socket(int socket) override {
        // Networking is handled separately
    }
};

// Global instance
static RaylibPlatformBridge g_raylib_bridge;

// Initialize the platform bridge (call from main.cpp on startup)
void init_raylib_bridge() {
    g_platform_bridge = &g_raylib_bridge;
}
