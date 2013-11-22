
#include <openssl/crypto.h>
#include <gcrypt.h>
#include <pthread.h>
#include "boost/format.hpp"
#include "logging.h"
#include "errors.h"

#include "http_req.h"

using boost::format;

static pthread_mutex_t *openssl_locks;
static int num_openssl_locks;

GCRY_THREAD_OPTION_PTHREAD_IMPL;

static void openssl_locking_callback(int mode, int i, const char *file, int line)
{
  if ((mode & CRYPTO_LOCK) != 0) {
    pthread_mutex_lock(&openssl_locks[i]);
  }
  else {
    pthread_mutex_unlock(&openssl_locks[i]);
  }
}

static u_long openssl_id_callback(void)
{
  return (u_long)pthread_self();
}

static int http_sockopt_callback(void *clientp, curl_socket_t curlfd, curlsocktype purpose)
{
  int i = 1;
  if(setsockopt(curlfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)))
    return CURL_SOCKOPT_ERROR;
  return CURL_SOCKOPT_OK;
}

static size_t http_read_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  HttpReq *req = (HttpReq *)userdata;
  return req->CurlReadCallback(ptr, size*nmemb);
}

static size_t http_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  HttpReq *req = (HttpReq *)userdata;
  return req->CurlWriteCallback(ptr, size*nmemb);
}

static int http_debug_callback(CURL *curl, curl_infotype infotype, char *buf, size_t len, void *userdata)
{
  HttpReq *req = (HttpReq*)userdata;
  return req->CurlDebugCallback(curl, infotype, buf, len);
}

HttpReq::HttpReq():
  recv_limit_(0), recv_size_(0),
  curl_(curl_easy_init()), curl_headers_(nullptr)
{
}

HttpReq::~HttpReq()
{
  if (curl_headers_ != NULL) {
    curl_slist_free_all(curl_headers_);
  }
  if (curl_ != NULL) {
    curl_easy_cleanup(curl_);
  }
}

void HttpReq::SetUrl(const string &url)
{
  url_ = url;
}

void HttpReq::AddHeader(const string& header)
{
  headers_.push_back(header);
}

void HttpReq::AddHeader(const string& name, const string& value)
{
  headers_.push_back(boost::str(format("%s: %s") % name % value));
}

void HttpReq::AddGetRangeHeader(uint64_t start, uint64_t end)
{
  if (start == std::numeric_limits<decltype(start)>::max() &&
      end == std::numeric_limits<decltype(end)>::max()) {
    return;
  }
  else if (start == std::numeric_limits<decltype(start)>::max()) {
    headers_.push_back(boost::str(format("Range: bytes=%0-%ld") % end));
  }
  else if (end == std::numeric_limits<decltype(end)>::max() || end == 0) {
    headers_.push_back(boost::str(format("Range: bytes=%ld-") % start));
  }
  else {
    headers_.push_back(boost::str(format("Range: bytes=%ld-%ld") % start % (end -1)));
  };
}

void HttpReq::ReportEvents(HttpReqEvents *events)
{
  events_ = events;
}

void HttpReq::SetCurlHeaders()
{
  for (auto header: headers_) {
    auto new_curl_headers = curl_slist_append(curl_headers_, header.c_str());
    if (new_curl_headers == NULL) {
      log_error("failed to allocate curl header");
      return;
    }
    curl_headers_ = new_curl_headers;
  }
}

void HttpReq::SetCurlOptions()
{
  curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1);

  // curl_easy_setopt(curl, CURLOPT_TIMEOUT, download_timeout_);
  curl_easy_setopt(curl_, CURLOPT_ERRORBUFFER, curl_error_buffer_);
  curl_easy_setopt(curl_, CURLOPT_FAILONERROR, 1);
  curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1);
  curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 1);
  curl_easy_setopt(curl_, CURLOPT_PRIVATE, this);

  curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());
  curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, curl_headers_);

  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0);

  curl_easy_setopt(curl_, CURLOPT_SOCKOPTFUNCTION, http_sockopt_callback);
  curl_easy_setopt(curl_, CURLOPT_SOCKOPTDATA, 0);

  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, http_write_callback);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);

  curl_easy_setopt(curl_, CURLOPT_READFUNCTION, http_read_callback);
  curl_easy_setopt(curl_, CURLOPT_READDATA, this);

  curl_easy_setopt(curl_, CURLOPT_DEBUGFUNCTION, http_debug_callback);
  curl_easy_setopt(curl_, CURLOPT_DEBUGDATA, this);

}

