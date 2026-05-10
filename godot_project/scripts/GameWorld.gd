extends Node3D
class_name GameWorldScreen

var core: Object = null
var chicken_nodes: Array[MeshInstance3D] = []
var potato_nodes: Array[MeshInstance3D] = []
var enemy_potato_nodes: Array[MeshInstance3D] = []

@onready var camera: Camera3D = $PlayerRig/Camera3D
@onready var player_rig: Node3D = $PlayerRig
@onready var world_plane: MeshInstance3D = $WorldPlane

var chicken_mesh := SphereMesh.new()
var potato_mesh := SphereMesh.new()
var enemy_potato_mesh := SphereMesh.new()
var chicken_material := StandardMaterial3D.new()
var potato_material := StandardMaterial3D.new()
var enemy_potato_material := StandardMaterial3D.new()
var _dbg_frame := 0
var _last_potato_count := 0

func _ready() -> void:
	chicken_mesh.radius = 0.5
	chicken_mesh.height = 1.0
	potato_mesh.radius = 0.16
	potato_mesh.height = 0.32
	enemy_potato_mesh.radius = 0.13
	enemy_potato_mesh.height = 0.26

	chicken_material.albedo_color = Color(0.95, 0.84, 0.22)
	potato_material.albedo_color = Color(0.76, 0.54, 0.28)
	potato_material.emission = Color(0.9, 0.6, 0.2)
	potato_material.emission_enabled = true
	enemy_potato_material.albedo_color = Color(0.85, 0.2, 0.2)

	world_plane.material_override = StandardMaterial3D.new()
	world_plane.material_override.albedo_color = Color(0.18, 0.2, 0.22)
	world_plane.position = Vector3.ZERO
	world_plane.scale = Vector3(80.0, 1.0, 80.0)

	core = ClassDB.instantiate("GameCoreBridge")
	if core == null:
		push_warning("GameCoreBridge is not available yet; running in placeholder mode.")
		return

	core.call("start_game", 0)

func _process(delta: float) -> void:
	if core == null:
		return

	_dbg_frame += 1
	if _dbg_frame >= 30:
		_dbg_frame = 0
		# debug raw input checks
		var raw_action_shoot := Input.is_action_pressed("shoot")
		var raw_mouse_left := Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT)
		print("[GameWorld RAW INPUT] action_shoot=", raw_action_shoot, " mouse_left=", raw_mouse_left)

	var input_state := _build_input_state()
	# Debug: print input state when movement or shooting occurs
	if input_state.get("move_forward", false) or input_state.get("move_backward", false) or input_state.get("move_left", false) or input_state.get("move_right", false) or input_state.get("shoot", false):
		print("[GameWorld] input:", input_state)
	core.call("update", input_state, delta)
	var snapshot: Dictionary = core.call("get_render_state")
	_apply_snapshot(snapshot)

func _build_input_state() -> Dictionary:
	var mouse_velocity := -Input.get_last_mouse_velocity() * 0.0026
	return {
		"move_forward": Input.is_action_pressed("move_forward") or Input.is_physical_key_pressed(KEY_W),
		"move_backward": Input.is_action_pressed("move_backward") or Input.is_physical_key_pressed(KEY_S),
		"move_left": Input.is_action_pressed("move_left") or Input.is_physical_key_pressed(KEY_A),
		"move_right": Input.is_action_pressed("move_right") or Input.is_physical_key_pressed(KEY_D),
		"jump": Input.is_action_pressed("jump") or Input.is_physical_key_pressed(KEY_SPACE),
		"sprint": Input.is_action_pressed("sprint") or Input.is_physical_key_pressed(KEY_SHIFT),
		"shoot": Input.is_action_pressed("shoot") or Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT),
		"pause": Input.is_action_just_pressed("pause") or Input.is_physical_key_pressed(KEY_ESCAPE),
		"mouse_delta_x": mouse_velocity.x,
		"mouse_delta_y": mouse_velocity.y,
	}

func _apply_snapshot(snapshot: Dictionary) -> void:
	if snapshot.is_empty():
		return

	var potatoes_list: Array = snapshot.get("potatoes", [])
	if potatoes_list.size() != _last_potato_count:
		# Print newly received potato positions
		for i in range(_last_potato_count, potatoes_list.size()):
			var p: Dictionary = potatoes_list[i]
			var pos: Dictionary = p.get("pos", {})
			print("[GameWorld SNAPSHOT] new_potato[", i, "] = ", pos)
		_last_potato_count = potatoes_list.size()

	var player_pos: Dictionary = snapshot.get("player_pos", {})
	player_rig.position = Vector3(
		player_pos.get("x", camera.position.x),
		player_pos.get("y", camera.position.y),
		player_pos.get("z", camera.position.z)
	)

	_sync_entities(snapshot.get("chickens", []), chicken_nodes, chicken_mesh, chicken_material, 0.0)
	_sync_entities(snapshot.get("potatoes", []), potato_nodes, potato_mesh, potato_material, 0.0)
	_sync_entities(snapshot.get("enemy_potatoes", []), enemy_potato_nodes, enemy_potato_mesh, enemy_potato_material, 0.0)

	var player_yaw := float(snapshot.get("player_yaw", 0.0))
	var player_pitch := float(snapshot.get("player_pitch", 0.0))
	player_rig.rotation.y = player_yaw + PI
	camera.rotation.x = player_pitch

	var active_wave := int(snapshot.get("wave", 1))
	if active_wave <= 1 and chicken_nodes.is_empty():
		world_plane.material_override.albedo_color = Color(0.16, 0.18, 0.2)

func _sync_entities(entity_list: Array, node_list: Array, mesh: Mesh, material: Material, y_offset: float) -> void:
	while node_list.size() < entity_list.size():
		var node := MeshInstance3D.new()
		node.mesh = mesh
		node.material_override = material
		add_child(node)
		node_list.append(node)

	for i in range(node_list.size()):
		var node: MeshInstance3D = node_list[i]
		if i >= entity_list.size():
			node.visible = false
			continue

		node.visible = true
		var entity: Dictionary = entity_list[i]
		var pos: Dictionary = entity.get("pos", {})
		node.position = Vector3(
			float(pos.get("x", 0.0)),
			float(pos.get("y", 0.0)) + y_offset,
			float(pos.get("z", 0.0))
		)
