#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <functional>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/count.hpp>

#include "stat_gen.h"
#include "logging.h"

using boost::numeric_cast;
using namespace boost::accumulators;

static bool exiting_g = false;

void StatGenerator::AddConnection(const string& url,
                                  const string& auth,
                                  uint64_t range_start,
                                  uint64_t range_end,
                                  size_t len)
{
  log_info("url=%s, auth=%s", url.c_str(), auth.c_str());
  CloudConnection *conn = CloudConnectionFactory::NewConnection(url, auth);
  if (conn) {
    conn->SetLimits(range_start, range_end, len);
    connections_.push_back(conn);
  }
}

static void OnExit(int sig)
{
  if (exiting_g) {
    exit(1);
  }
  exiting_g = true;
}

void StatGenerator::HandleCntrlC()
{
  struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = OnExit;
	sigaction(SIGINT, &sa, NULL);
}

void StatGenerator::Run(int count, int interval, bool repeat)
{
  accumulator_set<double, stats<tag::count, tag::min, tag::max, tag::mean > > time_acc;
  accumulator_set<double, stats<tag::count, tag::min, tag::max, tag::mean > > speed_acc;
  log_info("");
  HandleCntrlC();
  while (!exiting_g && count > 0) {
    for (auto conn: connections_) {
      Statistics stat;
      conn->PerformGet(&stat);
      time_acc(Statistics::Msec(stat.GetTotalTime()));
      speed_acc(Statistics::MBsec(stat.GetTotalTime(),
                                  stat.get_data_size()));
      DumpStatistics(stat);
    }
    sleep(interval);
    if (!repeat) {
      count -= 1;
    }
  }
  if (boost::accumulators::count(time_acc) > 0) {
    log_println("\ntime  min/avg/max = %.2f/%.2f/%.2f ms",
                min(time_acc),
                mean(time_acc),
                max(time_acc));
    log_println("speed min/avg/max = %.2f/%.2f/%.2f MB/s",
                min(speed_acc),
                mean(speed_acc),
                max(speed_acc));
  }
}

void StatGenerator::DumpStatistics(const Statistics &stat)
{
  auto start_time = stat.GetStartTime();
  auto total_time_msec = Statistics::Msec(stat.GetTotalTime());
  auto actual_speed_in_mb_sec = Statistics::MBsec(stat.GetTotalTime(),
                                                  stat.get_data_size());
  auto max_speed_in_mb_sec = Statistics::MBsec(stat.GetTotalTime(),
                                               std::max((size_t)1024*1024,
                                                        stat.get_data_size()));
  if (stat.IsSuccess()) {
    log_println("[%ld.%ld] %ld bytes from %s time=%.2f msec speed=%.2f[max %.2f] mb/sec",
                std::get<0>(start_time), std::get<1>(start_time),
                stat.get_data_size(),
                stat.get_url().c_str(),
                total_time_msec,
                actual_speed_in_mb_sec, max_speed_in_mb_sec);
  }
  else {
    log_println("[%ld.%ld] %s code=%ld",
                std::get<0>(start_time), std::get<1>(start_time),
                stat.get_url().c_str(), stat.get_http_code());
  }

}


Statistics::Statistics():
  url_(""), first_data_(true), data_size_(0)
{
  memset(times_, 0, sizeof(times_));
  memset(flags_, 0, sizeof(flags_));
}

bool Statistics::RecordEventOnFirstTime(EventType event)
{
  if (!flags_[event]) {
    gettimeofday(&times_[event], NULL);
    flags_[event] = true;
    return true;
  }
  return false;
}

void Statistics::RecordEvent(EventType event)
{
  gettimeofday(&times_[event], NULL);
}

void Statistics::OnReqSendHeaders()
{
  log_info("");
  RecordEventOnFirstTime(HEADERS_SEND_START);
}

void Statistics::OnReqRecvHeaders()
{
  log_info("");
  if (!RecordEventOnFirstTime(HEADERS_RECV_START)) {
    RecordEvent(HEADERS_RECV_END);
  }
}

void Statistics::OnComplete(unsigned long http_code)
{
  log_info("http_code=%ld", http_code);
  http_code_ = http_code;
}

void Statistics::OnReqRecvData(size_t size)
{
  log_info("size=%ld", size);
  RecordEventOnFirstTime(DATA_RECV_START);
  RecordEvent(DATA_RECV_END);
  data_size_ += size;
}

inline tuple<uint64_t, uint64_t> Statistics::GetStartTime() const
{
  const struct timeval *t = &times_[HEADERS_SEND_START];
  return std::make_tuple(t->tv_sec, t->tv_usec);
}


inline tuple<uint64_t, uint64_t> Statistics::GetTotalTime() const
{
  const struct timeval *t_start = &times_[HEADERS_SEND_START];
  // const struct timeval *t_start = &times_[DATA_RECV_START];
  const struct timeval *t_end = &times_[DATA_RECV_END];

  uint64_t sec = t_end->tv_sec - t_start->tv_sec;
  uint64_t usec = t_end->tv_usec - t_start->tv_usec;
  if (t_end->tv_usec < t_start->tv_usec) {
    sec -= 1;
    usec += 1000000;
  }
  return std::make_tuple(sec, usec);
}

/* static */
double Statistics::Msec(const tuple<uint64_t, uint64_t>& t)
{
  return std::get<0>(t)*1000 + (std::get<1>(t) / 1000.0);
}

double Statistics::MBsec(const tuple<uint64_t, uint64_t>& t, size_t size)
{
  double usec = std::get<0>(t)*1000000 + std::get<1>(t);
  double sec = usec / 1000000;
  return (boost::numeric_cast<double>(size) / 1048576) / sec;
}
