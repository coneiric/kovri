/**
 * Copyright (c) 2015-2018, The Kovri I2P Router Project
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SRC_UTIL_I2PCONTROL_CLIENT_H_
#define SRC_UTIL_I2PCONTROL_CLIENT_H_

#include <memory>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "client/api/i2p_control/data.h"

// Note: Credit goes to EinMByte.
// This is heavily inspired from i2pcontrol_client.h in qtoopie.

namespace kovri
{
namespace client
{
/**
 * @brief Provides functiality to communicate with an I2PControl server over HTTP.
 */
class I2PControlClient final
    : public std::enable_shared_from_this<I2PControlClient>
{
 public:
  // @brief I2PControlClient constructor
  // @param url the location of the HTTP document providing the JSONRPC API.
  explicit I2PControlClient(boost::asio::io_service&);

  // @brief Starts the ::I2PControlClient.
  // @param callback the function to be called when the client is connected
  // @throw std::exception on error
  void AsyncConnect(
      std::function<void(std::unique_ptr<I2PControlResponse>)> callback);

  // @brief Sends a request to the I2PControl server.
  // @details automatically sets the token for non auth request
  // @details automatically reconnects if token expired
  // @param callback the function to be called when the request has finished
  // @throw std::exception on error
  void AsyncSendRequest(
      std::shared_ptr<I2PControlRequest> request,
      std::function<void(std::unique_ptr<I2PControlResponse>)> callback);

  // @brief Sets the host of the i2p router
  // @param host ip or hostname to connect to
  // @throw std::bad_alloc when not enough memory
  void SetHost(const std::string& host);

  // @brief Sets the port of the i2p router
  // @param port port to connect to
  // @throw std::bad_alloc when not enough memory
  void SetPort(std::uint16_t port);

  // @brief Sets the password of the i2p router
  // @param password password to use
  // @throw std::bad_alloc when not enough memory
  void SetPassword(const std::string& password);

 private:
  // For convenience
  typedef I2PControlResponse Response;
  typedef I2PControlRequest Request;
  typedef Response::ErrorCode ErrorCode;
  typedef Request::Method Method;

  // @brief Effectively sends request without any modification
  // @param callback Callback to call
  // @throw std::exception on error
  void ProcessAsyncSendRequest(
      std::function<void(std::unique_ptr<Response>)> callback);

  void HandleAsyncResolve(
      const boost::system::error_code& ec,
      const boost::asio::ip::tcp::resolver::results_type& results);

  void HandleAsyncConnect(const boost::system::error_code& ec);

  void HandleAsyncWrite(const boost::system::error_code& ec, const std::size_t);

  // @brief Concatenate chunks as received and call callback when finished receiving
  // @param error Error as returned by underlying lib
  // @param bytes_transferred Bytes transferred reading network response
  // @throw boost::system::error on error
  void HandleHTTPResponse(
      boost::system::error_code const& error,
      std::size_t const bytes_transferred);

  std::string m_Host{"127.0.0.1"};
  std::uint16_t m_Port{7650};
  std::string m_Password{"itoopie"};
  std::string m_Token{};
  std::shared_ptr<boost::asio::io_service> m_Service;
  std::function<void(std::unique_ptr<Response>)> m_Callback;
  std::shared_ptr<Request> m_Request;
  boost::beast::flat_buffer m_Buffer;
  boost::beast::http::request<boost::beast::http::string_body> m_HTTPRequest;
  boost::beast::http::response<boost::beast::http::dynamic_body> m_HTTPResponse;
  boost::asio::ip::tcp::resolver m_Resolver;
  boost::asio::ip::tcp::socket m_Socket;
};

}  // namespace client
}  // namespace kovri

#endif  // SRC_UTIL_I2PCONTROL_CLIENT_H_
