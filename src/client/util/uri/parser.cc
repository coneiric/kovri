/**                                                                                           //
 * Copyright (c) 2015-2018, The Kovri I2P Router Project                                      //
 * Copyright (c) 2018 oneiric (oneiric at i2pmail dot org)                                    //
 * Copyright (c) 2018 jackarain (jack dot wgm at gmail dot com)                               //
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

#include "client/util/uri/parser.h"

namespace kovri
{
namespace client
{
void URI::ParseURL(
    const std::string& url,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  if (url.empty())
    {
      ec = URIError::syntax;
      return;
    }
  auto first = url.begin();
  const auto last = url.end();
  first = ParseScheme(first, last, out, ec);
  if (ec || first == last)
    {
      ec = URIError::syntax;
      return;
    }
  first = ParseAuthority(first, last, out, ec);
  if (ec)
    return;

  if (*first == '/')
    first = ParsePath(first, last, out, ec);

  if (ec)
    return;

  if (*first == '?')
    first = ParseQuery(first, last, out, ec);

  if (ec)
    return;

  if (*first == '#')
    ParseFragment(first, last, out, ec);
}

std::string URI::URLDecode(const std::string& encoded)
{
  std::string ret;

  for (auto it = encoded.begin(); it < encoded.end();)
    {
      if (*it == '%')
        {
          boost::system::error_code ec;
          auto ch = PercentDecode(++it, encoded.end(), ec);
          if (ec)
            throw std::runtime_error(
                std::string(__func__) + ": invalid URL-encoded value");
          ret += ch;
          it += 2;
        }
      else
        {
          ret += *it == '+' ? ' ' : *it;
          ++it;
        }
    }
  return ret;
}

char URI::PercentDecode(
    std::string::const_iterator first,
    std::string::const_iterator last,
    boost::system::error_code& ec)
{
  // Check iterators are in valid range, and next two characters are hex
  if (first + 2 > last || !is_hex(first[0]) || !is_hex(first[1]))
    {
      ec = URIError::syntax;
      return *first;
    }
  std::string s(first, first + 2);
  return static_cast<char>(std::stoi(s, nullptr, 16));
}

std::string::const_iterator URI::AppendDecodedOrChar(
    std::string::const_iterator first,
    std::string::const_iterator last,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  if (*first == '%')
    {
      auto ch = PercentDecode(++first, last, ec);
      if (ec)
        return first;
      out.push_back(ch);
      first += 2;
    }
  else
    out.push_back(*(first++));

  return first;
}

std::string::const_iterator URI::ParseScheme(
    std::string::const_iterator first,
    std::string::const_iterator last,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  using boost::beast::detail::ascii_tolower;

  /*  scheme         ; = ALPHA / *(ALPHA / DIGIT / "-" / "." / "+") / ":"
   */
  if (!is_alpha(*first))
    {
      ec = URIError::syntax;
      return first;
    }
  const auto scheme_delimiter = [](const char c) { return c == ':'; };
  const auto is_scheme_char = [](const char c) {
    return is_alpha(c) || is_digit(c) || c == '-' || c == '.' || c == '+'
           || c == ':';
  };
  constexpr bool ending_segment = false;
  auto delimeter = FindDelimiterOrMismatch(
      first, last, scheme_delimiter, is_scheme_char, ending_segment, ec);

  if (ec)
    return delimeter;

  while (first != delimeter)
    out.push_back(ascii_tolower(*(first++)));

  out.scheme(out.PartFrom(out.begin(), out.end()));
  out.push_back(*(first++));
  return first;
}

std::string::const_iterator URI::ParseUsername(
    std::string::const_iterator first,
    std::string::const_iterator last,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  /*  username       ; = *(unreserved / pct-encoded / sub_delims) / ":" / "@"
   */
  const auto size = out.size();
  const auto username_delimiter = [](const char c) {
    return c == ':' || c == '@';
  };
  constexpr bool ending_segment = false;
  const auto delimeter = FindDelimiterOrMismatch(
      first, last, username_delimiter, is_pchar, ending_segment, ec);

  if (ec)
    return delimeter;

  while (first != delimeter)
    {
      first = AppendDecodedOrChar(first, last, out, ec);
      if (ec)
        return first;
    }
  const auto start = out.begin() + size;
  out.username(out.PartFrom(start, out.end()));
  out.push_back(*(first++));
  return first;
}

std::string::const_iterator URI::ParsePassword(
    std::string::const_iterator first,
    std::string::const_iterator last,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  /*  password       ; = ":" / *(unreserved / pct-encoded / sub_delims) / "@"
   */
  const auto size = out.size();
  const auto password_delimiter = [](const char c) { return c == '@'; };
  constexpr bool ending_segment = false;
  auto delimeter = FindDelimiterOrMismatch(
      first, last, password_delimiter, is_pchar, ending_segment, ec);

  if (ec)
    return delimeter;

  while (first != delimeter)
    {
      first = AppendDecodedOrChar(first, last, out, ec);
      if (ec)
        return first;
    }
  const auto start = out.begin() + size;
  out.password(out.PartFrom(start, out.end()));
  out.push_back(*(first++));
  return first;
}

