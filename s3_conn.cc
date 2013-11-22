#include <boost/format.hpp>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include "s3_conn.h"
#include "stat_gen.h"
#include "logging.h"

static const string DATE_HEADER             = "Date";
static const string AUTH_HEADER             = "Authorization";
static const int    S3_DATE_BUF_SIZE        = 64;
static const int    S3_AUTH_BUF_SIZE        = 200;
static const string S3_DATE_BUF_FMT         = "%a, %d %b %Y %H:%M:%S GMT";

static void s3_base64_encode(char *buf, size_t bufsiz, const void *data, size_t len)
{
    BUF_MEM *bptr;
    BIO* bmem;
    BIO* b64;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, data, len);
    (void)BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);
    snprintf(buf, bufsiz, "%.*s",(int) bptr->length - 1, (char *)bptr->data);
    BIO_free_all(b64);
}

static string GetDate()
{
  char buf[S3_DATE_BUF_SIZE];
  time_t now = time(NULL);
  struct tm tm;

  strftime(buf, sizeof(buf), S3_DATE_BUF_FMT.c_str(), gmtime_r(&now, &tm));
  return buf;
}

static string GetAuth(const string& bucket,
                      const string& resource,
                      const string& secret_key,
                      const string& date)
{
  u_char hmac[SHA_DIGEST_LENGTH];
  u_int hmac_len;
  HMAC_CTX ctx;
  char buf[S3_AUTH_BUF_SIZE];

  HMAC_CTX_init(&ctx);
  HMAC_Init_ex(&ctx, secret_key.c_str(), secret_key.size(),
               EVP_sha1(), NULL);

  HMAC_Update(&ctx, (const u_char *)"GET", 3);
  HMAC_Update(&ctx, (const u_char *)"\n", 1);
  HMAC_Update(&ctx, (const u_char *)"\n", 1);
  HMAC_Update(&ctx, (const u_char *)"\n", 1);
  HMAC_Update(&ctx, (const u_char *)date.c_str(), date.size());
  HMAC_Update(&ctx, (const u_char *)"\n", 1);

  HMAC_Update(&ctx, (const u_char *)"/", 1);
  HMAC_Update(&ctx, (const u_char *)bucket.c_str(), bucket.size());
  HMAC_Update(&ctx, (const u_char *)resource.c_str(), resource.size());

  HMAC_Final(&ctx, hmac, &hmac_len);
  assert(hmac_len == SHA_DIGEST_LENGTH);
  HMAC_CTX_cleanup(&ctx);

  s3_base64_encode(buf, sizeof(buf), hmac, hmac_len);
  return buf;
}

void S3Connection::PerformGet(Statistics *stat)
{
  if (url_.find("/") == string::npos) {
    log_error("invalid s3 url: missing resource");
    return;
  }

  if (auth_.find(":") == string::npos) {
    log_error("invalid s3 auth string");
    return;
  }

  string host = "s3.amazonaws.com";
  string url = url_;
  if (url.find(":") != string::npos) {
    host = url.substr(0, url.find(":"));
    url = url.substr(url.find(":")+1);
  }
  string bucket = url.substr(0, url.find("/"));
  string resource = url.substr(url.find("/"));
  string access_key = auth_.substr(0, auth_.find(":"));
  string secret_key = auth_.substr(auth_.find(":")+1);

  string s3_url = string("http://") +  bucket + "." + host  + resource;
  if (url_.find(":") != string::npos) {
    s3_url = string("http://") + host + "/" + bucket + resource;
  }
  log_info("s3 url: %s", s3_url.c_str());

  HttpReq req;
  req.SetUrl(s3_url);

  string date = GetDate();
  req.AddHeader(DATE_HEADER, date.c_str());

  string auth = GetAuth(bucket, resource, secret_key, date);
  boost::format  auth_header("%s: AWS %s:%s");
  auth_header % AUTH_HEADER % access_key % auth;
  req.AddHeader(auth_header.str());

  stat->set_url(s3_url);
  ApplyLimits(&req);
  req.ReportEvents(stat);
  req.PerformGet();


}
