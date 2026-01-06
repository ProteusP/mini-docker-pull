#include "http_client.hpp"
#include <curl/curl.h>
#include <curl/easy.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>

std::string HttpHeaders::headerToKV(const Header &header) {
  return header.first + ": " + header.second;
}

void HttpHeaders::add(Header header) { headers_.push_back(header); }

Header HttpHeaders::get(const std::string &key) const {
  for (auto header : headers_) {
    if (header.first == key) {
      return header;
    }
  }
  return {};
}

std::vector<Header> &HttpHeaders::all() { return headers_; }

// buf should be a correct header!
Header HttpHeaders::KVToHeader(const std::string &buf) {
  size_t colon_pos = buf.find(':');

  std::string key = buf.substr(0, colon_pos);

  std::string value;
  if (colon_pos + 1 < buf.length()) {
    value = buf.substr(colon_pos + 1);
  }

  return Header{key, value};
}

size_t header_cb(char *buffer, size_t size, size_t nitems, void *userdata) {

  std::string KVHeader_str(buffer, nitems);
  if (KVHeader_str.find(':') == std::string::npos) {
    return nitems;
  }

  auto header = HttpHeaders::KVToHeader(KVHeader_str);
  auto http_headers = static_cast<HttpHeaders *>(userdata);
  http_headers->add(header);

  return nitems;
}

curl_slist *HttpHeaders::toCurlSlist() const {

  curl_slist *list = nullptr;

  for (auto &header : headers_) {
    list = curl_slist_append(list, headerToKV(header).c_str());
  }

  // slist_append creates list obj on heap, it`s ok
  return list;
}

HttpResponse HttpClient::get(std::string url, HttpHeaders headers) {

  CURL *curl = curl_easy_init();

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  auto headers_list = headers.toCurlSlist();
  HttpResponse response{};
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);

  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);
  // execution ...
  curl_easy_perform(curl);
  // ...
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status);

  // TODO: decide what to do with err

  /* if (response.status != CURLE_OK) {
      ???
  } */

  // Cleaning up...
  curl_slist_free_all(headers_list);
  curl_easy_cleanup(curl);

  return response;
}

// TODO: Refactor for HEAVY FILES (blobs)
size_t get_cb(void *data, size_t size, size_t nmemb, void *userdata) {
  size_t bytes = size * nmemb;
  auto out = (std::string *)userdata;
  out->append((char *)data, bytes);

  return bytes;
}