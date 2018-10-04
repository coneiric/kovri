/**                                                                                           //
 * Copyright (c) 2015-2018, The Kovri I2P Router Project                                      //
 * Copyright (c) 2018 oneiric (oneiric at i2pmail dot org)                                    //
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

#ifndef KOVRI_CLIENT_URI_PARTS_H_
#define KOVRI_CLIENT_URI_PARTS_H_

#include <boost/beast/core/string.hpp>

namespace kovri
{
namespace client
{
class Parts
{
 protected:
  std::string scheme_;
  std::string username_;
  std::string password_;
  std::string host_;
  std::string port_;
  std::string path_;
  std::string query_;
  std::string fragment_;

  void reset()
  {
    scheme_.clear();
    username_.clear();
    password_.clear();
    host_.clear();
    port_.clear();
    path_.clear();
    query_.clear();
    fragment_.clear();
  }

public:
  void scheme(std::string part) { scheme_ = part; }
  boost::string_view scheme() const noexcept { return scheme_; }

  void username(std::string part) { username_ = part; }
  boost::string_view username() const noexcept { return username_; }

  void password(std::string part) { password_ = part; }
  boost::string_view password() const noexcept { return password_; }

  boost::string_view user_info() const noexcept
  {
    return username_ + (password_.empty() ? "" : ':' + password_);
  }

  void host(std::string part) { host_ = part; }
  boost::string_view host() const noexcept { return host_; }

  void port(std::string part) { port_ = part; }
  boost::string_view port() const noexcept { return port_; }

  void path(std::string part) { path_ = part; }
  boost::string_view path() const noexcept { return path_; }

  void query(std::string part) { query_ = part; }
  boost::string_view query() const noexcept { return query_; }

  void fragment(std::string part) { fragment_ = part; }
  boost::string_view fragment() const noexcept { return fragment_; }
};

} //  client
} //  kovri

#endif  // KOVRI_CLIENT_URI_PARTS_H_
