#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "game_core_bridge.h"

using namespace godot;

static void initialize_chicken_potato_fps(ModuleInitializationLevel level) {
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    ClassDB::register_class<GameCoreBridge>();
}

static void uninitialize_chicken_potato_fps(ModuleInitializationLevel level) {
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
GDExtensionBool GDE_EXPORT chicken_fps_library_init(
    GDExtensionInterfaceGetProcAddress get_proc_address,
    GDExtensionClassLibraryPtr library,
    GDExtensionInitialization *initialization) {
    GDExtensionBinding::InitObject init_object(get_proc_address, library, initialization);
    init_object.register_initializer(initialize_chicken_potato_fps);
    init_object.register_terminator(uninitialize_chicken_potato_fps);
    init_object.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_object.init();
}
}
