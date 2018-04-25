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

#ifndef SRC_CLIENT_PROXY_HTTP_H_
#define SRC_CLIENT_PROXY_HTTP_H_

#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/asio.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

#include "client/destination.h"
#include "client/service.h"

namespace kovri {
namespace client {

struct HTTPResponseCodes{
  enum status_t {
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    partial_content = 206,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    not_supported = 405,
    not_acceptable = 406,
    request_timeout = 408,
    precondition_failed = 412,
    unsatisfiable_range = 416,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503,
    http_not_supported = 505,
    space_unavailable = 507
  };


  static char const* status_message(status_t status) {
    static char const ok_[] = "OK", created_[] = "Created",
                      accepted_[] = "Accepted", no_content_[] = "No Content",
                      multiple_choices_[] = "Multiple Choices",
                      moved_permanently_[] = "Moved Permanently",
                      moved_temporarily_[] = "Moved Temporarily",
                      not_modified_[] = "Not Modified",
                      bad_request_[] = "Bad Request",
                      unauthorized_[] = "Unauthorized",
                      forbidden_[] = "Fobidden", not_found_[] = "Not Found",
                      not_supported_[] = "Not Supported",
                      not_acceptable_[] = "Not Acceptable",
                      internal_server_error_[] = "Internal Server Error",
                      not_implemented_[] = "Not Implemented",
                      bad_gateway_[] = "Bad Gateway",
                      service_unavailable_[] = "Service Unavailable",
                      unknown_[] = "Unknown",
                      partial_content_[] = "Partial Content",
                      request_timeout_[] = "Request Timeout",
                      precondition_failed_[] = "Precondition Failed",
                      http_not_supported_[]= "HTTP Version Not Supported",
                      unsatisfiable_range_[] =
                          "Requested Range Not Satisfiable",
                      space_unavailable_[] =
                          "Insufficient Space to Store Resource";
    switch (status) {
      case ok:
        return ok_;
      case created:
        return created_;
      case accepted:
        return accepted_;
      case no_content:
        return no_content_;
      case multiple_choices:
        return multiple_choices_;
      case moved_permanently:
        return moved_permanently_;
      case moved_temporarily:
        return moved_temporarily_;
      case not_modified:
        return not_modified_;
      case bad_request:
        return bad_request_;
      case unauthorized:
        return unauthorized_;
      case forbidden:
        return forbidden_;
      case not_found:
        return not_found_;
      case not_supported:
        return not_supported_;
      case not_acceptable:
        return not_acceptable_;
      case internal_server_error:
        return internal_server_error_;
      case not_implemented:
        return not_implemented_;
      case bad_gateway:
        return bad_gateway_;
      case service_unavailable:
        return service_unavailable_;
      case partial_content:
        return partial_content_;
      case request_timeout:
        return request_timeout_;
      case precondition_failed:
        return precondition_failed_;
      case unsatisfiable_range:
        return unsatisfiable_range_;
      case http_not_supported:
        return http_not_supported_;
      case space_unavailable:
        return space_unavailable_;
      default:
        return unknown_;
    }
  }
};

/// @brief URI component parts
enum struct URI_t
{
  Host,
  Port,
  Path,
  Query,
  Fragment,
  URL,
};
/// @class URI
/// @brief Container for HTTP URI components
class URI
{
 public:
  URI() : m_Unknown("Unknown URI part") {}

  /// @brief Create URI from components
  URI(const std::string& host,
      const std::string& port,
      const std::string& path,
      const std::string& query,
      const std::string& fragment)
      : m_Host(host),
        m_Port(port),
        m_Path(path),
        m_Query(query),
        m_Fragment(fragment),
        m_URL(host),
        m_Unknown("Unknown URI part")
  {
    if (m_Port.empty())
      m_Port = "80";

    m_URL.append(":" + port);
    if (!m_Path.empty())
      m_URL.append("/" + m_Path);
    if (!m_Query.empty())
      m_URL.append("?" + m_Query);
    if (!m_Fragment.empty())
      m_URL.append("#" + m_Fragment);
  }

