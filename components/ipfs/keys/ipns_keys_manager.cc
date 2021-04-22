/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/keys/ipns_keys_manager.h"

#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_json_parser.h"
#include "brave/components/ipfs/ipfs_network_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace ipfs {

IpnsKeysManager::IpnsKeysManager(content::BrowserContext* context,
                                 const GURL& server_endpoint)
    : context_(context),
      server_endpoint_(server_endpoint) {
  DCHECK(context_);
  url_loader_factory_ =
      content::BrowserContext::GetDefaultStoragePartition(context)
          ->GetURLLoaderFactoryForBrowserProcess();
}

IpnsKeysManager::~IpnsKeysManager() {}

bool IpnsKeysManager::KeyExists(const std::string& name) const {
  return keys_.count(name);
}

void IpnsKeysManager::RemoveKey(const std::string& name,
                                RemoveKeyCallback callback) {
  if (!KeyExists(name)) {
    VLOG(1) << "Key " << name << " doesn't exist";
    if (callback)
      std::move(callback).Run(name, false);
    return;
  }

  auto generate_endpoint = server_endpoint_.Resolve(kAPIKeyRemoveEndpoint);
  GURL gurl =
      net::AppendQueryParameter(generate_endpoint, kArgQueryParam, name);

  auto url_loader = CreateURLLoader(gurl, "POST");

  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));
  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpnsKeysManager::OnKeyRemoved, base::Unretained(this),
                     iter, name, std::move(callback)));
}

void IpnsKeysManager::OnKeyRemoved(SimpleURLLoaderList::iterator iter,
                                   const std::string& key_to_remove,
                                   RemoveKeyCallback callback,
                                   std::unique_ptr<std::string> response_body) {
  auto* url_loader = iter->get();
  int error_code = url_loader->NetError();
  int response_code = -1;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();
  url_loaders_.erase(iter);

  std::string name;
  std::string value;
  bool success = (error_code == net::OK && response_code == net::HTTP_OK);
  std::unordered_map<std::string, std::string> removed_keys;
  success = success &&
            IPFSJSONParser::GetParseKeysFromJSON(*response_body, removed_keys);
  if (success) {
    if (removed_keys.count(key_to_remove))
      keys_.erase(key_to_remove);
  } else {
    VLOG(1) << "Fail to generate new key, error_code = " << error_code
            << " response_code = " << response_code;
  }
  if (callback)
    std::move(callback).Run(key_to_remove, success);
}

void IpnsKeysManager::GenerateNewKey(const std::string& name,
                                     GenerateKeyCallback callback) {
  if (KeyExists(name)) {
    VLOG(1) << "Key " << name << " already exists";
    if (callback)
      std::move(callback).Run(true, name, keys_[name]);
    return;
  }

  auto generate_endpoint = server_endpoint_.Resolve(kAPIKeyGenerateEndpoint);
  GURL gurl =
      net::AppendQueryParameter(generate_endpoint, kArgQueryParam, name);

  auto url_loader = CreateURLLoader(gurl, "POST");

  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));
  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpnsKeysManager::OnKeyCreated, base::Unretained(this),
                     iter, std::move(callback)));
}

void IpnsKeysManager::OnKeyCreated(SimpleURLLoaderList::iterator iter,
                                   GenerateKeyCallback callback,
                                   std::unique_ptr<std::string> response_body) {
  auto* url_loader = iter->get();
  int error_code = url_loader->NetError();
  int response_code = -1;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();
  url_loaders_.erase(iter);

  std::string name;
  std::string value;
  bool success = (error_code == net::OK && response_code == net::HTTP_OK);
  success = success && IPFSJSONParser::GetParseSingleKeyFromJSON(*response_body,
                                                                 &name, &value);
  if (success) {
    keys_[name] = value;
  } else {
    VLOG(1) << "Fail to generate new key, error_code = " << error_code
            << " response_code = " << response_code;
  }
  if (callback)
    std::move(callback).Run(success, name, value);
}

void IpnsKeysManager::SynchronizeKeys(SynchronizationCallback callback) {
  if (!pending_synchronization_callbacks_.empty()) {
    if (callback)
      pending_synchronization_callbacks_.push(std::move(callback));
    return;
  }
  if (callback)
    pending_synchronization_callbacks_.push(std::move(callback));

  auto url_loader = CreateURLLoader(
      server_endpoint_.Resolve(kAPIKeyListEndpoint), "POST");
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpnsKeysManager::OnKeysLoaded, base::Unretained(this),
                     iter));
}

void IpnsKeysManager::OnKeysLoaded(
    SimpleURLLoaderList::iterator iter,
    std::unique_ptr<std::string> response_body) {
  auto* url_loader = iter->get();
  int error_code = url_loader->NetError();
  int response_code = -1;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();
  url_loaders_.erase(iter);

  if (error_code != net::OK || response_code != net::HTTP_OK) {
    VLOG(1) << "Fail to get keys from node, error_code = " << error_code
            << " response_code = " << response_code;
    return;
  }

  keys_.clear();
  bool success = IPFSJSONParser::GetParseKeysFromJSON(*response_body, keys_);
  if (!success) {
     VLOG(1) << "Fail to parse keys from node response" << *response_body;
  };
  NotifyKeysSynchronized(success);
}

void IpnsKeysManager::NotifyKeysSynchronized(bool result) {
  while (!pending_synchronization_callbacks_.empty()) {
    if (pending_synchronization_callbacks_.front())
      std::move(pending_synchronization_callbacks_.front()).Run(result);
    pending_synchronization_callbacks_.pop();
  }
}

void IpnsKeysManager::SetServerEndpointForTest(const GURL& gurl) {
  server_endpoint_ = gurl;
}

}  // namespace ipfs