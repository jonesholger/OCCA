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

#ifndef OCCA_ARRAY_HEADER
#define OCCA_ARRAY_HEADER

#include "occa/defines.hpp"
#include "occa/base.hpp"
#include "occa/tools/misc.hpp"

namespace occa {
  static const int copyOnHost          = (1 << 0);
  static const int copyOnDevice        = (1 << 1);
  static const int copyOnHostAndDevice = (1 << 1);

  static const int dontUseIdxOrder = (1 << 0);
  static const int useIdxOrder     = (1 << 1);

  template <class TM, const int idxType = occa::dontUseIdxOrder>
  class array {
  public:
    occa::device device;
    occa::memory memory;

    TM *data_;

    int ks_[6];     // Passed to the kernel, not a problem for 32-bit
    udim_t s_[6];   // Strides

    int idxCount;
    udim_t fs_[7];  // Full Strides (used with idxOrder)
    int sOrder_[6]; // Stride Ordering

    array();

    template <class TM2, const int idxType2>
    array(const array<TM2,idxType2> &v);

    void initSOrder(const int idxCount_ = 1);

    template <class TM2, const int idxType2>
    array& operator = (const array<TM2,idxType2> &v);

    void free();

    //---[ Info ]-----------------------
    inline TM* data() {
      return data_;
    }

    inline udim_t entries() {
      return s_[0] * s_[1] * s_[2] * s_[3] * s_[4] * s_[5];
    }

    inline udim_t bytes() {
      return (entries() * sizeof(TM));
    }

    inline udim_t dim(const int i) {
      return s_[i];
    }

    inline operator occa::kernelArg () {
      occa::kernelArg ret;
      occa::kernelArg_t sizeArg;

      ret.arg.mHandle = memory.getMHandle();
      ret.arg.dHandle = memory.getDHandle();

      ret.arg.data.void_ = memory.getHandle();
      ret.arg.size       = sizeof(void*);
      ret.arg.info       = kArgInfo::usePointer;

      sizeArg.data.void_ = (void*) ks_;
      sizeArg.size       = maxBase2(idxCount) * sizeof(int);
      sizeArg.info       = kArgInfo::usePointer;
      ret.extraArgs.push_back(sizeArg);

      return ret;
    }

    std::string idxOrderStr();
    //==================================

    //---[ clone() ]--------------------
    array<TM,idxType> clone(const int copyOn = copyOnHostAndDevice);

    array<TM,idxType> cloneOnCurrentDevice(const int copyOn = copyOnHostAndDevice);

    array<TM,idxType> cloneOn(occa::device device_,
                              const int copyOn = copyOnHostAndDevice);

    template <class TM2, const int idxType2>
    array<TM2,idxType2> clone(const int copyOn = copyOnHostAndDevice);

    template <class TM2, const int idxType2>
    array<TM2,idxType2> cloneOnCurrentDevice(const int copyOn = copyOnHostAndDevice);

    template <class TM2, const int idxType2>
    array<TM2,idxType2> cloneOn(occa::device device_,
                                const int copyOn = copyOnHostAndDevice);
    //==================================

    //---[ array(...) ]----------------
    array(const int dim, const udim_t *d);

    array(const udim_t d0);
    array(const udim_t d0, const udim_t d1);
    array(const udim_t d0, const udim_t d1, const udim_t d2);
    array(const udim_t d0, const udim_t d1, const udim_t d2,
          const udim_t d3);
    array(const udim_t d0, const udim_t d1, const udim_t d2,
          const udim_t d3, const udim_t d4);
    array(const udim_t d0, const udim_t d1, const udim_t d2,
          const udim_t d3, const udim_t d4, const udim_t d5);
    //==================================

    //---[ array(device, ...) ]--------
    array(occa::device device_, const int dim, const udim_t *d);

    array(occa::device device_,
          const udim_t d0);
    array(occa::device device_,
          const udim_t d0, const udim_t d1);
    array(occa::device device_,
          const udim_t d0, const udim_t d1, const udim_t d2);
    array(occa::device device_,
          const udim_t d0, const udim_t d1, const udim_t d2,
          const udim_t d3);
    array(occa::device device_,
          const udim_t d0, const udim_t d1, const udim_t d2,
          const udim_t d3, const udim_t d4);
    array(occa::device device_,
          const udim_t d0, const udim_t d1, const udim_t d2,
          const udim_t d3, const udim_t d4, const udim_t d5);
    //==================================

