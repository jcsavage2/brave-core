/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/containers/contains.h"
#include "base/guid.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/browser/ipfs/ipfs_host_resolver.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/ipfs/imported_data.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/common/channel_info.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents_delegate.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"

namespace {

// We have to check both domain and _dnslink.domain
// https://dnslink.io/#can-i-use-dnslink-in-non-dns-systems
const char kDnsDomainPrefix[] = "_dnslink.";

// IPFS HTTP gateways can return an x-ipfs-path header with each response.
// The value of the header is the IPFS path of the returned payload.
const char kIfpsPathHeader[] = "x-ipfs-path";

// Message center notifier id for user notifications
const char kNotifierId[] = "service.ipfs";

// Imported shareable link should have filename parameter.
const char kImportFileNameParam[] = "filename";

// /ipfs/{cid}/path → ipfs://{cid}/path
// query and fragment are taken from source page url
GURL ParseURLFromHeader(const std::string& value) {
  if (value.empty())
    return GURL();
  std::vector<std::string> parts = base::SplitString(
      value, "/", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  // Default length of header is /[scheme]/cid so we have 3 parts after split.
  const int minimalPartsRequired = 3;
  if (parts.size() < minimalPartsRequired || !parts.front().empty())
    return GURL();
  std::string scheme = parts[1];
  if (scheme != ipfs::kIPFSScheme && scheme != ipfs::kIPNSScheme)
    return GURL();
  std::string cid = parts[2];
  if (scheme.empty() || cid.empty())
    return GURL();
  std::string path;
  // Add all other parts to url path.
  if (parts.size() > minimalPartsRequired) {
    for (size_t i = minimalPartsRequired; i < parts.size(); i++) {
      if (parts[i].empty())
        continue;
      if (!path.empty())
        path += "/";
      path += parts[i];
    }
  }
  std::string spec = scheme + "://" + cid;
  if (!path.empty())
    spec += "/" + path;
  return GURL(spec);
}

// Sets current executable as default protocol handler in a system.
void SetupIPFSProtocolHandler(const std::string& protocol) {
  auto isDefaultCallback = [](const std::string& protocol,
                              shell_integration::DefaultWebClientState state) {
    if (state == shell_integration::IS_DEFAULT) {
      VLOG(1) << protocol << " already has a handler";
      return;
    }
    VLOG(1) << "Set as default handler for " << protocol;
    // The worker pointer is reference counted. While it is running, the
    // sequence it runs on will hold references it will be automatically
    // freed once all its tasks have finished.
    base::MakeRefCounted<shell_integration::DefaultProtocolClientWorker>(
        protocol)
        ->StartSetAsDefault(base::NullCallback());
  };

  base::MakeRefCounted<shell_integration::DefaultProtocolClientWorker>(protocol)
      ->StartCheckIsDefault(base::BindOnce(isDefaultCallback, protocol));
}

std::u16string GetImportNotificationTitle(ipfs::ImportState state) {
  switch (state) {
    case ipfs::IPFS_IMPORT_SUCCESS:
      return l10n_util::GetStringUTF16(IDS_IPFS_IMPORT_NOTIFICATION_TITLE);
    case ipfs::IPFS_IMPORT_ERROR_REQUEST_EMPTY:
    case ipfs::IPFS_IMPORT_ERROR_ADD_FAILED:
      return l10n_util::GetStringUTF16(
          IDS_IPFS_IMPORT_ERROR_NOTIFICATION_TITLE);
    case ipfs::IPFS_IMPORT_ERROR_MKDIR_FAILED:
    case ipfs::IPFS_IMPORT_ERROR_MOVE_FAILED:
      return l10n_util::GetStringUTF16(
          IDS_IPFS_IMPORT_PARTLY_COMPLETED_NOTIFICATION_TITLE);
    default:
      NOTREACHED();
      break;
  }
  return std::u16string();
}

std::u16string GetImportNotificationBody(ipfs::ImportState state,
                                         const GURL& shareable_link) {
  switch (state) {
    case ipfs::IPFS_IMPORT_SUCCESS:
      return l10n_util::GetStringFUTF16(
          IDS_IPFS_IMPORT_NOTIFICATION_BODY,
          base::UTF8ToUTF16(shareable_link.spec()));
    case ipfs::IPFS_IMPORT_ERROR_REQUEST_EMPTY:
      return l10n_util::GetStringUTF16(IDS_IPFS_IMPORT_ERROR_NO_REQUEST_BODY);
    case ipfs::IPFS_IMPORT_ERROR_ADD_FAILED:
      return l10n_util::GetStringUTF16(IDS_IPFS_IMPORT_ERROR_ADD_FAILED_BODY);
    case ipfs::IPFS_IMPORT_ERROR_MKDIR_FAILED:
    case ipfs::IPFS_IMPORT_ERROR_MOVE_FAILED:
      return l10n_util::GetStringUTF16(
          IDS_IPFS_IMPORT_PARTLY_COMPLETED_NOTIFICATION_BODY);
    default:
      NOTREACHED();
      break;
  }
  return std::u16string();
}

std::unique_ptr<message_center::Notification> CreateMessageCenterNotification(
    const std::u16string& title,
    const std::u16string& body,
    const std::string& uuid,
    const GURL& link) {
  message_center::RichNotificationData notification_data;

  // hack to prevent origin from showing in the notification
  // since we're using that to get the notification_id to OpenSettings
  notification_data.context_message = u" ";
  auto notification = std::make_unique<message_center::Notification>(
      message_center::NOTIFICATION_TYPE_SIMPLE, uuid, title, body, gfx::Image(),
      std::u16string(), link,
      message_center::NotifierId(message_center::NotifierType::SYSTEM_COMPONENT,
                                 kNotifierId),
      notification_data, nullptr);

  return notification;
}

}  // namespace

namespace ipfs {

IPFSTabHelper::~IPFSTabHelper() = default;

IPFSTabHelper::IPFSTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  pref_service_ = user_prefs::UserPrefs::Get(web_contents->GetBrowserContext());
  auto* storage_partition = content::BrowserContext::GetDefaultStoragePartition(
      web_contents->GetBrowserContext());

