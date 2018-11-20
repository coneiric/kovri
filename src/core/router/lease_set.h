/**                                                                                           //
 * Copyright (c) 2013-2017, The Kovri I2P Router Project                                      //
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

#ifndef SRC_CORE_ROUTER_LEASE_SET_H_
#define SRC_CORE_ROUTER_LEASE_SET_H_

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "core/router/identity.h"

namespace kovri {
namespace core {

// TODO(unassigned): remove this forward declaration
class TunnelPool;

struct Lease {
  Lease() : tunnel_gateway(), tunnel_ID(0), end_date(0) {}

  /// @brief Conversion-ctor to create an initialized Lease
  /// @param gateway_id IdentHash of the Lease's gateway
  /// @param tunnel_id Tunnel ID for the Lease
  /// @param end_date Expiration time for the Lease
  /// @details Default Lease expiration is ten minutes after creation, see spec
  Lease(
      const core::IdentHash& gateway_id,
      const std::uint32_t tunnel_id,
      const std::uint64_t end_date = (std::chrono::steady_clock::now()
                                      + std::chrono::minutes(10))
                                         .time_since_epoch()
                                         .count())
      : tunnel_gateway(gateway_id), tunnel_ID(tunnel_id), end_date(end_date)
  {
  }

  IdentHash tunnel_gateway;
  std::uint32_t tunnel_ID;
  std::uint64_t end_date;
  bool operator<(const Lease& other) const {
    if (end_date != other.end_date)
      return end_date > other.end_date;
    else
      return tunnel_ID < other.tunnel_ID;
  }
};

enum LeaseSetSize : std::uint16_t
{
  MaxBuffer = 3072,
  MaxLeases = 16,
  NumLeaseLen = 1,
  GatewayID = 32,
  TunnelID = 4,
  EndDate = 8,
  LeaseSize = 44,  // GatewayID + TunnelID + EndDate
};

class LeaseSet : public RoutingDestination {
 public:
  LeaseSet(
      const std::uint8_t* buf,
      std::size_t len);

  explicit LeaseSet(
      const kovri::core::TunnelPool& pool);

  /// @brief Create a LeaseSet with N Leases
  /// @param local Local destination for the LeaseSet
  /// @param leases Leases to include in the LeaseSet
  LeaseSet(
      const core::LocalDestination& local,
      const std::vector<Lease>& leases);

  ~LeaseSet() {}

  void Update(
      const std::uint8_t* buf,
      std::size_t len);

  const IdentityEx& GetIdentity() const {
    return m_Identity;
  }

  const std::uint8_t* GetBuffer() const {
    const auto buf = m_Buffer.get();
    return buf;
  }

  std::size_t GetBufferLen() const {
    return m_BufferLen;
  }

  bool IsValid() const {
    return m_IsValid;
  }

  // implements RoutingDestination
  const IdentHash& GetIdentHash() const {
    return m_Identity.GetIdentHash();
  }

  std::uint8_t GetNumLeases() const
  {
    const std::uint8_t* num_leases = m_Buffer.get() + m_Identity.GetFullLen()
                                     + crypto::PkLen::ElGamal
                                     + m_Identity.GetSigningPublicKeyLen();
    if (num_leases)
      return *num_leases;
    else
      return 0;
  }

  const std::vector<Lease>& GetLeases() const {
    return m_Leases;
  }

  const std::vector<Lease> GetNonExpiredLeases(
      bool with_threshold = true) const;

  bool HasExpiredLeases() const;

  bool HasNonExpiredLeases() const;

  const std::uint8_t* GetEncryptionPublicKey() const {
    return m_EncryptionKey.data();
  }

  const std::uint8_t* GetSignature() const
  {
    return m_Buffer.get() + (m_BufferLen - m_Identity.GetSignatureLen() - 1);
  }

  bool IsDestination() const {
    return true;
  }

 private:
  void ReadFromBuffer();

 private:
  bool m_IsValid;
  std::vector<Lease> m_Leases;
  IdentityEx m_Identity;
  std::array<std::uint8_t, crypto::PkLen::ElGamal> m_EncryptionKey;
  std::unique_ptr<std::uint8_t[]> m_Buffer;
  std::size_t m_BufferLen;
};

}  // namespace core
}  // namespace kovri

#endif  // SRC_CORE_ROUTER_LEASE_SET_H_
