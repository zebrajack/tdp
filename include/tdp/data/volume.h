#pragma once 
#include <tdp/config.h>
#include <stdint.h>
#include <fstream>
#include <tdp/data/image.h>

namespace tdp {

template <class T>
class Volume {
 public:
  Volume()
    : w_(0), h_(0), d_(0), pitch_(0), pitchImg_(0), ptr_(nullptr),
    storage_(Storage::Unknown)
  {}
  Volume(size_t w, size_t h, size_t d, T* ptr, 
      enum Storage storage = Storage::Unknown)
    : w_(w), h_(h), d_(d), pitch_(w*sizeof(T)), 
    pitchImg_(w*h*sizeof(T)), ptr_(ptr), storage_(storage)
  {}
  Volume(const Volume& vol)
    : w_(vol.w_), h_(vol.h_), d_(vol.d_), pitch_(vol.pitch_),
    pitchImg_(vol.pitchImg_), ptr_(vol.ptr_), storage_(vol.storage_)
  {}
  ~Volume()
  {}

  TDP_HOST_DEVICE
  const T& operator()(size_t u, size_t v, size_t d) const {
    return *(RowPtr(v,d)+u);
  }

  TDP_HOST_DEVICE
  T& operator()(size_t u, size_t v, size_t d) {
    return *(RowPtr(v,d)+u);
  }

  TDP_HOST_DEVICE
  T* RowPtr(size_t v, size_t d) const { 
    return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(ptr_)+d*pitchImg_+v*pitch_); 
  }

  TDP_HOST_DEVICE
  T* ImagePtr(size_t d) const { 
    return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(ptr_)+d*pitchImg_); 
  }

  Image<T> GetImage(size_t d) const { 
    return Image<T>(w_,h_,pitch_,ImagePtr(d),storage_);
  }

  TDP_HOST_DEVICE
  size_t SizeBytes() const { return pitchImg_*d_; }

  TDP_HOST_DEVICE
  size_t Vol() const { return w_*h_*d_; }

  void Fill(T value) { for (size_t i=0; i<w_*d_*h_; ++i) ptr_[i] = value; }

  std::string Description() const {
    std::stringstream ss;
    ss << w_ << "x" << h_ << "x" << d_
      << " " << SizeBytes() << "bytes " 
      << " ptr: " << ptr_ << " storage: " << storage_;
    return ss.str();
  }

#ifdef CUDA_FOUND
  /// Perform pitched copy from the given src volume to this volume.
  /// Use type to specify from which memory to which memory to copy.
  void CopyFrom(const Volume<T>& src, cudaMemcpyKind type) {
    if (src.SizeBytes() == SizeBytes()) {
      checkCudaErrors(cudaMemcpy(ptr_, src.ptr_, src.SizeBytes(), type));
    } else {
      std::cerr << "ERROR: not copying volume since sizes dont match" 
        << std::endl << Description() 
        << std::endl << src.Description() 
        << std::endl;
    }
  }
  void CopyFrom(const Volume<T>& src) {
    CopyFrom(src, CopyKindFromTo(src.storage_, storage_));
  }
#endif

  size_t w_;
  size_t h_;
  size_t d_; // depth
  size_t pitch_;    // the number of bytes per row
  size_t pitchImg_; // the number of bytes per u,v slice (an image)
  T* ptr_;
  enum Storage storage_;
 private:
  
};

template<typename T>
void SaveVolume(const Volume<T>& V, std::ofstream& out) {

  out.write((const char*)&(V.w_),sizeof(size_t));
  out.write((const char*)&(V.h_),sizeof(size_t));
  out.write((const char*)&(V.d_),sizeof(size_t));
  for (size_t i=0; i < V.Vol(); ++i) {
    out.write((const char*)&(V.ptr_[i]),sizeof(T));

    // printf("function value: %f\n", (float) V.ptr_[i].f);
    // printf("weight: %f\n", (float) V.ptr_[i].w);

    // printf("red: %d\n", (int) V.ptr_[i].r);
    // printf("blue: %d\n", (int) V.ptr_[i].g);
    // printf("green: %d\n", (int) V.ptr_[i].b);
  }
}

template<typename T>
void SaveVolume(const Volume<T>& V, const std::string& path) {

  std::ofstream out;
  out.open(path, std::ios::out | std::ios::binary);
  SaveVolume(V, out);
  out.close();
}


}

