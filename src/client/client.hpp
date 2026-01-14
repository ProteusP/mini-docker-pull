#include "../http/http_client.hpp"
#include <unordered_map>
#include <vector>

const std::string realmKey = "realm";
const std::string serviceKey = "service";
const std::string scopeKey = "scope";
const std::string authChallengeHeaderKey = "www-authenticate";
const std::string authHeaderKey = "Authorization";

class Client {
public:
  HttpResponse getManifest(std::string image_url);

  std::unordered_map<std::string, std::string>
  parseChallenge(const std::string &challenge) const;

  std::string
  craftAuthUrl(std::unordered_map<std::string, std::string> &kvs) const;

  std::string getChallengeStr(HttpResponse &resp) const {
    return resp.headers.get(authChallengeHeaderKey).second;
  }

private:
  struct PullData {
    std::string authToken;
    std::string imageURL;
    std::string manifestDigest;
    std::vector<std::string> layersDigests;
    std::string layersURL;
  } pullData;
  HttpClient hClient_;

  Header getAuthHeader() {
    std::string authHeaderValue = "Bearer " + pullData.authToken;
    return {authHeaderKey, authHeaderValue};
  }

  void authenticate();

  std::string getAuthUrl();

  void fillManifestDigest();
};