/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveSingleCategorySettingsAdapter extends BraveClassVisitor {
    static String sSingleCategorySettingsClassName = "org/chromium/components/browser_ui/site_settings/SingleCategorySettings";
    static String sBraveSingleCategorySettingsClassName = "org/chromium/chrome/browser/site_settings/BraveSingleCategorySettings";

    public BraveSingleCategorySettingsAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sSingleCategorySettingsClassName, sBraveSingleCategorySettingsClassName);
    }
}
