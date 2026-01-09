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

// TODO: Refactor
HttpResponse Client::getManifest(std::string image_url) const {

  HttpHeaders headers{};
  headers.add(
      {"Accept", "application/vnd.docker.distribution.manifest.v2+json"});
  headers.add({"User-Agent", "my-docker-client/0.1"});

  auto resp = hClient_.get(image_url, headers);

  auto challenge = getChallengeStr(resp);

  auto challenge_KVS = parseChallenge(challenge);

  auto auth_url = craftAuthUrl(challenge_KVS);

  HttpHeaders auth_headers{};

  auth_headers.add({"User-Agent", "my-docker-client/0.1"});
  auth_headers.add({"Accept", "application/json"});

  auto auth_resp = hClient_.get(auth_url, auth_headers);

  auto body_json = nlohmann::json::parse(auth_resp.body);
  auto token = body_json["token"].get<std::string>();

  std::string auth_string = "Bearer " + token;

  Header auth_header = {"Authorization", auth_string};

  headers.add(auth_header);

  resp = hClient_.get(image_url, headers);

  auto manifest_json = nlohmann::json::parse(resp.body);

  std::cout << manifest_json.dump(4) << '\n';

  std::string digest{};

  for (auto man : manifest_json["manifests"]) {
    auto pl = man["platform"];
    if (pl["architecture"] == "amd64" && pl["os"] == "linux") {
      digest = man["digest"];
      break;
    }
  }

  // отправил запрос на url1 -> получил из хедеров url2 с адресом для токена
  // DONE отправил запрос на url2 -> получил из хедеров токен DONE положил
  // токен в хедер запроса -> отправил запрос на url1 получил манифест

  return {};
}