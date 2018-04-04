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

#include "client/proxy/http.h"

#include <atomic>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>


#include <boost/network/uri/decode.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <cassert>
#include <cstring>
#include <map>

#include <boost/foreach.hpp>
#include "client/api/streaming.h"
#include "client/context.h"
#include "client/tunnel.h"
#include "client/util/http.h"

#include "core/router/identity.h"

namespace kovri {
namespace client {

HTTPProxyServer::HTTPProxyServer(
    const std::string& name,
    const std::string& address,
    std::uint16_t port,
    std::shared_ptr<kovri::client::ClientDestination> local_destination)
    : TCPIPAcceptor(
          address,
          port,
          local_destination
              ? local_destination
              : kovri::client::context.GetSharedLocalDestination()),
      m_Name(name) {
}

std::shared_ptr<kovri::client::I2PServiceHandler>
HTTPProxyServer::CreateHandler(
    std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
  return std::make_shared<HTTPProxyHandler>(this, socket);
}

void HTTPProxyHandler::Handle() {
  LOG(debug) << "HTTPProxyHandler: async sock read";
  if (!m_Socket) {
    LOG(error) << "HTTPProxyHandler: no socket for read";
    return;
  }

  AsyncSockRead(m_Socket);
}

void HTTPProxyHandler::AsyncSockRead(
    std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
  // TODO(guzzi) but there's also use cases where you are providing an inproxy
  // service for
  // others
  // 00:27 < zzz2> for a full threat model including "slowloris" attacks,
  // you need to
  // enforce max header lines, max header line length
  // and a total header timeout
  // 00:27 < zzz2> (in addition to the typical read timeout)
  // read in header until \r\n\r\n, then read in small portions of body
  // and forward along
  // Read the request headers, which are terminated by a blank line.
  // TODO(guzzi)
  // per anonimal: unit test, One example is the addressbook unit-test
  // I merged earlier.
  // You can see how it works purely on stream data and not any actual file/disk
  // i/o"

  boost::asio::async_read_until(
      *socket,
      m_Protocol.m_Buffer,
      "\r\n\r\n",
      boost::bind(
          &HTTPProxyHandler::AsyncHandleReadHeaders,
          shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
}
void HTTPProxyHandler::AsyncHandleReadHeaders(
    const boost::system::error_code& error,
    std::size_t bytes_transfered) {
  if (error) {
    LOG(debug) << "HTTPProxy: error sock read: " << bytes_transfered;
    Terminate();
    return;
  }
  boost::asio::streambuf::const_buffers_type bufs = m_Protocol.m_Buffer.data();
  std::string tbuffer(
      boost::asio::buffers_begin(bufs),
      boost::asio::buffers_begin(bufs) + m_Protocol.m_Buffer.size());

  if (!m_Protocol.HandleData(tbuffer)) {
    LOG(debug) << "HTTPProxy: error HandleData() " <<  "check http proxy";
    HTTPRequestFailed();  // calls Terminate
    return;
  }
  if (m_Protocol.m_Buffer.size() < bytes_transfered) {
    // overflowed buffer
    LOG(debug) << "HTTPProxy:: error buffer overflow sock read: "
      << bytes_transfered;
    Terminate();
    return;
  }
  // There could be additional bytes on the buffer than what is
  // stated in bytes_transfered.
  // Actually most of the time this is the case.
  // these bytes are part of the body
  // TODO(guzzi) ; find a better way; possibility is read each char
  // until \r\n\r\n;
  std::size_t num_additional_bytes = m_Protocol.m_Buffer.size()
    - bytes_transfered;

  if (num_additional_bytes) {
    // make m_Buffer into string
    boost::asio::streambuf::const_buffers_type buf
        = m_Protocol.m_Buffer.data();
    std::string str(
        boost::asio::buffers_end(buf) - num_additional_bytes,
        boost::asio::buffers_end(buf));
    // add the additional bytes the m_body
    m_Protocol.m_Body = str;
  }
  // look for body
  auto it = std::find_if(
      m_Protocol.m_HeaderMap.begin(),
      m_Protocol.m_HeaderMap.end(),
      [](std::pair<std::string, std::string> arg) {
        boost::trim_left(arg.first);
        boost::trim_right(arg.first);
        return arg.first == "Content-Length";
      });
  if (it != m_Protocol.m_HeaderMap.end()) {
    // body found
    std::size_t clen = boost::lexical_cast<std::size_t>(
        boost::trim_left_copy(it->second));
    // buffer overflow should throw exception.
    if (clen < num_additional_bytes) {
      // num_additinal bytes too long for content length
      LOG(debug) << "HTTPProxy:: additional bytes longer than content length "
        << bytes_transfered;
      Terminate();
      return;
    }
    clen = clen - num_additional_bytes;
    if (clen) {
      // read additional body
      boost::asio::async_read(
          *m_Socket,
          m_Protocol.m_BodyBuffer,
          boost::asio::transfer_at_least(clen),
          boost::bind(
              &HTTPProxyHandler::HandleSockRecv,
              shared_from_this(),
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred));
    } else {
      // body already read fully
      CreateStream();
    }
  } else {
    // no body
    CreateStream();
  }
}
void HTTPProxyHandler::CreateStream() {
  try
    {
      LOG(debug) << "HTTPProxyHandler: sock recv: "
                 << m_Protocol.m_Buffer.size();
      if (m_Protocol.CreateHTTPRequest())
        {
          LOG(info) << "HTTPProxyHandler: proxy requested: "
                    << m_Protocol.m_URI.get(URI_t::URL);
          GetOwner()->CreateStream(
              std::bind(
                  &HTTPProxyHandler::HandleStreamRequestComplete,
                  shared_from_this(),
                  std::placeholders::_1),
              m_Protocol.m_URI.get(URI_t::Host),
              boost::lexical_cast<std::uint16_t>(
                  m_Protocol.m_URI.get(URI_t::Port)));
        }
    }
  catch (...)
  {  // Handle bad lexical cast
    kovri::core::Exception ex(__func__);
    ex.Dispatch();
    throw;
  }
}
void HTTPProxyHandler::HandleSockRecv(
    const boost::system::error_code& error,
    std::size_t bytes_transferred) {
  if (error) {
    LOG(debug) << "HTTPProxy: error sock read body: " << bytes_transferred;
    Terminate();
    return;
  }
  // TODO(guzzi) should not read entire body into memory
  // instead read a buffer full ie 512 bytes and I2pconnect and send.
  // if we read some buffer into the body buffer variable then save it to m_body
  // will need another function handler call that loops itself until all is read
  // and then finishes up below. if 0 is read it instead closes the connection
  boost::asio::streambuf::const_buffers_type buf
      = m_Protocol.m_BodyBuffer.data();
  std::string str(
      boost::asio::buffers_begin(buf),
      boost::asio::buffers_begin(buf) + m_Protocol.m_BodyBuffer.size());
  m_Protocol.m_Body += str;
  CreateStream();
}

bool HTTPMessage::HandleData(const std::string& protocol_string) {
  std::vector<std::string> header_body;
  std::vector<std::string> tokens_header_body;
  // get header info
  // initially set error response to bad_request
  m_ErrorResponse = HTTPResponse(HTTPResponseCodes::status_t::bad_request);
  if (boost::algorithm::split_regex(
          header_body, protocol_string, boost::regex("\r\n\r\n")).size()
      != HEADERBODY_LEN)
    return false;
  if (boost::algorithm::split_regex(tokens_header_body, header_body[0],
        boost::regex("\r\n"))
          .size()
      < REQUESTLINE_HEADERS_MIN)
    return false;
  m_RequestLine = tokens_header_body[0];
  // requestline
  std::vector<std::string> tokens_request;
  boost::split(tokens_request, m_RequestLine, boost::is_any_of(" \t"));
  if (tokens_request.size() == 3) {
    m_Method = tokens_request[0];
    m_Version = tokens_request[2];
    m_URI.set(URI_t::URL, tokens_request[1]);
  } else {
    return false;
  }
  // headersline
  m_Headers = tokens_header_body;
  // remove start line
  m_Headers.erase(m_Headers.begin());
  std::vector<std::pair<std::string, std::string>> headers;
  for (auto it = m_Headers.begin(); it != m_Headers.end(); it++) {
    std::vector<std::string> keyElement;
    boost::split(keyElement, *it, boost::is_any_of(":"));
    std::string key = keyElement[0];
    keyElement.erase(keyElement.begin());
    std::string value = boost::algorithm::join(keyElement, ":");
    // concatenate remaining : values ie times
    headers.push_back(std::pair<std::string, std::string>(key, value));
  }

  m_HeaderMap = headers;
  // reset error response to ok
  m_ErrorResponse = HTTPResponse(HTTPResponseCodes::status_t::ok);
  return true;
}

void HTTPProxyHandler::HandleStreamRequestComplete(
    std::shared_ptr<kovri::client::Stream> stream) {
  if (stream) {
    if (Kill())
      return;
    LOG(info) << "HTTPProxyHandler: new I2PTunnel connection";
    auto connection =
      std::make_shared<kovri::client::I2PTunnelConnection>(
          GetOwner(),
          m_Socket,
          stream);
    GetOwner()->AddHandler(connection);
    connection->I2PConnect(
        reinterpret_cast<const std::uint8_t*>(m_Protocol.m_Request.c_str()),
        m_Protocol.m_Request.size());
    Done(shared_from_this());
  } else {
    LOG(error) << "HTTPProxyHandler: stream is unavailable, try again soon";
    m_Protocol.m_ErrorResponse
        = HTTPResponse(HTTPResponseCodes::status_t::service_unavailable);
    HTTPRequestFailed();
  }
}
/// @brief all this to change the useragent
/// @param len length of string
bool HTTPMessage::CreateHTTPRequest(const bool save_address) {
  // TODO(oneiric): convert to try-catch block
  if (!ExtractIncomingRequest()) {
    // m_ErrorResponse is set in ExtractIncomingRequest
    return false;
  }

  // TODO(oneiric): convert jump service handling to try-catch block
  if (IsJumpServiceRequest())
    {
      if (!HandleJumpService())
        {
          m_ErrorResponse =
              HTTPResponse(HTTPResponseCodes::status_t::bad_request);
          return false;
        }
      // TODO(oneiric): this is very dangerous and broken
      // When converting to a proxy handler, we should prompt the user with an
      // HTTP redirect to a save form that should contain:
      // - host info: short address, base32 address, base64 destination
      // - save location options
      // - continue without saving option
      if (!SaveJumpServiceAddress() && save_address)
        {
          m_ErrorResponse =
              HTTPResponse(HTTPResponseCodes::status_t::internal_server_error);
          return false;
        }
    }

  // Set method, path, and version
  m_Request = m_Method;
  m_Request.push_back(' ');
  m_Request += m_URI.get(URI_t::URL);
  m_Request.push_back(' ');
  m_Request += m_Version + "\r\n";

  // find and remove/adjust headers
  auto it_user_agent = std::find_if(
      m_HeaderMap.begin(),
      m_HeaderMap.end(),
      [](std::pair<std::string, std::string> arg) {
        boost::trim_left(arg.first);
        boost::trim_right(arg.first);
        return arg.first == "User-Agent";
      });
  if (it_user_agent != m_HeaderMap.end())  //  found
    it_user_agent->second = " MYOB/6.66 (AN/ON)";
  auto it_referer = std::find_if(
      m_HeaderMap.begin(),
      m_HeaderMap.end(),
      [](std::pair<std::string, std::string> arg) {
        boost::trim_left(arg.first);
        boost::trim_right(arg.first);
        return arg.first == "Referer";
      });
  if (it_referer != m_HeaderMap.end())  //  found
    m_HeaderMap.erase(it_referer);
  for (std::vector<std::pair<std::string, std::string>>::iterator ii
       = m_HeaderMap.begin();
       ii != m_HeaderMap.end();
       ++ii) {
    m_Request = m_Request + ii->first + ":" + ii->second + "\r\n";
  }
  m_Request = m_Request + "\r\n";
  // concat body
  m_Request += m_Body;
  return true;
}

bool HTTPMessage::ExtractIncomingRequest() {
  m_ErrorResponse = HTTPResponse(HTTPResponseCodes::status_t::bad_request);
  LOG(debug)
    << "HTTPProxyHandler: method is: " << m_Method
    << " request is: " << m_URI.get(URI_t::URL);
  // Ugly hack to work around cppnetlib-uri failing to parse
  //     remaining URI components if no scheme is present
  // TODO(oneiric): patch cppnetlib-uri to parse with no scheme
  if (m_URI.get(URI_t::URL).find("http:") == std::string::npos
      && m_URI.get(URI_t::URL).find("https:") == std::string::npos)
    m_URI.set(URI_t::URL, "http:" + m_URI.get(URI_t::URL));
  boost::network::uri::uri uri(m_URI.get(URI_t::URL));
  m_URI.set(URI_t::Host, uri.host())
      .set(URI_t::Port, uri.port())
      .set(URI_t::Path, uri.path())
      .set(URI_t::Query, uri.query())
      .set(URI_t::Fragment, uri.fragment());
  LOG(debug) << "HTTPProxyHandler:"
             << " server is: " << m_URI.get(URI_t::Host)
             << ", port is: " << m_URI.get(URI_t::Port)
             << ", path is: " << m_URI.get(URI_t::Path)
             << ", query is: " << m_URI.get(URI_t::Query)
             << ", fragment is: " << m_URI.get(URI_t::Fragment);
  // Check for HTTP version
  if (m_Version != "HTTP/1.0" && m_Version != "HTTP/1.1") {
    LOG(error) << "HTTPProxyHandler: unsupported version: " << m_Version;
    m_ErrorResponse
        = HTTPResponse(HTTPResponseCodes::status_t::http_not_supported);
    return false;
  }
  m_ErrorResponse = HTTPResponse(HTTPResponseCodes::status_t::ok);
  return true;
}

bool HTTPMessage::HandleJumpService()
{
  // TODO(anonimal): add support for remaining services /
  // rewrite this function

  try
  {
    HandleJumpQuery();
    LOG(debug) << "HTTPProxyHandler: jump service for " << m_URI.get(URI_t::Host) 
               << " found at " << m_Base64Destination;
    // Remove jump service query from request URL
    m_URI.set(
        URI_t::URL,
        m_URI.get(URI_t::Host) + ":" + m_URI.get(URI_t::Port) + "/"
            + m_URI.get(URI_t::Path));
  }
  catch (...)
  {
    kovri::core::Exception ex;
    ex.Dispatch(__func__);
    return false;
  }
  return true;
}

void HTTPMessage::HandleJumpQuery()
{
  std::size_t pos = m_URI.get(URI_t::Query).find('=');
  if (pos != std::string::npos)
  {
    std::string const t_query = m_URI.get(URI_t::Query).substr(0, pos++);
    if (t_query == m_JumpService.Kovri || t_query == m_JumpService.I2P)
    {
      m_Base64Destination =
          boost::network::uri::decoded(m_URI.get(URI_t::Query).substr(pos));
      if (!m_Base64Destination.empty())
        return; // valid jump service query found

      // If we are this far, we have a bad jump service request.
      // Otherwise, we could still have a valid HTTP proxy request.
      m_ErrorResponse = HTTPResponse(HTTPResponseCodes::bad_request);
    }
  }
  throw std::runtime_error("HTTPProxy: invalid jump service request");
}

bool HTTPMessage::SaveJumpServiceAddress()
{
  try
    {
      LOG(debug) << "HTTPProxyHandler: inserting " << m_Address
                 << " into address book";
      kovri::client::context.GetAddressBook().InsertAddressIntoStorage(
          m_Address, m_Base64Destination);
    }
  catch (...)
    {
      core::Exception ex;
      ex.Dispatch("HTTPProxyHandler: unable to insert address into storage");
      return false;
    }

  return true;
}

/* All hope is lost beyond this point */
void HTTPProxyHandler::HTTPRequestFailed() {
  boost::asio::async_write(
      *m_Socket,
      boost::asio::buffer(
          m_Protocol.m_ErrorResponse.m_Response,
          m_Protocol.m_ErrorResponse.m_Response.size()),
      std::bind(
          &HTTPProxyHandler::SentHTTPFailed,
          shared_from_this(),
          std::placeholders::_1));
}

void HTTPProxyHandler::SentHTTPFailed(const boost::system::error_code& ecode) {
  if (!ecode) {
    Terminate();
  } else {
    LOG(error)
      << "HTTPProxyHandler: closing socket after sending failure: '"
      << ecode.message() << "'";
    Terminate();
  }
}

void HTTPProxyHandler::Terminate() {
  if (Kill())
    return;
  if (m_Socket) {
    LOG(debug) << "HTTPProxyHandler: terminating";
    m_Socket->close();
    m_Socket = nullptr;
  }
  Done(shared_from_this());
}

const std::string& URI::get(const URI_t& part) const noexcept
{
  switch (part)
  {
    case URI_t::Host:
      return m_Host;
    case URI_t::Port:
      return m_Port;
    case URI_t::Path:
      return m_Path;
    case URI_t::Query:
      return m_Query;
    case URI_t::Fragment:
      return m_Fragment;
    case URI_t::URL:
      return m_URL;
    default:
      LOG(error) << "HTTPProxy: unknown URI part";
      return m_Unknown;
  }
}

URI& URI::set(const URI_t& part, const std::string& value)
{
  switch (part)
  {
    case URI_t::Host:
      m_Host = value;
      break;
    case URI_t::Port:
      m_Port = value;
      break;
    case URI_t::Path:
      m_Path = value;
      break;
    case URI_t::Query:
      m_Query = value;
      break;
    case URI_t::Fragment:
      m_Fragment = value;
      break;
    case URI_t::URL:
      m_URL = value;
      break;
    default:
      LOG(error) << "HTTPProxy: unknown URI part";
      break;
  }
  return *this;
}
}  // namespace client
}  // namespace kovri
