#ifndef _STAT_GEN_H_
#define _STAT_GEN_H_

#include <string>
#include <vector>
#include <tuple>

#include "cloud_conn.h"
#include "http_req.h"

using std::string;
using std::vector;
using std::tuple;

enum EventType {
  HEADERS_SEND_START = 0,
  HEADERS_SEND_END,
  HEADERS_RECV_START,
  HEADERS_RECV_END,
  DATA_RECV_START,
  DATA_RECV_END,
  MAX_EVENTS
};
class Statistics : public HttpReqEvents
{
public:
  Statistics();
  virtual void OnReqSendHeaders();
  virtual void OnReqRecvHeaders();
  virtual void OnReqRecvData(size_t size);
  virtual void OnComplete(unsigned long http_code);

  void set_url(const string& url) { url_ = url; }
  const string& get_url() const { return url_; }

  unsigned long get_http_code() const { return http_code_;}
  bool IsSuccess() const { return http_code_ >= 200 && http_code_ < 300; }
  size_t get_data_size() const { return data_size_; }
  tuple<uint64_t, uint64_t> GetStartTime() const;
  tuple<uint64_t, uint64_t> GetTotalTime() const;
  static double Msec(const tuple<uint64_t, uint64_t>& t);
  static double MBsec(const tuple<uint64_t, uint64_t>& t, size_t size);
private:
  bool RecordEventOnFirstTime(EventType event);
  void RecordEvent(EventType event);
private:
  string url_;
  struct timeval times_[MAX_EVENTS+1];
  bool flags_[MAX_EVENTS+1];
  bool first_data_;
  size_t data_size_;
  unsigned long http_code_;
};

class StatGenerator
{
public:
  void AddConnection(const string& url, const string& auth,
                     uint64_t range_start,
                     uint64_t range_end,
                     size_t len);
  void Run(int count, int interval, bool repeat);
  void DumpStatistics(const Statistics &stat);
private:
  void HandleCntrlC();
  void OnStop(int sig);
private:
  vector<CloudConnection*> connections_;
};

#endif /* _STAT_GEN_H_ */
