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

#include "client/util/uri/parser.h"

struct URIFixture
{
  URIFixture()
  {
    expected.scheme("http");
    expected.host(host_ipv4);

    // Sanity check for collections of test hosts
    BOOST_CHECK_EQUAL(hosts.size(), parsed_hosts.size());
  }

  void doParse(boost::string_view url)
  {
    client::URIBuffer out;
    boost::system::error_code ec;
    client::URI::ParseURL(url.to_string(), out, ec);

    BOOST_CHECK(!ec);

    BOOST_CHECK_EQUAL(out.scheme(), expected.scheme());
    BOOST_CHECK_EQUAL(out.username(), expected.username());
    BOOST_CHECK_EQUAL(out.password(), expected.password());
    BOOST_CHECK_EQUAL(out.user_info(), expected.user_info());
    BOOST_CHECK_EQUAL(out.host(), expected.host());
    BOOST_CHECK_EQUAL(out.port(), expected.port());
    BOOST_CHECK_EQUAL(out.path(), expected.path());
    BOOST_CHECK_EQUAL(out.query(), expected.query());
    BOOST_CHECK_EQUAL(out.fragment(), expected.fragment());
  }

  void badParse(boost::string_view url)
  {
    client::URIBuffer out;
    boost::system::error_code ec;
    client::URI::ParseURL(url.to_string(), out, ec);

    BOOST_CHECK(ec);
  }

  client::URIBuffer expected;

  // Test hosts
  const std::string host_ipv6 = "::1";
  const std::string host_ipv4 = "1.1.1.1";
  const std::string host_registered = "kovri.i2p";
  const std::string http_host_ipv6 = "http://[" + host_ipv6 + "]";
  const std::string http_host_ipv4 = "http://" + host_ipv4;
  const std::string http_host_registered = "http://" + host_registered;

  const std::array<const std::string, 3> parsed_hosts{
      host_ipv6, host_ipv4, host_registered};

  const std::array<const std::string, 3> hosts{
      http_host_ipv6, http_host_ipv4, http_host_registered};

  // Character sets, see RFC3986
  const std::string alpha =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const std::string digit = "0123456789";
  const std::string unreserved = alpha + digit + "-._~";
  const std::string sub_delims = "!$&\'()*+,;=";

  // Example subset of percent-encoded characters, i.e. %HEXDIGIT
  const std::string percent_encoded =
      "\%6B\%6F\%76\%72\%69\%3D\%61\%77\%65\%73\%6F\%6D\%65";
  const std::string percent_decoded = "kovri=awesome";
  const std::string pchar_encoded =
      unreserved + percent_encoded + sub_delims + ":@";
  const std::string pchar_decoded =
      unreserved + percent_decoded + sub_delims + ":@";
};

BOOST_FIXTURE_TEST_SUITE(URITests, URIFixture)

BOOST_AUTO_TEST_CASE(ParsesSchemeToLower)
{
  expected.scheme("a");
  doParse("a://" + host_ipv4);
  doParse("A://" + host_ipv4);
}

BOOST_AUTO_TEST_CASE(ParsesCommonSchemes)
{
  expected.scheme("ws");
  doParse("ws://" + host_ipv4);

  expected.scheme("wss");
  doParse("wss://" + host_ipv4);

  expected.scheme("ftp");
  doParse("ftp://" + host_ipv4);

  expected.scheme("file");
  doParse("file:///" + host_ipv4);

  expected.scheme("http");
  doParse("http://" + host_ipv4);

  expected.scheme("https");
  doParse("https://" + host_ipv4);

  expected.scheme("gopher");
  doParse("gopher://" + host_ipv4);
}

BOOST_AUTO_TEST_CASE(ParsesUserInfo)
{
  expected.username("a");
  doParse("http://a@" + host_ipv4);
  doParse("http://a:@" + host_ipv4);

  expected.password("b");
  doParse("http://a:b@" + host_ipv4);
}

