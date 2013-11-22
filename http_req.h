#ifndef _HTTP_REQ_H_
#define _HTTP_REQ_H_

#include <string>
#include <vector>
#include <curl/curl.h>

using std::string;
using std::vector;

class HttpReqEvents
{
public:
  virtual void OnReqSendHeaders() = 0;
  virtual void OnReqRecvHeaders() = 0;
  virtual void OnReqRecvData(size_t size) = 0;
  virtual void OnComplete(unsigned long http_code) = 0;
};

class HttpReq
{
public:
  HttpReq();
  ~HttpReq();
  void SetUrl(const string &url);
  void AddHeader(const string& header);
  void AddHeader(const string& name, const string& value);
  void AddGetRangeHeader(uint64_t start, uint64_t end);
  void ReportEvents(HttpReqEvents *events);
  void PerformGet();

  size_t CurlReadCallback(char *data, size_t size);
  size_t CurlWriteCallback(char *data, size_t size);
  int CurlProgressCallback(double download_total, double download_now,
                           double upload_total, double upload_now);
  int CurlDebugCallback(CURL *curl, curl_infotype infotype, char *buf, size_t len);

  static int Init();
  static void Fini();

  void SetCurlOptions();
  void SetCurlHeaders();
  void SetDataLimit(size_t len);
  void InvokeCurl();

private:
  string url_;
  vector<string> headers_;
  HttpReqEvents *events_;
  size_t recv_limit_;
  size_t recv_size_;

  // curl
  CURL* curl_;
  struct curl_slist* curl_headers_;
  char curl_error_buffer_[CURL_ERROR_SIZE];

};

#endif /* _HTTP_REQ_H_ */