std::string::const_iterator URI::ParseHost(
    std::string::const_iterator first,
    std::string::const_iterator last,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  if (*first == '[')
    {
      first = ParseIPv6(first, last, out, ec);
      return first;
    }
  else
    {  // Parse IPv4 or registered name
      first = ParseIPv4Reg(first, last, out, ec);
      return first;
    }
}

std::string::const_iterator URI::ParseIPv6(
    std::string::const_iterator first,
    std::string::const_iterator last,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  /*  IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
   *
   *  IPv6address =                              6( h16 ":" ) ls32
   *                /                       "::" 5( h16 ":" ) ls32
   *                / [               h16 ] "::" 4( h16 ":" ) ls32
   *                / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
   *                / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
   *                / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
   *                / [ *4( h16 ":" ) h16 ] "::"              ls32
   *                / [ *5( h16 ":" ) h16 ] "::"              h16
   *                / [ *6( h16 ":" ) h16 ] "::"
   *
   *  TODO: need better IPv6 parsing.
   *  Current rule is serviceable, but doesn't cover all cases.
   */
  const auto size = out.size();
  const auto ipv6_delimiter = [](const char c) { return c == ']'; };
  const auto is_ipv6_char = [](const char c) { return c == ':' || is_hex(c); };
  constexpr bool ending_segment = false;
  // Pre-increment `first` to skip leading bracket
  const auto delimeter = FindDelimiterOrMismatch(
      ++first, last, ipv6_delimiter, is_ipv6_char, ending_segment, ec);

  if (ec)
    return delimeter;

  while (first != delimeter)
    out.push_back(*(first++));

  // Pre-increment `first` to skip trailing bracket
  if (++first != last && *first != ':' && *first != '/' && *first != '?'
      && *first != '#')
    {
      ec = URIError::syntax;
      return first;
    }
  const auto start = out.begin() + size;
  out.host(out.PartFrom(start, out.end()));
  if (first != last)
    out.push_back(*first);

  return first;
}

std::string::const_iterator URI::ParseIPv4Reg(
    std::string::const_iterator first,
    std::string::const_iterator last,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  /*  IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
   *
   *  dec-octet   = DIGIT                 ; 0-9
   *              / %x31-39 DIGIT         ; 10-99
   *              / "1" 2DIGIT            ; 100-199
   *              / "2" %x30-34 DIGIT     ; 200-249
   *              / "25" %x30-35          ; 250-255
   *
   *  reg-name    = *( unreserved / pct-encoded / sub-delims )
   *
   *  TODO: need distinct IPv4 parsing.
   *  Current rule captures IPv4, but is too loose, allowing invalid IPv4.
   */
  const auto size = out.size();
  const auto ipv4_delimiter = [](const char c) {
    return c == ':' || c == '/' || c == '?' || c == '#';
  };
  const auto is_ipv4_char = [](const char c) {
    return is_unreserved(c) || is_sub_delims(c) || c == '%';
  };
  constexpr bool ending_segment = true;
  const auto delimiter = FindDelimiterOrMismatch(
      first, last, ipv4_delimiter, is_ipv4_char, ending_segment, ec);

  if (ec)
    return delimiter;

  while (first != delimiter)
    {
      first = AppendDecodedOrChar(first, last, out, ec);
      if (ec)
        return first;
    }

  const auto start = out.begin() + size;
  out.host(out.PartFrom(start, out.end()));
  return first;
}

std::string::const_iterator URI::ParsePort(
    std::string::const_iterator first,
    std::string::const_iterator last,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  /*  port          ; = ":" / *DIGIT
   */
  if (*first != ':')
    {
      ec = URIError::syntax;
      return first;
    }
  out.push_back(*(first++));
  const auto size = out.size();
  const auto port_delimiter = [](const char c) {
    return c == '/' || c == '?' || c == '#';
  };
  constexpr bool ending_segment = true;
  const auto delimiter = FindDelimiterOrMismatch(
      first, last, port_delimiter, is_digit, ending_segment, ec);

  if (ec)
    return delimiter;

  while (first != delimiter)
    out.push_back(*(first++));

  const auto start = out.begin() + size;
  out.port(out.PartFrom(start, out.end()));
  if (first != last)
    out.push_back(*first);

  return first;
}

