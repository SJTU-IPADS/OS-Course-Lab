/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "lwip/opt.h"

/*
 * debug_flags dynamically controls what LWIP_DEBUGF() prints.
 *
 * LWIP_DEBUGF(debug, message) prints if (debug & debug_flags) is true
 * because:
 *  lwipopts.h defines LWIP_DBG_TYPES_ON as debug_flags
 *  lwip/debug.h defines LWIP_DEBUGF() to test (debug & LWIP_DBG_TYPS)
 *
 * You should also check lwipopts.h to understand how 'debug's are defined.
 *
 * You can enable/disable the debug messages with these flags:
 *
 * LWIP_DBG_ON - enables all debug messages
 * LWIP_DBG_OFF - disables all debug messages
 *
 * You can also selectively enable the debug messages by types:
 *
 * LWIP_DBG_TRACE - enables if debug includes LWP_DBG_TRACE
 * LWIP_DBG_STATE - enables if debug includes LWP_DBG_STATE
 * LWIP_DBG_FRESH - enables if debug includes LWP_DBG_FRESH
 */

unsigned char lwip_debug_flags = LWIP_DBG_ON;
