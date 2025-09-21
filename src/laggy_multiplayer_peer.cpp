// Copyright (c) 2025 Ricardo Nunes
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#include "laggy_multiplayer_peer.h"
#include "variant_utils.h"

#include <godot_cpp/classes/time.hpp>

static double get_time() {
	static Time *time = Time::get_singleton();
	return time->get_ticks_usec() / 1'000'000.0;
}

template <typename IterFunc>
static void process_packets(LaggyMultiplayerPeer::PeerChannelMap &p_map, double p_time, IterFunc p_callback) {
	for (auto &[_, channel_map] : p_map) {
		for (auto &[_, channel] : channel_map) {
			while (std::optional<LaggyPacket> packet = channel.take_next(p_time)) {
				p_callback(static_cast<const LaggyPacket &>(packet.value()));
			}
		}
	}
}

void LaggyMultiplayerPeer::on_peer_connected(Peer p_id) {
	emit_signal(SNAME("peer_connected"), p_id);
}

void LaggyMultiplayerPeer::on_peer_disconnected(Peer p_id) {
	send_channels.erase(p_id);
	receive_channels.erase(p_id);
	emit_signal(SNAME("peer_disconnected"), p_id);
}

void LaggyMultiplayerPeer::get_random_delay(double &out_delay, bool &out_drop_packet) const {
	out_delay = rng->randf_range(delay_minimum, Math::max(delay_minimum, delay_maximum));
	if (packet_loss > 0.0 && rng->randf() < packet_loss) {
		out_drop_packet = true;
	}
}

void LaggyMultiplayerPeer::call_handler(const Callable &p_handler, const char *p_handler_name, int32_t p_peer, TransferMode p_mode, int32_t p_channel, int32_t p_buffer_size, double &out_delay, bool &out_drop_packet) {
	Error err;
	Variant result;
	Dictionary params;
	params["peer"] = p_peer;
	params["mode"] = p_mode;
	params["channel"] = p_channel;
	params["size"] = p_buffer_size;

	drop_requested = false;
	running_handler = true;

	if (p_handler.get_argument_count() <= 1) {
		result = VariantUtils::call(p_handler, err, params);
	} else {
		result = VariantUtils::call(p_handler, err, params, this);
	}
	out_drop_packet = drop_requested;

	running_handler = false;
	drop_requested = false;

	if (err == OK && result.get_type() != Variant::Type::INT && result.get_type() != Variant::Type::FLOAT && result.get_type() != Variant::Type::NIL) {
		out_delay = 0.0;
		String type = UtilityFunctions::type_string(result.get_type());
		ERR_FAIL_MSG(vformat("%s must return an int, float, or null. Returned: %s.", p_handler_name, type));
	}

	out_delay = result.operator double();
	out_delay = Math::max(0.0, out_delay);
}

void LaggyMultiplayerPeer::retry(Vector<LaggyPacket> &p_retry_packets, const Callable &p_custom_handler, const char *p_handler_name, double p_time, PeerChannelMap &p_channel_map) {
	Vector<LaggyPacket> retry_again = {};

	for (LaggyPacket &packet : p_retry_packets) {
		ERR_CONTINUE(packet.mode != TRANSFER_MODE_RELIABLE);

		if (packet.time_of_delivery > p_time) {
			retry_again.push_back(packet);
			continue;
		}

		double delay = 0.0;
		bool drop_packet = false;
		if (p_custom_handler.is_valid()) {
			call_handler(p_custom_handler, p_handler_name, packet.peer, packet.mode, packet.channel, packet.data.size(), delay, drop_packet);
		} else {
			get_random_delay(delay, drop_packet);
		}

		packet.time_of_delivery = p_time + delay;

		if (drop_packet) {
			retry_again.push_back(packet);
		} else {
			p_channel_map[packet.peer][packet.channel].push(packet);
		}
	}

	p_retry_packets = retry_again;
}

Dictionary LaggyMultiplayerPeer::get_version() {
	static Dictionary version = []() {
		Dictionary v;
		v["major"] = 0;
		v["minor"] = 1;
		v["patch"] = 0;
		v.make_read_only();
		return v;
	}();
	return version;
}

Ref<LaggyMultiplayerPeer> LaggyMultiplayerPeer::create(const Ref<MultiplayerPeer> &p_wrapped_peer, double p_delay_minimum, double p_delay_maximum, double p_packet_loss) {
	Ref new_peer = memnew(LaggyMultiplayerPeer);
	new_peer->set_wrapped_peer(p_wrapped_peer);
	new_peer->set_delay_maximum(p_delay_maximum);
	new_peer->set_delay_minimum(p_delay_minimum);
	new_peer->set_packet_loss(p_packet_loss);
	return new_peer;
}

void LaggyMultiplayerPeer::drop_packet() {
	ERR_FAIL_COND_MSG(!running_handler, "Tried to call drop_packet() outside of handle_send or handle_receive!");
	drop_requested = true;
}