  /// @brief Access URI component parts
  /// @return The URI component part
  const std::string& get(const URI_t& part) const noexcept;

  /// @brief Set URI component parts
  /// @return A mutable reference to this URI instance
  URI& set(const URI_t& part, const std::string& value);

 private:
  std::string m_Host, m_Port, m_Path, m_Query, m_Fragment, m_URL, m_Unknown;
};

/// @class HTTPResponse
/// @brief response for http error messages
class HTTPResponse{
 public:
  std::string m_Response;
  explicit HTTPResponse(HTTPResponseCodes::status_t status){
    std::string htmlbody = "<html>";
    htmlbody+="<head>";
    htmlbody+="<title>HTTP Error</title>";
    htmlbody+="</head>";
    htmlbody+="<body>";
    htmlbody+="HTTP Error " + std::to_string(status) + " ";
    htmlbody+=HTTPResponseCodes::status_message(status);
    if (status == HTTPResponseCodes::status_t::service_unavailable) {
      htmlbody+=" Please wait for the router to integrate";
    }
    htmlbody+="</body>";
    htmlbody+="</html>";

    m_Response =
    "HTTP/1.0 " + std::to_string(status) + " " +
    HTTPResponseCodes::status_message(status)+"\r\n" +
    "Content-type: text/html;charset=UTF-8\r\n" +
    "Content-Encoding: UTF-8\r\n" +
    "Content-length:" + std::to_string(htmlbody.size()) + "\r\n\r\n" + htmlbody;
  }
};

/// @class HTTPMessage
/// @brief defines protocol; and read from socket algorithm
class HTTPMessage : public std::enable_shared_from_this<HTTPMessage>{
 public:
  HTTPMessage() : m_ErrorResponse(HTTPResponseCodes::status_t::ok) {}

  /// @brief loads variables in class;
  /// @param buf
  /// @param len
  /// return bool
  // TODO(oneiric): convert to void return, throw on error
  bool HandleData(const std::string  &buf);

  /// @brief Parses URI for base64 destination
  /// @return true on success
  // TODO(oneiric): convert to void return, throw on error
  bool HandleJumpService();

  /// @brief Performs regex, sets address/port/path, validates version
  ///   on request sent from user
  /// @return true on success
  // TODO(oneiric): convert to void return, throw on error
  bool ExtractIncomingRequest();

  /// @brief Processes original request: extracts, validates,
  ///   calls jump service, appends original request
  /// @param save_address Save address to disk
  /// @return true on success
  // TODO(unassigned): save address param is a hack until storage is separated from message
  bool CreateHTTPRequest(const bool save_address = true);

  enum msg_t {
    response,
    request
  };
  // TODO(oneiric): enumerate
  const unsigned int HEADERBODY_LEN = 2;
  const unsigned int REQUESTLINE_HEADERS_MIN = 1;
  /// @brief Jump service query string container
  struct JumpService
  {
    JumpService() : Kovri("kovrijumpservice"), I2P("i2paddresshelper") {}
    const std::string Kovri, I2P;
  };
  // TODO(oneiric): create struct to hold HTTP parts
  // TODO(oneiric): make data members private,
  //                  expose with member functions
  std::string m_RequestLine,
              m_HeaderLine,
              m_Request,
              m_Body,
              m_Method,
              m_Version,
              m_UserAgent;
  HTTPResponse m_ErrorResponse;
  std::vector<std::string> m_Headers;
  boost::asio::streambuf m_Buffer;
  boost::asio::streambuf m_BodyBuffer;
  // TODO(oneiric): why isn't this a std::map?
  std::vector<std::pair<std::string, std::string>> m_HeaderMap;
  // TODO(oneiric): create struct in address book to represent an address book entry
  std::string m_Address, m_Base64Destination;
  URI m_URI;
  JumpService m_JumpService;

 private:
  /// @brief Checks request for valid jump service query & extracts Base64 address
  /// @notes Throws if request does not have a valid jump service query
  void HandleJumpQuery();
  /// @brief Checks if request is a valid jump service request
  /// @return Index of jump service helper sub-string, 0 indicates failure
  // TODO(oneiric): convert to void return, throw on error
  std::size_t IsJumpServiceRequest() const;

