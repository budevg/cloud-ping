#include "cloud_conn.h"
#include "stat_gen.h"
#include "http_req.h"
#include "http_conn.h"
#include "s3_conn.h"
#include "cf_conn.h"


CloudConnection::CloudConnection(const string &url, const string &auth):
  url_(url), auth_(auth)
{
}

void CloudConnection::SetLimits(uint64_t range_start,
                                uint64_t range_end,
                                size_t recv_limit_size)
{
  range_start_ = range_start;
  range_end_ = range_end;
  recv_limit_size_ = recv_limit_size;
}

void CloudConnection::ApplyLimits(HttpReq *req)
{
  if (recv_limit_size_ > 0) {
    req->SetDataLimit(recv_limit_size_);
  }
  req->AddGetRangeHeader(range_start_, range_end_);
}

/* static */
CloudConnection *CloudConnectionFactory::NewConnection(const string &url,
                                                       const string &auth)
{
  auto protocol_end = url.find("://");
  if (protocol_end == string::npos) {
    return nullptr;
  }
  auto protocol = url.substr(0, protocol_end);
  auto url_rest = url.substr(protocol_end+3);
  if (protocol == "http") {
    return new HttpConnection(url_rest, auth);
  }
  else if (protocol == "s3") {
    return new S3Connection(url_rest, auth);
  }
  else if (protocol == "cf") {
    return new CloudFrontConnection(url_rest, auth);
  }
  return nullptr;
}
