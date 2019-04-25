/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// Copyright 2004-present Facebook.  All rights reserved.

#include <folly/Expected.h>
#include <folly/Optional.h>
#include <folly/String.h>
#include <folly/io/IOBuf.h>
#include <folly/portability/GTest.h>

#include <quic/QuicException.h>
#include <quic/codec/QuicInteger.h>

using namespace quic;
using namespace testing;
using namespace folly;

namespace quic {
namespace test {

struct IntegerParams {
  uint64_t decoded;
  std::string hexEncoded;
  bool error{false};
  uint8_t encodedLength{8};
};

class QuicIntegerDecodeTest : public TestWithParam<IntegerParams> {};
class QuicIntegerEncodeTest : public TestWithParam<IntegerParams> {};

TEST_P(QuicIntegerDecodeTest, DecodeTrim) {
  std::string encodedBytes = folly::unhexlify(GetParam().hexEncoded);

  for (int atMost = 0; atMost <= GetParam().encodedLength; atMost++) {
    auto wrappedEncoded = IOBuf::copyBuffer(encodedBytes);
    wrappedEncoded->trimEnd(std::min(
        wrappedEncoded->computeChainDataLength(),
        (unsigned long)(GetParam().encodedLength - atMost)));
    folly::io::Cursor cursor(wrappedEncoded.get());
    auto originalLength = cursor.length();
    auto decodedValue = decodeQuicInteger(cursor);
    if (GetParam().error || atMost != GetParam().encodedLength) {
      EXPECT_FALSE(decodedValue.hasValue());
      EXPECT_EQ(cursor.length(), originalLength);
    } else {
      EXPECT_EQ(decodedValue->first, GetParam().decoded);
      EXPECT_EQ(decodedValue->second, GetParam().encodedLength);
      EXPECT_EQ(cursor.length(), originalLength - GetParam().encodedLength);
    }
  }
}

TEST_P(QuicIntegerDecodeTest, DecodeAtMost) {
  std::string encodedBytes = folly::unhexlify(GetParam().hexEncoded);
  auto wrappedEncoded = IOBuf::copyBuffer(encodedBytes);

  for (int atMost = 0; atMost <= GetParam().encodedLength; atMost++) {
    folly::io::Cursor cursor(wrappedEncoded.get());
    auto originalLength = cursor.length();
    auto decodedValue = decodeQuicInteger(cursor, atMost);
    if (GetParam().error || atMost != GetParam().encodedLength) {
      EXPECT_FALSE(decodedValue.hasValue());
      EXPECT_EQ(cursor.length(), originalLength);
    } else {
      EXPECT_EQ(decodedValue->first, GetParam().decoded);
      EXPECT_EQ(decodedValue->second, GetParam().encodedLength);
      EXPECT_EQ(cursor.length(), originalLength - GetParam().encodedLength);
    }
  }
}

TEST_P(QuicIntegerEncodeTest, Encode) {
  IOBufQueue queue;
  folly::io::QueueAppender appender(&queue, 10);
  if (GetParam().error) {
    auto size = encodeQuicInteger(GetParam().decoded, appender);
    EXPECT_TRUE(size.hasError());
    EXPECT_EQ(size.error(), TransportErrorCode::INTERNAL_ERROR);
    return;
  }
  auto written = encodeQuicInteger(GetParam().decoded, appender);
  auto encodedValue =
      folly::hexlify(queue.move()->moveToFbString().toStdString());
  LOG(INFO) << "encoded=" << encodedValue;
  LOG(INFO) << "expected=" << GetParam().hexEncoded;

  EXPECT_EQ(encodedValue, GetParam().hexEncoded);
  EXPECT_EQ(*written, encodedValue.size() / 2);
}

TEST_P(QuicIntegerEncodeTest, GetSize) {
  auto size = getQuicIntegerSize(GetParam().decoded);
  if (GetParam().error) {
    EXPECT_TRUE(size.hasError());
    EXPECT_EQ(size.error(), TransportErrorCode::INTERNAL_ERROR);
    return;
  }
  EXPECT_EQ(*size, GetParam().hexEncoded.size() / 2);
}

INSTANTIATE_TEST_CASE_P(
    QuicIntegerTests,
    QuicIntegerDecodeTest,
    Values(
        (IntegerParams){151288809941952652, "c2197c5eff14e88c", false, 8},
        (IntegerParams){494878333, "9d7f3e7d", false, 4},
        (IntegerParams){15293, "7bbd", false, 2},
        (IntegerParams){37, "25", false, 1},
        (IntegerParams){37, "4025", false, 2},
        (IntegerParams){37, "80000025", false, 4},
        (IntegerParams){37, "C000000000000025", false, 8},
        (IntegerParams){37, "40", true}));

INSTANTIATE_TEST_CASE_P(
    QuicIntegerEncodeTests,
    QuicIntegerEncodeTest,
    Values(
        (IntegerParams){151288809941952652, "c2197c5eff14e88c", false},
        (IntegerParams){494878333, "9d7f3e7d", false},
        (IntegerParams){15293, "7bbd", false},
        (IntegerParams){37, "25", false},
        (IntegerParams){std::numeric_limits<uint64_t>::max(), "25", true}));

} // namespace test
} // namespace quic