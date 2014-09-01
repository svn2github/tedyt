# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

import re
import sys


def _CheckNoIOStreamInHeaders(input_api, output_api):
  """Checks to make sure no .h files include <iostream>."""
  files = []
  pattern = input_api.re.compile(r'^#include\s*<iostream>',
                                 input_api.re.MULTILINE)
  for f in input_api.AffectedSourceFiles(input_api.FilterSourceFile):
    if not f.LocalPath().endswith('.h'):
      continue
    contents = input_api.ReadFile(f)
    if pattern.search(contents):
      files.append(f)

  if len(files):
    return [ output_api.PresubmitError(
        'Do not #include <iostream> in header files, since it inserts static ' +
        'initialization into every file including the header. Instead, ' +
        '#include <ostream>. See http://crbug.com/94794',
        files) ]
  return []


def _CheckNoFRIEND_TEST(input_api, output_api):
  """Make sure that gtest's FRIEND_TEST() macro is not used, the
  FRIEND_TEST_ALL_PREFIXES() macro from testsupport/gtest_prod_util.h should be
  used instead since that allows for FLAKY_, FAILS_ and DISABLED_ prefixes."""
  problems = []

  file_filter = lambda f: f.LocalPath().endswith(('.cc', '.h'))
  for f in input_api.AffectedFiles(file_filter=file_filter):
    for line_num, line in f.ChangedContents():
      if 'FRIEND_TEST(' in line:
        problems.append('    %s:%d' % (f.LocalPath(), line_num))

  if not problems:
    return []
  return [output_api.PresubmitPromptWarning('WebRTC\'s code should not use '
      'gtest\'s FRIEND_TEST() macro. Include testsupport/gtest_prod_util.h and '
      'use FRIEND_TEST_ALL_PREFIXES() instead.\n' + '\n'.join(problems))]


def _CheckApprovedFilesLintClean(input_api, output_api,
                                 source_file_filter=None):
  """Checks that all new or whitelisted .cc and .h files pass cpplint.py.
  This check is based on _CheckChangeLintsClean in
  depot_tools/presubmit_canned_checks.py but has less filters and only checks
  added files."""
  result = []

  # Initialize cpplint.
  import cpplint
  # Access to a protected member _XX of a client class
  # pylint: disable=W0212
  cpplint._cpplint_state.ResetErrorCounts()

  # Justifications for each filter:
  #
  # - build/header_guard  : WebRTC coding style says they should be prefixed
  #                         with WEBRTC_, which is not possible to configure in
  #                         cpplint.py.
  cpplint._SetFilters('-build/header_guard')

  # Use the strictest verbosity level for cpplint.py (level 1) which is the
  # default when running cpplint.py from command line.
  # To make it possible to work with not-yet-converted code, we're only applying
  # it to new (or moved/renamed) files and files listed in LINT_FOLDERS.
  verbosity_level = 1
  files = []
  for f in input_api.AffectedSourceFiles(source_file_filter):
    # Note that moved/renamed files also count as added for svn.
    if (f.Action() == 'A'):
      files.append(f.AbsoluteLocalPath())

  for file_name in files:
    cpplint.ProcessFile(file_name, verbosity_level)

  if cpplint._cpplint_state.error_count > 0:
    if input_api.is_committing:
      # TODO(kjellander): Change back to PresubmitError below when we're
      # confident with the lint settings.
      res_type = output_api.PresubmitPromptWarning
    else:
      res_type = output_api.PresubmitPromptWarning
    result = [res_type('Changelist failed cpplint.py check.')]

  return result


def _CheckGypChanges(input_api, output_api):
  source_file_filter = lambda x: input_api.FilterSourceFile(
      x, white_list=(r'.+\.(gyp|gypi)$',))

  gyp_files = []
  for f in input_api.AffectedSourceFiles(source_file_filter):
    gyp_files.append(f.LocalPath())

  result = []
  if gyp_files:
    result.append(output_api.PresubmitNotifyResult(
        'As you\'re changing GYP files: please make sure corresponding '
        'BUILD.gn files are also updated.\nChanged GYP files:',
        items=gyp_files))
  return result