void HttpReq::SetDataLimit(size_t len)
{
  recv_limit_ = len;
}

void HttpReq::InvokeCurl()
{
  unsigned long http_code;
  curl_easy_perform(curl_);
  curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
  events_->OnComplete(http_code);
}

void HttpReq::PerformGet()
{
  SetCurlHeaders();
  SetCurlOptions();
  InvokeCurl();
}


size_t HttpReq::CurlReadCallback(char *data, size_t size)
{
  return size;
}

size_t HttpReq::CurlWriteCallback(char *data, size_t size)
{
  if (events_) {
    events_->OnReqRecvData(size);
  }
  recv_size_ += size;
  if (recv_limit_ > 0 && recv_size_ > recv_limit_) {
    return CURLE_WRITE_ERROR;
  }
  return size;
}

int HttpReq::CurlProgressCallback(double download_total, double download_now,
                                  double upload_total, double upload_now)
{
  log_info("download_total=%.2f, download_now=%.2f, upload_total=%.2f, upload_now=%.2f",
           download_total, download_now, upload_total, upload_now);
  return 0;
}

int HttpReq::CurlDebugCallback(CURL *curl,
                               curl_infotype infotype, char *buf, size_t len)
{
  switch (infotype) {
  case CURLINFO_TEXT:
    log_info("CURLINFO_TEXT buf=%s, len=%ld", buf, len);
    break;
  case CURLINFO_HEADER_IN:
    log_info("CURLINFO_HEADER_IN buf=%s, len=%ld", buf, len);
    events_->OnReqRecvHeaders();
    break;
  case CURLINFO_HEADER_OUT:
    log_info("CURLINFO_HEADER_OUT buf=%s, len=%ld", buf, len);
    events_->OnReqSendHeaders();
    break;
  case CURLINFO_DATA_IN:
    // log_info("CURLINFO_DATA_IN buf=%p, len=%ld", buf, len);
    break;
  case CURLINFO_DATA_OUT:
    // log_info("CURLINFO_DATA_OUT buf=%p, len=%ld", buf, len);
    break;
  case CURLINFO_SSL_DATA_IN:
    // log_info("CURLINFO_SSL_DATA_IN buf=%p, len=%ld", buf, len);
    break;
  case CURLINFO_SSL_DATA_OUT:
    // log_info("CURLINFO_SSL_DATA_OUT buf=%p, len=%ld", buf, len);
    break;
  default:
    break;
  }
  return 0;
}


/* static */
int HttpReq::Init()
{
  /* init gcrypt */
  gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
  /* Init openssl */
  num_openssl_locks = CRYPTO_num_locks();
  log_notice("Allocating %d locks for open ssl", num_openssl_locks);

  openssl_locks = (pthread_mutex_t *)malloc(num_openssl_locks * sizeof(pthread_mutex_t));
  if (openssl_locks == NULL) {
    log_error("Failed to allocate locks for open ssl");
    return RET_FAIL;
  }

  for (int i = 0; i < num_openssl_locks; i++) {
    int ret = pthread_mutex_init(&openssl_locks[i], NULL);
    if (ret != 0) {
      log_error("Failed to init mutex, ret=%d", ret);
      for (int j=0; j<i; j++) {
        pthread_mutex_destroy(&openssl_locks[j]);
      }
      return RET_FAIL;
    }
  }

  CRYPTO_set_locking_callback(openssl_locking_callback);
  CRYPTO_set_id_callback(openssl_id_callback);

  /* Init curl */
  curl_global_init(CURL_GLOBAL_ALL);

  return RET_OK;

}

/* static */
void HttpReq::Fini()
{
  /* Clean curl */
  curl_global_cleanup();

  /* Clean openssl */
  CRYPTO_set_locking_callback(NULL);
  CRYPTO_set_id_callback(NULL);

  for (int i=0; i<num_openssl_locks; i++) {
    pthread_mutex_destroy(&openssl_locks[i]);
  }

  free(openssl_locks);
  openssl_locks = NULL;
}