    //---[ allocate(...) ]--------------
  private:
    void allocate();

  public:
    void allocate(const int dim, const udim_t *d);

    void allocate(const udim_t d0);
    void allocate(const udim_t d0, const udim_t d1);
    void allocate(const udim_t d0, const udim_t d1, const udim_t d2);
    void allocate(const udim_t d0, const udim_t d1, const udim_t d2,
                  const udim_t d3);
    void allocate(const udim_t d0, const udim_t d1, const udim_t d2,
                  const udim_t d3, const udim_t d4);
    void allocate(const udim_t d0, const udim_t d1, const udim_t d2,
                  const udim_t d3, const udim_t d4, const udim_t d5);
    //==================================

    //---[ allocate(device, ...) ]------
    void allocate(occa::device device_, const int dim, const udim_t *d);

    void allocate(occa::device device_,
                  const udim_t d0);
    void allocate(occa::device device_,
                  const udim_t d0, const udim_t d1);
    void allocate(occa::device device_,
                  const udim_t d0, const udim_t d1, const udim_t d2);
    void allocate(occa::device device_,
                  const udim_t d0, const udim_t d1, const udim_t d2,
                  const udim_t d3);
    void allocate(occa::device device_,
                  const udim_t d0, const udim_t d1, const udim_t d2,
                  const udim_t d3, const udim_t d4);
    void allocate(occa::device device_,
                  const udim_t d0, const udim_t d1, const udim_t d2,
                  const udim_t d3, const udim_t d4, const udim_t d5);
    //==================================

    //---[ reshape(...) ]---------------
    void reshape(const int dim, const udim_t *d);

    void reshape(const udim_t d0);
    void reshape(const udim_t d0, const udim_t d1);
    void reshape(const udim_t d0, const udim_t d1, const udim_t d2);
    void reshape(const udim_t d0, const udim_t d1, const udim_t d2,
                 const udim_t d3);
    void reshape(const udim_t d0, const udim_t d1, const udim_t d2,
                 const udim_t d3, const udim_t d4);
    void reshape(const udim_t d0, const udim_t d1, const udim_t d2,
                 const udim_t d3, const udim_t d4, const udim_t d5);
    //==================================

    //---[ setIdxOrder(...) ]-----------
    void updateFS(const int idxCount_ = 1);

    void setIdxOrder(const int dim, const int *o);
    void setIdxOrder(const std::string &default_,
                     const std::string &given);

    void setIdxOrder(const int o0);
    void setIdxOrder(const int o0, const int o1);
    void setIdxOrder(const int o0, const int o1, const int o2);
    void setIdxOrder(const int o0, const int o1, const int o2,
                     const int o3);
    void setIdxOrder(const int o0, const int o1, const int o2,
                     const int o3, const int o4);
    void setIdxOrder(const int o0, const int o1, const int o2,
                     const int o3, const int o4, const int o5);
    //==================================

    //---[ Access Operators ]-----------
    inline TM& operator [] (const udim_t i0);

    inline TM& operator () (const udim_t i0);
    inline TM& operator () (const udim_t i0, const udim_t i1);
    inline TM& operator () (const udim_t i0, const udim_t i1, const udim_t i2);
    inline TM& operator () (const udim_t i0, const udim_t i1, const udim_t i2,
                            const udim_t i3);
    inline TM& operator () (const udim_t i0, const udim_t i1, const udim_t i2,
                            const udim_t i3, const udim_t i4);
    inline TM& operator () (const udim_t i0, const udim_t i1, const udim_t i2,
                            const udim_t i3, const udim_t i4, const udim_t i5);
    //==================================

    //---[ Operators ]------------------
    //==================================

    //---[ Syncs ]----------------------
    void startManaging();
    void stopManaging();

    void syncToDevice();
    void syncFromDevice();

    bool needsSync();
    void sync();
    void dontSync();
    //==================================
  };
}

#include "occa/array/array.tpp"

#endif
