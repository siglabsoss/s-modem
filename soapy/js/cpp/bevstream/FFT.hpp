#include "BevStream.hpp"
#include "convert.hpp"

#include <vector>
#include <complex>
#include <mutex>
#include <eigen3/Eigen/Core>
#include <eigen3/unsupported/Eigen/FFT>

namespace BevStream {



class FFT : public BevStream
{
public:

  FFT(size_t removeInputCp = 0, size_t addOutputCp = 0, bool print=false);
  virtual ~FFT();

  void stopThreadDerivedClass();
  void setBufferOptions(bufferevent* in, bufferevent* out);
  void gotData(struct bufferevent *bev, struct evbuffer *buf, size_t len);

  // below are custom members for this Stream

  void gotDataCore(struct bufferevent *bev, struct evbuffer *buf, unsigned char *data, size_t lengthIn);

  constexpr static size_t sz = 1024;
  constexpr static size_t element_sz = 8*2; // 8 bytes per float, 2 floats per cplx value
  constexpr static size_t byte_sz = 1024*8*2; // 8 bytes per float, 2 floats per cplx value

  Eigen::FFT<double> fft;

  std::vector<std::complex<double>> vec;
  std::vector<std::complex<double>> results;

  size_t removeCp;
  size_t addCp;


};




}
