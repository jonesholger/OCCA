/* The MIT License (MIT)
 *
 * Copyright (c) 2014-2016 David Medina and Tim Warburton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 */

#include "occa/modes/serial/kernel.hpp"
#include "occa/tools/env.hpp"
#include "occa/tools/io.hpp"
#include "occa/base.hpp"

namespace occa {
  namespace serial {
    kernel::kernel(const occa::properties &properties_) :
      occa::kernel_v(properties_) {
      dlHandle = NULL;
      handle   = NULL;
    }

    kernel::~kernel() {}

    void* kernel::getHandle(const occa::properties &props) {
      const std::string type = props["type"];

      if (type == "dl_handle") {
        return dlHandle;
      }
      if (type == "function") {
        return &handle;
      }
      return NULL;
    }

    void kernel::build(const std::string &filename,
                       const std::string &functionName,
                       const occa::properties &props) {

      const occa::properties allProps = properties + props;
      name = functionName;

      hash_t hash = occa::hashFile(filename);
      hash ^= allProps.hash();

      const std::string sourceFile = getSourceFilename(filename, hash);
      const std::string binaryFile = getBinaryFilename(filename, hash);
      bool foundBinary = true;

      const std::string hashTag = "serial-kernel";
      if (!io::haveHash(hash, hashTag)) {
        io::waitForHash(hash, hashTag);
      } else if (sys::fileExists(binaryFile)) {
        io::releaseHash(hash, hashTag);
      } else {
        foundBinary = false;
      }

      if (foundBinary) {
        if (settings.get("verboseCompilation", true)) {
          std::cout << "Found cached binary of [" << io::shortname(filename) << "] in [" << io::shortname(binaryFile) << "]\n";
        }
        return buildFromBinary(binaryFile, functionName, props);
      }

      std::string kernelDefines;
      if (properties.has("occa::kernelDefines")) {
        kernelDefines = properties["occa::kernelDefines"].getString();
      } else {
        kernelDefines = io::cacheFile(env::OCCA_DIR + "/include/occa/modes/serial/kernelDefines.hpp",
                                      "serialKernelDefines.hpp");
      }

      const std::string vectorDefines =
        io::cacheFile(env::OCCA_DIR + "/include/occa/defines/vector.hpp",
                      "vectorDefines.hpp");

      std::stringstream ss, command;
      ss << "#include \"" << kernelDefines << "\"\n"
         << "#include \"" << vectorDefines << "\"\n"
         << allProps["headers"].getString() << '\n'
         << "#if defined(OCCA_IN_KERNEL) && !OCCA_IN_KERNEL\n"
         << "using namespace occa;\n"
         << "#endif\n";

      const std::string cachedSourceFile = io::cacheFile(filename,
                                                         kc::sourceFile,
                                                         hash,
                                                         ss.str(),
                                                         allProps["footer"].getString());

      const std::string &compilerEnvScript = allProps["compilerEnvScript"].getString();
      if (compilerEnvScript.size()) {
        command << compilerEnvScript << " && ";
      }

#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_OSX_OS))
      command << allProps["compiler"].getString()
              << ' '    << allProps["compilerFlags"].getString()
              << ' '    << cachedSourceFile
              << " -o " << binaryFile
              << " -I"  << env::OCCA_DIR << "include"
              << " -L"  << env::OCCA_DIR << "lib -locca"
              << std::endl;
#else
#  if (OCCA_DEBUG_ENABLED)
      const std::string occaLib = env::OCCA_DIR + "lib/libocca_d.lib ";
#  else
      const std::string occaLib = env::OCCA_DIR + "lib/libocca.lib ";
#  endif

      command << allProps["compiler"]
              << " /D MC_CL_EXE"
              << " /D OCCA_OS=OCCA_WINDOWS_OS"
              << " /EHsc"
              << " /wd4244 /wd4800 /wd4804 /wd4018"
              << ' '       << allProps["compilerFlags"]
              << " /I"     << env::OCCA_DIR << "/include"
              << ' '       << sourceFile
              << " /link " << occaLib
              << " /OUT:"  << binaryFile
              << std::endl;
#endif

      const std::string &sCommand = command.str();

      if (settings.get("verboseCompilation", true)) {
        std::cout << "Compiling [" << functionName << "]\n" << sCommand << "\n";
      }

#if (OCCA_OS & (OCCA_LINUX_OS | OCCA_OSX_OS))
      const int compileError = system(sCommand.c_str());
#else
      const int compileError = system(("\"" +  sCommand + "\"").c_str());
#endif

      if (compileError) {
        io::releaseHash(hash, hashTag);
        OCCA_ERROR("Compilation error", compileError);
      }

      dlHandle = sys::dlopen(binaryFile, hash, hashTag);
      handle   = sys::dlsym(dlHandle, functionName, hash, hashTag);

      io::releaseHash(hash, hashTag);
    }

    void kernel::buildFromBinary(const std::string &filename,
                                 const std::string &functionName,
                                 const occa::properties &props) {

      name = functionName;

      dlHandle = sys::dlopen(filename);
      handle   = sys::dlsym(dlHandle, functionName);
    }

    int kernel::maxDims() {
      return 3;
    }

    dim kernel::maxOuterDims() {
      return dim(-1,-1,-1);
    }

    dim kernel::maxInnerDims() {
      return dim(-1,-1,-1);
    }

    void kernel::runFromArguments(const int kArgc, const kernelArg *kArgs) {
      int occaKernelArgs[6];

      occaKernelArgs[0] = outer.z; occaKernelArgs[3] = inner.z;
      occaKernelArgs[1] = outer.y; occaKernelArgs[4] = inner.y;
      occaKernelArgs[2] = outer.x; occaKernelArgs[5] = inner.x;

      int argc = 0;
      for (int i = 0; i < kArgc; ++i) {
        const kernelArg_t &arg = kArgs[i].arg;
        const dim_t extraArgCount = kArgs[i].extraArgs.size();
        const kernelArg_t *extraArgs = extraArgCount ? &(kArgs[i].extraArgs[0]) : NULL;

        vArgs[argc++] = arg.ptr();
        for (int j = 0; j < extraArgCount; ++j) {
          vArgs[argc++] = extraArgs[j].ptr();
        }
      }

      int occaInnerId0 = 0, occaInnerId1 = 0, occaInnerId2 = 0;
      sys::runFunction(handle,
                       occaKernelArgs,
                       occaInnerId0, occaInnerId1, occaInnerId2,
                       argc, vArgs);
    }

    void kernel::free() {
      sys::dlclose(dlHandle);
    }
  }
}
