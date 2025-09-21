extends Node

func _ready() -> void:
	if not ClassDB.class_exists(&"LaggyMultiplayerPeer"):
		OS.alert("Class 'LaggyMultiplayerPeer' is not defined, which means the GDExtension probably failed to load! Quitting...")
		get_tree().quit(1)
