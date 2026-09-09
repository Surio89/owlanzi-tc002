// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <functional>
namespace owlanzi {
struct HttpRequest {
 std::string method, url, body;
 std::vector<std::pair<std::string,std::string>> headers;
};
struct HttpResponse {int status=0;std::string body;};
class HttpTransport {
public:
 virtual ~HttpTransport()=default;
 virtual HttpResponse perform(const HttpRequest& request)=0;
 virtual bool cancelled()const{return false;}
};
// Fixed HTTPS URLs, mandatory peer/hostname verification, bounded bodies/time.
std::unique_ptr<HttpTransport> makeHttpsTransport(const std::string& caBundle = "", std::function<bool()> cancelled = {});
}
