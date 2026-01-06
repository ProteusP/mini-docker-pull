#include "../http/http_client.hpp"
#include <iostream>

int main() {

  HttpClient client{};
  HttpHeaders headers{};
  headers.add(
      {"Accept", "application/vnd.docker.distribution.manifest.v2+json"});
  headers.add({"User-Agent", "my-docker-client/0.1"});
  auto resp = client.get(
      "https://registry-1.docker.io/v2/library/ubuntu/manifests/latest",
      headers);

  for (auto h : resp.headers.all()) {
    std::cout << h.first << " " << h.second << '\n';
  }

  std::cout << "\nEND OF PROGRAM!" << std::endl;
}