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

#include "tests/fuzz_tests/i2pcontrol.h"

#include "client/api/i2p_control/data.h"
#include "core/util/exception.h"

namespace kovri
{
namespace fuzz
{
int I2PControl::Initialize(int*, char***)
{
  // nothing to do
  return 0;
}

int I2PControl::Impl(const uint8_t* data, size_t size)
{
  using Method = kovri::client::I2PControlDataTraits::Method;

  try
    {
      std::stringstream stream;
      stream.write(reinterpret_cast<const char*>(data), size);

      client::I2PControlRequest request;
      client::I2PControlResponse response;

      // Parse stream as I2PControl methods
      request.Parse(stream);
      response.Parse(Method::Authenticate, stream);
      response.Parse(Method::Echo, stream);
      response.Parse(Method::GetRate, stream);
      response.Parse(Method::I2PControl, stream);
      response.Parse(Method::RouterInfo, stream);
      response.Parse(Method::RouterManager, stream);
      response.Parse(Method::NetworkSetting, stream);
    }
  catch (...)
    {
      core::Exception ex;
      ex.Dispatch(__func__);
      return 0;
    }
  return 0;
}

}  // namespace fuzz
}  // namespace kovri
