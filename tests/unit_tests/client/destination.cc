/**                                                                                           //
 * Copyright (c) 2015-2018, The Kovri I2P Router Project                                      //
 *                                                                                            //
 * All rights reserved.                                                                       //
 *                                                                                            //
 * Redistribution and use in source and binary forms, with or without modification, are       //
 * permitted provided that the following conditions are met:                                  //
 *                                                                                            //
 * 1. Redistributions of source code must retain the above copyright notice, this list of     //
 *    conditions and the following disclaimer.                                                //
 *                                                                                            //
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list     //
 *    of conditions and the following disclaimer in the documentation and/or other            //
 *    materials provided with the distribution.                                               //
 *                                                                                            //
 * 3. Neither the name of the copyright holder nor the names of its contributors may be       //
 *    used to endorse or promote products derived from this software without specific         //
 *    prior written permission.                                                               //
 *                                                                                            //
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY        //
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF    //
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL     //
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,       //
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,               //
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS    //
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,          //
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF    //
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.               //
 */

#include "tests/unit_tests/main.h"

#include "client/destination.h"

struct StubDestination : public client::ClientDestination
{
  StubDestination(
      const core::PrivateKeys& keys,
      bool is_pub,
      const std::map<std::string, std::string>* param = nullptr)
      : client::ClientDestination(keys, is_pub, param)
  {
  }

  void Start()
  {
    client::ClientDestination::Start();
  }

  void Stop()
  {
    client::ClientDestination::Stop();
  }
};

struct DestinationFixture
{
  DestinationFixture()
  {
    using param_map = const std::map<std::string, std::string>;

    // Add explicit peer to NetDb
    AddFloodfill();

    // Construct w/ params
    std::unique_ptr<param_map> prm(new param_map{
        {client::I2CP_PARAM_INBOUND_TUNNEL_LENGTH, "1"},
        {client::I2CP_PARAM_OUTBOUND_TUNNEL_LENGTH, "1"},
        {client::I2CP_PARAM_INBOUND_TUNNELS_QUANTITY, "1"},
        {client::I2CP_PARAM_OUTBOUND_TUNNELS_QUANTITY, "1"},
        {client::I2CP_PARAM_EXPLICIT_PEERS, ri->GetIdentHash().ToBase64()}});

    auto const keys = core::PrivateKeys::CreateRandomKeys();
    dest = std::make_unique<StubDestination>(keys, true, prm.get());
  }

  /// @brief Add stub floodfill router to netdb
  void AddFloodfill()
  {
    using points = std::vector<std::pair<std::string, std::uint16_t>>;

    auto const keys = core::PrivateKeys::CreateRandomKeys(
        core::DEFAULT_ROUTER_SIGNING_KEY_TYPE);
    auto const caps =
        core::RouterInfo::Cap::Reachable | core::RouterInfo::Cap::Floodfill;

    ri = std::make_shared<const core::RouterInfo>(
        keys, points{{"::1", 0}}, std::make_pair(true, true), caps);
    core::netdb.AddRouterInfo(ri->data(), ri->size());
  }

  /// @brief Add stub tunnels to tunnel pool
  void AddTunnels()
  {
    if (!ri)
      AddFloodfill();

    dest->GetTunnelPool()->SetActive(true);
    std::vector<std::shared_ptr<const core::RouterInfo>> peers{{ri}};

    // Create inbound tunnel
    auto const in_cfg =
        std::make_shared<const core::TunnelConfig>(peers, nullptr);
    auto in_tun = std::make_shared<core::InboundTunnel>(in_cfg);
    in_tun->SetState(core::TunnelState::e_TunnelStateEstablished);

    // Create outbound tunnel
    auto const out_cfg = std::make_shared<core::TunnelConfig>(peers, in_cfg);
    auto out_tun = std::make_shared<core::OutboundTunnel>(out_cfg);

    // Insert tunnels into the tunnel pool
    dest->GetTunnelPool()->TunnelCreated(in_tun);
    dest->GetTunnelPool()->TunnelCreated(out_tun);
  }

  /// @brief Create an I2CP data message
  /// @param type Protocol type for the message
  std::shared_ptr<core::I2NPMessage> CreateDataMessage(std::uint8_t type)
  {
    std::array<std::uint8_t, 14> const pay {{
      // size (uint32)
      0x00, 0x00, 0x00, 0x0a,
      // unused
      0xFF, 0xFF, 0xFF, 0xFF,
      // source port
      0x00, 0x00,
      // dest port
      0x00, 0x00,
      // unused
      0xFF,
      // protocol type
      type
    }};

    auto msg = core::ToSharedI2NPMessage(core::NewI2NPShortMessage());
    std::copy(pay.begin(), pay.end(), msg->GetPayload());
    msg->FillI2NPMessageHeader(core::I2NPData);
    return msg;
  }

  std::unique_ptr<StubDestination> dest;
  std::shared_ptr<const core::RouterInfo> ri;
};

