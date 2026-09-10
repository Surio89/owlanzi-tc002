// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "owlanzi/config.hpp"
#include <functional>
#include <memory>
namespace owlanzi {
struct Release {std::string version,file,sha256,notes;std::size_t size=0;};
bool newerVersion(const std::string& candidate,const std::string& current);
Release parseRelease(const Json& value);
std::string updateManifestUrl(bool daily);
struct UpdateContext {bool online=false,critical=false,daily=true;std::int64_t utc=0;std::uint64_t uptimeMs=0;};
class UpdateDriver {
public:
 virtual ~UpdateDriver()=default;
 virtual Json inspect()=0;
 virtual Json fetchManifest(bool daily)=0;
 virtual void stage(const Release&,std::function<void(std::size_t)> progress,std::function<bool()> cancel)=0;
 virtual void activate(const std::string& version)=0;
 virtual void rollback()=0;
};
class UpdateService {
public:
 UpdateService(std::unique_ptr<UpdateDriver>,std::string directory);
 ~UpdateService();
 void start(std::function<UpdateContext()> context);void stop();
 void request(const std::string& action);
 Json status()const;
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
