#include "BevStream.hpp"

namespace BevStream {



class Turnstile : public BevStream
{
public:

  Turnstile(bool print=false);
  virtual ~Turnstile();

  void stopThreadDerivedClass();
  void setBufferOptions(bufferevent* in, bufferevent* out);
  void gotData(struct bufferevent *bev, struct evbuffer *buf, size_t len);

  // below are custom members for this Stream

  std::atomic<size_t> pending_advance;

  void advance(size_t a);


};




}
