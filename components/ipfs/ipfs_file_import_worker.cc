/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_file_import_worker.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/task_runner_util.h"
#include "brave/components/ipfs/import_utils.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/mime_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "url/gurl.h"

namespace {

const char kFileMimeType[] = "application/octet-stream";

}  // namespace

namespace ipfs {

IpfsFileImportWorker::IpfsFileImportWorker(content::BrowserContext* context,
                                           const GURL& endpoint,
                                           ImportCompletedCallback callback,
                                           const base::FilePath& path)
    : IpfsImportWorkerBase(context, endpoint, std::move(callback)),
      weak_factory_(this) {
  std::string filename = path.BaseName().MaybeAsASCII();
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()}, base::BindOnce(&CalculateFileSize, path),
      base::BindOnce(&IpfsFileImportWorker::CreateRequestWithFile,
                     weak_factory_.GetWeakPtr(), path, kFileMimeType,
                     filename));
}

IpfsFileImportWorker::~IpfsFileImportWorker() = default;

}  // namespace ipfs