  resolver_.reset(new IPFSHostResolver(storage_partition->GetNetworkContext(),
                                       kDnsDomainPrefix));
  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.Add(
      kIPFSResolveMethod,
      base::BindRepeating(&IPFSTabHelper::UpdateDnsLinkButtonState,
                          base::Unretained(this)));
  ipfs_service_ = ipfs::IpfsServiceFactory::GetForContext(
      web_contents->GetBrowserContext());
}

// static
bool IPFSTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  if (!ipfs::IpfsServiceFactory::GetForContext(
          web_contents->GetBrowserContext())) {
    return false;
  }

  CreateForWebContents(web_contents);
  return true;
}

void IPFSTabHelper::IPFSLinkResolved(const GURL& ipfs) {
  ipfs_resolved_url_ = ipfs;
  if (pref_service_->GetBoolean(kIPFSAutoRedirectDNSLink)) {
    content::OpenURLParams params(GetIPFSResolvedURL(), content::Referrer(),
                                  WindowOpenDisposition::CURRENT_TAB,
                                  ui::PAGE_TRANSITION_LINK, false);
    web_contents()->OpenURL(params);
    return;
  }
  UpdateLocationBar();
}

void IPFSTabHelper::HostResolvedCallback(const std::string& host,
                                         const std::string& dnslink) {
  GURL current = web_contents()->GetURL();
  if (current.host() != host || !current.SchemeIsHTTPOrHTTPS())
    return;
  if (dnslink.empty())
    return;
  GURL::Replacements replacements;
  replacements.SetSchemeStr(kIPNSScheme);
  GURL resolved_url(current.ReplaceComponents(replacements));
  if (resolved_url.is_valid())
    IPFSLinkResolved(resolved_url);
}

void IPFSTabHelper::UpdateLocationBar() {
  if (web_contents()->GetDelegate())
    web_contents()->GetDelegate()->NavigationStateChanged(
        web_contents(), content::INVALIDATE_TYPE_URL);
}

