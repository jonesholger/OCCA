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

#include "occa/device.hpp"
#include "occa/base.hpp"
#include "occa/mode.hpp"
#include "occa/tools/sys.hpp"
#include "occa/tools/io.hpp"
#include "occa/parser/parser.hpp"

namespace occa {
  //---[ device_v ]---------------------
  device_v::device_v(const occa::properties &properties_) :
    hasProperties() {

    mode = properties_["mode"].getString();
    properties = properties_;

    uvaEnabled_ = this->properties.get<bool>("uva", false);
    currentStream = NULL;
    bytesAllocated = 0;
  }

  device_v::~device_v() {}

  void device_v::initFrom(const device_v &m) {
    properties = m.properties;

    uvaEnabled_    = m.uvaEnabled_;
    uvaMap         = m.uvaMap;
    uvaStaleMemory = m.uvaStaleMemory;

    currentStream = m.currentStream;
    streams       = m.streams;

    bytesAllocated = m.bytesAllocated;
  }

  bool device_v::hasUvaEnabled() {
    return uvaEnabled_;
  }
  //====================================

  //---[ device ]-----------------------
  device::device() {
    dHandle = NULL;
  }

  device::device(device_v *dHandle_) :
    dHandle(dHandle_) {}

  device::device(const occa::properties &props) {
    setup(props);
  }

  device::device(const device &d) :
    dHandle(d.dHandle) {}

  device& device::operator = (const device &d) {
    dHandle = d.dHandle;

    return *this;
  }

  void* device::getHandle(const occa::properties &props) {
    return dHandle->getHandle(props);
  }

  device_v* device::getDHandle() {
    return dHandle;
  }

  void device::setup(const occa::properties &props) {
    dHandle = occa::newModeDevice(props);
    dHandle->uvaEnabled_ = dHandle->properties.get<bool>("uva", false);

    stream newStream = createStream();
    dHandle->currentStream = newStream.handle;
  }

  occa::properties& device::properties() {
    return dHandle->properties;
  }

  udim_t device::memorySize() const {
    return dHandle->memorySize();
  }

  udim_t device::memoryAllocated() const {
    return dHandle->bytesAllocated;
  }

  bool device::hasUvaEnabled() {
    return dHandle->hasUvaEnabled();
  }

  const std::string& device::mode() {
    return dHandle->mode;
  }

  void device::finish() {
    if (dHandle->hasSeparateMemorySpace()) {
      const size_t staleEntries = uvaStaleMemory.size();
      for (size_t i = 0; i < staleEntries; ++i) {
        occa::memory_v *mem = uvaStaleMemory[i];

        mem->copyTo(mem->uvaPtr, mem->size, 0, occa::properties("async: true"));

        mem->memInfo &= ~uvaFlag::inDevice;
        mem->memInfo &= ~uvaFlag::isStale;
      }
      if (staleEntries) {
        uvaStaleMemory.clear();
      }
    }

    dHandle->finish();
  }

  //  |---[ Stream ]--------------------
  stream device::createStream() {
    stream newStream(dHandle, dHandle->createStream());
    dHandle->streams.push_back(newStream.handle);

    return newStream;
  }

  void device::freeStream(stream s) {
    const int streamCount = dHandle->streams.size();

    for (int i = 0; i < streamCount; ++i) {
      if (dHandle->streams[i] == s.handle) {
        if (dHandle->currentStream == s.handle)
          dHandle->currentStream = NULL;

        dHandle->freeStream(dHandle->streams[i]);
        dHandle->streams.erase(dHandle->streams.begin() + i);

        break;
      }
    }
  }

  stream device::getStream() {
    return stream(dHandle, dHandle->currentStream);
  }

  void device::setStream(stream s) {
    dHandle->currentStream = s.handle;
  }

  stream device::wrapStream(void *handle_) {
    return stream(dHandle, dHandle->wrapStream(handle_));
  }

  streamTag device::tagStream() {
    return dHandle->tagStream();
  }

  void device::waitFor(streamTag tag) {
    dHandle->waitFor(tag);
  }

  double device::timeBetween(const streamTag &startTag, const streamTag &endTag) {
    return dHandle->timeBetween(startTag, endTag);
  }
  //  |=================================

