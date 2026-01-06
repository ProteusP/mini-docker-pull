#include "client.hpp"

HttpResponse Client::getManifest(std::string image_url) const {

  HttpHeaders headers{};
  headers.add(
      {"Accept", "application/vnd.docker.distribution.manifest.v2+json"});
  headers.add({"User-Agent", "my-docker-client/0.1"});

  auto resp = hClient_.get(image_url, headers);

  // отправил запрос на url1 -> получил из хедеров url2 с адресом для токена
  // отправил запрос на url2 -> получил из хедеров токен
  // положил токен в хедер запроса -> отправил запрос на url1
  // получил манифест

  return {};
}