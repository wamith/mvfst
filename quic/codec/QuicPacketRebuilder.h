/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// Copyright 2004-present Facebook.  All rights reserved.
#pragma once

#include <quic/codec/QuicPacketBuilder.h>
#include <quic/state/StateData.h>

namespace quic {

/**
 * A PacketRebuilder is a packet builder that takes in a list of frames, and
 * pass them onto a wrapped builder to rebuild the packet. Note that you still
 * buildPacket() from the wrapped in builder.
 * TODO: The cloning builder only clones stream data that has not already been
 * resset or closed. It is possible that a packet may have contained data for
 * only closed streams. In that case we would only write out the header.
 * This is a waste, so we should do something about this in the future.
 */
class PacketRebuilder {
 public:
  PacketRebuilder(
      RegularQuicPacketBuilder& regularBuilder,
      QuicConnectionStateBase& conn);

  folly::Optional<PacketEvent> rebuildFromPacket(OutstandingPacket& packet);

  // TODO: Same as passing cipherOverhead into the CloningScheduler, this really
  // is a sad way to solve the writableBytes problem.
  uint64_t getHeaderBytes() const;

 private:
  /**
   * A helper function that takes a OutstandingPacket that's not processed, and
   * return its associatedEvent. If this packet has never been cloned, then
   * create the associatedEvent and add it into outstandingPacketEvents first.
   */
  PacketEvent cloneOutstandingPacket(OutstandingPacket& packet);

  bool retransmittable(const QuicStreamState& stream) const {
    return matchesStates<
        StreamStateData,
        StreamStates::Open,
        StreamStates::HalfClosedRemote>(stream.state);
  }

  Buf cloneCryptoRetransmissionBuffer(
      const WriteCryptoFrame& frame,
      const QuicCryptoStream& stream);

  Buf cloneRetransmissionBuffer(
      const WriteStreamFrame& frame,
      const QuicStreamState* stream);

 private:
  RegularQuicPacketBuilder& builder_;
  QuicConnectionStateBase& conn_;
};
} // namespace quic