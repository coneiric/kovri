/**
 * Copyright (c) 2015-2018, The Kovri I2P Router Project
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tests/unit_tests/main.h"

#include "core/router/transports/ssu/session.h"
#include "core/util/byte_stream.h"

#include "tests/unit_tests/core/router/transports/ssu/packet.h"

/**
 *
 * Global fixtures
 *
 */

struct SSUSessionFixture : public SSUTestVectorsFixture
{
  SSUSessionFixture() : session_packet(nullptr) {}

  void BuildPacket(
      const std::vector<std::uint8_t>& head_buf,
      std::vector<std::uint8_t>& pkt_buf)
  {
    std::uint16_t body_len = head_buf.size() + pkt_buf.size()
                            - (core::SSUSize::MAC + core::SSUSize::IV);

    std::uint8_t const pad_len = body_len % 16 ? 16 - (body_len % 16) : 0;

    if (pad_len)
      {
        auto it = pkt_buf.end();

        // TODO(unassigned): it is a hack to assume signature length is 64 (EdDSA).
        //   In actual impl, signature length can be read from remote Identity.
        if (head_buf[32] >> 4 & core::SSUPayloadType::SessionConfirmed)
          it -= 64;

        pkt_buf.insert(it, pad_len, 0x00);
      }

    packet_arr.fill(0x00);
    std::copy(head_buf.begin(), head_buf.end(), packet_arr.begin());
    std::copy(
        pkt_buf.begin(), pkt_buf.end(), packet_arr.begin() + head_buf.size());

    if (session_packet)
      session_packet.reset(nullptr);

    session_packet = std::make_unique<core::SSUSessionPacket>(
        packet_arr.data(), head_buf.size() + pkt_buf.size());
  }

  core::SSUSessionPacket& get_session_packet() const
  {
    if (!session_packet)
      throw std::runtime_error(__func__ + std::string(": null session packet"));
    return *session_packet;
  }

  std::array<std::uint8_t, core::SSUSize::IntroKey> aes_key {{
    0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13,
    0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13,
    0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13,
    0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13
  }};

  std::array<std::uint8_t, core::SSUSize::IntroKey> mac_key {{
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42
  }};

  std::array<std::uint8_t, core::SSUSize::MTUv6> packet_arr;  ///< packet data buffer
  std::unique_ptr<core::SSUSessionPacket> session_packet;
};

BOOST_FIXTURE_TEST_SUITE(SSUSessionPacketTests, SSUSessionFixture)

BOOST_AUTO_TEST_CASE(GoodSSUSessionPacketBuild)
{
  std::vector<std::uint8_t> packet{session_request.begin(),
                                   session_request.end()};

  BOOST_CHECK_NO_THROW(
      BuildPacket({header_plain.begin(), header_plain.end()}, packet));

  BOOST_CHECK_NO_THROW(get_session_packet());
}

BOOST_AUTO_TEST_CASE(BadSSUSessionPacketBuild)
{
  std::vector<std::uint8_t> too_big(core::SSUSize::MTUv6);

  BOOST_CHECK_THROW(
      BuildPacket({header_plain.begin(), header_plain.end()}, too_big),
      std::exception);
}

BOOST_AUTO_TEST_CASE(GoodSessionValidation)
{
  std::vector<std::uint8_t> packet{session_request.begin(),
                                   session_request.end()};

  BOOST_CHECK_NO_THROW(
      BuildPacket({header_plain.begin(), header_plain.end()}, packet));

  BOOST_CHECK_NO_THROW(
      session_packet->CalculateMAC(mac_key.data(), session_packet->get_mac()));

  BOOST_CHECK(session_packet->Validate(mac_key.data()));
}

BOOST_AUTO_TEST_CASE(GoodSessionCrypto)
{
  std::vector<std::uint8_t> packet{session_request.begin(),
                                   session_request.end()};

  BOOST_CHECK_NO_THROW(
      BuildPacket({header_plain.begin(), header_plain.end()}, packet));

  BOOST_CHECK_NO_THROW(session_packet->Encrypt(aes_key.data()));
  BOOST_CHECK_NO_THROW(session_packet->Decrypt(aes_key.data()));

  packet = std::vector<std::uint8_t>(header_plain.begin() + 32, header_plain.end());
  packet.insert(packet.end(), session_request.begin(), session_request.end());

  BOOST_CHECK_EQUAL_COLLECTIONS(
      session_packet->body,
      session_packet->body + packet.size(),
      packet.begin(),
      packet.end());
}

BOOST_AUTO_TEST_CASE(BadSessionCrypto)
{
  std::vector<std::uint8_t> packet{session_request.begin(),
                                   session_request.end()};

  BOOST_CHECK_NO_THROW(
      BuildPacket({header_plain.begin(), header_plain.end()}, packet));

  // Invalid packet length / padding
  session_packet->data_len--;

  BOOST_CHECK_THROW(session_packet->Encrypt(aes_key.data()), std::exception);
  BOOST_CHECK_THROW(session_packet->Decrypt(aes_key.data()), std::exception);

  // Invalid key length
  std::vector<std::uint8_t> bad_key(aes_key.begin(), aes_key.end());
  bad_key.pop_back();

  BOOST_CHECK_THROW(session_packet->Encrypt(bad_key.data()), std::exception);
  BOOST_CHECK_THROW(session_packet->Decrypt(bad_key.data()), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()
