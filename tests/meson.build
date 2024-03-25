# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

tests_prefix = 'xnvme_tests_'
tests_source = [
  'async_intf.c',
  'buf.c',
  'cli.c',
  'compare.c',
  'copy.c',
  'delay_identification.c',
  'enum.c',
  'ioworker.c',
  'kvs.c',
  'lblk.c',
  'scc.c',
  'xnvme_cli.c',
  'xnvme_file.c',
  'znd_append.c',
  'znd_explicit_open.c',
  'znd_state.c',
  'znd_zrwa.c',
]

if not is_windows
  tests_source += ['map.c']
endif

foreach test_source : tests_source
  executable(
    tests_prefix + fs.stem(test_source),
    test_source,
    include_directories: [conf_inc, xnvme_inc],
    link_with: xnvmelib_static,
    link_args: link_args_hardening,
    install: true,
  )
endforeach
