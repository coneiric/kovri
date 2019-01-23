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

#include "tests/fuzz_tests/identity.h"
#include "tests/fuzz_tests/i2pcontrol.h"
#include "tests/fuzz_tests/lease_set.h"
#include "tests/fuzz_tests/su3.h"
#include "tests/fuzz_tests/routerinfo.h"

#include "core/util/log.h"

namespace kovri
{
namespace fuzz
{
namespace bpo = boost::program_options;

std::unique_ptr<FuzzTarget> fuzz_target;

void PrintUsage()
{
  LOG(info) << "Usage: ./fuzzer --target=<target>"
            << " [libFuzzer options] [RAW CORPUS] [PRUNED CORPUS]";
}

void PrintAvailableTargets()
{
  LOG(info) << "Available targets : ";
  LOG(info) << "\tidentity";
  LOG(info) << "\ti2pcontrol";
  LOG(info) << "\tleaseset";
  LOG(info) << "\trouterinfo";
  LOG(info) << "\tsu3";

}

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
  try
    {
      bpo::options_description fuzz_cfg("Fuzzer configuration");
      fuzz_cfg.add_options()("help", "Print fuzzer usage")(
          "list,l", "List available targets")(
          "target", bpo::value<std::string>(), "fuzz target");

      // Parse cli options
      bpo::variables_map vm;
      bpo::store(
          bpo::command_line_parser(*argc, *argv)
              .options(fuzz_cfg)
              .allow_unregistered()
              .run(),
          vm);
      bpo::notify(vm);

      if (vm.count("help"))
        {
          PrintUsage();
          exit(0);
        }

      if (vm.count("list"))
        {
          PrintAvailableTargets();
          exit(0);
        }

      if (vm.count("target"))
        {
          auto tgt = vm["target"].as<std::string>();

          if (tgt == "i2pcontrol")
            fuzz_target = std::make_unique<I2PControl>();
          else if (tgt == "identity")
            fuzz_target = std::make_unique<IdentityEx>();
          else if (tgt == "leaseset")
            fuzz_target = std::make_unique<LeaseSet>();
          else if (tgt == "routerinfo")
            fuzz_target = std::make_unique<RouterInfo>();
          else if (tgt == "su3")
            fuzz_target = std::make_unique<SU3>();
          else
            {
              LOG(error) << "Fuzzer: unknown target supplied";
              exit(1);
            }
        }
      else
        {
          LOG(error) << "Fuzzer: no fuzz target";
          exit(1);
        }
      fuzz_target->Initialize(argc, argv);
    }
  catch (...)
    {
      std::string const err_msg{"Fuzzer: " + std::string(__func__)};
      core::Exception ex(err_msg.c_str());
      ex.Dispatch();
      exit(1);
    }
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  if (!fuzz_target)
    {
      LOG(error) << "Fuzzer: no fuzz target";
      exit(1);
    }

  fuzz_target->Impl(data, size);
  return 0;
}

}  // namespace fuzz
}  // namespace kovri
