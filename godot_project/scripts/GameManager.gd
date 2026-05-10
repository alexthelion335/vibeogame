extends Node

signal screen_changed(screen_name)

var current_screen: String = "MainMenu"
var last_input_state: Dictionary = {}

const SCREEN_SCENES := {
	"MainMenu": "res://scenes/MainMenu.tscn",
	"OnlineSetup": "res://scenes/OnlineSetup.tscn",
	"GameWorld": "res://scenes/GameWorld.tscn",
	"HUD": "res://scenes/HUD.tscn",
	"Shop": "res://scenes/Shop.tscn",
	"Inventory": "res://scenes/Inventory.tscn",
	"PauseMenu": "res://scenes/PauseMenu.tscn",
	"DeathScreen": "res://scenes/DeathScreen.tscn",
	"Highscores": "res://scenes/Highscores.tscn",
}

func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
	current_screen = "MainMenu"
	screen_changed.emit(current_screen)

func _process(_delta: float) -> void:
	last_input_state = _gather_input_state()

func _show_screen(screen_name: String) -> void:
	current_screen = screen_name
	screen_changed.emit(screen_name)

	var scene_path: String = SCREEN_SCENES.get(screen_name, "")
	if scene_path.is_empty():
		return

	var error := get_tree().change_scene_to_file(scene_path)
	if error != OK:
		push_warning("Failed to change scene to %s" % scene_path)

func _gather_input_state() -> Dictionary:
	return {
		"move_forward": Input.is_action_pressed("move_forward"),
		"move_backward": Input.is_action_pressed("move_backward"),
		"move_left": Input.is_action_pressed("move_left"),
		"move_right": Input.is_action_pressed("move_right"),
		"jump": Input.is_action_pressed("jump"),
		"sprint": Input.is_action_pressed("sprint"),
		"shoot": Input.is_action_pressed("shoot"),
		"pause": Input.is_action_just_pressed("pause"),
	}

func start_singleplayer() -> void:
	_show_screen("GameWorld")
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)

func start_coop() -> void:
	_show_screen("GameWorld")
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)

func open_online_setup() -> void:
	_show_screen("OnlineSetup")

func open_highscores() -> void:
	_show_screen("Highscores")

func back_to_menu() -> void:
	_show_screen("MainMenu")
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
