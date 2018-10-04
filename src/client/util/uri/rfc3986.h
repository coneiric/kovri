/**                                                                                           //
 * Copyright (c) 2013-2018, The Kovri I2P Router Project                                      //
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

#ifndef KOVRI_CLIENT_URI_RFC3986_H_
#define KOVRI_CLIENT_URI_RFC3986_H_

#include "client/util/uri/error.h"

namespace kovri
{
namespace client
{
/// @brief Check if character is alphabetical (only ASCII)
/// @param c Character to check
/// @return Boolean result of the check
inline bool is_alpha(const char c)
{
  unsigned constexpr a = 'a';
  return ((static_cast<unsigned>(c) | 32) - a) < 26U;
}

/// @brief Check if character is a digit
/// @param c Character to check
/// @return Boolean result of the check
inline bool is_digit(const char c)
{
  unsigned constexpr zero = '0';
  return (static_cast<unsigned>(c) - zero) < 10;
}

/// @brief Check if character is hexidecimal
/// @param c Character to check
/// @return Boolean result of the check
inline bool is_hex(const char c)
{
  return (
      c == 'a' || c == 'b' || c == 'c' || c == 'd' || c == 'e' || c == 'f'
      || c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' || c == 'F'
      || is_digit(c));
}

/// @brief Check if character is a general delimiter (see RFC3986)
/// @param c Character to check
/// @return Boolean result of the check
/// @detail gen-delims      = ":" / "/" / "?" / "#" / "[" / "]" / "@"
inline bool is_gen_delims(const char c)
{
  return c == ':' || c == '/' || c == '?' || c == '#' || c == '[' || c == ']'
         || c == '@';
}

/// @brief Check if character is a sub-delimiter (see RFC3986)
/// @param c Character to check
/// @return Boolean result of the check
/// @detail sub-delims      = "!" / "$" / "&" / "'" / "(" / ")"
///                           / "*" / "+" / "," / ";" / "="
inline bool is_sub_delims(const char c)
{
  return c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' || c == ')'
         || c == '*' || c == '+' || c == ',' || c == ';' || c == '=';
}

/// @brief Check if character is reserved (see RFC3986)
/// @param c Character to check
/// @return Boolean result of the check
/// @detail reserved        = gen-delims / sub-delims
/// @note may need to add a customization point for parsing custom schemes
inline bool is_reserved(const char c)
{
  return is_gen_delims(c) || is_sub_delims(c);
}

/// @brief Check if character is unreserved (see RFC3986)
/// @param c Character to check
/// @return Boolean result of the check
/// @detail unreserved      = ALPHA / DIGIT / "-" / "." / "_" / "~"
inline bool is_unreserved(const char c)
{
  return is_alpha(c) || is_digit(c) || c == '-' || c == '.' || c == '_'
         || c == '~';
}

/// @brief Check if character is a pchar (see RFC3986)
/// @param c Character to check
/// @return Boolean result of the check
/// @detail pchar           = unreserved / pct-encoded / sub-delims / ":" / "@"
inline bool is_pchar(const char c)
{
  return is_unreserved(c) || is_sub_delims(c) || c == '%' || c == ':' || c == '@';
}

/// @brief Check if character is a qchar (see RFC3986)
/// @param c Character to check
/// @return Boolean result of the check
/// @detail qchar           = pchar / "/" / "?"
inline bool is_qchar(const char c)
{
  return is_pchar(c) || c == '/' || c == '?';
}

/// @brief Check if character is a uchar (see RFC3986)
/// @param c Character to check
/// @return Boolean result of the check
/// @detail uchar           = unreserved / ";" / "?" / "&" / "="
inline bool is_uchar(const char c)
{
  return is_unreserved(c) || c == ';' || c == '?' || c == '&' || c == '=';
}

/// @brief Check if character is an hsegment (see RFC3986)
/// @param c Character to check
/// @return Boolean result of the check
/// @detail hsegment        = uchar / ":" / "@"
inline bool is_hsegment(const char c)
{
  return is_uchar(c) || c == ':' || c == '@';
}

} // client
} // kovri

#endif  // KOVRI_CLIENT_URI_RFC3986_H_
