#include <boost/format.hpp>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "cf_conn.h"
#include "logging.h"
#include "http_req.h"
#include "stat_gen.h"


#define CANNED_POLICY "{\"Statement\":[{\"Resource\":\"http://%s\",\"Condition\":{\"DateLessThan\":{\"AWS:EpochTime\":%ld}}}]}"
#define SIGN_POLICY "echo -n '%s' | openssl sha1 -sign %s | openssl base64 | tr '+=/' '-_~' | tr -d '\n'"
#define SIGNED_URL "http://%s?Expires=%ld&Signature=%s&Key-Pair-Id=%s"

static string Popen(const string& cmd)
{
  string ret;
  log_info("CMD: %s", cmd.c_str());
  FILE *f = popen(cmd.c_str(), "r");
  if (f == NULL) {
    return ret;
  }

  char buf[1024];
  while (fgets(buf, sizeof(buf), f)) {
    ret.append(buf);
  }
  fclose(f);
  return ret;
}

string CloudFrontConnection::BuildSignedUrl()
{
  string key_pair_id = auth_.substr(0, auth_.find(":"));
  string priv_key_path = auth_.substr(auth_.find(":")+1);

  uint64_t expires = time(NULL);
  expires += 60*60*24;
  expires = 1387605457;

  string policy = str(boost::format(CANNED_POLICY) % url_ % expires);
  string cmd = str(boost::format(SIGN_POLICY) % policy % priv_key_path);
  string encoded_policy = Popen(cmd);
  return str(boost::format(SIGNED_URL) % url_ % expires % encoded_policy % key_pair_id);
}

void CloudFrontConnection::PerformGet(Statistics *stat)
{
  if (auth_.find(":") == string::npos) {
    log_error("invalid cloud front auth string");
    return;
  }

  string signed_url = BuildSignedUrl();
  log_info("signed url: %s", signed_url.c_str());
  HttpReq req;
  req.SetUrl(signed_url);
  stat->set_url(url_);
  ApplyLimits(&req);
  req.ReportEvents(stat);
  req.PerformGet();

}
