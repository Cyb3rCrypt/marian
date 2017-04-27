#pragma once
#include <sstream>
#include <vector>
#include <cassert>
#include "types-fpga.h"
#include "matrix_functions.h"

namespace amunmt {
namespace FPGA {

template<typename T>
class Array
{
public:
  Array(const OpenCLInfo &openCLInfo)
  :openCLInfo_(openCLInfo)
  ,size_(0)
  ,arrSize_(0)
  {
  }

  Array(const OpenCLInfo &openCLInfo, size_t size)
  :openCLInfo_(openCLInfo)
  ,size_(size)
  ,arrSize_(0)
  {
    cl_int err;
    mem_ = clCreateBuffer(openCLInfo.context,  CL_MEM_READ_WRITE,  sizeof(T) * size, NULL, &err);
    CheckError(err);
  }

  Array(const OpenCLInfo &openCLInfo, size_t size, const T &value)
  :Array(openCLInfo, size)
  {
    CheckError( clEnqueueFillBuffer(openCLInfo.commands, mem_, &value, sizeof(T), 0, size_ * sizeof(T), 0, NULL, NULL) );
    CheckError( clFinish(openCLInfo.commands) );
  }

  Array(const OpenCLInfo &openCLInfo, const std::vector<T> &vec)
  :openCLInfo_(openCLInfo)
  ,size_(vec.size())
  ,arrSize_(vec.size())
  {
    cl_int err;
    mem_ = clCreateBuffer(openCLInfo.context,  CL_MEM_COPY_HOST_PTR,  sizeof(T) * size_, (void*) vec.data(), &err);
    CheckError(err);

  }

  ~Array()
  {
    //CheckError( clReleaseMemObject(mem_) );
  }

  size_t size() const
  { return size_; }

  const uint &sizeUInt() const
  { return size_; }

  cl_mem &data()
  { return mem_;  }

  const cl_mem &data() const
  { return mem_;  }

  const OpenCLInfo &GetOpenCLInfo() const
  { return openCLInfo_; }

  void Swap(Array &other)
  {
    assert(&openCLInfo_ == &other.openCLInfo_);
    std::swap(size_, other.size_);
    std::swap(arrSize_, other.arrSize_);
    std::swap(mem_, other.mem_);
  }

  void Set(const T &val)
  {
    CheckError( clEnqueueFillBuffer(openCLInfo_.commands, mem_, &val, sizeof(T), 0, size() * sizeof(T), 0, NULL, NULL) );
    CheckError( clFinish(openCLInfo_.commands) );
  }

  void Set(const T *arr, size_t size)
  {
    size_t bytes = size * sizeof(T);
    CheckError( clEnqueueWriteBuffer(
                    openCLInfo_.commands,
                    mem_,
                    CL_TRUE,
                    0,
                    bytes,
                    arr,
                    0,
                    NULL,
                    NULL) );
    CheckError( clFinish(openCLInfo_.commands) );
  }

  void Set(const std::vector<T> &vec)
  {
    assert(vec.size() <= size_);
    Set(vec.data(), vec.size());
  }

  void Get(T *arr, size_t size) const
  {
    CheckError( clEnqueueReadBuffer( openCLInfo_.commands, mem_, CL_TRUE, 0, sizeof(T) * size, arr, 0, NULL, NULL ) );
  }

  virtual void resize(size_t newSize)
  {
    if (newSize > arrSize_) {
      // create new, bigger buffer and copy old data to it
      cl_int err;

      cl_mem newMem = clCreateBuffer(openCLInfo_.context,  CL_MEM_READ_WRITE,  sizeof(T) * newSize, NULL, &err);
      CheckError(err);

      if (size_) {
        //cerr << "resize: clEnqueueCopyBuffer " << oldSize << endl;
        CheckError( clEnqueueCopyBuffer(openCLInfo_.commands, mem_, newMem, 0, 0, sizeof(T) * size_, 0, NULL, NULL) );
      }

      mem_ = newMem;
      arrSize_ = newSize;
    }

    size_ = newSize;
  }

  virtual std::string Debug(size_t verbosity = 1) const
  {
    std::stringstream strm;
    strm << mem_ << " size_=" << size_ << " arrSize_=" << arrSize_;

    if (verbosity) {
      float sum = mblas::SumSizet(openCLInfo_, mem_, size_);
      strm << " sum=" << sum << std::flush;
    }

    if (verbosity == 2) {
      T results[size_];
      Get(results, size_);

      for (size_t i = 0; i < size_; ++i) {
        strm << " " << results[i];
      }
    }

    return strm.str();
  }

protected:
  const OpenCLInfo &openCLInfo_;

  uint size_;
  uint arrSize_;
  cl_mem mem_;

};

}
}
