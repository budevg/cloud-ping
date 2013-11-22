#include "http_conn.h"
#include "http_req.h"
#include "stat_gen.h"

void HttpConnection::PerformGet(Statistics *stat)
{
  HttpReq req;
  stat->set_url(url_);
  req.SetUrl("http://" + url_);
  ApplyLimits(&req);
  req.ReportEvents(stat);
  req.PerformGet();
}
