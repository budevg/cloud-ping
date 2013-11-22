#ifndef _HTTP_CONN_H_
#define _HTTP_CONN_H_

#include "cloud_conn.h"

class HttpConnection : public CloudConnection
{
public:
  HttpConnection(const string &url, const string &auth):
    CloudConnection(url, auth) {}
  virtual void PerformGet(Statistics *stat);

};


#endif /* _HTTP_CONN_H_ */
