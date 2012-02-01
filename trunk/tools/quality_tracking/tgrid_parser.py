#!/usr/bin/env python
#-*- coding: utf-8 -*-
#  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
#  Use of this source code is governed by a BSD-style license
#  that can be found in the LICENSE file in the root of the source
#  tree. An additional intellectual property rights grant can be found
#  in the file PATENTS.  All contributing project authors may
#  be found in the AUTHORS file in the root of the source tree.

"""Contains functions for parsing the build master's transposed grid page."""

__author__ = 'phoglund@webrtc.org (Patrik Höglund)'

import re


class FailedToParseBuildStatus(Exception):
  pass


def _parse_builds(revision, html):
  """Parses the bot list, which is a sequence of <td></td> lines.

     Example input:
     <td class="build success"><a href="builders/Android/builds/119">OK</a></td>
     The first regular expression group captures Android, second 119, third OK.
  """
  result = {}

  for match in re.finditer('<td.*?>.*?<a href="builders/(.+?)/builds/(\d+)">'
                           '(OK|failed|building)</a>.*?</td>', html, re.DOTALL):
    revision_and_bot_name = revision + "--" + match.group(1)
    build_number_and_status = match.group(2) + "--" + match.group(3)

    result[revision_and_bot_name] = build_number_and_status

  return result


def parse_tgrid_page(html):
  """Parses the build master's tgrid page.

     Example input:
     <tr>
       <td valign="bottom" class="sourcestamp">1568</td>
       LIST OF BOTS
     </tr>
     The first regular expression group captures 1568, second group captures
     everything in LIST OF BOTS. The list of bots is then passed into a
     separate function for parsing.

     Args:
         html: The raw HTML from the tgrid page.

     Returns: A dictionary with <svn revision>--<bot name> mapped to
         <bot build number>--<status>, where status is either OK, failed or
         building.
  """
  result = {}

  for match in re.finditer('<td.*?class="sourcestamp">(\d+)</td>(.*?)</tr>',
                           html, re.DOTALL):
    revision = match.group(1)
    builds_for_revision_html = match.group(2)
    result.update(_parse_builds(revision, builds_for_revision_html))

  if not result:
   raise FailedToParseBuildStatus('Could not find any build statuses in %s.' %
                                  html)

  return result
