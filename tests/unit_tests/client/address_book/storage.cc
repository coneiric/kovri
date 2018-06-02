/**                                                                                           //
 * Copyright (c) 2015-2018, The Kovri I2P Router Project                                      //
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

#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include <iostream>

#include "client/address_book/storage.h"
#include "client/util/http.h"

#include "core/crypto/radix.h"
#include "core/router/context.h"

struct AddressBookStorageFixture
{
  AddressBookStorageFixture() : m_Storage(nullptr)
  {  // Setup temporary custom data directory
    kovri::core::context.SetCustomDataDir(temp_path.string());
    kovri::core::EnsurePath(
        kovri::core::GetPath(kovri::core::Path::Client));
    kovri::core::EnsurePath(
        kovri::core::GetPath(kovri::core::Path::AddressBook));
    m_Storage = GetStorageInstance();
  }

  ~AddressBookStorageFixture()
  {
    RemoveFiles();
  }

  void RemoveFiles() try
    {
      boost::filesystem::directory_iterator end_itr;
      for (boost::filesystem::directory_iterator dir_itr(
               kovri::core::GetPath(kovri::core::Path::AddressBook));
           dir_itr != end_itr;
           ++dir_itr)
        {  // Remove files created by AddressBookStorage
          auto temp_file = dir_itr->path().filename();
          if (boost::ends_with(temp_file.string(), host_pub_suffix)
              || boost::ends_with(temp_file.string(), address_suffix)
              || boost::ends_with(temp_file.string(), catalog_pub_suffix))
            boost::filesystem::remove(temp_file);
        }
      boost::filesystem::remove_all(
          kovri::core::GetPath(kovri::core::Path::Client));
    }
  catch (boost::filesystem::filesystem_error& er)
    {
      std::cerr << __func__ << ": " << er.what();
    }

  std::unique_ptr<kovri::client::AddressBookStorage> GetStorageInstance()
  {  // Needed to initialize storage after custom data dir is set
    return std::make_unique<kovri::client::AddressBookStorage>();
  }

  void CreatePublisherFile(
      const std::string& publisher_filename,
      const std::string& metadata)
  {
    // Write custom publisher metadata to disk
    kovri::core::OutputFileStream out_file(
        publisher_filename, std::ios::out | std::ios::trunc);
    if (out_file.Good())
      out_file.Write(const_cast<char*>(metadata.data()), metadata.size());
    else
      {
        std::cerr << __func__ << ": unable to write to " << publisher_filename;
        return;
      }
  }

  std::string GetUniquePath()
  {
    if (!m_Storage)
      m_Storage = GetStorageInstance();
    std::array<std::uint8_t, 32> rand_data;
    kovri::core::RandBytes(rand_data.data(), rand_data.size());
    const std::string rand_filename = kovri::core::Base32::Encode(rand_data.data(), rand_data.size());
    return (m_Storage->GetPublishersPath() / rand_filename).string();
  }

  boost::filesystem::path temp_path = boost::filesystem::temp_directory_path();
  std::unique_ptr<kovri::client::AddressBookStorage> m_Storage;
  std::vector<kovri::client::HTTPStorage> publishers{};
  const std::string address_suffix{".b32"};
  const std::string host_pub_suffix{".txt"};
  const std::string catalog_pub_suffix{".csv"};
};

BOOST_FIXTURE_TEST_SUITE(AddressBookStorageTests, AddressBookStorageFixture)

BOOST_AUTO_TEST_CASE(ValidLoadPublishers)
{
  // Check we have a storage instance
  BOOST_CHECK(m_Storage);

  // Write default publisher metadata to disk
  CreatePublisherFile(GetUniquePath(), m_Storage->GetDefaultPublisherURI());

  // Write custom publisher metadata to disk
  const std::string valid_metadata =
      "http://pub.example.com/hosts.txt,E:W/Some-ETag,L:20180601T010203";
  CreatePublisherFile(GetUniquePath(), valid_metadata);

  BOOST_CHECK(publishers.size());
  for (const auto& pub : publishers)
    {
      BOOST_CHECK(!pub.GetPreviousURI().empty());
      if (pub.GetPreviousURI() == m_Storage->GetDefaultPublisherURI())
        {  // Check default publisher only loaded the URI
          BOOST_CHECK(pub.GetPreviousETag().empty());
          BOOST_CHECK(pub.GetPreviousLastModified().empty());
        }
      else
        {  // Check custom publisher ETag & Last-Modified were parsed
          BOOST_CHECK(!pub.GetPreviousETag().empty());
          BOOST_CHECK(!pub.GetPreviousLastModified().empty());
          BOOST_CHECK(
              pub.GetPreviousLastModified().rfind("GMT") != std::string::npos);
        }
    }
}

BOOST_AUTO_TEST_CASE(InvalidLoadPublishers)
{
  // Check we have a storage instance
  BOOST_CHECK(m_Storage);

  // Write publisher file with only ETag
  const std::string etag_metadata = ",W/Some-ETag,";
  BOOST_CHECK_NO_THROW(CreatePublisherFile(GetUniquePath(), etag_metadata));

  // Write publisher file with only Last-Modified
  const std::string last_modified_metadata = ",,20180601T010203";
  BOOST_CHECK_NO_THROW(
      CreatePublisherFile(GetUniquePath(), last_modified_metadata));

  // Write publisher file with only URI and ETag
  const std::string uri_etag_metadata =
      "http://invalidpub.example.com/hosts.txt,W/Some-ETag,";
  BOOST_CHECK_NO_THROW(
      CreatePublisherFile(GetUniquePath(), uri_etag_metadata));

  // Write publisher file with only URI and Last-Modified
  const std::string uri_last_modified_metadata =
      "http://invalidpub.example.com/hosts.txt,,20180601T010203";
  BOOST_CHECK_NO_THROW(CreatePublisherFile(
      GetUniquePath(), uri_last_modified_metadata));

  // Write publisher file with out of order metadata
  const std::string out_of_order_metadata =
      "20180601T010203,http://invalidpub.example.com/hosts.txt,W/Some-ETag";
  BOOST_CHECK_NO_THROW(
      CreatePublisherFile(GetUniquePath(), out_of_order_metadata));

  // Write publisher file with no metadata, only delimiters
  const std::string only_delimiters_metadata = ",,";
  BOOST_CHECK_NO_THROW(
      CreatePublisherFile(GetUniquePath(), only_delimiters_metadata));

  // Write empty publisher file
  const std::string empty_metadata = "\n";
  BOOST_CHECK_NO_THROW(
      CreatePublisherFile(GetUniquePath(), empty_metadata));

  BOOST_CHECK_NO_THROW(m_Storage->LoadPublishers(publishers));
  BOOST_CHECK(publishers.empty());
}

BOOST_AUTO_TEST_SUITE_END()