BOOST_FIXTURE_TEST_SUITE(ClientDestinationTests, DestinationFixture)

BOOST_AUTO_TEST_CASE(DestinationSetupTeardown)
{
  BOOST_CHECK_NO_THROW(dest->Start());
  BOOST_CHECK(dest->IsRunning());
  BOOST_CHECK_NO_THROW(dest->Stop());
}

BOOST_AUTO_TEST_CASE(DestinationLeaseSet)
{
  // Add stub tunnels to tunnel pool
  AddTunnels();

  // Set LeaseSet updated
  BOOST_CHECK_NO_THROW(dest->SetLeaseSetUpdated());

  auto const dls = dest->GetLeaseSet();

  // Check LeaseSet is valid
  BOOST_CHECK(dls && dls->IsValid());

  // Store remote LeaseSet
  auto const dmsg = core::CreateDatabaseStoreMsg(dls, 0x42);
  BOOST_CHECK(dmsg);
  BOOST_CHECK_NO_THROW(
      dest->HandleI2NPMessage(dmsg->GetBuffer(), dmsg->GetLength(), nullptr));

  // Find stored LeaseSet, and check it matches our destination LeaseSet
  std::shared_ptr<const core::LeaseSet> ls;
  BOOST_CHECK_NO_THROW(ls = dest->FindLeaseSet(dls->GetIdentHash()));
  BOOST_CHECK(ls && ls->GetIdentHash() == dls->GetIdentHash());

  // Request destination
  BOOST_CHECK(dest->RequestDestination(
      ls->GetIdentHash(), [](std::shared_ptr<core::LeaseSet>) {}));

  // Run the IO service to fire bound handlers
  dest->GetService().poll();
  std::this_thread::sleep_for(std::chrono::microseconds(10));
}

BOOST_AUTO_TEST_CASE(DestinationStreams)
{
  std::shared_ptr<client::StreamingDestination> sd;
  auto const acc = [](std::shared_ptr<client::Stream>){};

  // Create streams
  BOOST_CHECK_NO_THROW(
      dest->CreateStream(acc, dest->GetLeaseSet()->GetIdentHash()));
  BOOST_CHECK(dest->CreateStream(dest->GetLeaseSet()));

  // Create streaming destination
  BOOST_CHECK_NO_THROW(sd = dest->CreateStreamingDestination(0));
  BOOST_CHECK(sd == dest->GetStreamingDestination());

  // Accept streams
  BOOST_CHECK_NO_THROW(dest->AcceptStreams(acc));
  BOOST_CHECK(dest->IsAcceptingStreams());
  BOOST_CHECK_NO_THROW(dest->StopAcceptingStreams());
}

BOOST_AUTO_TEST_CASE(DestinationSessionKey)
{
  // Session key and tag
  std::array<std::uint8_t, 32> const k{}, t{};
  BOOST_CHECK_NO_THROW(dest->SubmitSessionKey(k.data(), t.data()));
}

BOOST_AUTO_TEST_CASE(DestinationMessages)
{
  // Create streaming & datagram dests to handle I2NP data messages
  BOOST_CHECK(dest->CreateStreamingDestination(0));
  BOOST_CHECK(dest->CreateDatagramDestination());
  BOOST_CHECK(dest->GetDatagramDestination());

  // Create I2NP message w/ streaming I2CP payload
  auto data = CreateDataMessage(client::PROTOCOL_TYPE_STREAMING);
  BOOST_CHECK_NO_THROW(dest->HandleI2NPMessage(data->GetBuffer(), 0, nullptr));

  // Create I2NP message w/ datagram I2CP payload
  data = CreateDataMessage(client::PROTOCOL_TYPE_DATAGRAM);
  BOOST_CHECK_NO_THROW(dest->HandleI2NPMessage(data->GetBuffer(), 0, nullptr));

  // Create and process garlic message
  auto const gmsg = dest->WrapMessage(dest->GetLeaseSet(), data, {});
  BOOST_CHECK_NO_THROW(dest->ProcessGarlicMessage(gmsg));

  // Handle delivery status message
  auto const stat = core::CreateDeliveryStatusMsg(0);
  BOOST_CHECK_NO_THROW(dest->HandleI2NPMessage(stat->GetBuffer(), 0, nullptr));
  BOOST_CHECK_NO_THROW(dest->ProcessDeliveryStatusMessage(stat));

  // Handle database search reply message
  auto const srch =
      core::CreateDatabaseSearchReply(dest->GetIdentity().GetIdentHash(), {});
  BOOST_CHECK_NO_THROW(dest->HandleI2NPMessage(srch->GetBuffer(), 0, nullptr));

  // Handle default short I2NP message
  auto const shrt = core::NewI2NPShortMessage();
  BOOST_CHECK_NO_THROW(dest->HandleI2NPMessage(shrt->GetBuffer(), 0, nullptr));
}

BOOST_AUTO_TEST_SUITE_END()
