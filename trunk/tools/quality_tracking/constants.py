#!/usr/bin/env python
#-*- coding: utf-8 -*-
#  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
#  Use of this source code is governed by a BSD-style license
#  that can be found in the LICENSE file in the root of the source
#  tree. An additional intellectual property rights grant can be found
#  in the file PATENTS.  All contributing project authors may
#  be found in the AUTHORS file in the root of the source tree.

"""Contains tweakable constants for quality dashboard utility scripts."""

__author__ = 'phoglund@webrtc.org (Patrik Höglund)'

# This identifies our application using the information we got when we
# registered the application on Google appengine.
# TODO(phoglund): update to the right value when we have registered the app.
DASHBOARD_SERVER = 'localhost:8080'
DASHBOARD_SERVER_HTTP = 'http://' + DASHBOARD_SERVER
CONSUMER_KEY = DASHBOARD_SERVER
CONSUMER_SECRET_FILE = 'consumer.secret'
ACCESS_TOKEN_FILE = 'access.token'

# OAuth URL:s.
REQUEST_TOKEN_URL = DASHBOARD_SERVER_HTTP + '/_ah/OAuthGetRequestToken'
AUTHORIZE_TOKEN_URL = DASHBOARD_SERVER_HTTP + '/_ah/OAuthAuthorizeToken'
ACCESS_TOKEN_URL = DASHBOARD_SERVER_HTTP + '/_ah/OAuthGetAccessToken'

# The build master URL.
BUILD_MASTER_SERVER = 'webrtc-cb-linux-master.cbf.corp.google.com:8010'
BUILD_MASTER_LATEST_BUILD_URL = '/one_box_per_builder'

# The build-bot user which runs build bot jobs.
BUILD_BOT_USER = 'phoglund'

# Dashboard data input URLs.
ADD_COVERAGE_DATA_URL = '/add_coverage_data'
ADD_BUILD_STATUS_DATA_URL = '/add_build_status_data'
