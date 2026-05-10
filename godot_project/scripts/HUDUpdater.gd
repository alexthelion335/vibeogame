extends Control
class_name HUDUpdater

@onready var health_label: Label = $MarginContainer/VBoxContainer/HealthLabel
@onready var score_label: Label = $MarginContainer/VBoxContainer/ScoreLabel
@onready var wave_label: Label = $MarginContainer/VBoxContainer/WaveLabel
@onready var status_label: Label = $MarginContainer/VBoxContainer/StatusLabel

func update_from_snapshot(snapshot: Dictionary) -> void:
    health_label.text = "Health: %s" % snapshot.get("player_health", 100)
    score_label.text = "Score: %s" % snapshot.get("score", 0)
    wave_label.text = "Wave: %s" % snapshot.get("wave", 1)
    status_label.text = snapshot.get("net_status", "Ready")