BOOST_AUTO_TEST_CASE(ParsesHosts)
{
  for (std::uint8_t ind = 0; ind < hosts.size(); ++ind)
    {
      expected.host(parsed_hosts[ind]);
      doParse(hosts[ind]);
    }

  // TODO(unassigned): additional IPv6, IPv4 and registered name checks
  // TODO(unassigned): international registered name checks
  //
  // Additional IPv6 check
  expected.host("fe80:1010::1010");
  doParse("http://[fe80:1010::1010]");
}

BOOST_AUTO_TEST_CASE(ParsesEndOfHost)
{
  for (std::uint8_t ind = 0; ind < hosts.size(); ++ind)
    {
      expected.host(parsed_hosts[ind]);
      expected.path("/");
      doParse(hosts[ind] + "/");

      expected.path("");
      doParse(hosts[ind] + "?");
      doParse(hosts[ind] + "#");
    }
}

BOOST_AUTO_TEST_CASE(ParsesPort)
{
  for (std::uint8_t ind = 0; ind < hosts.size(); ++ind)
    {
      expected.host(parsed_hosts[ind]);
      expected.port("80");
      doParse(hosts[ind] + ":80");
    }
}

BOOST_AUTO_TEST_CASE(ParsesEndOfPort)
{
  expected.port("80");
  doParse(http_host_ipv4 + ":80?");
  doParse(http_host_ipv4 + ":80#");

  expected.path("/");
  doParse(http_host_ipv4 + ":80/");
}

BOOST_AUTO_TEST_CASE(ParsesPath)
{
  expected.path("/");
  doParse(http_host_ipv4 + "/");

  expected.path("/a");
  doParse(http_host_ipv4 + "/a");

  expected.path("/a/b");
  doParse(http_host_ipv4 + "/a/b");

  expected.path("/" + pchar_decoded);
  doParse(http_host_ipv4 + "/" + pchar_encoded);
}

BOOST_AUTO_TEST_CASE(ParsesEndOfPath)
{
  expected.path("/");
  doParse(http_host_ipv4 + "/?");
  doParse(http_host_ipv4 + "/#");
}

BOOST_AUTO_TEST_CASE(ParsesQuery)
{
  expected.query(pchar_decoded + "/?");
  doParse(http_host_ipv4 + "?" + pchar_encoded + "/?");
}

BOOST_AUTO_TEST_CASE(ParsesFragment)
{
  expected.fragment(pchar_decoded + "/?");
  doParse(http_host_ipv4 + "#" + pchar_encoded + "/?");
}

BOOST_AUTO_TEST_CASE(ParsesPathWithQuery)
{
  expected.path("/a");
  expected.query("b");
  doParse(http_host_ipv4 + "/a?b");

  expected.query("b=1");
  doParse(http_host_ipv4 + "/a?b=1");
}

BOOST_AUTO_TEST_CASE(ParsesPathWithFragment)
{
  expected.path("/a");
  doParse(http_host_ipv4 + "/a#");

  expected.fragment("a");
  doParse(http_host_ipv4 + "/a#a");

  expected.path("/");
  expected.fragment("a");
  doParse(http_host_ipv4 + "/#a");
}

BOOST_AUTO_TEST_CASE(ParsesPathWithQueryAndFragment)
{
  expected.path("/a");
  expected.query("b=1");
  doParse(http_host_ipv4 + "/a?b=1#");

  expected.fragment("a");
  doParse(http_host_ipv4 + "/a?b=1#a");
}

BOOST_AUTO_TEST_CASE(RejectsInvalidPath)
{
  badParse(http_host_ipv4 + "//");
  badParse(http_host_ipv4 + "/<");
  badParse(http_host_ipv4 + "/>");
  badParse(http_host_ipv4 + "/[");
  badParse(http_host_ipv4 + "/]");
  badParse(http_host_ipv4 + "/{");
  badParse(http_host_ipv4 + "/}");
  badParse(http_host_ipv4 + "/^");
  badParse(http_host_ipv4 + "/%");
  badParse(http_host_ipv4 + "/|");
  badParse(http_host_ipv4 + "/`");
}