void LaggyMultiplayerPeer::set_wrapped_peer(const Ref<MultiplayerPeer> &p_peer) {
	if (p_peer == wrapped_peer) {
		return;
	}
	if (wrapped_peer.is_valid()) {
		wrapped_peer->disconnect("peer_connected", callable_mp(this, &LaggyMultiplayerPeer::on_peer_connected));
		wrapped_peer->disconnect("peer_disconnected", callable_mp(this, &LaggyMultiplayerPeer::on_peer_disconnected));
	}
	wrapped_peer = p_peer;
	if (wrapped_peer.is_valid()) {
		wrapped_peer->connect("peer_connected", callable_mp(this, &LaggyMultiplayerPeer::on_peer_connected));
		wrapped_peer->connect("peer_disconnected", callable_mp(this, &LaggyMultiplayerPeer::on_peer_disconnected));
	}
	send_channels.clear();
	receive_channels.clear();
	available_packets.clear();
}

Ref<MultiplayerPeer> LaggyMultiplayerPeer::get_wrapped_peer() const {
	return wrapped_peer;
}

Error LaggyMultiplayerPeer::_get_packet(const uint8_t **r_buffer, int32_t *r_buffer_size) {
	ERR_FAIL_COND_V(available_packets.size() == 0, ERR_UNAVAILABLE);
	current_packet = available_packets[0];
	*r_buffer = current_packet.data.ptrw();
	*r_buffer_size = current_packet.data.size();
	available_packets.remove_at(0);
	return OK;
}