  //  |---[ Kernel ]--------------------
  kernel device::buildKernel(const std::string &filename,
                             const std::string &functionName,
                             const occa::properties &props) {
    occa::properties allProps = properties() + props;

    const std::string realFilename = io::filename(filename);
    const bool usingParser         = io::fileNeedsParser(filename);

    kernel ker;

    kernel_v *&k = ker.kHandle;

    if (usingParser) {
      k          = newModeKernel(occa::properties("mode: 'Serial'"));
      k->dHandle = newModeDevice(occa::properties("mode: 'Serial'"));

      hash_t hash = occa::hashFile(realFilename);
      hash ^= props.hash();

      const std::string hashDir    = io::hashDir(realFilename, hash);
      const std::string parsedFile = hashDir + "parsedSource.occa";
      k->metadata = io::parseFileForFunction(mode(),
                                             realFilename,
                                             parsedFile,
                                             functionName,
                                             props);

      kernelInfo info(props);
      info.addDefine("OCCA_LAUNCH_KERNEL", 1);

      k->build(parsedFile,
               functionName,
               k->dHandle->properties + info);
      k->nestedKernels.clear();

      if (k->metadata.nestedKernels) {
        std::stringstream ss;

        const bool vc_f = settings.get("verboseCompilation", true);

        for (int ki = 0; ki < k->metadata.nestedKernels; ++ki) {
          ss << ki;

          const std::string sKerName = k->metadata.baseName + ss.str();
          ss.str("");

          kernel sKer;
          sKer.kHandle = dHandle->buildKernel(parsedFile, sKerName, allProps);

          sKer.kHandle->metadata               = k->metadata;
          sKer.kHandle->metadata.name          = sKerName;
          sKer.kHandle->metadata.nestedKernels = 0;
          sKer.kHandle->metadata.removeArg(0); // remove nestedKernels **
          k->nestedKernels.push_back(sKer);

          // Only show compilation the first time
          if (ki == 0) {
            settings["verboseCompilation"] = false;
          }
        }
        settings["verboseCompilation"] = vc_f;
      }
    } else {
      k = dHandle->buildKernel(realFilename,
                               functionName,
                               allProps);
      k->dHandle = dHandle;
    }

    return ker;
  }

  kernel device::buildKernelFromString(const std::string &content,
                                       const std::string &functionName,
                                       const occa::properties &props) {
    const occa::properties allProps = properties() + props;
    hash_t hash = occa::hash(content);
    hash ^= allProps.hash();

    const std::string hashDir = io::hashDir(hash);
    const std::string hashTag = "occa-device";

    std::string stringSourceFile = hashDir;
    const std::string language = allProps.get<std::string>("language", "OKL");

    if (language == "OCCA") {
      stringSourceFile += "stringSource.occa";
    } else if (language == "OFL") {
      stringSourceFile += "stringSource.ofl";
    } else {
      stringSourceFile += "stringSource.okl";
    }

    if (!io::haveHash(hash, hashTag)) {
      io::waitForHash(hash, hashTag);

      return buildKernelFromBinary(hashDir + kc::binaryFile,
                                   functionName,
                                   props);
    }

    io::write(stringSourceFile, content);

    kernel k = buildKernel(stringSourceFile,
                           functionName,
                           allProps);

    io::releaseHash(hash, hashTag);

    return k;
  }

  kernel device::buildKernelFromBinary(const std::string &filename,
                                       const std::string &functionName,
                                       const occa::properties &props) {
    kernel ker;
    ker.kHandle = dHandle->buildKernelFromBinary(filename, functionName, props);
    ker.kHandle->dHandle = dHandle;

    return ker;
  }
  //  |=================================

  //  |---[ Memory ]--------------------
  memory device::malloc(const dim_t bytes,
                        void *src,
                        const occa::properties &props) {
    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= 0);

    memory mem;
    mem.mHandle          = dHandle->malloc(bytes, src, props);
    mem.mHandle->dHandle = dHandle;

    dHandle->bytesAllocated += bytes;

    return mem;
  }

  void* device::uvaAlloc(const dim_t bytes,
                         void *src,
                         const occa::properties &props) {
    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= 0);

    memory mem = malloc(bytes, src, props);
    mem.setupUva();

    if (props.get<bool>("managed", true)) {
      mem.startManaging();
    }

    return mem.mHandle->uvaPtr;
  }

  occa::memory device::wrapMemory(void *handle_,
                                  const dim_t bytes,
                                  const occa::properties &props) {
    OCCA_ERROR("Trying to wrap memory with negative bytes (" << bytes << ")",
               bytes >= 0);

    memory mem;
    mem.mHandle          = dHandle->wrapMemory(handle_, bytes, props);
    mem.mHandle->dHandle = dHandle;

    dHandle->bytesAllocated += bytes;

    return mem;
  }
  //  |=================================

  void device::free() {
    const int streamCount = dHandle->streams.size();

    for (int i = 0; i < streamCount; ++i)
      dHandle->freeStream(dHandle->streams[i]);

    dHandle->free();

    delete dHandle;
    dHandle = NULL;
  }
  //====================================

  //---[ stream ]-----------------------
  stream::stream() :
    dHandle(NULL),
    handle(NULL) {}

  stream::stream(device_v *dHandle_, stream_t handle_) :
    dHandle(dHandle_),
    handle(handle_) {}

  stream::stream(const stream &s) :
    dHandle(s.dHandle),
    handle(s.handle) {}

  stream& stream::operator = (const stream &s) {
    dHandle = s.dHandle;
    handle  = s.handle;

    return *this;
  }

  void* stream::getHandle(const occa::properties &props) {
    return handle;
  }

  void stream::free() {
    if (dHandle != NULL) {
      device(dHandle).freeStream(*this);
    }
  }
  //====================================
}
