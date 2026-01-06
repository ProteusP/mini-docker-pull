#include "../http/http_client.hpp"

class Client {
public:
  HttpResponse getManifest(std::string image_url) const;

private:
  HttpClient hClient_;
};