GURL IPFSTabHelper::GetIPFSResolvedURL() const {
  if (!ipfs_resolved_url_.is_valid())
    return GURL();
  GURL current = web_contents()->GetURL();
  GURL::Replacements replacements;
  replacements.SetQueryStr(current.query_piece());
  replacements.SetRefStr(current.ref_piece());
  return ipfs_resolved_url_.ReplaceComponents(replacements);
}

void IPFSTabHelper::ResolveIPFSLink() {
  GURL current = web_contents()->GetURL();
  if (!current.SchemeIsHTTPOrHTTPS())
    return;

  const auto& host_port_pair = net::HostPortPair::FromURL(current);

  auto resolved_callback = base::BindOnce(&IPFSTabHelper::HostResolvedCallback,
                                          weak_ptr_factory_.GetWeakPtr());
  const auto& key =
      web_contents()->GetMainFrame()
          ? web_contents()->GetMainFrame()->GetNetworkIsolationKey()
          : net::NetworkIsolationKey();
  resolver_->Resolve(host_port_pair, key, net::DnsQueryType::TXT,
                     std::move(resolved_callback));
}

bool IPFSTabHelper::IsDNSLinkCheckEnabled() const {
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      pref_service_->GetInteger(kIPFSResolveMethod));

  return (resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_LOCAL ||
          resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
}

void IPFSTabHelper::UpdateDnsLinkButtonState() {
  if (!IsDNSLinkCheckEnabled()) {
    if (ipfs_resolved_url_.is_valid()) {
      ipfs_resolved_url_ = GURL();
      UpdateLocationBar();
    }
    return;
  }

  GURL current = web_contents()->GetURL();
  if (ipfs_resolved_url_.is_valid() && resolver_->host() != current.host()) {
    ipfs_resolved_url_ = GURL();
    UpdateLocationBar();
  }
}

void IPFSTabHelper::MaybeShowDNSLinkButton(content::NavigationHandle* handle) {
  UpdateDnsLinkButtonState();
  if (!IsDNSLinkCheckEnabled() || !handle->GetResponseHeaders())
    return;
  GURL current = web_contents()->GetURL();
  if (ipfs_resolved_url_.is_valid() || !current.SchemeIsHTTPOrHTTPS() ||
      IsDefaultGatewayURL(current, web_contents()->GetBrowserContext()))
    return;
  int response_code = handle->GetResponseHeaders()->response_code();
  if (response_code >= net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR &&
      response_code <= net::HttpStatusCode::HTTP_VERSION_NOT_SUPPORTED) {
    ResolveIPFSLink();
  } else if (handle->GetResponseHeaders()->HasHeader(kIfpsPathHeader)) {
    std::string ipfs_path_value;
    if (!handle->GetResponseHeaders()->GetNormalizedHeader(kIfpsPathHeader,
                                                           &ipfs_path_value))
      return;
    GURL resolved_url = ParseURLFromHeader(ipfs_path_value);
    if (resolved_url.is_valid())
      IPFSLinkResolved(resolved_url);
  }
}

void IPFSTabHelper::MaybeSetupIpfsProtocolHandlers(const GURL& url) {
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      pref_service_->GetInteger(kIPFSResolveMethod));
  auto* browser_context = web_contents()->GetBrowserContext();
  if (resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_ASK &&
      IsDefaultGatewayURL(url, browser_context)) {
    auto infobar_count = pref_service_->GetInteger(kIPFSInfobarCount);
    if (!infobar_count) {
      pref_service_->SetInteger(kIPFSInfobarCount, infobar_count + 1);
      SetupIPFSProtocolHandler(ipfs::kIPFSScheme);
      SetupIPFSProtocolHandler(ipfs::kIPNSScheme);
    }
  }
}

void IPFSTabHelper::DidFinishNavigation(content::NavigationHandle* handle) {
  DCHECK(handle);
  if (!handle->IsInMainFrame() || !handle->HasCommitted() ||
      handle->IsSameDocument()) {
    return;
  }
  if (handle->GetResponseHeaders() &&
      handle->GetResponseHeaders()->HasHeader(kIfpsPathHeader)) {
    MaybeSetupIpfsProtocolHandlers(handle->GetURL());
  }
  MaybeShowDNSLinkButton(handle);
}

