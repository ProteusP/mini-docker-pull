#pragma once
#include <curl/curl.h>
#include <string>
#include <utility>
#include <vector>

enum HttpMethod { GET, HEAD };

using Header = std::pair<std::string, std::string>;

struct HttpHeaders {
public:
  bool has(Header header);
  Header get(const std::string &key) const;
  void set(std::string key, std::string value);
  void add(Header header);

  std::vector<Header> &all();

  static std::string headerToKV(const Header &header);

  static Header KVToHeader(const std::string &buf);

  curl_slist *toCurlSlist() const;

private:
  std::vector<Header> headers_;
};

class HttpResponse {
public:
  HttpHeaders headers;
  long status;
  std::string body; // for Manifest
};

class HttpRequest {
public:
  HttpMethod method;
  std::string url;
  HttpHeaders headers;
};

class HttpClient {
public:
  static HttpResponse get(std::string url, HttpHeaders headers);
  static HttpResponse head(std::string url, HttpHeaders headers);
  // void setAuth();
  // void setTimeout(ms);
};

size_t header_cb(char *buffer, size_t size, size_t nitems, void *userdata);

size_t get_cb(void *data, size_t size, size_t nmemb, void *userdata);