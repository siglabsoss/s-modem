#include "BevStream.hpp"

namespace BevStream {



class IShortToIFloat64 : public BevStream
{
public:

  IShortToIFloat64(bool print=false);
  virtual ~IShortToIFloat64();

  void stopThreadDerivedClass();
  void setBufferOptions(bufferevent* in, bufferevent* out);
  void gotData(struct bufferevent *bev, struct evbuffer *buf, size_t len);

  // below are custom members for this Stream

  constexpr static size_t element_sz = 4;
  constexpr static size_t out_element_sz = 8*2;
  constexpr static size_t byte_sz = element_sz;


};












}