BOOST_AUTO_TEST_CASE(RejectsInvalidQuery)
{
  badParse(http_host_ipv4 + "?<");
  badParse(http_host_ipv4 + "?>");
  badParse(http_host_ipv4 + "?[");
  badParse(http_host_ipv4 + "?]");
  badParse(http_host_ipv4 + "?{");
  badParse(http_host_ipv4 + "?}");
  badParse(http_host_ipv4 + "?^");
  badParse(http_host_ipv4 + "?%");
  badParse(http_host_ipv4 + "?|");
  badParse(http_host_ipv4 + "?`");
}

BOOST_AUTO_TEST_CASE(RejectsInvalidFragment)
{
  badParse(http_host_ipv4 + "#<");
  badParse(http_host_ipv4 + "#>");
  badParse(http_host_ipv4 + "#[");
  badParse(http_host_ipv4 + "#]");
  badParse(http_host_ipv4 + "#{");
  badParse(http_host_ipv4 + "#}");
  badParse(http_host_ipv4 + "#^");
  badParse(http_host_ipv4 + "#%");
  badParse(http_host_ipv4 + "#|");
  badParse(http_host_ipv4 + "#`");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(URLDecodeTests, URIFixture)

BOOST_AUTO_TEST_CASE(URLDecode)
{
  const std::string encoded("http://\%6B\%6F\%76\%72\%69\%2E\%69\%32\%70");
  BOOST_CHECK_EQUAL(http_host_registered, client::URI::URLDecode(encoded));
}

BOOST_AUTO_TEST_CASE(BadURLDecode)
{
  // Should throw error for non-hex values in URL-encoded pair
  BOOST_CHECK_THROW(client::URI::URLDecode("\%G0"), std::exception);
  BOOST_CHECK_THROW(client::URI::URLDecode("\%0G"), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()

// Attack test-cases courtesy of Orange Tsai:
//   A New Era Of SSRF Exploiting URL Parser In Trending Programming Languages
BOOST_FIXTURE_TEST_SUITE(RequestSmugglingTests, URIFixture)

BOOST_AUTO_TEST_CASE(ParsesSmuggledRequestInPath)
{
  // Path could be misinterpreted as a request using another protocol
  expected.host(host_registered);
  expected.fragment("");
  expected.path("/\r\nSLAVEOF kovri.i2p 6379\r\n");
  doParse("http://kovri.i2p/%0D%0ASLAVEOF%20kovri.i2p%206379%0D%0A");
}

BOOST_AUTO_TEST_CASE(ParsesSmuggledRequestInFragment)
{
  // Fragment could be misinterpreted as the host
  expected.host(host_registered);
  expected.fragment("@evil.com/");
  doParse("http://kovri.i2p#@evil.com/");
}

BOOST_AUTO_TEST_CASE(RejectsWhitespaceAuthority)
{
  badParse("http://1.1.1.1 &@2.2.2.2# @3.3.3.3/");
  badParse("http://0\r\n SLAVEOF kovri.i2p 6379\r\n :80");
}

BOOST_AUTO_TEST_CASE(RejectsSmuggledSMTPRequests)
{
  badParse(
      "http://127.0.0.1:25/\%0D\%0AHELO kovri.i2p\%0D\%0AMAIL FROM: "
      "admin@kovri.i2p:25");
  badParse(
      "https://127.0.0.1 \%0D\%0AHELO kovri.i2p\%0D\%0AMAIL FROM: "
      "admin@kovri.i2p:25");
}

BOOST_AUTO_TEST_CASE(RejectsMultiplePorts)
{
  badParse("http://127.0.0.1:11211:80");
}

BOOST_AUTO_TEST_CASE(RejectsMultipleUserInfoAndHosts)
{
  badParse("http://foo@evil.com:80@kovri.i2p/");
  badParse("http://foo@127.0.0.1 @kovri.i2p/");
  badParse("http://foo@127.0.0.1:11211@kovri.i2p:80");
  badParse("http://foo@127.0.0.1 @kovri.i2p:11211");
}

BOOST_AUTO_TEST_CASE(RejectsInvalidPathCharacters)
{
  badParse("http://kovri.i2p/\xFF\x2E\xFF\x2E");
}

BOOST_AUTO_TEST_SUITE_END()
