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

#include "client/address_book/impl.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/asio.hpp>
#include <boost/network/uri.hpp>

#include <array>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <utility>

#include "core/crypto/rand.h"

#include "core/router/net_db/impl.h"

namespace kovri {
namespace client {

/**
 * VOCABULARY:
 *
 * Publisher:
 *   Entity that publishes a 'subscription'; usually from their website
 *
 * Subscription:
 *   Text file containing a list of TLD .i2p hosts paired with base64 address
 *   (see I2P naming and address book specification)
 *
 * Subscriber:
 *   Entity that subscribes (downloads + processes) a publisher's subscription
 *
 * NARRATIVE:
 *
 * 1. A trusted publisher publishes a subscription
 * 2. Subscription contains spec-defined host=base64 pairing; one host per line
 * 3. Kovri checks if we have a list of publishers; if not, uses default
 * 4. Kovri hooks its subscriber to into an asio timer that regularly
 *    updates a subscription (only downloads new subscription if Etag is set)
 * 5. If available, kovri loads default packaged subscription before downloading
 * 6. Kovri's subscriber checks if downloads subscription/updated subscription
 * 7. Kovri saves subscription to storage
 * 8. Kovri repeats download ad infinitum with a timer based on specified constants
 */

void AddressBook::Start(
    std::shared_ptr<ClientDestination> local_destination) {
  // We need tunnels so we can download in-net
  if (!local_destination) {
    LOG(error)
      << "AddressBook: won't start: we need a client destination";
    return;
  }
  LOG(info) << "AddressBook: starting service";
  m_SharedLocalDestination = local_destination;
  m_SubscriberUpdateTimer =
    std::make_unique<boost::asio::deadline_timer>(
        m_SharedLocalDestination->GetService());
  m_SubscriberUpdateTimer->expires_from_now(
      boost::posix_time::minutes{static_cast<long>(SubscriberTimeout::InitialUpdate)});
  m_SubscriberUpdateTimer->async_wait(
      std::bind(
          &AddressBook::SubscriberUpdateTimer,
          this,
          std::placeholders::_1));
}

void AddressBook::SubscriberUpdateTimer(
    const boost::system::error_code& ecode) {
  LOG(debug) << "AddressBook: begin " << __func__;
  if (ecode) {
    LOG(error)
      << "AddressBook: " << __func__ << " exception: " << ecode.message();
    return;
  }
  LoadPublishers();
  if (m_SharedLocalDestination->IsReady())
      LoadSubscriptionFromPublisher();  // Attempt subscription download, timer updated internally
  else
    {  // Try again after timeout
      m_SubscriberUpdateTimer->expires_from_now(boost::posix_time::minutes{
          static_cast<long>(SubscriberTimeout::InitialRetry)});
      m_SubscriberUpdateTimer->async_wait(
          std::bind(
              &AddressBook::SubscriberUpdateTimer,
              this,
              std::placeholders::_1));
    }
}

void AddressBook::LoadPublishers() {
  if (!m_Storage)
    {
      LOG(debug) << "AddressBook: creating new storage instance";
      m_Storage = GetNewStorageInstance();
    }
  std::vector<HTTPStorage> publishers{};
  // Attempt to load publisher metadata from storage
  m_Storage->LoadPublishers(publishers);
  // Must iterate, cannot create std::unique_ptr from copyable type
  for (auto it = std::begin(publishers); it != std::end(publishers); ++it)
    {
      URI uri(it->GetPreviousURI());
      if (!uri.is_valid())
        {
          LOG(warning) << "AddressBook: invalid publisher URI";
          continue;
        }
      // Index subscribers by hostname and path for publishers with
      //   multiple subscriptions.
      std::string const sub_index = uri.host() + uri.path();
      if (m_Subscribers[sub_index])
        {
          if (it->GetPreviousLastModified()
              <= m_Subscribers.at(sub_index)->GetLastModified())
            continue;  // Latest publisher information already loaded
          else
            {  // Update with newer publisher information from storage
              m_Subscribers[sub_index] =
                  std::make_unique<AddressBookSubscriber>(
                      *this,
                      it->GetPreviousURI(),
                      it->GetPreviousETag(),
                      it->GetPreviousLastModified());
            }
        }
      else
        {
          m_Subscribers.emplace(
              sub_index,
              std::make_unique<AddressBookSubscriber>(
                  *this,
                  it->GetPreviousURI(),
                  it->GetPreviousETag(),
                  it->GetPreviousLastModified()));
        }
    }
}

void AddressBook::LoadSubscriptionFromPublisher() {
  // Ensure subscriber is loaded with publisher(s) before service "starts"
  // (Note: look at how client tunnels start)
  if (m_Subscribers.empty())
    LoadPublishers();
  // Ensure we have a storage instance ready
  if (!m_Storage) {
    LOG(debug) << "AddressBook: creating new storage instance";
    m_Storage = GetNewStorageInstance();
  }
  m_SubscriptionIsLoaded = false;
  // If addresses are unloaded, try local subscriptions
  if (m_DefaultAddresses.empty())
    {
      LoadLocalSubscription(Subscription::Default);
      LoadLocalSubscription(Subscription::User);
      LoadLocalSubscription(Subscription::Private);
      // If local subscription successfully loaded,
      //   prevent unecessarily downloading subscription on startup.
      m_SubscriptionIsLoaded = !m_DefaultAddresses.empty()
                               || !m_UserAddresses.empty()
                               || !m_PrivateAddresses.empty();
    }
  DownloadSubscription();
  HostsDownloadComplete(m_SubscriptionIsLoaded);
}

void AddressBook::LoadLocalSubscription(Subscription source)
{
  // Ensure we have a storage instance ready
  if (!m_Storage)
    {
      LOG(debug) << "AddressBook: creating new storage instance";
      m_Storage = GetNewStorageInstance();
    }
  bool loaded = false;
  // Attempt to load from storage catalog
  switch (source)
    {
      case Subscription::Default:
        loaded = m_DefaultAddresses.empty()
                     ? m_Storage->Load(m_DefaultAddresses, source)
                     : true;
        break;
      case Subscription::User:
        loaded = m_UserAddresses.empty()
                     ? m_Storage->Load(m_UserAddresses, source)
                     : true;
        break;
      case Subscription::Private:
        loaded = m_PrivateAddresses.empty()
                     ? m_Storage->Load(m_PrivateAddresses, source)
                     : true;
        break;
      default:
        throw std::invalid_argument("AddressBook: unknown subscription source");
    }
  if (!loaded)
    {
      auto filename = GetSubscriptionFilename(source);
      std::ifstream file(
          (core::GetPath(core::Path::AddressBook) / filename).string());
      if (file)
        SaveSubscription(file, source);
      else
        LOG(warning) << "AddressBook: unable to open subscription " << filename;
    }
}

void AddressBook::DownloadSubscription() {
  // Get number of available publishers (guaranteed > 0)
  auto publisher_count = m_Subscribers.size();
  LOG(debug)
    << "AddressBook: picking random subscription from total publisher count: "
    << publisher_count;
  try {
    for (auto& sub : m_Subscribers)
      {
        // Check if download was successful last round, or if a local
        //   subscription was loaded (we're on startup round).
        if (m_SubscriptionIsLoaded)
          break;
        // Check for updates from unloaded subscriptions
        if (sub.second && m_SharedLocalDestination)
          {
            // TODO(unassigned): remove check after GPG verfication implemented
            if (sub.second->GetURI() == GetDefaultPublisherURI())
              continue;  // Skip default subscription, if already loaded
            if (!sub.second->IsDownloading() && !sub.second->IsLoaded()
                && m_SharedLocalDestination->IsReady())
              {
                sub.second->DownloadSubscription();
                m_SubscriptionIsLoaded = sub.second->IsLoaded();
              }
          }
      }
  } catch (const std::exception& ex) {
    LOG(error) << "AddressBook: download subscription exception: " << ex.what();
  } catch (...) {
    LOG(error) << "AddressBook: download subscription unknown exception";
  }
}

void AddressBookSubscriber::DownloadSubscription() {
  // TODO(unassigned): exception handling
  LOG(debug) << "AddressBookSubscriber: creating thread for download";
  m_Downloading = true;
  try
    {
      std::thread download(
          &AddressBookSubscriber::DownloadSubscriptionImpl, this);
      if (download.joinable())
        download.join();
    }
  catch (const std::exception& ex)
    {
      LOG(error) << "AddressBook: download subscription exception: "
                 << ex.what();
    }
  catch (...)
    {
      LOG(error) << "AddressBook: download subscription unknown exception";
    }
  m_Downloading = false;
}

void AddressBookSubscriber::DownloadSubscriptionImpl() {
  // TODO(anonimal): ensure thread safety
  LOG(info)
    << "AddressBookSubscriber: downloading subscription "
    << m_HTTP.GetPreviousURI()
    << " ETag: " << m_HTTP.GetPreviousETag()
    << " Last-Modified: " << m_HTTP.GetPreviousLastModified();
  m_Loaded = m_HTTP.Download();
  if (m_Loaded) {
    std::stringstream stream(m_HTTP.GetDownloadedContents());
    // Determine where to save addresses
    auto source = m_HTTP.GetPreviousURI() == GetDefaultPublisherURI()
                      ? Subscription::Default
                      : Subscription::User;
    // Set loaded status based on successful save
    m_Loaded = m_Book.SaveSubscription(stream, source);
  }
}

void AddressBook::HostsDownloadComplete(
    bool success) {
  LOG(debug) << "AddressBook: subscription download complete";
  if (m_SubscriberUpdateTimer) {
    m_SubscriberUpdateTimer->expires_from_now(
        boost::posix_time::minutes{
        static_cast<long>(
            success
            ? SubscriberTimeout::ContinuousUpdate
            : SubscriberTimeout::ContinuousRetry)});
    m_SubscriberUpdateTimer->async_wait(
        std::bind(
            &AddressBook::SubscriberUpdateTimer,
            this,
            std::placeholders::_1));
  }
}

bool AddressBook::SaveSubscription(
    std::istream& stream,
    Subscription source) {
  std::unique_lock<std::mutex> lock(m_AddressBookMutex);
  // Ensure we have a storage instance ready
  if (!m_Storage)
    {
      LOG(debug) << "AddressBook: creating new storage instance";
      m_Storage = GetNewStorageInstance();
    }
  m_SubscriptionIsLoaded = false;  // TODO(anonimal): see TODO for multiple subscriptions
  try {
    auto addresses = ValidateSubscription(stream);
    if (!addresses.empty()) {
      LOG(debug) << "AddressBook: processing " << addresses.size() << " addresses";
      // Stream may be a file or downloaded stream.
      // Save hosts and matching identities
      std::map<std::string, kovri::core::IdentityEx> storage_addresses{};
      for (auto const& address : addresses) {
        const std::string& host = address.first;
        const auto& ident = address.second;
        try
          {
            // Only stores subscription lines for addresses not already loaded
            InsertAddress(host, ident.GetIdentHash(), source);
            // Save entry to storage map
            storage_addresses[host] = ident;
            // Save entry to ident_hash.b32 file for simple identity lookup
            m_Storage->AddAddress(ident);
          }
        catch (...)
          {
            m_Exception.Dispatch(__func__);
            continue;
          }
      }
      // Save a *list* of hosts within subscription to a catalog (CSV) file
      switch (source)
      {
        case Subscription::Default:
          m_Storage->Save(m_DefaultAddresses, source);
          break;
        case Subscription::User:
          m_Storage->Save(m_UserAddresses, source);
          break;
        case Subscription::Private:
          m_Storage->Save(m_PrivateAddresses, source);
          break;
        default:
          throw std::invalid_argument("AddressBook: unknown subscription source");
      }
      // Update storage subscription.
      if (!storage_addresses.empty())
        m_Storage->SaveSubscription(storage_addresses, source);
      m_SubscriptionIsLoaded = true;
    }
  } catch (...) {
    m_Exception.Dispatch(__func__);
  }
  return m_SubscriptionIsLoaded;
}

// TODO(anonimal): unit-test

const std::map<std::string, kovri::core::IdentityEx>
AddressBook::ValidateSubscription(std::istream& stream) {
  LOG(debug) << "AddressBook: validating subscription";
  // Map host to address identity
  std::map<std::string, kovri::core::IdentityEx> addresses;
  // To ensure valid Hostname=Base64Address
  std::string line;
  // To ensure valid hostname
  // Note: uncomment if this regexp fails on some locales (to not rely on [a-z])
  //const std::string alpha = "abcdefghijklmnopqrstuvwxyz";
  // TODO(unassigned): expand when we want to venture beyond the .i2p TLD
  // TODO(unassigned): IDN ccTLDs support?
  // TODO(unassigned): investigate alternatives to regex (maybe Boost.Spirit?)
  std::regex regex("(?=^.{1,253}$)(^(((?!-)[a-zA-Z0-9-]{1,63})|((?!-)[a-zA-Z0-9-]{1,63}\\.)+[a-zA-Z]+[(i2p)]{2,63})$)");
  try {
    // Read through stream, add to address book
    while (std::getline(stream, line)) {
      // Skip empty / too large lines
      if (line.empty() || line.size() > AddressBookSize::SubscriptionLine)
        continue;
      // Trim whitespace before and after line
      boost::trim(line);
      // Parse Hostname=Base64Address from line
      kovri::core::IdentityEx ident;
      std::size_t pos = line.find('=');
      if (pos != std::string::npos) {
        const std::string host = line.substr(0, pos++);
        const std::string addr = line.substr(pos);
        // Ensure only valid lines
        try
          {
            if (host.empty() || !std::regex_search(host, regex))
              throw std::runtime_error("AddressBook: invalid hostname");
            ident.FromBase64(addr);
          }
        catch (...)
          {
            m_Exception.Dispatch(__func__);
            LOG(warning) << "AddressBook: malformed address, skipping";
            continue;
          }
        addresses[host] = ident;  // Host is valid, save
      }
    }
  } catch (const std::exception& ex) {
    LOG(error) << "AddressBook: exception during validation: ", ex.what();
    addresses.clear();
  } catch (...) {
    throw std::runtime_error("AddressBook: unknown exception during validation");
  }
  return addresses;
}

// For in-net download only
bool AddressBook::CheckAddressIdentHashFound(
    const std::string& address,
    kovri::core::IdentHash& ident) {
  auto pos = address.find(".b32.i2p");
  if (pos != std::string::npos)
    {
      try
        {
          ident.FromBase32(address.substr(0, pos));
        }
      catch (...)
        {
          core::Exception ex;
          ex.Dispatch("AddressBook: invalid Base32 address");
          return false;
        }
      return true;
    }
  else {
    pos = address.find(".i2p");
    if (pos != std::string::npos) {
      auto ident_hash = GetLoadedAddressIdentHash(address);
      if (ident_hash) {
        ident = *ident_hash;
        return true;
      } else {
        return false;
      }
    }
  }
  // If not .b32, test for full base64 address
  kovri::core::IdentityEx dest;
  try
    {
      dest.FromBase64(address);
    }
  catch (...)
    {
      m_Exception.Dispatch(__func__);
      return false;
    }
  ident = dest.GetIdentHash();
  return true;
}

// For in-net download only
std::unique_ptr<const kovri::core::IdentHash> AddressBook::GetLoadedAddressIdentHash(
    const std::string& address) {
  auto found_addr = std::make_unique<const kovri::core::IdentHash>();
  // Check if the target address is in the supplied address map.
  auto find_address =
      [&found_addr,
       address](std::map<std::string, kovri::core::IdentHash> const& addr_map) {
        auto it = addr_map.find(address);
        if (it != addr_map.end())
          found_addr = std::make_unique<const kovri::core::IdentHash>(it->second);
        else
          found_addr = nullptr;
      };
  if (!m_SubscriptionIsLoaded)
    LoadSubscriptionFromPublisher();
  if (!m_DefaultAddresses.empty())
    find_address(m_DefaultAddresses);
  if (!found_addr && !m_UserAddresses.empty())
    find_address(m_UserAddresses);
  if (!found_addr && !m_PrivateAddresses.empty())
    find_address(m_PrivateAddresses);
  return found_addr;
}

void AddressBook::InsertAddress(
    const std::string& host,
    const kovri::core::IdentHash& address,
    Subscription source)
{
  // Check if a host is loaded into memory.
  auto check_host_loaded =
      [host](std::map<std::string, kovri::core::IdentHash>& addr_map) {
        if (!addr_map[host].IsZero())
          {
            // Entry for hostname found. If caller wishes to update the found entry,
            //   a separate "update entry" function should be called. This will help
            //   prevent silently updating user address entries, which could be the
            //   result of an attack from a malicious subscription.
            throw std::runtime_error(
                "AddressBook: host already loaded into memory");
          }
        else
          {
            addr_map.erase(host);  // Clean up default-constructed entry
            return;  // No entry for host exists in this address map
          }
      };
  // Check if an address is loaded into memory.
  auto check_address_loaded =
      [address](const std::map<std::string, kovri::core::IdentHash>& addr_map) {
        for (const auto& entry : addr_map)
          {
            if (entry.second == address)
              throw std::runtime_error(
                  "AddressBook: address already loaded into memory");
          }
      };
  // Ensure address book only inserts unique entries
  if (!m_DefaultAddresses.empty())
    {
      check_host_loaded(m_DefaultAddresses);
      check_address_loaded(m_DefaultAddresses);
    }
  if (!m_UserAddresses.empty())
    {
      check_host_loaded(m_UserAddresses);
      check_address_loaded(m_UserAddresses);
    }
  if (!m_PrivateAddresses.empty())
    {
      check_host_loaded(m_PrivateAddresses);
      // TODO(unassigned): Java I2P allows private address collisions, should Kovri?
      check_address_loaded(m_PrivateAddresses);
    }
  // Can now be reasonably sure inserting an entry is safe
  switch (source)
    {
      case Subscription::Default:
        m_DefaultAddresses[host] = address;
        break;
      case Subscription::User:
        m_UserAddresses[host] = address;
        break;
      case Subscription::Private:
        m_PrivateAddresses[host] = address;
        break;
      default:
        throw std::invalid_argument("AddressBook: unknown subscription source");
    }
}

// Used only by HTTP Proxy
void AddressBook::InsertAddressIntoStorage(
    const std::string& address,
    const std::string& base64)
{
  try
    {
      kovri::core::IdentityEx ident;
      ident.FromBase64(base64);
      const auto& ident_hash = ident.GetIdentHash();
      InsertAddress(address, ident_hash, Subscription::User);
      if (!m_Storage)
        m_Storage = GetNewStorageInstance();
      m_Storage->AddAddress(ident);
      LOG(info) << "AddressBook: " << address << "->"
                << kovri::core::GetB32Address(ident_hash)
                << " added";
    }
  catch (...)
    {
      m_Exception.Dispatch(__func__);
      throw;
    }
}

void AddressBook::Stop() {
  // Kill subscriber timer
  if (m_SubscriberUpdateTimer) {
    m_SubscriberUpdateTimer->cancel();
    m_SubscriberUpdateTimer.reset(nullptr);
  }
  // Finish downloading
  if (m_SubscriberIsDownloading) {
    LOG(info)
      << "AddressBook: subscription is downloading, waiting for termination";
    for (std::size_t seconds = 0;
         seconds < static_cast<std::uint16_t>(kovri::client::Timeout::Receive);
         seconds++) {
      if (!m_SubscriberIsDownloading) {
        LOG(info) << "AddressBook: subscription download complete";
        break;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    LOG(error) << "AddressBook: subscription download hangs";
    m_SubscriberIsDownloading = false;
  }
  // Save addresses to storage
  if (m_Storage) {
    m_Storage->Save(m_DefaultAddresses, Subscription::Default);
    if (!m_UserAddresses.empty())
      m_Storage->Save(m_UserAddresses, Subscription::User);
    if (!m_PrivateAddresses.empty())
      m_Storage->Save(m_PrivateAddresses, Subscription::Private);
    m_Storage.reset(nullptr);
  }
  m_Subscribers.clear();
}

BookEntry::BookEntry(
    const std::string& host,
    const kovri::core::IdentHash& address) try : m_Host(host),
                                                 m_Address(address)
  {
    if (m_Host.empty())
      throw std::invalid_argument("AddressBook: empty entry hostname");
  }
catch (...)
  {
    kovri::core::Exception ex(__func__);
    ex.Dispatch();
    throw;
  }

BookEntry::BookEntry(
    const std::string& host,
    const std::string& address) try : m_Host(host)
  {
    if (m_Host.empty())
      throw std::invalid_argument("AddressBook: empty entry hostname");
    core::IdentityEx ident;
    ident.FromBase64(address);
    m_Address = ident.GetIdentHash();
  }
catch (...)
  {
    kovri::core::Exception ex(__func__);
    ex.Dispatch();
    throw;
  }

BookEntry::BookEntry(const std::string& subscription_line) try
  {
    if (subscription_line.empty())
      throw std::invalid_argument("AddressBook: empty subscription line");
    std::size_t pos = subscription_line.find('=');
    if (pos == std::string::npos)
      throw std::runtime_error("AddressBook: invalid subscription line");
    m_Host = subscription_line.substr(0, pos++);
    if (m_Host.empty())
      throw std::runtime_error("AddressBook: empty entry hostname");
    core::IdentityEx ident;
    ident.FromBase64(subscription_line.substr(pos));
    m_Address = ident.GetIdentHash();
  }
catch (...)
  {
    kovri::core::Exception ex(__func__);
    ex.Dispatch();
    throw;
  }

const std::string& BookEntry::get_host() const noexcept
{
  return m_Host;
}

const core::IdentHash& BookEntry::get_address() const noexcept
{
  return m_Address;
}
/*
// TODO(unassigned): currently unused
void AddressBook::InsertAddress(
    const kovri::core::IdentityEx& address) {
  if (!m_Storage)
    m_Storage = GetNewStorageInstance();
  m_Storage->AddAddress(address);
}

// TODO(unassigned): currently unused
bool AddressBook::GetAddress(
    const std::string& address,
    kovri::core::IdentityEx& identity) {
  if (!m_Storage)
    m_Storage = GetNewStorageInstance();
  kovri::core::IdentHash ident;
  if (!GetAddressIdentHash(address, ident)) return false;
  return m_Storage->GetAddress(ident, identity);
}
*/

}  // namespace client
}  // namespace kovri
