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

#include "client/util/http.h"

#include <exception>
#include <functional>
#include <memory>
#include <vector>

#include "client/address_book/impl.h"
#include "client/context.h"

#include "core/router/context.h"

#include "core/util/byte_stream.h"
#include "core/util/filesystem.h"

namespace kovri {
namespace client {

namespace http = boost::beast::http;

// TODO(unassigned): currently unused but will be useful
// without needing to create a new object for each given URI
bool HTTP::Download(
    const std::string& uri) {
  SetURI(uri);
  return Download();
}

bool HTTP::Download() {
  if (!GetURI().is_valid()) {
    LOG(error) << "URI: invalid URI";
    return false;
  }
  // TODO(anonimal): ideally, we simply swapout the request/response handler
  // with cpp-netlib so we don't need two separate functions
  if (HostIsI2P())
    {
      AmendURI();
      return DownloadViaI2P();
    }
  return DownloadViaClearnet();
}

bool HTTP::HostIsI2P() const
{
  const auto uri = GetURI();
  if (!(uri.host().substr(uri.host().size() - 4) == ".i2p"))
    return false;
  return true;
}

void HTTP::AmendURI()
{
  auto uri = GetURI();
  if (!uri.port().empty())
    return;
  // We must assign a port if none was assigned (for internal reasons)
  LOG(trace) << "HTTP : Amending URI";
  std::string port;
  if (uri.scheme() == "https")
    port.assign("443");
  else
    port.assign("80");
  // If user supplied user:password, we must append @
  std::string user_info;
  if (!uri.user_info().empty())
    user_info.assign(uri.user_info() + "@");
  // TODO(anonimal): easier way with cpp-netlib?
  std::string new_uri(
      uri.scheme() + "://" + user_info
      + uri.host() + ":" + port
      + uri.path() + uri.query() + uri.fragment());
  SetURI(new_uri);
}

bool HTTP::DownloadViaClearnet() {
  using boost::asio::ip::tcp;
  namespace ssl = boost::asio::ssl;

  const auto uri = GetURI();
  // Create and set options
  LOG(debug) << "HTTP: Download Clearnet with timeout : "
             << kovri::core::GetType(Timeout::Request);
  // Ensure that we only download from explicit TLS-enabled hosts
  if (!core::context.GetOpts()["disable-https"].as<bool>())
    {
      try
        {
          const std::string cert = uri.host() + ".crt";
          const boost::filesystem::path cert_path =
              core::GetPath(core::Path::TLS) / cert;
          if (!boost::filesystem::exists(cert_path))
            {
              LOG(error) << "HTTP: certificate unavailable: " << cert_path;
              return false;
            }
          LOG(trace) << "HTTP: Cert exists : " << cert_path;
          boost::asio::io_context ioc;

          ssl::context ctx(ssl::context::tlsv12_client);
          ctx.load_verify_file(cert_path.string());
          ctx.set_verify_mode(ssl::verify_peer);
          ctx.set_options(
              ssl::context::no_sslv2 | ssl::context::no_sslv3
              | ssl::context::no_tlsv1 | ssl::context::no_tlsv1_1
              | ssl::context::default_workarounds
              | ssl::context::single_dh_use);

          if (uri.path() != GetPreviousPath())
            SetPath(uri.path());

          // These objects perform our I/O
          tcp::resolver resolver(ioc);
          ssl::stream<tcp::socket> stream(ioc, ctx);

          // Set SNI Hostname (many hosts need this to handshake successfully)
          if (!SSL_set_tlsext_host_name(
                  stream.native_handle(), uri.host().c_str()))
            {
              boost::system::error_code ec{
                  static_cast<int>(::ERR_get_error()),
                  boost::asio::error::get_ssl_category()};
              throw boost::system::system_error{ec};
            }

          SSL_set_cipher_list(
              stream.native_handle(),
              "DH+AESGCM:DH+AESGCM"
              ":ECDH+AES256:DH+AES256"
              ":ECDH+AES128:DH+AES"
              ":RSA+AESGCM:RSA+AES"
              ":!aNULL:!MD5");

          // Look up the domain name
          const auto port = uri.port().empty() ? "443" : uri.port();
          LOG(debug) << "HTTP: resolving host: " << uri.host()
                     << " port: " << port;
          const auto results = resolver.resolve(uri.host(), port);

          boost::asio::connect(
              stream.next_layer(), results.begin(), results.end());
          stream.handshake(ssl::stream_base::client);

          // Set up an HTTP GET request message
          http::request<http::dynamic_body> req(http::verb::get, uri.path(), 11);
          req.set(http::field::host, uri.host());
          req.set(http::field::user_agent, "Wget/1.11.4");
          req.set(http::field::etag, GetPreviousETag());
          req.set(http::field::last_modified, GetPreviousLastModified());
          req.set(
              http::field::timeout,
              static_cast<std::uint8_t>(Timeout::Request));
          LOG(trace) << "HTTP: Request: "
                     << kovri::core::LogNetMessageToString(req);

          // Send the HTTP request to the remote host & process response
          http::write(stream, req);
          boost::beast::flat_buffer buffer;
          http::response<http::dynamic_body> res;
          http::read(stream, buffer, res);
          LOG(trace) << "HTTP: Response: "
                     << kovri::core::LogNetMessageToString(res);
          std::ostringstream os;
          const auto header = res.base();
          switch (res.result())
            {
              case http::status::ok:
                if (header["ETag"] != GetPreviousETag())
                  SetETag(header["ETag"].to_string());

                if (header["Last-Modified"] != GetPreviousLastModified())
                  SetLastModified(header["Last-Modified"].to_string());

                os << boost::beast::buffers(res.body().data());
                SetDownloadedContents(os.str());
                break;
              case http::status::not_modified:
                LOG(info) << "HTTP: no updates available from " << uri.host();
                break;
              default:
                LOG(warning) << "HTTP: response code: " << res.result();
                return false;
            }
        }
      catch (const std::exception& ex)
        {
          LOG(error) << "HTTP: unable to complete download: " << ex.what();
          return false;
        }
    }
  else
    {
      LOG(error) << "HTTP: not using HTTPS";
      return false;
    }
  return true;
}

// TODO(anonimal): cpp-netlib refactor: request/response handler
bool HTTP::DownloadViaI2P()
{
  LOG(debug) << "HTTP: Download via I2P";
  // Clear buffers (for when we're only using a single instance)
  m_Request.clear();
  m_Response.clear();
  // Get URI
  const auto uri = GetURI();
  // Reference the only instantiated address book instance in the singleton client context
  auto& address_book = kovri::client::context.GetAddressBook();
  // For identity hash of URI host
  kovri::core::IdentHash ident;
  // Get URI host's ident hash then find its lease-set
  if (address_book.CheckAddressIdentHashFound(uri.host(), ident)
      && address_book.GetSharedLocalDestination()) {
    std::condition_variable new_data_received;
    std::mutex new_data_received_mutex;
    auto lease_set = address_book.GetSharedLocalDestination()->FindLeaseSet(ident);
    if (!lease_set)
      {
        LOG(debug) << "HTTP: Lease-set not available, request";
        std::unique_lock<std::mutex> lock(new_data_received_mutex);
        address_book.GetSharedLocalDestination()->RequestDestination(
            ident,
            [&new_data_received,
             &lease_set](std::shared_ptr<kovri::core::LeaseSet> ls) {
              lease_set = ls;
              new_data_received.notify_all();
            });
        // TODO(anonimal): request times need to be more consistent.
        //   In testing, even after integration, results vary dramatically.
        //   This could be a router issue or something amiss during the refactor.
        if (new_data_received.wait_for(
                lock,
                std::chrono::seconds(
                    static_cast<std::uint8_t>(Timeout::Request)))
            == std::cv_status::timeout)
          LOG(error) << "HTTP: lease-set request timeout expired";
      }
    // Test against requested lease-set
    if (!lease_set) {
      LOG(error) << "HTTP: lease-set for address " << uri.host() << " not found";
    } else {
      PrepareI2PRequest();  // TODO(anonimal): remove after refactor
      // Send request
      auto stream =
        kovri::client::context.GetAddressBook().GetSharedLocalDestination()->CreateStream(
            lease_set,
            std::stoi(uri.port()));
      stream->Send(
          reinterpret_cast<const std::uint8_t *>(m_Request.str().c_str()),
          m_Request.str().length());
      // Receive response
      std::array<std::uint8_t, 4096> buf;  // Arbitrary buffer size
      bool end_of_data = false;
      while (!end_of_data) {
        stream->AsyncReceive(
            boost::asio::buffer(
              buf.data(),
              buf.size()),
            [&](const boost::system::error_code& ecode,
              std::size_t bytes_transferred) {
                if (bytes_transferred)
                  m_Response.write(
                      reinterpret_cast<char *>(buf.data()),
                      bytes_transferred);
                if (ecode == boost::asio::error::timed_out || !stream->IsOpen())
                  end_of_data = true;
                new_data_received.notify_all();
              },
            static_cast<std::uint8_t>(Timeout::Receive));
        std::unique_lock<std::mutex> lock(new_data_received_mutex);
        // Check if we timeout
        if (new_data_received.wait_for(
                lock,
                std::chrono::seconds(
                    static_cast<std::uint8_t>(Timeout::Request)))
            == std::cv_status::timeout)
          LOG(error) << "HTTP: in-net timeout expired";
      }
      // Process remaining buffer
      while (std::size_t len = stream->ReadSome(buf.data(), buf.size()))
        m_Response.write(reinterpret_cast<char *>(buf.data()), len);
    }
  } else {
    LOG(error) << "HTTP: can't resolve I2P address: " << uri.host();
    return false;
  }
  return ProcessI2PResponse();  // TODO(anonimal): remove after refactor
}

// TODO(anonimal): remove after refactor
void HTTP::PrepareI2PRequest() {
  // Create header
  const auto uri = GetURI();
  std::string header =
    "GET " + uri.path() + " HTTP/1.1\r\n" +
    "Host: " + uri.host() + "\r\n" +
    "Accept: */*\r\n" +
    "User-Agent: Wget/1.11.4\r\n" +
    "Connection: Close\r\n";
  // Add header to request
  m_Request << header;
  // Check fields
  if (!GetPreviousETag().empty())  // Send previously set ETag if available
    m_Request << "If-None-Match" << ": \"" << GetPreviousETag() << "\"\r\n";
  if (!GetPreviousLastModified().empty())  // Send previously set Last-Modified if available
    m_Request << "If-Modified-Since" << ": " << GetPreviousLastModified() << "\r\n";
  m_Request << "\r\n";  // End of header
}

// TODO(anonimal): remove after refactor
bool HTTP::ProcessI2PResponse() {
  std::string http_version;
  std::uint16_t response_code = 0;
  m_Response >> http_version;
  m_Response >> response_code;
  if (http::int_to_status(response_code) == http::status::ok)
    {
      bool is_chunked = false;
      std::string header, status_message;
      std::getline(m_Response, status_message);
      // Read response until end of header (new line)
      while (std::getline(m_Response, header) && header != "\r")
        {
          const auto colon = header.find(':');
          if (colon != std::string::npos)
            {
              std::string field = header.substr(0, colon);
              header.resize(header.length() - 1);  // delete \r
              // We currently don't differentiate between strong or weak ETags
              // We currently only care if an ETag is present
              if (field == "ETag")
                SetETag(header.substr(colon + 1));
              else if (field == "Last-Modified")
                SetLastModified(header.substr(colon + 1));
              else if (field == "Transfer-Encoding")
                is_chunked =
                    !header.compare(colon + 1, std::string::npos, "chunked");
            }
        }
      // Get content after header
      std::stringstream content;
      while (std::getline(m_Response, header))
        {
          // TODO(anonimal): this can be improved but since we
          // won't need this after the refactor, it 'works' for now
          const auto colon = header.find(':');
          if (colon != std::string::npos)
            continue;
          else
            content << header << std::endl;
        }
      // Test if response is chunked / save downloaded contents
      if (!content.eof())
        {
          if (is_chunked)
            {
              std::stringstream merged;
              MergeI2PChunkedResponse(content, merged);
              SetDownloadedContents(merged.str());
            }
          else
            {
              SetDownloadedContents(content.str());
            }
        }
    }
  else if (http::int_to_status(response_code) == http::status::not_modified)
    {
      LOG(info) << "HTTP: no new updates available from " << GetURI().host();
    }
  else
    {
      LOG(warning) << "HTTP: response code: " << response_code;
      return false;
    }
  return true;
}

// TODO(anonimal): remove after refactor
// Note: Transfer-Encoding is handled automatically by cpp-netlib
void HTTP::MergeI2PChunkedResponse(
    std::istream& response,
    std::ostream& merged) {
  // Read in hex value of length
  std::string hex;
  while (std::getline(response, hex)) {
    std::istringstream hex_len(hex);
    // Convert to integer value
    std::size_t len(0);
    // CID 146759 complains of TAINTED_SCALAR but we're guaranteed a useful value because:
    // 1. The HTTP response is chunked and prepends hex value of chunk size
    // 2. If length is null, we'll break before new buffer
    // Note: verifying the validity of stated chunk size against ensuing content size
    // will require more code (so, better to complete cpp-netlib refactor instead)
    if (!(hex_len >> std::hex >> len).fail()) {
      // If last chunk, break
      if (!len)
        break;
      // Read in chunk content of chunk size
      auto buf = std::make_unique<char[]>(len);
      response.read(buf.get(), len);
      merged.write(buf.get(), len);
      std::getline(response, hex);  // read \r\n after chunk
    } else {
      LOG(error) << "HTTP: stream error, unable to read line from chunked response";
      break;
    }
  }
}

}  // namespace client
}  // namespace kovri
