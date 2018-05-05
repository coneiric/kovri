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

#include "client/address_book/storage.h"

#include <fstream>

namespace kovri {
namespace client {

AddressBookStorage::AddressBookStorage() {
  kovri::core::EnsurePath(GetAddressesPath());
  kovri::core::EnsurePath(GetPublishersPath());
}

bool AddressBookStorage::GetAddress(
    const kovri::core::IdentHash& ident,
    kovri::core::IdentityEx& address) const {
  auto filename = GetAddressesPath() / (ident.ToBase32() + ".b32");
  std::ifstream file(filename.string(), std::ifstream::binary);
  if (!file)
    return false;
  file.seekg(0, std::ios::end);
  const std::size_t len = file.tellg();
  if (len < kovri::core::DEFAULT_IDENTITY_SIZE) {
    LOG(error)
      << "AddressBookStorage: file " << filename << " is too short. " << len;
    return false;
  }
  file.seekg(0, std::ios::beg);
  auto buf = std::make_unique<std::uint8_t[]>(len);
  file.read(reinterpret_cast<char *>(buf.get()), len);
  // For sanity, the validity of identity length is incumbent upon the parent caller.
  // For now, we only care about returning success for an open/available file
  // TODO(unassigned): triple check that this is the case
  address.FromBuffer(buf.get(), len);
  return true;
}

void AddressBookStorage::AddAddress(const kovri::core::IdentityEx& address)
{
  auto filename =
      GetAddressesPath() / (address.GetIdentHash().ToBase32() + ".b32");
  kovri::core::OutputFileStream file(filename.string(), std::ofstream::binary);
  if (!file.Good())
    throw std::runtime_error("failed to open file for address writing");
  const std::size_t len = address.GetFullLen();
  std::vector<std::uint8_t> buf(len);
  address.ToBuffer(buf.data(), buf.size());
  if (!file.Write(buf.data(), buf.size()))
    throw std::runtime_error("failed to write address file");
}

/**
// TODO(unassigned): currently unused
void AddressBookStorage::RemoveAddress(
    const kovri::core::IdentHash& ident) {
  auto filename = GetPath() / (ident.ToBase32() + ".b32");
  if (boost::filesystem::exists(filename))
    boost::filesystem::remove(filename);
}
**/

std::size_t AddressBookStorage::Load(
    std::map<std::string, kovri::core::IdentHash>& addresses,
    Subscription source)
{
  std::size_t num = 0;
  auto filename = core::GetPath(core::Path::AddressBook) / GetAddressesFilename(source);
  std::ifstream file(filename.string());
  if (!file) {
    LOG(warning) << "AddressBookStorage: " << filename << " not found";
  } else {
    addresses.clear();
    std::string host;
    while (std::getline(file, host)) {
      // TODO(anonimal): how much more hardening do we want?
      if (!host.length())
        continue;  // skip empty line
      // TODO(anonimal): use new CSV utility after it's expanded?
      std::size_t pos = host.find(',');
      if (pos != std::string::npos) {
        std::string name = host.substr(0, pos++);
        std::string addr = host.substr(pos);
        kovri::core::IdentHash ident;
        if (!addr.empty())
          {
            ident.FromBase32(addr);
            addresses[name] = ident;
            num++;
          }
      }
    }
    LOG(debug) << "AddressBookStorage: " << num << " addresses loaded";
  }
  return num;
}

std::size_t AddressBookStorage::Save(
    const std::map<std::string, kovri::core::IdentHash>& addresses,
    Subscription source)
{
  std::size_t num = 0;
  try
    {
      auto filename =
          core::GetPath(core::Path::AddressBook) / GetAddressesFilename(source);
      // Overwrite previous contents of the catalog file. Addresses should contain
      //   the entire latest set of subscription addresses.
      kovri::core::OutputFileStream file(
          filename.string(), std::ios::out | std::ios::trunc);
      if (!file.Good())
        {
          throw std::runtime_error(
              "AddressBookStorage: can't open addresses file "
              + GetAddressesFilename(source));
        }
      for (auto const& it : addresses)
        {
          std::string const line(it.first + "," + it.second.ToBase32() + "\n");
          file.Write(const_cast<char*>(line.c_str()), line.size());
          ++num;
        }
    }
  catch (...)
    {
      kovri::core::Exception ex(__func__);
      ex.Dispatch();
    }
  LOG(info) << "AddressBookStorage: " << num << " addresses saved";
  return num;
}

std::size_t AddressBookStorage::SaveSubscription(
    const std::map<std::string, kovri::core::IdentityEx>& addresses,
    Subscription source)
{
  std::size_t num = 0;
  try
    {
      // TODO(oneiric): GPG verification of the downloaded subcription
      //   needs to be implemented to safely update the default subscription.
      if (source == Subscription::Default)
        {
          throw std::runtime_error(
              "AddressBookSubscription: default subscription is read-only");
        }
      auto filename = core::GetPath(core::Path::AddressBook)
                      / GetSubscriptionFilename(source);
      // Append subscription entries to storage file. On first call, all
      //   entries from the subscription stream will be added. Every call
      //   after will only append unique entries from the subscription.
      //   Caller must ensure unique entries.
      kovri::core::OutputFileStream file(
          filename.string(), std::ios::out | std::ios::app);
      if (!file.Good())
        {
          throw std::runtime_error(
              "AddressBookStorage: can't open subscription file "
              + GetSubscriptionFilename(source));
        }
      for (auto const& it : addresses)
        {
          std::string const line(it.first + "=" + it.second.ToBase64() + "\n");
          file.Write(const_cast<char*>(line.c_str()), line.size());
          ++num;
        }
    }
  catch (...)
    {
      kovri::core::Exception ex(__func__);
      ex.Dispatch();
    }
  LOG(info) << "AddressBookStorage: " << num << " addresses saved";
  return num;
}

std::string AddressBookDefaults::GetSubscriptionFilename(
    AddressBookDefaults::Subscription source)
{
  std::string filename("hosts.txt");
  switch (source)
    {
      case Subscription::Default:
        break;
      case Subscription::User:
        filename.insert(0, "user_");
        break;
      case Subscription::Private:
        filename.insert(0, "private_");
        break;
      default:
        throw std::invalid_argument("AddressBookStorage: unknown subscription source");
    };
  return filename;
}

std::string AddressBookDefaults::GetAddressesFilename(
    AddressBookDefaults::Subscription source)
{
  std::string filename("addresses.csv");
  switch (source)
    {
      case Subscription::Default:
        break;
      case Subscription::User:
        filename.insert(0, "user_");
        break;
      case Subscription::Private:
        filename.insert(0, "private_");
        break;
      default:
        throw std::invalid_argument("AddressBookStorage: unknown subscription source");
    };
  return filename;
}

void AddressBookStorage::LoadPublishers(std::vector<HTTPStorage>& publishers)
{
  // Parse metadata from the supplied file
  auto parse_publisher_metadata =
      [&publishers](std::istream& metadata_stream) {
        if (!metadata_stream.good())
          {
            LOG(error) << "AddressBook: unable to read publisher metadata";
            return;
          }
        // Publisher metadata buffer, no publisher line should be over 1KB
        std::array<char, 1024> pub_buffer{};
        metadata_stream.getline(pub_buffer.data(), pub_buffer.size());
        std::string publisher(std::begin(pub_buffer), std::end(pub_buffer));
        // Remove whitespace from beginning and end of line
        boost::trim(publisher);
        if (!publisher.length())
        {
          LOG(debug) << "AddressBook: empty publisher metadata";
          return;  // Empty publisher file
        }
        // Parse file metadata
        boost::char_separator<char> sep(",");
        using tokenizer = boost::tokenizer<boost::char_separator<char> >;
        tokenizer tokens(publisher, sep);
        std::string uri, etag, last_modified;
        // Necessary delimiters until proper database implementation
        // TODO(oneiric): replace with proper LMDB cursors
        const std::string uri_key{"http"}, etag_key{"E:"}, lm_key{"L:"};
        for (tokenizer::iterator tok_it = tokens.begin();
             tok_it != tokens.end();
             ++tok_it)
          {
            if (tok_it->find(uri_key) != std::string::npos)
              uri = *tok_it;
            else if (tok_it->find(etag_key) != std::string::npos)
              {  // Set ETag to value following the key
                etag = tok_it->substr(tok_it->find(etag_key) + etag_key.size());
              }
            else if (tok_it->find(lm_key) != std::string::npos)
              {  // Set last-Modified to value following the key
                last_modified =
                    tok_it->substr(tok_it->find(lm_key) + lm_key.size());
              }
          }
        // Check if metadata was parsed
        if (!uri.empty())
          {
            if (etag.empty() || last_modified.empty())
              {
                LOG(debug) << "AddressBook: only URI metadata was parsed";
                // Only store URI
                publishers.emplace_back(uri, "", "");
              }
            else
              publishers.emplace_back(uri, etag, last_modified);
          }
      };
  // Load default publisher
  auto default_pub_path = kovri::core::GetPath(core::Path::AddressBook)
                                / GetDefaultPublishersFilename();
  if (boost::filesystem::exists(default_pub_path))
    {
      std::ifstream default_pub_file;
      default_pub_file.open(default_pub_path.string(), std::ios::in);
      if (!default_pub_file.is_open())
        {
          LOG(warning) << "AddressBook: unable to open: "
                       << default_pub_path.string();
        }
      else
        {
          parse_publisher_metadata(default_pub_file);
          default_pub_file.close();
        }
    }
  // Load user publishers
  if (boost::filesystem::is_directory(GetPublishersPath()))
    {
      boost::filesystem::directory_iterator end_itr;
      // Iterate over all files in the "publishers" directory
      for (boost::filesystem::directory_iterator dir_itr(GetPublishersPath());
           dir_itr != end_itr;
           ++dir_itr)
        {
          auto publisher_path = dir_itr->path().filename();
          std::ifstream publisher_file;
          publisher_file.open(
              (GetPublishersPath() / publisher_path).string(), std::ios::in);
          if (!publisher_file.is_open())
            {
              LOG(warning) << "AddressBook: unable to open: "
                           << publisher_path.string();
            }
          else
            {
              parse_publisher_metadata(publisher_file);
              publisher_file.close();
            }
        }
    }
  else
    {
      LOG(warning) << "AddressBook: unable to find publisher directory";
    }
  LOG(info) << "AddressBook: " << publishers.size()
            << " publishers loaded";
}
}  // namespace client
}  // namespace kovri
