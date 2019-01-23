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
 *                                                                                            //
 * Parts of the project are originally copyright (c) 2013-2015 The PurpleI2P Project          //
 */

#include "core/router/lease_set.h"

#include <cstring>

#include "core/crypto/rand.h"

#include "core/router/net_db/impl.h"
#include "core/router/tunnel/pool.h"

#include "core/util/log.h"
#include "core/util/timestamp.h"

namespace kovri {
namespace core {

LeaseSet::LeaseSet(
    const std::uint8_t* buf,
    std::size_t len)
    : m_IsValid(true) {
  m_Buffer = std::make_unique<std::uint8_t[]>(len);
  memcpy(m_Buffer.get(), buf, len);
  m_BufferLen = len;
  ReadFromBuffer();
}

LeaseSet::LeaseSet(const kovri::core::TunnelPool& pool) : m_IsValid(true)
{
  // header
  const kovri::core::LocalDestination* local_destination = pool.GetLocalDestination();
  if (!local_destination) {
    m_Buffer.reset(nullptr);
    m_BufferLen = 0;
    m_IsValid = false;
    LOG(error) << "LeaseSet: destination for local LeaseSet doesn't exist";
    return;
  }
  m_Buffer = std::make_unique<std::uint8_t[]>(LeaseSetSize::MaxBuffer);
  m_BufferLen = local_destination->GetIdentity().ToBuffer(
      m_Buffer.get(), LeaseSetSize::MaxBuffer);
  memcpy(
      m_Buffer.get() + m_BufferLen,
      local_destination->GetEncryptionPublicKey(),
      crypto::PkLen::ElGamal);
  m_BufferLen += crypto::PkLen::ElGamal;
  auto signing_key_len = local_destination->GetIdentity().GetSigningPublicKeyLen();
  memset(m_Buffer.get() + m_BufferLen, 0, signing_key_len);
  m_BufferLen += signing_key_len;
  auto tunnels = pool.GetInboundTunnels(5);  // 5 tunnels maximum
  m_Buffer[m_BufferLen] = tunnels.size();  // num leases
  m_BufferLen++;
  // leases
  for (auto it : tunnels)
    {
      memcpy(
          m_Buffer.get() + m_BufferLen,
          it->GetNextIdentHash(),
          LeaseSetSize::GatewayID);
      m_BufferLen += LeaseSetSize::GatewayID;
      core::OutputByteStream::Write<std::uint32_t>(
          m_Buffer.get() + m_BufferLen, it->GetNextTunnelID());
      m_BufferLen += LeaseSetSize::TunnelID;
      // 1 minute before expiration
      std::uint64_t ts = it->GetCreationTime()
                         + kovri::core::TUNNEL_EXPIRATION_TIMEOUT
                         - kovri::core::TUNNEL_EXPIRATION_THRESHOLD;
      ts *= 1000;  // in milliseconds
      ts += kovri::core::RandInRange32(0, 5);  // + random milliseconds
      core::OutputByteStream::Write<std::uint64_t>(
          m_Buffer.get() + m_BufferLen, ts);
      m_BufferLen += LeaseSetSize::EndDate;
    }
  // signature
  local_destination->Sign(
      m_Buffer.get(),
      m_BufferLen,
      m_Buffer.get() + m_BufferLen);
  m_BufferLen += local_destination->GetIdentity().GetSignatureLen();
  LOG(debug)
    << "LeaseSet: local LeaseSet of " << tunnels.size() << " leases created";
  ReadFromBuffer();
}

LeaseSet::LeaseSet(
    const core::LocalDestination& local,
    const std::vector<Lease>& leases)
    : m_Buffer(new std::uint8_t[LeaseSetSize::MaxBuffer])
{
    // Check if leases exceed the limit, see spec
    if (leases.size() > LeaseSetSize::MaxLeases)
      throw std::invalid_argument(std::string(__func__) + ": too many leases");

    // Prepare the LeaseSet
    m_Leases = leases;
    m_Identity = local.GetIdentity();

    // Copy destination identity to buffer
    core::OutputByteStream stream(m_Buffer.get(), LeaseSetSize::MaxBuffer);
    stream.SkipBytes(m_Identity.GetFullLen());
    m_Identity.ToBuffer(m_Buffer.get(), LeaseSetSize::MaxBuffer);

    const std::uint8_t* crypto_pubkey = local.GetEncryptionPublicKey();

    // Copy destination encryption public key to data member
    std::copy(
        crypto_pubkey,
        crypto_pubkey + crypto::PkLen::ElGamal,
        m_EncryptionKey.data());

    // Copy destination encryption public key to buffer
    stream.WriteData(m_EncryptionKey.data(), m_EncryptionKey.size());

    // Zero-out unused signing key
    const std::vector<std::uint8_t> empty_sign_key(
        m_Identity.GetSigningPublicKeyLen());
    stream.WriteData(empty_sign_key.data(), empty_sign_key.size());

    // Set the number of leases
    stream.Write<std::uint8_t>(m_Leases.size());

    for (const auto lease : m_Leases)
      {
        // Copy the lease's gateway id to the buffer
        stream.WriteData(lease.tunnel_gateway(), LeaseSetSize::GatewayID);

        // Copy the lease's tunnel id to the buffer
        stream.Write<std::uint32_t>(lease.tunnel_ID);

        // Copy the lease's expiration date to the buffer
        stream.Write<std::uint64_t>(lease.end_date);
      }

    // Sign the LeaseSet
    const std::size_t bytes_written = stream.size() - stream.gcount();
    std::uint8_t* signature = stream.data() + bytes_written;
    local.Sign(stream.data(), bytes_written, signature);

    // Verify LeaseSet signature
    if (!m_Identity.Verify(stream.data(), bytes_written, signature))
      throw std::runtime_error(std::string(__func__) + ": invalid signature");

    m_BufferLen = bytes_written + m_Identity.GetSignatureLen();
    m_IsValid = true;
}

void LeaseSet::Update(
    const std::uint8_t* buf,
    std::size_t len) {
  m_Leases.clear();
  if (len > m_BufferLen) {
    m_Buffer = std::make_unique<std::uint8_t[]>(len);
  }
  memcpy(m_Buffer.get(), buf, len);
  m_BufferLen = len;
  ReadFromBuffer();
}

void LeaseSet::ReadFromBuffer()
{
  const auto set_invalid = [this](const char* err_msg){
    LOG(error) << "LeaseSet: " << err_msg;
    m_IsValid = false;
  };
  std::size_t size = m_Identity.FromBuffer(m_Buffer.get(), m_BufferLen);
  if (!size)
    {
      set_invalid("invalid identity");
      return;
    }
  const auto sign_key_len = m_Identity.GetSigningPublicKeyLen();
  const auto metadata_len =
      crypto::PkLen::ElGamal + sign_key_len + LeaseSetSize::NumLeaseLen;
  if (size + metadata_len > m_BufferLen)
    {
      set_invalid("metadata exceeds remaining buffer length");
      return;
    }
  memcpy(m_EncryptionKey.data(), m_Buffer.get() + size, crypto::PkLen::ElGamal);
  size += crypto::PkLen::ElGamal;  // encryption key
  size += sign_key_len;  // unused signing key
  const std::uint8_t num = m_Buffer[size];
  size++;  // num
  LOG(debug) << "LeaseSet: num=" << static_cast<int>(num);
  const std::size_t sig_len = m_Identity.GetSignatureLen();
  const std::uint16_t leases_len = num * LeaseSetSize::LeaseSize;
  if (size + leases_len + sig_len > m_BufferLen)
    {
      set_invalid("signature exceeds remaining buffer length");
      return;
    }
  if (!num)
    {
      LOG(debug) << "LeaseSet: no leases";
    }
  else if (num > LeaseSetSize::MaxLeases)
    {
      set_invalid("lease number exceeds the specified maximum");
      return;
    }
  else
    {
      if (size + leases_len > m_BufferLen)
        {
          set_invalid("leases exceed remaining buffer length");
          return;
        }
      if ((m_BufferLen - size - sig_len) % LeaseSetSize::LeaseSize != 0)
        {
          set_invalid("number of leases is not a whole number");
          return;
        }
      // process leases
      const std::uint8_t* leases = m_Buffer.get() + size;
      for (std::uint8_t i = 0; i < num; ++i)
        {
          Lease lease;
          lease.tunnel_gateway = leases;
          leases += LeaseSetSize::GatewayID;
          lease.tunnel_ID = core::InputByteStream::Read<std::uint32_t>(leases);
          leases += LeaseSetSize::TunnelID;
          lease.end_date = core::InputByteStream::Read<std::uint64_t>(leases);
          leases += LeaseSetSize::EndDate;
          m_Leases.push_back(lease);
          // check if lease's gateway is in our netDb
          if (!netdb.FindRouter(lease.tunnel_gateway))
            {
              // if not found request it
              LOG(debug)
                  << "LeaseSet: lease's tunnel gateway not found, requesting";
              netdb.RequestDestination(lease.tunnel_gateway);
            }
        }
      size += leases_len;
    }
  // verify
  const std::uint8_t* signature = m_Buffer.get() + size;
  if (!m_Identity.Verify(m_Buffer.get(), size, signature))
    set_invalid("verification failed");
}

const std::vector<Lease> LeaseSet::GetNonExpiredLeases(
    bool with_threshold) const {
  auto ts = kovri::core::GetMillisecondsSinceEpoch();
  std::vector<Lease> leases;
  for (auto& it : m_Leases) {
    auto end_date = it.end_date;
    if (!with_threshold)
      end_date -= kovri::core::TUNNEL_EXPIRATION_THRESHOLD * 1000;
    if (ts < end_date)
      leases.push_back(it);
  }
  return leases;
}

bool LeaseSet::HasExpiredLeases() const {
  auto ts = kovri::core::GetMillisecondsSinceEpoch();
  for (auto& it : m_Leases)
    if (ts >= it.end_date)
      return true;
  return false;
}

bool LeaseSet::HasNonExpiredLeases() const {
  auto ts = kovri::core::GetMillisecondsSinceEpoch();
  for (auto& it : m_Leases)
    if (ts < it.end_date)
      return true;
  return false;
}

}  // namespace core
}  // namespace kovri
