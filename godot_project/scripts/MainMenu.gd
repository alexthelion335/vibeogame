extends Control
class_name MainMenuScreen

func _ready() -> void:
    $VBoxContainer/SingleplayerButton.pressed.connect(_on_singleplayer_pressed)
    $VBoxContainer/CoopButton.pressed.connect(_on_coop_pressed)
    $VBoxContainer/OnlineButton.pressed.connect(_on_online_pressed)
    $VBoxContainer/HighscoresButton.pressed.connect(_on_highscores_pressed)

func _on_singleplayer_pressed() -> void:
    GameManager.start_singleplayer()

func _on_coop_pressed() -> void:
    GameManager.start_coop()

func _on_online_pressed() -> void:
    GameManager.open_online_setup()

func _on_highscores_pressed() -> void:
    GameManager.open_highscores()
