extends Node2D

@onready var player_scene: PackedScene = $ResourcePreloader.get_resource(&"Player")
@onready var spawner: MultiplayerSpawner = $MultiplayerSpawner
@onready var label_canvas: CanvasLayer = $LabelCanvas
@onready var lag_settings_canvas: CanvasLayer = $LagSettingsCanvas
@onready var min_delay_label: Label = %MinDelayLabel
@onready var max_delay_label: Label = %MaxDelayLabel
@onready var packet_loss_label: Label = %PacketLossLabel

var port := 5050
var players: Dictionary[int, Node2D]
var peer: LaggyMultiplayerPeer

func _ready() -> void:
	label_canvas.show()
	lag_settings_canvas.hide()
	spawner.spawn_function = spawn_player
	if "--server" in OS.get_cmdline_args():
		setup_server()
	else:
		setup_client()

func spawn_player(data: Dictionary) -> Node:
	var player := player_scene.instantiate()
	player.position = data.position
	player.name = data.name
	return player

func add_player(id: int) -> void:
	players[id] = spawner.spawn({ name = str(id), position = Vector2(100 + 100 * players.size(), 100) })

func remove_player(id: int) -> void:
	if id in players:
		players[id].queue_free()
		players.erase(id)

func setup_server() -> void:
	get_window().title += " (Server)"
	var enet := ENetMultiplayerPeer.new()
	var err := enet.create_server(port)
	if err:
		fatal("Failed to create server: %s" % error_string(err))
		return
	multiplayer.peer_connected.connect(add_player)
	multiplayer.peer_disconnected.connect(remove_player)
	multiplayer.multiplayer_peer = enet
	label_canvas.hide()
	add_player(1)

func setup_client() -> void:
	get_window().title += " (Client)"
	var enet := ENetMultiplayerPeer.new()
	var err := enet.create_client("localhost", port)
	if err:
		fatal("Failed to create client: %s" % error_string(err))
		return
	multiplayer.connection_failed.connect(func(): fatal("Failed to connect to server"))
	multiplayer.server_disconnected.connect(func(): fatal("Lost connection to server"))
	multiplayer.connected_to_server.connect(label_canvas.hide)
	multiplayer.connected_to_server.connect(lag_settings_canvas.show)
	print("LaggyMultiplayerPeer version: ", LaggyMultiplayerPeer.get_version())
	peer = LaggyMultiplayerPeer.create(enet)
	multiplayer.multiplayer_peer = peer

func fatal(message: String) -> void:
	OS.alert(message)
	get_tree().quit(1)

func _on_min_delay_slider_value_changed(value: float) -> void:
	peer.delay_minimum = value
	min_delay_label.text = "%dms" % (value * 1000)

func _on_max_delay_slider_value_changed(value: float) -> void:
	peer.delay_maximum = value
	max_delay_label.text = "%dms" % (value * 1000)
 
func _on_packet_loss_slider_value_changed(value: float) -> void:
	peer.packet_loss = value
	packet_loss_label.text = "%d%%" % (value * 100)
