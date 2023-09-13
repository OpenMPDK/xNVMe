# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

if not is_windows
  xnvmescript_driver = custom_target(
    'xnvme-driver-script',
    input: 'xnvme-driver.sh',
    output: 'xnvme-driver',
    command: [find_program('cp'), '@INPUT@', '@OUTPUT@'],
    install: true,
    install_dir: get_option('bindir')
  )
endif
