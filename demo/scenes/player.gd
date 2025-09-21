extends Sprite2D

@onready var ping_scene: PackedScene = $ResourcePreloader.get_resource(&"Ping")

var target_position: Vector2
var reliable_counter := 0

func _enter_tree() -> void:
	set_multiplayer_authority(name.to_int())

func _ready() -> void:
	$Label.text = name
	target_position = global_position
	# Make remote players semi-trasnslucent
	if not is_multiplayer_authority():
		modulate.a = 0.5
		z_index = -1

func _unhandled_input(event: InputEvent) -> void:
	if is_multiplayer_authority():
		if event is InputEventMouseButton or event is InputEventMouseMotion:
			if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
				target_position = get_global_mouse_position()

func _process(delta: float) -> void:
	if is_multiplayer_authority():
		if Input.is_action_pressed(&"ui_select"):
			handle_reliable_counter.rpc(reliable_counter)
			reliable_counter += 1
		global_position = global_position.move_toward(target_position, 500 * delta)

func _input(event: InputEvent) -> void:
	if is_multiplayer_authority() and event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_RIGHT:
		create_ping.rpc(get_global_mouse_position())

@rpc("authority", "call_local", "reliable")
func create_ping(spawn_position: Vector2) -> void:
	var ping: Node2D = ping_scene.instantiate()
	ping.position = spawn_position
	add_child(ping)

@rpc("any_peer", "reliable")
func handle_reliable_counter(count: int) -> void:
	print("[%d] Received reliable counter from peer %d: %d" % [multiplayer.get_unique_id(), multiplayer.get_remote_sender_id(), count])
