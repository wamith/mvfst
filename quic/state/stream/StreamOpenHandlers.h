/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// override-include-guard

#include <quic/state/QuicStreamUtilities.h>

namespace quic {
template <typename Event>
void invokeStreamSendStateMachine(
    QuicConnectionStateBase&,
    QuicStreamState& stream,
    Event event);
inline void
Handler<StreamReceiveStateMachine, StreamReceiveStates::Open, ReadStreamFrame>::
    handle(
        QuicStreamState::Recv& state,
        ReadStreamFrame frame,
        QuicStreamState& stream) {
  VLOG_IF(10, frame.fin) << "Open: Received data with fin"
                         << " stream=" << stream.id << " " << stream.conn;
  appendDataToReadBuffer(
      stream, StreamBuffer(std::move(frame.data), frame.offset, frame.fin));
  if (isAllDataReceived(stream)) {
    VLOG(10) << "Open: Transition to Closed"
             << " stream=" << stream.id << " " << stream.conn;
    transit<StreamReceiveStates::Closed>(state);
    if (stream.inTerminalStates()) {
      stream.conn.streamManager->addClosed(stream.id);
    }
  }
  stream.conn.streamManager->updateReadableStreams(stream);
}

inline void
Handler<StreamSendStateMachine, StreamSendStates::Open, StopSendingFrame>::
    handle(
        QuicStreamState::Send& state,
        StopSendingFrame frame,
        QuicStreamState& stream) {
  CHECK(
      isBidirectionalStream(stream.id) ||
      isSendingStream(stream.conn.nodeType, stream.id));
  stream.conn.streamManager->addStopSending(stream.id, frame.errorCode);
}

inline void
Handler<StreamReceiveStateMachine, StreamReceiveStates::Open, RstStreamFrame>::
    handle(
        QuicStreamState::Recv& state,
        RstStreamFrame rst,
        QuicStreamState& stream) {
  if (matchesStates<StreamSendStateData, StreamSendStates::Open>(
          stream.send.state)) {
    // TODO: remove.
    invokeStreamSendStateMachine(
        stream.conn,
        stream,
        StreamEvents::SendReset(GenericApplicationErrorCode::NO_ERROR));
  }
  // We transit the receive state machine to Closed before invoking
  // onResetQuicStream because it will check the state of the stream for flow
  // control.
  transit<StreamReceiveStates::Closed>(state);
  if (stream.inTerminalStates()) {
    stream.conn.streamManager->addClosed(stream.id);
  }
  onResetQuicStream(stream, std::move(rst));
}

inline void Handler<
    StreamSendStateMachine,
    StreamSendStates::Open,
    StreamEvents::SendReset>::
    handle(
        QuicStreamState::Send& state,
        StreamEvents::SendReset rst,
        QuicStreamState& stream) {
  resetQuicStream(stream, rst.errorCode);
  appendPendingStreamReset(stream.conn, stream, rst.errorCode);
  // Move the state machine:
  transit<StreamSendStates::ResetSent>(state);
}

inline void Handler<
    StreamSendStateMachine,
    StreamSendStates::Open,
    StreamEvents::AckStreamFrame>::
    handle(
        QuicStreamState::Send& state,
        StreamEvents::AckStreamFrame ack,
        QuicStreamState& stream) {
  // Clean up the acked buffers from the retransmissionBuffer.

  auto ackedBuffer = std::lower_bound(
      stream.retransmissionBuffer.begin(),
      stream.retransmissionBuffer.end(),
      ack.ackedFrame.offset,
      [](const auto& buffer, const auto& offset) {
        return buffer.offset < offset;
      });

  if (ackedBuffer != stream.retransmissionBuffer.end()) {
    // Since the StreamFrames that are ACKed are computed from the outstanding
    // packets, we always know that the retransmission buffer corresponds to
    // 1 buffer in the retranmission buffer.
    DCHECK_EQ(ackedBuffer->offset, ack.ackedFrame.offset);

    DCHECK_EQ(ackedBuffer->data.chainLength(), ack.ackedFrame.len);
    DCHECK_EQ(ackedBuffer->eof, ack.ackedFrame.fin);

    VLOG(10) << "Open: acked stream data stream=" << stream.id
             << " offset=" << ackedBuffer->offset
             << " len=" << ackedBuffer->data.chainLength()
             << " eof=" << ackedBuffer->eof << " " << stream.conn;
    stream.retransmissionBuffer.erase(ackedBuffer);
  }

  // This stream may be able to invoke some deliveryCallbacks:
  stream.conn.streamManager->addDeliverable(stream.id);

  // Check for whether or not we have ACKed all bytes until our FIN.
  if (allBytesTillFinAcked(stream)) {
    transit<StreamSendStates::Closed>(state);
    if (stream.inTerminalStates()) {
      stream.conn.streamManager->addClosed(stream.id);
    }
  }
}
} // namespace quic
