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

#include "laggy_packet.h"
#include "variant_utils.h"

void LaggyPacketChannel::drop_out_of_order_packets() {
	while (packets.size() > 0 && packets[0].mode == MultiplayerPeer::TRANSFER_MODE_UNRELIABLE_ORDERED && packets[0].sequence <= last_handled_ordered) {
		packets.remove_at(0);
	}
}

void LaggyPacketChannel::generate_sequence(LaggyPacket &p_packet) {
	ERR_FAIL_COND(p_packet.sequence != 0);
	switch (p_packet.mode) {
		case MultiplayerPeer::TRANSFER_MODE_UNRELIABLE:
			break;
		case MultiplayerPeer::TRANSFER_MODE_UNRELIABLE_ORDERED:
			p_packet.sequence = ordered_sequence++;
			break;
		case MultiplayerPeer::TRANSFER_MODE_RELIABLE:
			p_packet.sequence = reliable_sequence++;
			break;
	}
}

void LaggyPacketChannel::push(const LaggyPacket &p_packet) {
	ERR_FAIL_COND(p_packet.mode != MultiplayerPeer::TRANSFER_MODE_UNRELIABLE && p_packet.sequence == 0);
	packets.push_back(p_packet);
}

std::optional<LaggyPacket> LaggyPacketChannel::take_next(double p_time) {
	drop_out_of_order_packets();
	for (Size i = 0; i < packets.size(); i++) {
		if (const LaggyPacket &packet = packets[i]; packet.time_of_delivery <= p_time) {
			bool consume = true;
			switch (packet.mode) {
				case MultiplayerPeer::TRANSFER_MODE_UNRELIABLE:
					break;
				case MultiplayerPeer::TRANSFER_MODE_UNRELIABLE_ORDERED:
					last_handled_ordered = packet.sequence;
					break;
				case MultiplayerPeer::TRANSFER_MODE_RELIABLE:
					if (packet.sequence == last_handled_reliable + 1) {
						last_handled_reliable = packet.sequence;
					} else {
						consume = false;
					}
					break;
			}
			if (consume) {
				std::optional result = { packet };
				packets.remove_at(i);
				return result;
			}
		}
	}
	return {};
}
