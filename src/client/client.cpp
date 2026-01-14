#include "client.hpp"
#include <iostream>
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

std::string Client::getRepoPath() {
  auto last_slash = pullData.imageURL.find_last_of('/');
  auto path_without_ref = pullData.imageURL.substr(0, last_slash);

  auto last_slash_2 = path_without_ref.find_last_of('/');
  auto repo_path = path_without_ref.substr(0, last_slash_2);

  return repo_path;
}

void Client::fillBlobsURL() {
  pullData.blobsURL = getRepoPath() + '/' + "blobs";
}

void Client::fillBlobsData(nlohmann::json manifest_json) {
  fillBlobsURL();

  // fill config digest
  pullData.configDigest = manifest_json["config"]["digest"];
  // fill layers digests
  for (auto j : manifest_json["layers"]) {
    pullData.layersDigests.push_back(j["digest"]);
  }
}

void Client::downloadBlobs() {

  downloadConfig();

  downloadLayers();
}

void Client::downloadLayer(std::string digest) {
  auto path = pullData.blobsURL + '/' + digest;

  HttpHeaders headers{};

  headers.add({"Accept", "application/vnd.oci.image.layer.v1.tar+gzip"});
  headers.add({"User-Agent", "my-docker-client/0.1"});

  headers.add(getAuthHeader());

  auto resp = hClient_.download(path, headers, "./" + digest + ".tar.gzip");
}

void Client::downloadLayers() {
  for (auto layer_digest : pullData.layersDigests) {
    downloadLayer(layer_digest);
  }
}

void Client::downloadConfig() {
  auto path = pullData.blobsURL + '/' + pullData.configDigest;
  HttpHeaders headers{};

  headers.add({"Accept", "application/vnd.oci.image.config.v1+json"});
  headers.add({"User-Agent", "my-docker-client/0.1"});

  headers.add(getAuthHeader());

  auto resp = hClient_.get(path, headers);

  auto man_j = nlohmann::json::parse(resp.body);

  std::cout << man_j.dump(4) << '\n';
}

nlohmann::json Client::getManifest() {
  HttpHeaders headers{};
  headers.add(
      {"Accept", "application/vnd.docker.distribution.manifest.v2+json"});
  headers.add({"User-Agent", "my-docker-client/0.1"});

  headers.add(getAuthHeader());

  auto last_slash = pullData.imageURL.find_last_of('/');
  auto path = pullData.imageURL.substr(0, last_slash);

  const auto img_url = path + '/' + pullData.manifestDigest;

  auto resp = hClient_.get(img_url, headers);

  auto man_j = nlohmann::json::parse(resp.body);

  std::cout << man_j.dump(4) << '\n';

  return man_j;
}

// TODO: Refactor
HttpResponse Client::getImage(std::string image_url) {
  pullData.imageURL = image_url;

  authenticate();

  fillManifestDigest();

  fillBlobsData(getManifest());

  downloadBlobs();

  return {};
}