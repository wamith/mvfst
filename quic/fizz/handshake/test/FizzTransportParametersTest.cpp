/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <fizz/record/test/ExtensionTestsBase.h>
#include <quic/common/test/TestUtils.h>
#include <quic/fizz/handshake/FizzTransportParameters.h>

using namespace fizz;
using namespace fizz::test;
using namespace folly;

namespace quic {
namespace test {

class QuicExtensionsTest : public testing::Test {
 protected:
  Buf getBuf(folly::StringPiece hex) {
    auto data = unhexlify(hex);
    return folly::IOBuf::copyBuffer(data.data(), data.size());
  }

  std::vector<Extension> getExtensions(folly::StringPiece hex) {
    auto buf = QuicExtensionsTest::getBuf(hex);
    folly::io::Cursor cursor(buf.get());
    Extension ext;
    EXPECT_EQ(fizz::detail::read(ext, cursor), buf->computeChainDataLength());
    EXPECT_TRUE(cursor.isAtEnd());
    std::vector<Extension> exts;
    exts.push_back(std::move(ext));
    return exts;
  }

  template <class T>
  void checkEncode(
      T&& ext,
      folly::StringPiece expectedHex,
      QuicVersion encodingVersion) {
    auto encoded = encodeExtension(std::forward<T>(ext), encodingVersion);
    auto buf = folly::IOBuf::create(0);
    folly::io::Appender appender(buf.get(), 10);
    fizz::detail::write(encoded, appender);
    EXPECT_TRUE(folly::IOBufEqualTo()(buf, getBuf(expectedHex)));
  }
};

StringPiece clientParamsD24{"ffa5000a0008000400049d7f3e7d"};
StringPiece clientParamsD27{"ffa5000604049d7f3e7d"};
StringPiece serverParamsD24{"ffa5000a0008000400049d7f3e7d"};
StringPiece serverParamsD27{"ffa5000604049d7f3e7d"};
StringPiece ticketParamsD24{"ffa5000a0008000400049d7f3e7d"};
StringPiece ticketParamsD27{"ffa5000604049d7f3e7d"};

TEST_F(QuicExtensionsTest, TestClientParamsD24) {
  auto exts = getExtensions(clientParamsD24);
  auto ext = getClientExtension(exts, QuicVersion::MVFST_D24);
  EXPECT_EQ(ext->parameters.size(), 1);
  EXPECT_EQ(
      ext->parameters[0].parameter, TransportParameterId::initial_max_data);
  EXPECT_EQ(
      *getIntegerParameter(
          TransportParameterId::initial_max_data, ext->parameters),
      494878333ULL);
  checkEncode(std::move(*ext), clientParamsD24, QuicVersion::MVFST_D24);
}

TEST_F(QuicExtensionsTest, TestClientParamsD27) {
  auto exts = getExtensions(clientParamsD27);
  auto ext = getClientExtension(exts, QuicVersion::QUIC_DRAFT);
  EXPECT_EQ(ext->parameters.size(), 1);
  EXPECT_EQ(
      ext->parameters[0].parameter, TransportParameterId::initial_max_data);
  EXPECT_EQ(
      *getIntegerParameter(
          TransportParameterId::initial_max_data, ext->parameters),
      494878333ULL);
  checkEncode(std::move(*ext), clientParamsD27, QuicVersion::QUIC_DRAFT);
}

TEST_F(QuicExtensionsTest, TestServerParamsD24) {
  auto exts = getExtensions(serverParamsD24);
  auto ext = getServerExtension(exts, QuicVersion::MVFST_D24);

  EXPECT_EQ(
      ext->parameters[0].parameter, TransportParameterId::initial_max_data);
  EXPECT_EQ(
      *getIntegerParameter(
          TransportParameterId::initial_max_data, ext->parameters),
      494878333ULL);
  checkEncode(std::move(*ext), serverParamsD24, QuicVersion::MVFST_D24);
}

TEST_F(QuicExtensionsTest, TestServerParamsD27) {
  auto exts = getExtensions(serverParamsD27);
  auto ext = getServerExtension(exts, QuicVersion::QUIC_DRAFT);

  EXPECT_EQ(
      ext->parameters[0].parameter, TransportParameterId::initial_max_data);
  EXPECT_EQ(
      *getIntegerParameter(
          TransportParameterId::initial_max_data, ext->parameters),
      494878333ULL);
  checkEncode(std::move(*ext), serverParamsD27, QuicVersion::QUIC_DRAFT);
}

TEST_F(QuicExtensionsTest, TestTicketParamsD24) {
  auto exts = getExtensions(ticketParamsD24);
  auto ext = getTicketExtension(exts, QuicVersion::MVFST_D24);

  EXPECT_EQ(ext->parameters.size(), 1);
  EXPECT_EQ(
      ext->parameters[0].parameter, TransportParameterId::initial_max_data);
  EXPECT_EQ(
      *getIntegerParameter(
          TransportParameterId::initial_max_data, ext->parameters),
      494878333ULL);
  checkEncode(std::move(*ext), ticketParamsD24, QuicVersion::MVFST_D24);
}

TEST_F(QuicExtensionsTest, TestTicketParamsD27) {
  auto exts = getExtensions(ticketParamsD27);
  auto ext = getTicketExtension(exts, QuicVersion::QUIC_DRAFT);

  EXPECT_EQ(ext->parameters.size(), 1);
  EXPECT_EQ(
      ext->parameters[0].parameter, TransportParameterId::initial_max_data);
  EXPECT_EQ(
      *getIntegerParameter(
          TransportParameterId::initial_max_data, ext->parameters),
      494878333ULL);
  checkEncode(std::move(*ext), ticketParamsD27, QuicVersion::QUIC_DRAFT);
}

} // namespace test
} // namespace quic
