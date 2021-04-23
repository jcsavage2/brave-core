/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
 
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, HistoryNodeFaviconState) {
  HistoryNodeFaviconStateInvalidFavIcon,
  HistoryNodeFaviconStateLoadingFavIcon,
  HistoryNodeFaviconStateLoadedFavIcon,
};

// @protocol HistoryModelObserver;
// @protocol HistoryModelListener;

@class IOSHistoryNode;

NS_SWIFT_NAME(HistoryNode)
OBJC_EXPORT
@interface IOSHistoryNode : NSObject

@property(nonatomic, strong, readonly) NSString *title;
@property(nonatomic, strong, readonly) NSString* guid;
@property(nonatomic, strong, readonly) NSURL* url;
@property(nonatomic, strong, readonly) NSDate* dateAdded;

@property(nonatomic, nullable, copy, readonly) UIImage* icon;
@property(nonatomic, nullable, copy, readonly) NSURL* iconUrl;

- (void)setTitle:(NSString*)title;

- (instancetype)initWithTitle:(NSString*)title
                         guid:(NSString* _Nullable)guid
                          url:(NSURL*)url
                    dateAdded:(NSDate*)dateAdded;
@end

NS_SWIFT_NAME(BraveHistoryAPI)
OBJC_EXPORT
@interface BraveHistoryAPI : NSObject
@property(class, readonly, getter = sharedHistoryAPI) BraveHistoryAPI* shared;

@property(nonatomic, readonly) bool isLoaded;

// - (id<BookmarkModelListener>)addObserver:(id<HistoryModelObserver>)observer;
// - (void)removeObserver:(id<HistoryModelListener>)observer;

- (void)addHistory:(IOSHistoryNode*)history;

- (void)removeHistory:(IOSHistoryNode*)history;
- (void)removeAll;

- (void)searchWithQuery:(NSString*)query maxCount:(NSUInteger)maxCount
                                       completion:(void(^)(NSArray<IOSHistoryNode*>* historyResults))completion;

@end

NS_ASSUME_NONNULL_END