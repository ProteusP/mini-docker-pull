#include "../http/http_client.hpp"
#include <unordered_map>

const std::string realmKey = "realm";
const std::string serviceKey = "service";
const std::string scopeKey = "scope";
const std::string authHeaderKey = "www-authenticate";

class Client {
public:
  HttpResponse getManifest(std::string image_url) const;

  std::unordered_map<std::string, std::string>
  parseChallenge(const std::string &challenge) const;

  std::string
  craftAuthUrl(std::unordered_map<std::string, std::string> &kvs) const;

  std::string getChallengeStr(HttpResponse &resp) const {
    return resp.headers.get(authHeaderKey).second;
  }

private:
  HttpClient hClient_;
};