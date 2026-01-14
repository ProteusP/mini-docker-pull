#include "../client/client.hpp"
#include "../http/http_client.hpp"
#include <cassert>
#include <iostream>

int main() {
  std::string url =
      "https://registry-1.docker.io/v2/library/alpine/manifests/latest";
  Client cl{};

  
  cl.getManifest(url);

  std::cout << "\nEND OF PROGRAM!" << std::endl;
}