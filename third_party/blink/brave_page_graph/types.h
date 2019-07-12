/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_TYPES_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace blink {
using DOMNodeId = int;
class KURL;
enum class ResourceType : uint8_t;
class ScriptSourceCode;
class WebString;
}  // namespace blink

namespace v8 {
class Isolate;
}

namespace WTF {
class String;
}  // namespace WTF

namespace brave_page_graph {

class Element;
class Edge;
class EdgeRequestResponse;
class EdgeRequestStart;
class GraphItem;
class Node;
class NodeHTML;

typedef std::string ItemDesc;
typedef std::string ItemName;
typedef std::string GraphMLXML;
typedef std::string GraphMLId;
typedef std::vector<const GraphMLXML> GraphMLXMLList;

typedef enum {
  kGraphMLAttrDefAttrName = 0,
  kGraphMLAttrDefBeforeNodeId,
  kGraphMLAttrDefCallArgs,
  kGraphMLAttrDefEdgeType,
  kGraphMLAttrDefEventListenerId,
  kGraphMLAttrDefIsStyle,
  kGraphMLAttrDefKey,
  kGraphMLAttrDefMethodName,
  kGraphMLAttrDefNodeId,
  kGraphMLAttrDefNodeTag,
  kGraphMLAttrDefNodeText,
  kGraphMLAttrDefNodeType,
  kGraphMLAttrDefParentNodeId,
  kGraphMLAttrDefRequestId,
  kGraphMLAttrDefRequestType,
  kGraphMLAttrDefResourceType,
  kGraphMLAttrDefScriptId,
  kGraphMLAttrDefScriptType,
  kGraphMLAttrDefStatus,
  kGraphMLAttrDefSuccess,
  kGraphMLAttrDefUrl,
  kGraphMLAttrDefValue,
  kGraphMLAttrDefUnknown,
} GraphMLAttrDef;

typedef enum {
  kGraphMLAttrTypeString = 0,
  kGraphMLAttrTypeBoolean,
  kGraphMLAttrTypeInt,
  kGraphMLAttrTypeLong,
  kGraphMLAttrTypeFloat,
  kGraphMLAttrTypeDouble,
  kGraphMLAttrTypeUnknown
} GraphMLAttrType;
std::string RequestTypeToString(const GraphMLAttrType type) noexcept;

typedef enum {
  kGraphMLAttrForTypeNode = 0,
  kGraphMLAttrForTypeEdge,
  kGraphMLAttrForTypeUnknown
} GraphMLAttrForType;
std::string GraphMLForToString(const GraphMLAttrForType type) noexcept;

typedef enum {
  kRequestTypeAJAX = 0,
  kRequestTypeAudio,
  kRequestTypeCSS,
  kRequestTypeDocument,
  kRequestTypeFont,
  kRequestTypeImage,
  kRequestTypeScriptClassic,
  kRequestTypeScriptModule,
  kRequestTypeSVG,
  kRequestTypeVideo,
  kRequestTypeUnknown
} RequestType;
std::string RequestTypeToString(const RequestType type) noexcept;

std::string ResourceTypeToString(const blink::ResourceType type) noexcept;

typedef enum {
  kScriptTypeClassic = 0,
  kScriptTypeModule,
  kScriptTypeExtension,
  kScriptTypeUnknown
} ScriptType;
std::string ScriptTypeToString(const ScriptType type) noexcept;

typedef enum {
  kRequestStatusStart = 0,
  kRequestStatusComplete,
  kRequestStatusError,
  kRequestStatusBlocked
} RequestStatus;
std::string RequestStatusToString(const RequestStatus status) noexcept;

typedef enum {
  kStorageLocationCookie = 0,
  kStorageLocationLocalStorage,
  kStorageLocationSessionStorage
} StorageLocation;
std::string StorageLocationToString(const StorageLocation location) noexcept;

typedef unsigned SourceCodeHash;
typedef unsigned UrlHash;
typedef int ScriptId;
typedef int EventListenerId;
typedef uint64_t PageGraphId;
typedef std::string MethodName;
typedef std::string RequestUrl;
typedef uint64_t InspectorId;

typedef std::vector<const Edge*> EdgeList;
typedef std::vector<Node*> NodeList;
typedef std::vector<std::unique_ptr<const Edge> > EdgeUniquePtrList;
typedef std::vector<std::unique_ptr<Node> > NodeUniquePtrList;
typedef std::vector<const GraphItem*> GraphItemList;
typedef std::vector<NodeHTML*> HTMLNodeList;
typedef std::vector<blink::DOMNodeId> DOMNodeIdList;
typedef std::vector<ScriptId> ScriptIdList;
typedef std::map<const std::string, const std::string> AttributeMap;

typedef std::map<SourceCodeHash, ScriptId> HashToScriptIdMap;
typedef std::map<ScriptId, SourceCodeHash> ScriptIdToHashMap;
typedef std::map<SourceCodeHash, UrlHash> SourceToUrlMap;
typedef std::map<UrlHash, SourceCodeHash> UrlToSourceMap;

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_TYPES_H_
