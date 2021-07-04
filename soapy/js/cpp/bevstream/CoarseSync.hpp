#include "BevStream.hpp"
#include "convert.hpp"

#include <vector>
#include <complex>
#include <mutex>

namespace BevStream {



class CoarseSync : public BevStream
{
public:

  CoarseSync(std::vector<uint32_t> *_coarseResults, std::mutex *m, bool print=false);
  virtual ~CoarseSync();

  void stopThreadDerivedClass();
  void setBufferOptions(bufferevent* in, bufferevent* out);
  void gotData(struct bufferevent *bev, struct evbuffer *buf, size_t len);

  // below are custom members for this Stream

  void gotDataCore(struct bufferevent *bev, struct evbuffer *buf, unsigned char *data, size_t lengthIn);

  constexpr static size_t sz = 1024;
  constexpr static size_t byte_sz = 1024*4;


  void buildVectors();
  
  void _resetCoarseSyncMembers();
  void triggerCoarse();
  void triggerCoarseAt(size_t);
  void callCoarseSyncCallback(uint32_t);


  size_t getExpIndexArg(int arg);
  size_t getExpOffsetArg(int arg);
  size_t getExpOffset();
  std::vector<std::complex<double>>* getExpBuffer();

  size_t lifetime_frame = 0; // lifetime frames through this class
  size_t triggered_frame = 0;

  bool pending_trigger_at = false;
  size_t pending_trigger_at_frame = 0;

  bool pending_trigger = false;
  bool doing_coarse = false;

  bool print1 = false;
  bool print2 = false;
  bool print3 = false;
  bool print4 = false;
  size_t ofdmNum = 20;
  bool triggerAtStart = false;

  std::complex<double> cumulative_sum = {0,0};

  std::vector<std::complex<double>> expCplxWhole;
  std::vector<std::vector<std::complex<double>>> exp;

  std::vector<std::complex<double>> chunk0;
  std::vector<std::complex<double>> zeros;

  std::mutex *coarseMutex = 0;
  std::vector<uint32_t> *coarseResults;


};




}
