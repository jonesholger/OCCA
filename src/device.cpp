/* The MIT License (MIT)
 *
 * Copyright (c) 2014-2017 David Medina and Tim Warburton
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
  device_v::device_v(const occa::properties &properties_) {
    mode = properties_["mode"].string();
    properties = properties_;

    currentStream = NULL;
    bytesAllocated = 0;
  }

  device_v::~device_v() {}

  void device_v::initFrom(const device_v &m) {
    properties = m.properties;

    uvaMap         = m.uvaMap;
    uvaStaleMemory = m.uvaStaleMemory;

    currentStream = m.currentStream;
    streams       = m.streams;

    bytesAllocated = m.bytesAllocated;
  }
  //====================================

  //---[ device ]-----------------------
  device::device() :
    dHandle(NULL) {}

  device::device(device_v *dHandle_) :
    dHandle(NULL) {
    setDHandle(dHandle_);
  }

  device::device(const occa::properties &props) :
    dHandle(NULL) {
    setup(props);
  }

  device::device(const device &d) :
    dHandle(NULL) {
    setDHandle(d.dHandle);
  }

  device& device::operator = (const device &d) {
    setDHandle(d.dHandle);
    return *this;
  }

  device::~device() {
    removeDHandleRef();
  }

  void device::setDHandle(device_v *dHandle_) {
    if (dHandle != dHandle_) {
      removeDHandleRef();
      dHandle = dHandle_;
      dHandle->addRef();
    }
  }

  void device::removeDHandleRef() {
    if (dHandle && !dHandle->removeRef()) {
      free();
      delete dHandle;
      dHandle = NULL;
    }
  }

  void device::dontUseRefs() {
    if (dHandle) {
      dHandle->dontUseRefs();
    }
  }

  bool device::operator == (const occa::device &d) const {
    return (dHandle == d.dHandle);
  }

  void device::free() {
    if (dHandle == NULL) {
      return;
    }
    const int streamCount = dHandle->streams.size();

    for (int i = 0; i < streamCount; ++i) {
      dHandle->freeStream(dHandle->streams[i]);
    }
    dHandle->streams.clear();
    dHandle->free();
  }

  bool device::isInitialized() {
    return (dHandle != NULL);
  }

  void* device::getHandle(const occa::properties &props) {
    return dHandle->getHandle(props);
  }

  device_v* device::getDHandle() const {
    return dHandle;
  }

  void device::setup(const occa::properties &props) {
    setDHandle(occa::newModeDevice(props));

    stream newStream = createStream();
    dHandle->currentStream = newStream.handle;
  }

  const std::string& device::mode() {
    return dHandle->mode;
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

  void device::finish() {
    if (dHandle->hasSeparateMemorySpace()) {
      const size_t staleEntries = uvaStaleMemory.size();
      for (size_t i = 0; i < staleEntries; ++i) {
        occa::memory_v *mem = uvaStaleMemory[i];

        mem->copyTo(mem->uvaPtr, mem->size, 0, "async: true");

        mem->memInfo &= ~uvaFlag::inDevice;
        mem->memInfo &= ~uvaFlag::isStale;
      }
      if (staleEntries) {
        uvaStaleMemory.clear();
      }
    }

    dHandle->finish();
  }

  bool device::hasSeparateMemorySpace() {
    return dHandle->hasSeparateMemorySpace();
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

  stream device::wrapStream(void *handle_, const occa::properties &props) {
    return stream(dHandle, dHandle->wrapStream(handle_, props));
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
                             const std::string &kernelName,
                             const occa::properties &props) {

    occa::properties allProps = properties() + props;

    const std::string realFilename = io::filename(filename);

    if (!allProps.get("OKL", true)) {
      return kernel(dHandle->buildKernel(realFilename, kernelName, allProps));
    }

    hash_t hash = occa::hashFile(realFilename);
    hash ^= props.hash();
    hash ^= occa::hash(mode());

    occa::properties launcherProps("mode: 'Serial'");
    kernel ker(newModeKernel(launcherProps));
    kernel_v *k = ker.kHandle;
    k->dHandle = newModeDevice(launcherProps);

    const std::string hashDir = io::hashDir(realFilename, hash);
    const std::string parsedFile = hashDir + "parsedSource.occa";
    k->metadata = io::parseFileForFunction(mode(),
                                           realFilename,
                                           parsedFile,
                                           kernelName,
                                           props);

    occa::properties launchKernelProps;
    launchKernelProps["mode"] = "Serial";
    launchKernelProps["defines/OCCA_LAUNCH_KERNEL"] = 1;

    k->build(parsedFile,
             kernelName,
             k->dHandle->properties + launchKernelProps);
    k->nestedKernels.clear();

    if (k->metadata.nestedKernels) {
      std::stringstream ss;

      const bool vc_f = settings()["verboseCompilation"];

      for (int ki = 0; ki < k->metadata.nestedKernels; ++ki) {
        ss << ki;

        const std::string sKerName = k->metadata.baseName + ss.str();
        ss.str("");

        kernel sKer(dHandle->buildKernel(parsedFile, sKerName, allProps));
        sKer.kHandle->metadata               = k->metadata;
        sKer.kHandle->metadata.name          = sKerName;
        sKer.kHandle->metadata.nestedKernels = 0;
        sKer.kHandle->metadata.removeArg(0); // remove nestedKernels **
        k->nestedKernels.push_back(sKer);

        // Only show compilation the first time
        if (ki == 0) {
          settings()["verboseCompilation"] = false;
        }
      }
      settings()["verboseCompilation"] = vc_f;
    }
    return ker;
  }

  kernel device::buildKernelFromString(const std::string &content,
                                       const std::string &kernelName,
                                       const occa::properties &props) {
    const occa::properties allProps = properties() + props;
    hash_t hash = occa::hash(content);
    hash ^= allProps.hash();

    const std::string hashDir = io::hashDir(hash);
    const std::string hashTag = "occa-device";

    std::string stringSourceFile = hashDir;
    stringSourceFile += "stringSource.okl";

    if (!io::haveHash(hash, hashTag)) {
      io::waitForHash(hash, hashTag);

      return buildKernel(stringSourceFile,
                         kernelName,
                         allProps);
    }

    if (!sys::fileExists(stringSourceFile)) {
      io::write(stringSourceFile, content);
    }

    kernel k = buildKernel(stringSourceFile,
                           kernelName,
                           allProps);

    io::releaseHash(hash, hashTag);

    return k;
  }

  kernel device::buildKernelFromBinary(const std::string &filename,
                                       const std::string &kernelName,
                                       const occa::properties &props) {

    kernel ker(dHandle->buildKernelFromBinary(filename, kernelName, props));
    ker.kHandle->dHandle = dHandle;
    return ker;
  }
  //  |=================================

  //  |---[ Memory ]--------------------
  memory device::malloc(const dim_t bytes,
                        const void *src,
                        const occa::properties &props) {
    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= 0);

    memory mem(dHandle->malloc(bytes, src, props));
    mem.mHandle->dHandle = dHandle;

    dHandle->bytesAllocated += bytes;

    return mem;
  }

  void* device::umalloc(const dim_t bytes,
                        const void *src,
                        const occa::properties &props) {
    OCCA_ERROR("Trying to allocate negative bytes (" << bytes << ")",
               bytes >= 0);

    memory mem = malloc(bytes, src, props);
    mem.setupUva();
    mem.dontUseRefs();

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

    memory mem(dHandle->wrapMemory(handle_, bytes, props));
    mem.mHandle->dHandle = dHandle;
    mem.dontUseRefs();

    dHandle->bytesAllocated += bytes;

    return mem;
  }
  //  |=================================

  template <>
  hash_t hash(const occa::device &device) {
    return device.getDHandle()->hash();
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