def _CheckUnwantedDependencies(input_api, output_api):
  """Runs checkdeps on #include statements added in this
  change. Breaking - rules is an error, breaking ! rules is a
  warning.
  """
  # Copied from Chromium's src/PRESUBMIT.py.

  # We need to wait until we have an input_api object and use this
  # roundabout construct to import checkdeps because this file is
  # eval-ed and thus doesn't have __file__.
  original_sys_path = sys.path
  try:
    sys.path = sys.path + [input_api.os_path.join(
        input_api.PresubmitLocalPath(), 'buildtools', 'checkdeps')]
    import checkdeps
    from cpp_checker import CppChecker
    from rules import Rule
  finally:
    # Restore sys.path to what it was before.
    sys.path = original_sys_path

  added_includes = []
  for f in input_api.AffectedFiles():
    if not CppChecker.IsCppFile(f.LocalPath()):
      continue

    changed_lines = [line for _line_num, line in f.ChangedContents()]
    added_includes.append([f.LocalPath(), changed_lines])

  deps_checker = checkdeps.DepsChecker(input_api.PresubmitLocalPath())

  error_descriptions = []
  warning_descriptions = []
  for path, rule_type, rule_description in deps_checker.CheckAddedCppIncludes(
      added_includes):
    description_with_path = '%s\n    %s' % (path, rule_description)
    if rule_type == Rule.DISALLOW:
      error_descriptions.append(description_with_path)
    else:
      warning_descriptions.append(description_with_path)

  results = []
  if error_descriptions:
    results.append(output_api.PresubmitError(
        'You added one or more #includes that violate checkdeps rules.',
        error_descriptions))
  if warning_descriptions:
    results.append(output_api.PresubmitPromptOrNotify(
        'You added one or more #includes of files that are temporarily\n'
        'allowed but being removed. Can you avoid introducing the\n'
        '#include? See relevant DEPS file(s) for details and contacts.',
        warning_descriptions))
  return results


def _CommonChecks(input_api, output_api):
  """Checks common to both upload and commit."""
  # TODO(kjellander): Use presubmit_canned_checks.PanProjectChecks too.
  results = []
  results.extend(input_api.canned_checks.RunPylint(input_api, output_api,
      black_list=(r'^.*gviz_api\.py$',
                  r'^.*gaeunit\.py$',
                  # Embedded shell-script fakes out pylint.
                  r'^build/.*\.py$',
                  r'^buildtools/.*\.py$',
                  r'^chromium/.*\.py$',
                  r'^out.*/.*\.py$',
                  r'^talk/site_scons/site_tools/talk_linux.py$',
                  r'^testing/.*\.py$',
                  r'^third_party/.*\.py$',
                  r'^tools/clang/.*\.py$',
                  r'^tools/gn/.*\.py$',
                  r'^tools/gyp/.*\.py$',
                  r'^tools/perf_expectations/.*\.py$',
                  r'^tools/protoc_wrapper/.*\.py$',
                  r'^tools/python/.*\.py$',
                  r'^tools/python_charts/data/.*\.py$',
                  r'^tools/refactoring/.*\.py$',
                  r'^tools/swarming_client/.*\.py$',
                  # TODO(phoglund): should arguably be checked.
                  r'^tools/valgrind-webrtc/.*\.py$',
                  r'^tools/valgrind/.*\.py$',
                  # TODO(phoglund): should arguably be checked.
                  r'^webrtc/build/.*\.py$',
                  r'^xcodebuild.*/.*\.py$',),

      disabled_warnings=['F0401',  # Failed to import x
                         'E0611',  # No package y in x
                         'W0232',  # Class has no __init__ method
                        ]))
  results.extend(input_api.canned_checks.CheckLongLines(
      input_api, output_api, maxlen=80))
  results.extend(input_api.canned_checks.CheckChangeHasNoTabs(
      input_api, output_api))
  results.extend(input_api.canned_checks.CheckChangeHasNoStrayWhitespace(
      input_api, output_api))
  results.extend(input_api.canned_checks.CheckChangeTodoHasOwner(
      input_api, output_api))
  results.extend(_CheckApprovedFilesLintClean(input_api, output_api))
  results.extend(_CheckNoIOStreamInHeaders(input_api, output_api))
  results.extend(_CheckNoFRIEND_TEST(input_api, output_api))
  results.extend(_CheckGypChanges(input_api, output_api))
  results.extend(_CheckUnwantedDependencies(input_api, output_api))
  return results


