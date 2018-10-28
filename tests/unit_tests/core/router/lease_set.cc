/**                                                                                           //
 * Copyright (c) 2013-2018, The Kovri I2P Router Project                                      //
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

#include "src/core/router/lease_set.h"

struct LeaseSetFixture
{
  LeaseSetFixture() : m_LocalDest(core::PrivateKeys::CreateRandomKeys(), true)
  {
    constexpr std::uint8_t num_leases = 2;
    std::vector<core::Lease> leases;

    for (std::uint8_t n = 0; n < num_leases; ++n)
      leases.emplace_back(
          core::Lease(m_LocalDest.GetIdentHash(), core::Rand<std::uint32_t>()));

    m_LeaseSet = std::make_unique<core::LeaseSet>(m_LocalDest, leases);
  }

  client::ClientDestination m_LocalDest;
  std::unique_ptr<core::LeaseSet> m_LeaseSet;
};


BOOST_FIXTURE_TEST_SUITE(LeaseSetTests, LeaseSetFixture)

BOOST_AUTO_TEST_CASE(ValidatesSignedLeaseSet)
{
  BOOST_CHECK(m_LeaseSet->IsValid());
}

BOOST_AUTO_TEST_CASE(CreatesValidLeaseSetFromBuffer)
{
  std::unique_ptr<core::LeaseSet> buffer_lease_set;

  BOOST_CHECK(m_LeaseSet->IsValid());
  BOOST_CHECK_NO_THROW(
      buffer_lease_set = std::make_unique<core::LeaseSet>(
          m_LeaseSet->GetBuffer(), m_LeaseSet->GetBufferLen()));
  BOOST_CHECK(buffer_lease_set->IsValid());
}

BOOST_AUTO_TEST_CASE(HasDestination)
{
  core::IdentityEx ident;

  BOOST_CHECK(
      ident.FromBuffer(m_LeaseSet->GetBuffer(), m_LeaseSet->GetBufferLen()));
  BOOST_CHECK_EQUAL(m_LeaseSet->GetIdentHash(), ident.GetIdentHash());
}

BOOST_AUTO_TEST_CASE(HasEncryptionKey)
{
  const std::uint8_t* crypto_pubkey = m_LeaseSet->GetEncryptionPublicKey();
  const std::uint8_t* buf_crypto_pubkey =
      m_LeaseSet->GetBuffer() + m_LeaseSet->GetIdentity().GetFullLen();

  BOOST_CHECK(crypto_pubkey);
  BOOST_CHECK_EQUAL_COLLECTIONS(
      crypto_pubkey,
      crypto_pubkey + crypto::PkLen::ElGamal,
      buf_crypto_pubkey,
      buf_crypto_pubkey + crypto::PkLen::ElGamal);
}

BOOST_AUTO_TEST_CASE(HasNullSigningKey)
{
  const auto& ident = m_LeaseSet->GetIdentity();
  const auto sign_key_len = ident.GetSigningPublicKeyLen();
  const std::vector<std::uint8_t> null_key(sign_key_len);
  const std::uint8_t* sign_key =
      m_LeaseSet->GetBuffer() + ident.GetFullLen() + crypto::PkLen::ElGamal;

  BOOST_CHECK_EQUAL_COLLECTIONS(
      sign_key, sign_key + sign_key_len, null_key.begin(), null_key.end());
}

BOOST_AUTO_TEST_CASE(HasNumLeases)
{
  const auto& ident = m_LeaseSet->GetIdentity();
  const std::uint8_t* num_leases = m_LeaseSet->GetBuffer() + ident.GetFullLen()
                                   + crypto::PkLen::ElGamal
                                   + ident.GetSigningPublicKeyLen();
  BOOST_CHECK(num_leases);
  BOOST_CHECK_EQUAL(*num_leases, m_LeaseSet->GetNumLeases());
  BOOST_CHECK_EQUAL(m_LeaseSet->GetNumLeases(), m_LeaseSet->GetLeases().size());
}

BOOST_AUTO_TEST_CASE(HasValidLeases)
{
  const auto& ident_hash = m_LeaseSet->GetIdentHash();

  for (const auto& lease : m_LeaseSet->GetLeases())
    {
      BOOST_CHECK_EQUAL(lease.tunnel_gateway, ident_hash);

      BOOST_CHECK(sizeof(lease.tunnel_ID) == core::LeaseSetSize::TunnelID);
      BOOST_CHECK(sizeof(lease.end_date) == core::LeaseSetSize::EndDate);
    }
}

BOOST_AUTO_TEST_CASE(AllowsNullLeases)
{
  constexpr std::uint8_t num_leases = 0;

  BOOST_CHECK_NO_THROW(
      m_LeaseSet = std::make_unique<core::LeaseSet>(
          m_LocalDest, std::vector<core::Lease>{}));
  BOOST_CHECK(m_LeaseSet->IsValid());
  BOOST_CHECK_EQUAL(m_LeaseSet->GetNumLeases(), num_leases);
  BOOST_CHECK_EQUAL(m_LeaseSet->GetLeases().size(), num_leases);
}

BOOST_AUTO_TEST_CASE(HasSignature)
{
  const auto& ident = m_LeaseSet->GetIdentity();
  const std::uint8_t* signature =
      m_LeaseSet->GetBuffer() + ident.GetFullLen() + crypto::PkLen::ElGamal
      + ident.GetSigningPublicKeyLen()
      + m_LeaseSet->GetNumLeases() * core::LeaseSetSize::LeaseSize;
  const auto sig_len = ident.GetSignatureLen();

  BOOST_CHECK(signature);
  BOOST_CHECK(m_LeaseSet->GetSignature());
  BOOST_CHECK_EQUAL_COLLECTIONS(
      signature,
      signature + sig_len,
      m_LeaseSet->GetSignature(),
      m_LeaseSet->GetSignature() + sig_len);
}

BOOST_AUTO_TEST_CASE(RejectsTooManyLeases)
{
  constexpr std::uint8_t num_leases = core::LeaseSetSize::MaxLeases + 1;

  BOOST_CHECK_THROW(
      core::LeaseSet(m_LocalDest, std::vector<core::Lease>(num_leases)),
      std::exception);
}

BOOST_AUTO_TEST_CASE(RejectsInvalidSignature)
{
  const std::uint8_t* lease_set_buf = m_LeaseSet->GetBuffer();
  const auto lease_set_len = m_LeaseSet->GetBufferLen();
  auto lease_set_raw = std::make_unique<std::uint8_t[]>(lease_set_len);

  // Copy valid lease set
  std::copy(lease_set_buf, lease_set_buf + lease_set_len, lease_set_raw.get());

  // Invalidate the signature
  lease_set_raw[lease_set_len - 1] = 0x42;

  core::LeaseSet buf_lease_set(lease_set_raw.get(), lease_set_len);
  m_LeaseSet->Update(lease_set_raw.get(), lease_set_len);

  BOOST_CHECK_EQUAL(buf_lease_set.IsValid(), false);
  BOOST_CHECK_EQUAL(m_LeaseSet->IsValid(), false);
}

BOOST_AUTO_TEST_SUITE_END()
