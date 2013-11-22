#ifndef _CLOUD_CONN_H_
#define _CLOUD_CONN_H_

#include <string>

using std::string;

class Statistics;
class HttpReq;

class CloudConnection
{
public:
  CloudConnection(const string &url, const string &auth);
  void SetLimits(uint64_t range_start,
                 uint64_t range_end,
                 size_t recv_limit_size);

  virtual void PerformGet(Statistics *stat) = 0;
protected:
  void ApplyLimits(HttpReq *req);
protected:
  string url_;
  string auth_;
  uint64_t range_start_;
  uint64_t range_end_;
  uint64_t recv_limit_size_;
};

class CloudConnectionFactory
{
public:
  static CloudConnection *NewConnection(const string &url,
                                        const string &auth);
};

#endif /* _CLOUD_CONN_H_ */
