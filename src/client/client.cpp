#include "client.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <unordered_map>

std::string
Client::craftAuthUrl(std::unordered_map<std::string, std::string> &kvs) const {

  return kvs[realmKey] + '?' + "service=" + kvs[serviceKey] + '&' +
         "scope=" + kvs[scopeKey];
}

std::unordered_map<std::string, std::string>
Client::parseChallenge(const std::string &challenge) const {
  std::unordered_map<std::string, std::string> params;

  std::string data = challenge;

  data.erase(0, data.find_first_not_of(" \t"));
  if (data.find("Bearer ", 0) == 0) {
    data = data.substr(7);
  }

  std::stringstream ss(data);
  std::string token;

  while (std::getline(ss, token, ',')) {

    token.erase(0, token.find_first_not_of(" \t\r\n"));
    token.erase(token.find_last_not_of(" \t\r\n") + 1);

    size_t eq_pos = token.find('=');
    if (eq_pos == std::string::npos) {
      continue;
    }

    std::string key = token.substr(0, eq_pos);

    std::string value = token.substr(eq_pos + 1);

    if (!value.empty() && value.front() == '"' && value.back() == '"') {
      value = value.substr(1, value.length() - 2);
    }

    params[key] = value;
  }

  return params;
}

std::string Client::getAuthUrl() {
  HttpHeaders headers{};
  headers.add(
      {"Accept", "application/vnd.docker.distribution.manifest.v2+json"});
  headers.add({"User-Agent", "my-docker-client/0.1"});

  auto resp = hClient_.get(pullData.imageURL, headers);

  auto challenge = getChallengeStr(resp);

  auto challenge_KVS = parseChallenge(challenge);

  return craftAuthUrl(challenge_KVS);
}

// TODO: add auth errors checks
void Client::authenticate() {
  HttpHeaders auth_headers{};

  auth_headers.add({"User-Agent", "my-docker-client/0.1"});
  auth_headers.add({"Accept", "application/json"});

  auto auth_resp = hClient_.get(getAuthUrl(), auth_headers);

  auto body_json = nlohmann::json::parse(auth_resp.body);

  pullData.authToken = body_json["token"].get<std::string>();
}

// TODO: add errors checks
void Client::fillManifestDigest() {
  HttpHeaders headers{};
  headers.add(
      {"Accept", "application/vnd.docker.distribution.manifest.v2+json"});
  headers.add({"User-Agent", "my-docker-client/0.1"});

  headers.add(getAuthHeader());

  auto resp = hClient_.get(pullData.imageURL, headers);

  auto manifest_json = nlohmann::json::parse(resp.body);

  for (auto man : manifest_json["manifests"]) {
    auto pl = man["platform"];
    if (pl["architecture"] == "amd64" && pl["os"] == "linux") {
      pullData.manifestDigest = man["digest"];
      break;
    }
  }
}

// TODO: Refactor
HttpResponse Client::getManifest(std::string image_url) {
  pullData.imageURL = image_url;

  HttpHeaders headers{};
  headers.add(
      {"Accept", "application/vnd.docker.distribution.manifest.v2+json"});
  headers.add({"User-Agent", "my-docker-client/0.1"});

  authenticate();

  fillManifestDigest();

  headers.add(getAuthHeader());

  auto last_slash = image_url.find_last_of('/');
  auto path = image_url.substr(0, last_slash);

  const auto img_url = path + '/' + pullData.manifestDigest;

  auto resp = hClient_.get(img_url, headers);

  auto man_j = nlohmann::json::parse(resp.body);

  std::cout << man_j.dump(4) << '\n';
  std::cout << man_j["layers"] << '\n';

  // отправил запрос на url1 -> получил из хедеров url2 с адресом для токена
  // DONE отправил запрос на url2 -> получил из хедеров токен DONE положил
  // токен в хедер запроса -> отправил запрос на url1 получил СПИСОК манифестОВ
  // выбрали нужный манифест, взяли digest -> отправляем запрос на получение
  // digest`ов blob`сов
  // -> качаем blobs

  return {};
}