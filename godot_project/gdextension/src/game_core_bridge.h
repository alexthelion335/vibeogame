#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include "../../../src/game_core.h"

namespace godot {

class GameCoreBridge : public RefCounted {
    GDCLASS(GameCoreBridge, RefCounted)

private:
    GameState state_;
    int screen_width_ = 1280;
    int screen_height_ = 720;

    static InputState input_from_dictionary(const Dictionary &dictionary);
    static Dictionary snapshot_to_dictionary(const RenderSnapshot &snapshot);
    static Dictionary vector3_to_dictionary(const ::Vector3 &value);
    static ::Vector3 dictionary_to_vector3(const Dictionary &dictionary, const ::Vector3 &fallback);

protected:
    static void _bind_methods();

public:
    GameCoreBridge();

    void update(const Dictionary &input, double delta);
    Dictionary get_render_state() const;
    void start_game(int mode);
    void set_resolution(int width, int height);
    void reset();
};

} // namespace godot
