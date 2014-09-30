#!/usr/bin/env python
#-*- coding: utf-8 -*-
#  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
#  Use of this source code is governed by a BSD-style license
#  that can be found in the LICENSE file in the root of the source
#  tree. An additional intellectual property rights grant can be found
#  in the file PATENTS.  All contributing project authors may
#  be found in the AUTHORS file in the root of the source tree.

"""Connects all URLs with their respective handlers."""

import webapp2

import add_coverage_data
import dashboard

app = webapp2.WSGIApplication([('/', dashboard.ShowDashboard),
                               ('/add_coverage_data',
                                add_coverage_data.AddCoverageData)],
                              debug=True)
