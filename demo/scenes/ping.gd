extends Node2D

func _enter_tree() -> void:
	set_multiplayer_authority(get_parent().get_multiplayer_authority())

func _ready() -> void:
	scale = Vector2.ONE * 0.01
	if not is_multiplayer_authority():
		modulate = Color.DIM_GRAY

func _process(delta: float) -> void:
	scale = scale.move_toward(Vector2.ONE, delta)
	modulate.a = move_toward(modulate.a, 0.0, delta)
	if modulate.a == 0:
		queue_free()
