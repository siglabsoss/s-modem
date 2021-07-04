#include "BevStream.hpp"

namespace BevStream {



class GainStream : public BevStream
{
public:

  GainStream(bool print=false);
  virtual ~GainStream();

  void stopThreadDerivedClass();
  void setBufferOptions(bufferevent* in, bufferevent* out);
  void gotData(struct bufferevent *bev, struct evbuffer *buf, size_t len);

  // below are custom members for this Stream

  uint32_t gain = 2;

};












}
