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

#ifndef KOVRI_CLIENT_UTIL_URI_PARSER_H_
#define KOVRI_CLIENT_UTIL_URI_PARSER_H_

#include <functional>

#include <boost/beast/core/string.hpp>

#include "client/util/uri/buffer.h"
#include "client/util/uri/error.h"
#include "client/util/uri/rfc3986.h"

namespace kovri
{
namespace client
{
/// @class URI
/// @brief URI parser based on RFC3986
class URI
{
 public:
  /// @brief Parse URL based on RFC3986
  /// @param out Buffer for storing parsed result
  /// @param in URL to parse into component parts
  /// @param ec URI error code
  /// @detail [scheme:][//[user[:pass]@]host[:port]][/path][?query][#fragment]
  static void
  ParseURL(const std::string& in, URIBuffer& out, boost::system::error_code& ec);

  /// @brief Decode a URL-encoded string
  /// @param encoded URL-encoded string to decode
  /// @return URL-decoded string
  static std::string URLDecode(const std::string& encoded);

 private:
  static char PercentDecode(
      std::string::const_iterator first,
      std::string::const_iterator last,
      boost::system::error_code& ec);

  static std::string::const_iterator AppendDecodedOrChar(
      std::string::const_iterator first,
      std::string::const_iterator last,
      URIBuffer& out,
      boost::system::error_code& ec);

  static std::string::const_iterator FindDelimiterOrMismatch(
      std::string::const_iterator first,
      std::string::const_iterator last,
      std::function<bool(char)> delimiter_func,
      std::function<bool(char)> match_func,
      const bool ending_segment,
      boost::system::error_code& ec)
  {
    // Search for segment delimiter
    auto delimiter = std::find_if(first, last, delimiter_func);
    if (delimiter == last && !ending_segment)
      {
        ec = URIError::syntax;
        return delimiter;
      }
    // Search for characters that do not belong in the segment
    auto mismatch = std::find_if_not(first, delimiter, match_func);
    if (mismatch != delimiter)
      {
        ec = URIError::syntax;
        return mismatch;
      }
    return delimiter;
  }

  static std::string::const_iterator ParseScheme(
      std::string::const_iterator first,
      std::string::const_iterator last,
      URIBuffer& out,
      boost::system::error_code& ec); 

  static std::string::const_iterator ParseUsername(
      std::string::const_iterator first,
      std::string::const_iterator last,
      URIBuffer& out,
      boost::system::error_code& ec);

  static std::string::const_iterator ParsePassword(
      std::string::const_iterator first,
      std::string::const_iterator last,
      URIBuffer& out,
      boost::system::error_code& ec);

  static std::string::const_iterator ParseHost(
      std::string::const_iterator first,
      std::string::const_iterator last,
      URIBuffer& out,
      boost::system::error_code& ec);

  static std::string::const_iterator ParseIPv6(
      std::string::const_iterator first,
      std::string::const_iterator last,
      URIBuffer& out,
      boost::system::error_code& ec);

  static std::string::const_iterator ParseIPv4Reg(
      std::string::const_iterator first,
      std::string::const_iterator last,
      URIBuffer& out,
      boost::system::error_code& ec);

  static std::string::const_iterator ParsePort(
      std::string::const_iterator first,
      std::string::const_iterator last,
      URIBuffer& out,
      boost::system::error_code& ec);

  static std::string::const_iterator ParseAuthority(
      std::string::const_iterator first,
      std::string::const_iterator last,
      URIBuffer& out,
      boost::system::error_code& ec);

  static std::string::const_iterator SearchUserInfo(
      std::string::const_iterator first,
      std::string::const_iterator last,
      boost::system::error_code& ec);

  static std::string::const_iterator ParsePath(
      std::string::const_iterator first,
      std::string::const_iterator last,
      URIBuffer& out,
      boost::system::error_code& ec);

  static std::string::const_iterator ParseQuery(
      std::string::const_iterator first,
      std::string::const_iterator last,
      URIBuffer& out,
      boost::system::error_code& ec);

  static void ParseFragment(
      std::string::const_iterator first,
      std::string::const_iterator last,
      URIBuffer& out,
      boost::system::error_code& ec);
};

}  // namespace client
}  // namespace kovri

#endif  // KOVRI_CLIENT_UTIL_URI_PARSER_H_
