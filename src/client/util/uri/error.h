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
 *                                                                                            //
 * Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)                   //
 *                                                                                            //
 * Distributed under the Boost Software License, Version 1.0. (See accompanying               //
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)                      //
 *                                                                                            //
 * Official repository: https://github.com/boostorg/beast                                     //
 */

#ifndef KOVRI_CLIENT_URI_ERROR_H_
#define KOVRI_CLIENT_URI_ERROR_H_

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/error.hpp>

namespace kovri
{
namespace client
{
enum class URIError
{
  /// An input did not match a structural element (soft error)
  mismatch = 1,

  /// A syntax error occurred
  syntax,

  /// The parser encountered an invalid input
  invalid
};
}  // namespace client
}  // namespace kovri

namespace boost
{
namespace system
{
template <>
struct is_error_code_enum<kovri::client::URIError>
{
  static bool const value = true;
};
}  // namespace system
}  // namespace boost

namespace kovri
{
namespace client
{
class URIErrorCategory : public boost::system::error_category
{
 public:
  const char* name() const noexcept override
  {
    return "beast.http.uri";
  }

  std::string message(int ev) const override
  {
    switch (static_cast<URIError>(ev))
      {
        case URIError::mismatch:
          return "mismatched element";
        case URIError::syntax:
          return "syntax error";
        case URIError::invalid:
          return "invalid input";
        default:
          return "beast.http.uri error";
      }
  }

  boost::system::error_condition default_error_condition(int ev) const
      noexcept override
  {
    return boost::system::error_condition{ev, *this};
  }

  bool equivalent(int ev, boost::system::error_condition const& condition) const
      noexcept override
  {
    return condition.value() == ev && &condition.category() == this;
  }

  bool equivalent(boost::system::error_code const& error, int ev) const
      noexcept override
  {
    return error.value() == ev && &error.category() == this;
  }
};

inline boost::system::error_category const& get_uri_error_category()
{
  static URIErrorCategory const cat{};
  return cat;
}

inline boost::system::error_code make_error_code(URIError ev)
{
  return boost::system::error_code{
      static_cast<std::underlying_type<URIError>::type>(ev),
      get_uri_error_category()};
}

}  // namespace client
}  // namespace kovri

#endif  // KOVRI_CLIENT_URI_ERROR_H_