std::string::const_iterator URI::ParseAuthority(
    std::string::const_iterator first,
    std::string::const_iterator last,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  if (first + 2 >= last || first[0] != '/' || first[1] != '/')
    {
      ec = URIError::syntax;
      return first;
    }
  out.push_back(*first++);
  out.push_back(*first++);
  if (out.scheme() == "file")
    {
      if (first != last && *first == '/')
        out.push_back(*first++);
      else
        {
          ec = URIError::syntax;
          return first;
        }
    }
  // Check for a username[:password] section
  const auto search = SearchUserInfo(first, last, ec);
  if (!ec && *search == '@')
    {
      first = ParseUsername(first, last, out, ec);
      if (!ec && first != last && *(first - 1) == ':')
        first = ParsePassword(first, last, out, ec);
    }
  // Valid authority needs a host
  if ((ec && ec != URIError::mismatch) || first == last)
    {
      ec = URIError::syntax;
      return first;
    }
  // Clear a potential mismatch error
  ec.assign(0, ec.category());
  first = ParseHost(first, last, out, ec);
  if (ec)
    return first;

  if (*first == ':')
    first = ParsePort(first, last, out, ec);

  return first;
}

std::string::const_iterator URI::SearchUserInfo(
    std::string::const_iterator first,
    std::string::const_iterator last,
    boost::system::error_code& ec)
{
  const auto user_info_delimiter = [](const char c) { return c == '@'; };
  const auto is_user_info_char = [](const char c) {
    return c != '/' && c != '?' && c != '#'
           && (is_uchar(c) || is_sub_delims(c) || c == '%' || c == ':');
  };
  constexpr bool ending_segment = false;
  first = FindDelimiterOrMismatch(
      first, last, user_info_delimiter, is_user_info_char, ending_segment, ec);

  if (ec)
    ec = URIError::mismatch;

  return first;
}

std::string::const_iterator URI::ParsePath(
    std::string::const_iterator first,
    std::string::const_iterator last,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  /*  path          ; = path-absolute = "/" [ segment-nz *( "/" segment ) ]
   *  segment       ; = *pchar
   *  segment-nz    ; = 1*pchar
   */
  const auto size = out.size();
  if (*first != '/')
    {
      ec = URIError::syntax;
      return first;
    }
  out.push_back(*(first++));
  // Path cannot start with "//", see spec
  if (*first == '/')
    {
      ec = URIError::syntax;
      return first;
    }
  const auto path_delimiter = [](const char c) { return c == '?' || c == '#'; };
  const auto is_path_char = [](const char c) {
    return c == '/' || is_pchar(c);
  };
  constexpr bool ending_segment = true;
  const auto delimiter = FindDelimiterOrMismatch(
      first, last, path_delimiter, is_path_char, ending_segment, ec);

  if (ec)
    return delimiter;

  while (first != delimiter)
    {
      first = AppendDecodedOrChar(first, last, out, ec);
      if (ec)
        return first;
    }
  const auto start = out.begin() + size;
  out.path(out.PartFrom(start, out.end()));
  if (first != last)
    out.push_back(*first);

  return first;
}

std::string::const_iterator URI::ParseQuery(
    std::string::const_iterator first,
    std::string::const_iterator last,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  /*  query         ; = "?" / *( pchar / "/" / "?" ) / "#"
   */
  if (*first != '?')
    {
      ec = URIError::syntax;
      return first;
    }
  out.push_back(*(first++));
  const auto size = out.size();
  const auto query_delimiter = [](const char c){ return c == '#'; };
  const auto is_query_char = [](const char c) {
    return is_pchar(c) || c == '/' || c == '?';
  };
  constexpr bool ending_segment = true;
  const auto delimiter = FindDelimiterOrMismatch(
      first, last, query_delimiter, is_query_char, ending_segment, ec);

  if (ec)
    return delimiter;

  while (first != delimiter)
    {
      first = AppendDecodedOrChar(first, last, out, ec);
      if (ec)
        return first;
    }
  if (out.size() == size)
    return first;

  const auto start = out.begin() + size;
  out.query(out.PartFrom(start, out.end()));
  return first;
}

void URI::ParseFragment(
    std::string::const_iterator first,
    std::string::const_iterator last,
    URIBuffer& out,
    boost::system::error_code& ec)
{
  /*  fragment      ; = = "#" / *( pchar / "/" / "?" )
   */
  if (*first != '#')
    {
      ec = URIError::syntax;
      return;
    }
  out.push_back(*(first++));
  const auto size = out.size();
  const auto is_fragment_char = [](const char c) {
    return is_pchar(c) || c == '/' || c == '?';
  };
  const auto mismatch = std::find_if_not(first, last, is_fragment_char);
  if (mismatch != last)
    {
      ec = URIError::syntax;
      return;
    }
  while (first != last)
    {
      first = AppendDecodedOrChar(first, last, out, ec);
      if (ec)
        return;
    }
  if (out.size() == size)
    return;

  const auto start = out.begin() + size;
  out.fragment(out.PartFrom(start, out.end()));
}

}  // namespace client
}  // namespace kovri