Error LaggyMultiplayerPeer::_put_packet(const uint8_t *p_buffer, int32_t p_buffer_size) {
	ERR_FAIL_COND_V(wrapped_peer.is_null(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(target_peer <= 0, ERR_INVALID_PARAMETER);

	double delay = 0.0;
	bool drop_packet = false;
	if (handle_send.is_valid()) {
		call_handler(handle_send, "handle_send", target_peer, transfer_mode, transfer_channel, p_buffer_size, delay, drop_packet);
	} else {
		get_random_delay(delay, drop_packet);
	}

	PackedByteArray data;
	Error err = static_cast<Error>(data.resize(p_buffer_size));
	ERR_FAIL_COND_V(err != OK, err);
	memcpy(data.ptrw(), p_buffer, p_buffer_size);
	LaggyPacket packet = {
		data,
		transfer_mode,
		0,
		transfer_channel,
		target_peer,
		get_time() + delay,
	};

	LaggyPacketChannel &channel = send_channels[target_peer][transfer_channel];
	channel.generate_sequence(packet);

	if (drop_packet) {
		if (transfer_mode == TRANSFER_MODE_RELIABLE) {
			retry_send_packets.push_back(packet);
		}
		return OK;
	}

	channel.push(packet);
	return OK;
}

int32_t LaggyMultiplayerPeer::_get_packet_channel() const {
	ERR_FAIL_COND_V(available_packets.size() == 0, 0);
	return available_packets[0].channel;
}

MultiplayerPeer::TransferMode LaggyMultiplayerPeer::_get_packet_mode() const {
	ERR_FAIL_COND_V(available_packets.size() == 0, TransferMode::TRANSFER_MODE_RELIABLE);
	return available_packets[0].mode;
}

int32_t LaggyMultiplayerPeer::_get_packet_peer() const {
	ERR_FAIL_COND_V(available_packets.size() == 0, 0);
	return available_packets[0].peer;
}

bool LaggyMultiplayerPeer::_is_server() const {
	ERR_FAIL_COND_V(wrapped_peer.is_null(), true);
	return wrapped_peer->get_unique_id() == TARGET_PEER_SERVER;
}

void LaggyMultiplayerPeer::_poll() {
	ERR_FAIL_COND(wrapped_peer.is_null());
	wrapped_peer->poll();
	if (get_connection_status() != CONNECTION_CONNECTED) {
		return;
	}

	double current_time = get_time();

	// Send packets
	process_packets(send_channels, current_time, [&](const LaggyPacket &packet) {
		wrapped_peer->set_target_peer(packet.peer);
		wrapped_peer->set_transfer_mode(packet.mode);
		wrapped_peer->set_transfer_channel(packet.channel);

		Error err = wrapped_peer->put_packet(packet.data);
		ERR_FAIL_COND_MSG(err != OK, vformat("wrapped_peer->put_packet(packet.data) returned error: %s", UtilityFunctions::error_string(err)));
	});

	// Receive packets
	while (wrapped_peer->get_available_packet_count() > 0) {
		TransferMode packet_mode = wrapped_peer->get_packet_mode();
		int32_t packet_channel = wrapped_peer->get_packet_channel();
		int32_t packet_sender = wrapped_peer->get_packet_peer();

		PackedByteArray data = wrapped_peer->get_packet();
		Error err = wrapped_peer->get_packet_error();
		ERR_CONTINUE_MSG(err != OK, vformat("wrapped_peer->get_packet_error() returned error: %s", UtilityFunctions::error_string(err)));

		double delay = 0.0;
		bool drop_packet = false;
		if (handle_receive.is_valid()) {
			call_handler(handle_receive, "handle_receive", packet_sender, packet_mode, packet_channel, data.size(), delay, drop_packet);
		} else {
			get_random_delay(delay, drop_packet);
		}

		LaggyPacket packet = {
			data,
			packet_mode,
			0,
			packet_channel,
			packet_sender,
			current_time + delay,
		};
		LaggyPacketChannel &channel = receive_channels[packet_sender][packet_channel];
		channel.generate_sequence(packet);

		if (drop_packet) {
			if (packet_mode == TRANSFER_MODE_RELIABLE) {
				retry_receive_packets.push_back(packet);
			}
		} else {
			channel.push(packet);
		}
	}

	// Retry dropped packets
	retry(retry_send_packets, handle_send, "handle_send", current_time, send_channels);
	retry(retry_receive_packets, handle_receive, "handle_receive", current_time, receive_channels);

	// Enqueue available packets
	process_packets(receive_channels, current_time, [&](const LaggyPacket &packet) {
		available_packets.push_back(packet);
	});
}

void LaggyMultiplayerPeer::_close() {
	ERR_FAIL_COND(wrapped_peer.is_null());
	wrapped_peer->close();
}

void LaggyMultiplayerPeer::_disconnect_peer(int32_t p_peer, bool p_force) {
	ERR_FAIL_COND(wrapped_peer.is_null());
	wrapped_peer->disconnect_peer(p_peer, p_force);
}

int32_t LaggyMultiplayerPeer::_get_unique_id() const {
	ERR_FAIL_COND_V(wrapped_peer.is_null(), 0);
	return wrapped_peer->get_unique_id();
}

void LaggyMultiplayerPeer::_set_refuse_new_connections(bool p_enable) {
	ERR_FAIL_COND(wrapped_peer.is_null());
	wrapped_peer->set_refuse_new_connections(p_enable);
}

bool LaggyMultiplayerPeer::_is_refusing_new_connections() const {
	return wrapped_peer.is_valid() && wrapped_peer->is_refusing_new_connections();
}

bool LaggyMultiplayerPeer::_is_server_relay_supported() const {
	ERR_FAIL_COND_V(wrapped_peer.is_null(), false);
	return wrapped_peer->is_server_relay_supported();
}

MultiplayerPeer::ConnectionStatus LaggyMultiplayerPeer::_get_connection_status() const {
	ERR_FAIL_COND_V(wrapped_peer.is_null(), ConnectionStatus::CONNECTION_DISCONNECTED);
	return wrapped_peer->get_connection_status();
}

void LaggyMultiplayerPeer::_bind_methods() {
	ClassDB::bind_static_method("LaggyMultiplayerPeer", D_METHOD("get_version"), &LaggyMultiplayerPeer::get_version);
	ClassDB::bind_static_method("LaggyMultiplayerPeer", D_METHOD("create", "wrapped_peer", "delay_minimum", "delay_maximum", "packet_loss"), &LaggyMultiplayerPeer::create, DEFVAL(0.0), DEFVAL(0.0), DEFVAL(0.0));

	ClassDB::bind_method(D_METHOD("drop_packet"), &LaggyMultiplayerPeer::drop_packet);

	ClassDB::bind_method(D_METHOD("set_wrapped_peer", "peer"), &LaggyMultiplayerPeer::set_wrapped_peer);
	ClassDB::bind_method(D_METHOD("get_wrapped_peer"), &LaggyMultiplayerPeer::get_wrapped_peer);
	ClassDB::bind_method(D_METHOD("set_handle_send", "callback"), &LaggyMultiplayerPeer::set_handle_send);
	ClassDB::bind_method(D_METHOD("get_handle_send"), &LaggyMultiplayerPeer::get_handle_send);
	ClassDB::bind_method(D_METHOD("set_handle_receive", "callback"), &LaggyMultiplayerPeer::set_handle_receive);
	ClassDB::bind_method(D_METHOD("get_handle_receive"), &LaggyMultiplayerPeer::get_handle_receive);
	ClassDB::bind_method(D_METHOD("set_delay_minimum", "value"), &LaggyMultiplayerPeer::set_delay_minimum);
	ClassDB::bind_method(D_METHOD("get_delay_minimum"), &LaggyMultiplayerPeer::get_delay_minimum);
	ClassDB::bind_method(D_METHOD("set_delay_maximum", "value"), &LaggyMultiplayerPeer::set_delay_maximum);
	ClassDB::bind_method(D_METHOD("get_delay_maximum"), &LaggyMultiplayerPeer::get_delay_maximum);
	ClassDB::bind_method(D_METHOD("set_packet_loss", "loss"), &LaggyMultiplayerPeer::set_packet_loss);
	ClassDB::bind_method(D_METHOD("get_packet_loss"), &LaggyMultiplayerPeer::get_packet_loss);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "wrapped_peer", PROPERTY_HINT_RESOURCE_TYPE, "MultiplayerPeer"), "set_wrapped_peer", "get_wrapped_peer");
	ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "handle_send"), "set_handle_send", "get_handle_send");
	ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "handle_receive"), "set_handle_receive", "get_handle_receive");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "delay_minimum"), "set_delay_minimum", "get_delay_minimum");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "delay_maximum"), "set_delay_maximum", "get_delay_maximum");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "packet_loss"), "set_packet_loss", "get_packet_loss");
}