def CheckChangeOnUpload(input_api, output_api):
  results = []
  results.extend(_CommonChecks(input_api, output_api))
  return results


def CheckChangeOnCommit(input_api, output_api):
  results = []
  results.extend(_CommonChecks(input_api, output_api))
  results.extend(input_api.canned_checks.CheckOwners(input_api, output_api))
  results.extend(input_api.canned_checks.CheckChangeWasUploaded(
      input_api, output_api))
  results.extend(input_api.canned_checks.CheckChangeHasDescription(
      input_api, output_api))
  results.extend(input_api.canned_checks.CheckChangeHasBugField(
      input_api, output_api))
  results.extend(input_api.canned_checks.CheckChangeHasTestField(
      input_api, output_api))
  results.extend(input_api.canned_checks.CheckTreeIsOpen(
      input_api, output_api,
      json_url='http://webrtc-status.appspot.com/current?format=json'))
  return results


def GetDefaultTryConfigs(bots=None):
  """Returns a list of ('bot', set(['tests']), optionally filtered by [bots].

  For WebRTC purposes, we always return an empty list of tests, since we want
  to run all tests by default on all our trybots.
  """
  return { 'tryserver.webrtc': dict((bot, []) for bot in bots)}


# pylint: disable=W0613
def GetPreferredTryMasters(project, change):
  files = change.LocalPaths()

  android_gn_bots = [
      'android_gn',
      'android_gn_rel',
  ]
  android_bots = [
      'android',
      'android_arm64',
      'android_apk',
      'android_apk_rel',
      'android_rel',
      'android_clang',
  ] + android_gn_bots
  ios_bots = [
      'ios',
      'ios_rel',
  ]
  linux_gn_bots = [
      'linux_gn',
      'linux_gn_rel',
  ]
  linux_bots = [
      'linux',
      'linux_asan',
      'linux_baremetal',
      'linux_memcheck',
      'linux_rel',
      'linux_tsan2',
  ] + linux_gn_bots
  mac_bots = [
      'mac',
      'mac_asan',
      'mac_baremetal',
      'mac_rel',
      'mac_x64_rel',
  ]
  win_bots = [
      'win',
      'win_asan',
      'win_baremetal',
      'win_drmemory_light',
      'win_rel',
      'win_x64_rel',
  ]
  if not files or all(re.search(r'[\\/]OWNERS$', f) for f in files):
    return {}
  if all(re.search(r'[\\/]BUILD.gn$', f) for f in files):
    return GetDefaultTryConfigs(android_gn_bots + linux_gn_bots)
  if all(re.search('\.(m|mm)$|(^|[/_])mac[/_.]', f) for f in files):
    return GetDefaultTryConfigs(mac_bots)
  if all(re.search('(^|[/_])win[/_.]', f) for f in files):
    return GetDefaultTryConfigs(win_bots)
  if all(re.search('(^|[/_])android[/_.]', f) for f in files):
    return GetDefaultTryConfigs(android_bots)
  if all(re.search('[/_]ios[/_.]', f) for f in files):
    return GetDefaultTryConfigs(ios_bots)

  return GetDefaultTryConfigs(android_bots + ios_bots + linux_bots + mac_bots +
                              win_bots)
