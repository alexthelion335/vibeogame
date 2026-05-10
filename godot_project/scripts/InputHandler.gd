extends Node
class_name InputHandler

signal input_state_changed(state)

func _process(_delta: float) -> void:
    input_state_changed.emit(_build_input_state())

func _build_input_state() -> Dictionary:
    var mouse_delta := Vector2.ZERO
    if Input.get_mouse_mode() == Input.MOUSE_MODE_CAPTURED:
        mouse_delta = Input.get_last_mouse_velocity() * 0.016

    return {
        "move_forward": Input.is_action_pressed("move_forward"),
        "move_backward": Input.is_action_pressed("move_backward"),
        "move_left": Input.is_action_pressed("move_left"),
        "move_right": Input.is_action_pressed("move_right"),
        "sprint": Input.is_action_pressed("sprint"),
        "jump": Input.is_action_just_pressed("jump"),
        "shoot": Input.is_action_pressed("shoot"),
        "pause": Input.is_action_just_pressed("pause"),
        "mouse_delta_x": mouse_delta.x,
        "mouse_delta_y": mouse_delta.y,
        "touch_camera_delta_x": 0.0,
        "touch_camera_delta_y": 0.0,
        "touch_fire_tap": false,
        "touch_fire_hold": false,
    }
