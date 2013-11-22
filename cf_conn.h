#ifndef _CF_CONN_H_
#define _CF_CONN_H_

#include "cloud_conn.h"

class CloudFrontConnection : public CloudConnection
{
public:
  CloudFrontConnection(const string &url, const string &auth):
    CloudConnection(url, auth) {}
  virtual void PerformGet(Statistics *stat);
private:
  string BuildSignedUrl();
};



#endif /* _CF_CONN_H_ */
