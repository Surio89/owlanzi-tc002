// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/http.hpp"
#include "owlanzi/version.hpp"
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#else
#include <curl/curl.h>
#endif
namespace owlanzi {
namespace {
constexpr std::size_t BodyLimit=1024*1024;
#ifdef _WIN32
std::wstring wide(const std::string& value) {
 int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0);
 if(size<=0) throw std::runtime_error("Invalid request encoding");
 std::wstring out(size,0); MultiByteToWideChar(CP_UTF8,0,value.data(),static_cast<int>(value.size()),&out[0],size);return out;
}
struct Handle {HINTERNET value=nullptr; ~Handle(){if(value)WinHttpCloseHandle(value);} operator HINTERNET()const{return value;}};
class NativeHttps final:public HttpTransport {
 std::function<bool()> cancel_;
public:
 explicit NativeHttps(std::function<bool()> cancel):cancel_(std::move(cancel)){}
 bool cancelled()const override{return cancel_&&cancel_();}
 HttpResponse perform(const HttpRequest& r) override {
  if(cancelled())throw std::runtime_error("Request cancelled");
  if(r.url.rfind("https://",0)!=0) throw std::runtime_error("HTTPS required");
  auto url=wide(r.url); URL_COMPONENTS parts{};parts.dwStructSize=sizeof(parts);parts.dwHostNameLength=parts.dwUrlPathLength=parts.dwExtraInfoLength=static_cast<DWORD>(-1);
  if(!WinHttpCrackUrl(url.c_str(),0,0,&parts)) throw std::runtime_error("Invalid HTTPS URL");
  std::wstring host(parts.lpszHostName,parts.dwHostNameLength), path(parts.lpszUrlPath,parts.dwUrlPathLength);
  if(parts.dwExtraInfoLength)path.append(parts.lpszExtraInfo,parts.dwExtraInfoLength);
  Handle session{WinHttpOpen(L"Owlanzi-TC002/0.1.0",WINHTTP_ACCESS_TYPE_NO_PROXY,nullptr,nullptr,0)};
  if(!session.value) throw std::runtime_error("HTTPS initialization failed");
  WinHttpSetTimeouts(session,10000,10000,15000,15000);
  DWORD protocols=WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2; WinHttpSetOption(session,WINHTTP_OPTION_SECURE_PROTOCOLS,&protocols,sizeof(protocols));
  Handle connection{WinHttpConnect(session,host.c_str(),parts.nPort,0)};
  auto method=wide(r.method);
  Handle request{WinHttpOpenRequest(connection,method.c_str(),path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE)};
  if(!connection.value||!request.value)throw std::runtime_error("HTTPS initialization failed");
  DWORD redirect=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&redirect,sizeof(redirect));
  std::wstring headers=L"Accept: application/json\r\nContent-Type: application/json\r\n";
  for(const auto& h:r.headers) headers+=wide(h.first+": "+h.second+"\r\n");
  if(!WinHttpSendRequest(request,headers.c_str(),static_cast<DWORD>(headers.size()),r.body.empty()?WINHTTP_NO_REQUEST_DATA:const_cast<char*>(r.body.data()),static_cast<DWORD>(r.body.size()),static_cast<DWORD>(r.body.size()),0)||!WinHttpReceiveResponse(request,nullptr))
   throw std::runtime_error("HTTPS connection failed (network, certificate or timeout)");
  DWORD code=0,size=sizeof(code);WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&code,&size,nullptr);
  HttpResponse response;response.status=static_cast<int>(code);char block[8192];DWORD got=0;
  auto started=GetTickCount64();
  for(;;) {
   if(cancelled())throw std::runtime_error("Request cancelled");
   if(!WinHttpReadData(request,block,sizeof(block),&got))throw std::runtime_error("HTTPS response interrupted");
   if(!got)break;
   if(response.body.size()+got>BodyLimit||GetTickCount64()-started>20000)throw std::runtime_error("HTTPS response exceeded limit");
   response.body.append(block,got);
  }
  return response;
 }
};
#else
class NativeHttps final:public HttpTransport {
 std::string ca_;
 std::function<bool()> cancel_;
 static int progress(void* context,curl_off_t,curl_off_t,curl_off_t,curl_off_t) {return static_cast<NativeHttps*>(context)->cancelled()?1:0;}
 static std::size_t receive(char* bytes,std::size_t size,std::size_t count,void* user) {
  auto& body=*static_cast<std::string*>(user);auto n=size*count;
  if(n>BodyLimit||body.size()>BodyLimit-n)return 0;body.append(bytes,n);return n;
 }
public:
 explicit NativeHttps(std::string ca,std::function<bool()> cancel):ca_(std::move(ca)),cancel_(std::move(cancel)) {
  static int initialized=curl_global_init(CURL_GLOBAL_DEFAULT);if(initialized!=CURLE_OK)throw std::runtime_error("HTTPS initialization failed");
 }
 bool cancelled()const override{return cancel_&&cancel_();}
 HttpResponse perform(const HttpRequest& r)override {
  if(cancelled())throw std::runtime_error("Request cancelled");
  if(r.url.rfind("https://",0)!=0)throw std::runtime_error("HTTPS required");
  auto curl=curl_easy_init();if(!curl)throw std::runtime_error("HTTPS initialization failed");
  curl_slist* headers=nullptr;headers=curl_slist_append(headers,"Accept: application/json");headers=curl_slist_append(headers,"Content-Type: application/json");
  for(auto& h:r.headers)headers=curl_slist_append(headers,(h.first+": "+h.second).c_str());
  HttpResponse response;
  curl_easy_setopt(curl,CURLOPT_URL,r.url.c_str());curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
  curl_easy_setopt(curl,CURLOPT_USERAGENT,"Owlanzi-TC002/" OWLANZI_APP_VERSION);
  curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);
  curl_easy_setopt(curl,CURLOPT_SSLVERSION,CURL_SSLVERSION_TLSv1_2);
  curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,0L);curl_easy_setopt(curl,CURLOPT_PROTOCOLS,CURLPROTO_HTTPS);
  curl_easy_setopt(curl,CURLOPT_PROXY,"");curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1L);
  curl_easy_setopt(curl,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(curl,CURLOPT_XFERINFOFUNCTION,progress);curl_easy_setopt(curl,CURLOPT_XFERINFODATA,this);
  curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,10L);curl_easy_setopt(curl,CURLOPT_TIMEOUT,25L);
  curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,receive);curl_easy_setopt(curl,CURLOPT_WRITEDATA,&response.body);
  if(!ca_.empty())curl_easy_setopt(curl,CURLOPT_CAINFO,ca_.c_str());
  if(r.method=="POST") {curl_easy_setopt(curl,CURLOPT_POST,1L);curl_easy_setopt(curl,CURLOPT_POSTFIELDS,r.body.c_str());curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,static_cast<long>(r.body.size()));}
  auto result=curl_easy_perform(curl);long code=0;curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&code);response.status=static_cast<int>(code);
  curl_slist_free_all(headers);curl_easy_cleanup(curl);
  if(result!=CURLE_OK)throw std::runtime_error("HTTPS connection failed (network, certificate, timeout or size)");
  return response;
 }
};
#endif
}
std::unique_ptr<HttpTransport> makeHttpsTransport(const std::string& ca,std::function<bool()> cancel) {
#ifdef _WIN32
 (void)ca;return std::make_unique<NativeHttps>(std::move(cancel));
#else
 return std::make_unique<NativeHttps>(ca,std::move(cancel));
#endif
}
}
