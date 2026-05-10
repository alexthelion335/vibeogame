#include "game_core_bridge.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/string.hpp>
#include <sstream>

namespace godot {

static Dictionary vector3_to_dictionary(const ::Vector3 &value) {
    Dictionary dictionary;
    dictionary["x"] = value.x;
    dictionary["y"] = value.y;
    dictionary["z"] = value.z;
    return dictionary;
}

GameCoreBridge::GameCoreBridge() {
    game_core::initialize(state_);
}

void GameCoreBridge::_bind_methods() {
    ClassDB::bind_method(D_METHOD("update", "input", "delta"), &GameCoreBridge::update);
    ClassDB::bind_method(D_METHOD("get_render_state"), &GameCoreBridge::get_render_state);
    ClassDB::bind_method(D_METHOD("start_game", "mode"), &GameCoreBridge::start_game);
    ClassDB::bind_method(D_METHOD("set_resolution", "width", "height"), &GameCoreBridge::set_resolution);
    ClassDB::bind_method(D_METHOD("reset"), &GameCoreBridge::reset);
}

InputState GameCoreBridge::input_from_dictionary(const Dictionary &dictionary) {
    InputState input;
    input.move_forward = dictionary.get("move_forward", false);
    input.move_backward = dictionary.get("move_backward", false);
    input.move_left = dictionary.get("move_left", false);
    input.move_right = dictionary.get("move_right", false);
    input.sprint = dictionary.get("sprint", false);
    input.jump = dictionary.get("jump", false);
    input.shoot = dictionary.get("shoot", false);
    input.pause = dictionary.get("pause", false);
    input.toggle_inventory = dictionary.get("toggle_inventory", false);
    input.toggle_shop = dictionary.get("toggle_shop", false);
    input.weapon_switch_next = dictionary.get("weapon_switch_next", false);
    input.weapon_switch_prev = dictionary.get("weapon_switch_prev", false);
    input.activate_shield = dictionary.get("activate_shield", false);
    input.activate_scythe = dictionary.get("activate_scythe", false);
    input.mouse_delta_x = dictionary.get("mouse_delta_x", 0.0);
    input.mouse_delta_y = dictionary.get("mouse_delta_y", 0.0);
    input.touch_camera_delta_x = dictionary.get("touch_camera_delta_x", 0.0);
    input.touch_camera_delta_y = dictionary.get("touch_camera_delta_y", 0.0);
    input.touch_fire_tap = dictionary.get("touch_fire_tap", false);
    input.touch_fire_hold = dictionary.get("touch_fire_hold", false);
    input.ui_up = dictionary.get("ui_up", false);
    input.ui_down = dictionary.get("ui_down", false);
    input.ui_left = dictionary.get("ui_left", false);
    input.ui_right = dictionary.get("ui_right", false);
    input.ui_select = dictionary.get("ui_select", false);
    input.ui_back = dictionary.get("ui_back", false);

    Dictionary coop = dictionary.get("coop", Dictionary());
    input.coop.move_forward = coop.get("move_forward", false);
    input.coop.move_backward = coop.get("move_backward", false);
    input.coop.move_left = coop.get("move_left", false);
    input.coop.move_right = coop.get("move_right", false);
    input.coop.sprint = coop.get("sprint", false);
    input.coop.jump = coop.get("jump", false);
    input.coop.shoot = coop.get("shoot", false);

    return input;
}

Dictionary GameCoreBridge::vector3_to_dictionary(const ::Vector3 &value) {
    return ::godot::vector3_to_dictionary(value);
}

static Dictionary chicken_to_dictionary(const Chicken &chicken) {
    Dictionary dictionary;
    dictionary["pos"] = vector3_to_dictionary(chicken.pos);
    dictionary["hp"] = chicken.hp;
    dictionary["radius"] = chicken.radius;
    dictionary["is_brown"] = chicken.isBrown;
    dictionary["facing_yaw"] = chicken.facingYaw;
    return dictionary;
}

static Dictionary potato_to_dictionary(const Potato &potato) {
    Dictionary dictionary;
    dictionary["pos"] = vector3_to_dictionary(potato.pos);
    dictionary["radius"] = potato.radius;
    return dictionary;
}

static Dictionary enemy_potato_to_dictionary(const EnemyPotato &potato) {
    Dictionary dictionary;
    dictionary["pos"] = vector3_to_dictionary(potato.pos);
    dictionary["radius"] = potato.radius;
    return dictionary;
}

::Vector3 GameCoreBridge::dictionary_to_vector3(const Dictionary &dictionary, const ::Vector3 &fallback) {
    if (dictionary.is_empty()) {
        return fallback;
    }

    ::Vector3 value;
    value.x = dictionary.get("x", fallback.x);
    value.y = dictionary.get("y", fallback.y);
    value.z = dictionary.get("z", fallback.z);
    return value;
}

Dictionary GameCoreBridge::snapshot_to_dictionary(const RenderSnapshot &snapshot) {
    Dictionary dictionary;
    dictionary["player_pos"] = vector3_to_dictionary(snapshot.player_pos);
    dictionary["player_yaw"] = snapshot.player_yaw;
    dictionary["player_pitch"] = snapshot.player_pitch;
    dictionary["player_health"] = snapshot.player_health;
    dictionary["coop_health"] = snapshot.coop_health;
    dictionary["score"] = snapshot.score;
    dictionary["wave"] = snapshot.wave;
    dictionary["dead"] = snapshot.dead;
    dictionary["death_cause"] = static_cast<int>(snapshot.death_cause);
    dictionary["chickens_remaining"] = snapshot.chickens_remaining;
    dictionary["wave_total_to_spawn"] = snapshot.wave_total_to_spawn;
    dictionary["wave_spawned"] = snapshot.wave_spawned;
    dictionary["wave_title_timer"] = snapshot.wave_title_timer;
    dictionary["chicken_nuggets"] = snapshot.chicken_nuggets;
    dictionary["current_weapon_slot"] = snapshot.current_weapon_slot;
    dictionary["shop_open"] = snapshot.shop_open;
    dictionary["inventory_open"] = snapshot.inventory_open;
    dictionary["paused"] = snapshot.paused;
    dictionary["shield_active"] = snapshot.shield_active;
    dictionary["shield_timer"] = snapshot.shield_timer;
    dictionary["owns_scythe"] = snapshot.owns_scythe;
    dictionary["damage_flash_timer"] = snapshot.damage_flash_timer;
    dictionary["net_status"] = String(snapshot.net_status.c_str());
    dictionary["net_role"] = static_cast<int>(snapshot.net_role);
    dictionary["entering_initials"] = snapshot.entering_initials;
    dictionary["pending_initials"] = String(snapshot.pending_initials.c_str());
    dictionary["pending_score"] = snapshot.pending_score;

    Array chickens;
    for (const Chicken &chicken : snapshot.chickens) {
        chickens.push_back(chicken_to_dictionary(chicken));
    }
    dictionary["chickens"] = chickens;

    Array potatoes;
    for (const Potato &potato : snapshot.potatoes) {
        potatoes.push_back(potato_to_dictionary(potato));
    }
    dictionary["potatoes"] = potatoes;

    Array enemy_potatoes;
    for (const EnemyPotato &potato : snapshot.enemy_potatoes) {
        enemy_potatoes.push_back(enemy_potato_to_dictionary(potato));
    }
    dictionary["enemy_potatoes"] = enemy_potatoes;

    Array audio_events;
    for (const std::string &event : snapshot.audio_events) {
        audio_events.push_back(String(event.c_str()));
    }
    dictionary["audio_events"] = audio_events;

    return dictionary;
}

void GameCoreBridge::update(const Dictionary &input, double delta) {
    InputState core_input = input_from_dictionary(input);
    // Debug: print shoot flag and potato count before update
    {
        std::ostringstream ss;
        ss << "[GameCoreBridge] before update: shoot=" << (core_input.shoot ? 1 : 0)
           << " potatoes=" << state_.potatoes.size()
           << " player_health=" << state_.player_health
           << " paused=" << (state_.paused ? 1 : 0)
           << " shop_open=" << (state_.shop_open ? 1 : 0)
           << " inventory_open=" << (state_.inventory_open ? 1 : 0)
           << " shoot_cooldown=" << state_.shoot_cooldown
           << " weapon_slot=" << state_.current_weapon_slot;
        UtilityFunctions::print(String(ss.str().c_str()));
    }

    size_t before_count = state_.potatoes.size();
    game_core::update(state_, core_input, static_cast<float>(delta));

    // Debug: print potato count after update and positions for newly spawned potatoes
    {
        size_t after_count = state_.potatoes.size();
        std::ostringstream ss;
        ss << "[GameCoreBridge] after update: potatoes=" << after_count
           << " shoot_cooldown=" << state_.shoot_cooldown;
        UtilityFunctions::print(String(ss.str().c_str()));
        if (after_count > before_count) {
            for (size_t i = before_count; i < after_count; ++i) {
                const auto &p = state_.potatoes[i];
                std::ostringstream ps;
                ps << "[GameCoreBridge] new_potato[" << i << "] pos=(" << p.pos.x << "," << p.pos.y << "," << p.pos.z << ") vel=(" << p.vel.x << "," << p.vel.y << "," << p.vel.z << ")";
                UtilityFunctions::print(String(ps.str().c_str()));
            }
        }
    }
}

Dictionary GameCoreBridge::get_render_state() const {
    return snapshot_to_dictionary(game_core::get_render_snapshot(state_));
}

void GameCoreBridge::start_game(int mode) {
    reset();
    state_.game_mode = static_cast<GameMode>(mode);
    state_.screen_state = ScreenState::Playing;
}

void GameCoreBridge::set_resolution(int width, int height) {
    screen_width_ = width;
    screen_height_ = height;
    UtilityFunctions::print(String("GameCoreBridge resolution set to ") + String::num_int64(width) + "x" + String::num_int64(height));
}

void GameCoreBridge::reset() {
    game_core::reset_game(state_);
}

} // namespace godot
