#ifndef _S3_CONN_H_
#define _S3_CONN_H_

#include "cloud_conn.h"

class S3Connection : public CloudConnection
{
public:
  S3Connection(const string &url, const string &auth):
    CloudConnection(url, auth) {}
  virtual void PerformGet(Statistics *stat);
};

#endif /* _S3_CONN_H_ */
