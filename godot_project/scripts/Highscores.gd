extends Control
class_name HighscoresScreen

func _ready() -> void:
    $VBoxContainer/BackButton.pressed.connect(_on_back_pressed)

func _on_back_pressed() -> void:
    GameManager.back_to_menu()