  /// @brief Extracts & url-decodes base64 destination from URL
  /// @param pos Index of jump service helper sub-string in URL
  /// @return True on success
  // TODO(oneiric): convert to void return, throw on error
  bool ExtractBase64Destination(std::size_t const pos);

  /// @brief Saves found address in address book
  /// @return True on success
  // TODO(oneiric): convert to async proxy handler
  bool SaveJumpServiceAddress();
};
/// @class HTTPProxyServer
/// setup asio service
class HTTPProxyServer
    : public kovri::client::TCPIPAcceptor {
 public:
  /// @param name Proxy server service name
  /// @param address Proxy binding address
  /// @param port Proxy binding port
  /// @param local_destination Client destination
  //
  HTTPProxyServer(
      const std::string& name,
      const std::string& address,
      std::uint16_t port,
      std::shared_ptr<kovri::client::ClientDestination> local_destination
              = nullptr);

  ~HTTPProxyServer() {}

  /// @brief Implements TCPIPAcceptor
  std::shared_ptr<kovri::client::I2PServiceHandler> CreateHandler(
      std::shared_ptr<boost::asio::ip::tcp::socket> socket);

  /// @brief Gets name of proxy service
  /// @return Name of proxy service
  std::string GetName() const {
    return m_Name;
  }

 private:
  std::string m_Name;
};

typedef HTTPProxyServer HTTPProxy;

/// @class HTTPProxyHandler
/// @brief setup handler for asio service
/// each service needs a handler
class HTTPProxyHandler
    : public kovri::client::I2PServiceHandler,
      public std::enable_shared_from_this<HTTPProxyHandler> {
 public:
  HTTPMessage m_Protocol;
  /// @param parent Pointer to parent server
  /// @param socket Shared pointer to bound socket
  HTTPProxyHandler(
      HTTPProxyServer* parent,
      std::shared_ptr<boost::asio::ip::tcp::socket> socket)
      : I2PServiceHandler(parent),
        m_Socket(socket) {
        }

  ~HTTPProxyHandler() {
    Terminate();
  }
  void CreateStream();
  /// @brief reads data sent to proxy server
  /// virtual function;
  /// handle reading the protocol
  void Handle();

  /// @brief Handles buffer received from socket if Handle successful
  void HandleSockRecv(const boost::system::error_code & error,
    std::size_t bytes_transfered);

 private:
  /*! @brief read from a socket
   *
   *AsyncSockRead             - perform async read
   *  -AsyncHandleReadHeaders - handle read header info
   *    -HTTPMessage::HandleData   - handle header info
   *    -HandleSockRecv       - read body if needed
   *      -CreateStream
   *        -HTTPMessage::CreateHTTPStreamRequest -  create stream request
   *        -HandleStreamRequestComplete           -  connect to i2p tunnel
   */
  void AsyncSockRead(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
  // @brief handle read data
  void AsyncHandleReadHeaders(const boost::system::error_code & error,
    std::size_t bytes_transferred);


  /// @brief Handles stream created by service through proxy handler
  void HandleStreamRequestComplete(
      std::shared_ptr<kovri::client::Stream> stream);

  /// @brief Generic request failure handler
  void HTTPRequestFailed();

  /// @brief Tests if our sent response to browser has failed
  /// @param ecode Boost error code
  void SentHTTPFailed(
      const boost::system::error_code& ecode);

  /// @brief Kills handler for Service, closes socket
  void Terminate();

 private:
  /// @enum Size::Size
  /// @brief Constant for size
  enum Size : const std::uint16_t {
    /// @var buffer
    /// @brief Buffer size for async sock read
    buffer = 8192
  };

  /// @var m_Buffer
  /// @brief Buffer for async socket read
//  std::array<std::uint8_t, static_cast<std::size_t>(Size::buffer)> m_Buffer;
  std::shared_ptr<boost::asio::ip::tcp::socket> m_Socket;
};

}  // namespace client
}  // namespace kovri

#endif  // SRC_CLIENT_PROXY_HTTP_H_
