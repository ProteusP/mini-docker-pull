#include "../client/client.hpp"
#include "../http/http_client.hpp"
#include <iostream>

int main() {

  Client cl{};
  cl.getManifest(
      "https://registry-1.docker.io/v2/library/ubuntu/manifests/latest");
  std::cout << "\nEND OF PROGRAM!" << std::endl;
}