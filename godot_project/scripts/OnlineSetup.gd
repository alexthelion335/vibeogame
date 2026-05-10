extends Control
class_name OnlineSetupScreen

func _ready() -> void:
    $VBoxContainer/HostButton.pressed.connect(_on_host_pressed)
    $VBoxContainer/JoinButton.pressed.connect(_on_join_pressed)

func _on_host_pressed() -> void:
    GameManager.start_singleplayer()

func _on_join_pressed() -> void:
    GameManager.start_singleplayer()