void IPFSTabHelper::ImportLinkToIpfs(const GURL& url) {
  DCHECK(ipfs_service_);
  ipfs_service_->ImportLinkToIpfs(
      url, base::BindOnce(&IPFSTabHelper::OnImportCompleted,
                          weak_ptr_factory_.GetWeakPtr()));
}

void IPFSTabHelper::ImportTextToIpfs(const std::string& text) {
  DCHECK(ipfs_service_);
  ipfs_service_->ImportTextToIpfs(
      text, web_contents()->GetURL().host(),
      base::BindOnce(&IPFSTabHelper::OnImportCompleted,
                     weak_ptr_factory_.GetWeakPtr()));
}

void IPFSTabHelper::ImportFileToIpfs(const base::FilePath& path) {
  DCHECK(ipfs_service_);
  ipfs_service_->ImportFileToIpfs(
      path, base::BindOnce(&IPFSTabHelper::OnImportCompleted,
                           weak_ptr_factory_.GetWeakPtr()));
}

GURL IPFSTabHelper::CreateAndCopyShareableLink(const ipfs::ImportedData& data) {
  if (data.hash.empty())
    return GURL();
  std::string ipfs = ipfs::kIPFSScheme + std::string("://") + data.hash;
  auto shareable_link =
      ipfs::ToPublicGatewayURL(GURL(ipfs), web_contents()->GetBrowserContext());
  if (!shareable_link.is_valid())
    return GURL();
  if (!data.filename.empty())
    shareable_link = net::AppendQueryParameter(
        shareable_link, kImportFileNameParam, data.filename);
  ui::ScopedClipboardWriter(ui::ClipboardBuffer::kCopyPaste)
      .WriteText(base::UTF8ToUTF16(shareable_link.spec()));
  ipfs_service_->PreWarmShareableLink(shareable_link);
  return shareable_link;
}

void IPFSTabHelper::OnImportCompleted(const ipfs::ImportedData& data) {
  auto link = CreateAndCopyShareableLink(data);
  if (!link.is_valid()) {
    // Open node diagnostic page if import failed
    link = GURL(kIPFSWebUIURL);
  }
  PushNotification(GetImportNotificationTitle(data.state),
                   GetImportNotificationBody(data.state, link), link);
  if (data.state == ipfs::IPFS_IMPORT_SUCCESS) {
    GURL url = ResolveWebUIFilesLocation(data.directory, chrome::GetChannel());
    content::OpenURLParams params(url, content::Referrer(),
                                  WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                  ui::PAGE_TRANSITION_LINK, false);
    web_contents()->OpenURL(params);
  }
}

void IPFSTabHelper::PushNotification(const std::u16string& title,
                                     const std::u16string& body,
                                     const GURL& link) {
  auto notification =
      CreateMessageCenterNotification(title, body, base::GenerateGUID(), link);
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  auto* display_service = NotificationDisplayService::GetForProfile(profile);
  display_service->Display(NotificationHandler::Type::SEND_TAB_TO_SELF,
                           *notification, /*metadata=*/nullptr);
}

void IPFSTabHelper::FileSelected(const base::FilePath& path,
                                 int index,
                                 void* params) {
  ImportFileToIpfs(path);
  select_file_dialog_.reset();
}

void IPFSTabHelper::FileSelectionCanceled(void* params) {
  select_file_dialog_.reset();
}

void IPFSTabHelper::SelectFileForImport() {
  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, std::make_unique<ChromeSelectFilePolicy>(web_contents()));

  if (!select_file_dialog_)
    return;
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  const base::FilePath directory = profile->last_selected_directory();
  gfx::NativeWindow parent_window = web_contents()->GetTopLevelNativeWindow();
  ui::SelectFileDialog::FileTypeInfo file_types;
  file_types.allowed_paths =
      ui::SelectFileDialog::FileTypeInfo::ANY_PATH_OR_URL;
  select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_OPEN_FILE, std::u16string(), directory,
      &file_types, 0, base::FilePath::StringType(), parent_window, NULL);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(IPFSTabHelper)

}  // namespace ipfs
