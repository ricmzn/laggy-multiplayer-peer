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

#pragma once

#include "laggy_packet.h"

#include <godot_cpp/classes/multiplayer_peer_extension.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

class LaggyMultiplayerPeer : public MultiplayerPeerExtension {
	GDCLASS(LaggyMultiplayerPeer, MultiplayerPeerExtension)

protected:
	static void _bind_methods();

public:
	typedef HashMap<LaggyPacket::Channel, LaggyPacketChannel> ChannelMap;
	typedef HashMap<LaggyPacket::Peer, ChannelMap> PeerChannelMap;

	typedef LaggyPacket::Sequence Sequence;
	typedef LaggyPacket::Channel Channel;
	typedef LaggyPacket::Peer Peer;

private:
	Ref<RandomNumberGenerator> rng = memnew(RandomNumberGenerator);
	PeerChannelMap send_channels;
	PeerChannelMap receive_channels;
	Vector<LaggyPacket> available_packets;
	Vector<LaggyPacket> retry_send_packets;
	Vector<LaggyPacket> retry_receive_packets;
	LaggyPacket current_packet = {};

	TransferMode transfer_mode = TRANSFER_MODE_RELIABLE;
	Channel transfer_channel = 0;
	Peer target_peer = 0;

	Ref<MultiplayerPeer> wrapped_peer;

	Callable handle_send;
	Callable handle_receive;
	bool running_handler = false;
	bool drop_requested = false;

	double delay_minimum = 0.0;
	double delay_maximum = 0.0;
	double packet_loss = 0.0;

	void on_peer_connected(Peer p_id);
	void on_peer_disconnected(Peer p_id);

	void get_random_delay(double &out_delay, bool &out_drop_packet) const;
	void call_handler(const Callable &p_handler, const char *p_handler_name, int32_t p_peer, TransferMode p_mode, int32_t p_channel, int32_t p_buffer_size, double &out_delay, bool &out_drop_packet);
	void retry(Vector<LaggyPacket> &p_retry_packets, const Callable &p_custom_handler, const char *p_handler_name, double p_time, PeerChannelMap &p_channel_map);

public:
	LaggyMultiplayerPeer() {}
	~LaggyMultiplayerPeer() override {}

	static Dictionary get_version();
	static Ref<LaggyMultiplayerPeer> create(const Ref<MultiplayerPeer> &p_wrapped_peer, double p_delay_minimum, double p_delay_maximum, double p_packet_loss);

	void drop_packet();

	void set_wrapped_peer(const Ref<MultiplayerPeer> &p_peer);
	Ref<MultiplayerPeer> get_wrapped_peer() const;

	void set_handle_send(const Callable &p_callback) { handle_send = p_callback; }
	Callable get_handle_send() const { return handle_send; }

	void set_handle_receive(const Callable &p_callback) { handle_receive = p_callback; }
	Callable get_handle_receive() const { return handle_receive; }

	void set_delay_minimum(double p_value) { delay_minimum = Math::max(p_value, 0.0); }
	double get_delay_minimum() const { return delay_minimum; }

	void set_delay_maximum(double p_value) { delay_maximum = Math::max(p_value, 0.0); }
	double get_delay_maximum() const { return delay_maximum; }

	void set_packet_loss(double p_loss) { packet_loss = Math::clamp(p_loss, 0.0, 1.0); }
	double get_packet_loss() const { return packet_loss; }

	/* Virtual methods */
	Error _get_packet(const uint8_t **r_buffer, int32_t *r_buffer_size) override;
	Error _put_packet(const uint8_t *p_buffer, int32_t p_buffer_size) override;
	int32_t _get_available_packet_count() const override { return available_packets.size(); }
	int32_t _get_packet_channel() const override;
	TransferMode _get_packet_mode() const override;
	int32_t _get_packet_peer() const override;
	void _set_transfer_channel(int32_t p_channel) override { transfer_channel = p_channel; }
	int32_t _get_transfer_channel() const override { return transfer_channel; }
	void _set_transfer_mode(TransferMode p_mode) override { transfer_mode = p_mode; }
	TransferMode _get_transfer_mode() const override { return transfer_mode; }
	void _set_target_peer(int32_t p_peer) override { target_peer = p_peer; }
	bool _is_server() const override;
	void _poll() override;
	void _close() override;
	void _disconnect_peer(int32_t p_peer, bool p_force) override;
	int32_t _get_unique_id() const override;
	void _set_refuse_new_connections(bool p_enable) override;
	bool _is_refusing_new_connections() const override;
	bool _is_server_relay_supported() const override;
	ConnectionStatus _get_connection_status() const override;
};
