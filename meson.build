# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

project(
  'xnvme',
  'c',
  version: '0.7.1',
  license: 'BSD-3-Clause',
  default_options: [
    'c_std=gnu11',
    'warning_level=2',
    'buildtype=release',
    'default_library=both',
    'b_staticpic=true'
# This seems broken on Windows
#    'b_lto=true',
  ],
  meson_version: '>=0.58'
)
pconf = import('pkgconfig')
fs = import('fs')

#
# Pre-flight check, operating system and compiler arguments
#
system_support = ['freebsd', 'linux', 'windows', 'darwin']
system = host_machine.system()
message('host_machine.system:', system)
if not system_support.contains(system)
    error('Unsupported system type "@0@"'.format(exec_env))
endif
foreach sys : system_support
    set_variable('is_' + sys, system == sys)
endforeach


cc = meson.get_compiler('c')
cc_flags = [
  '-Wno-missing-braces',
  '-Wno-cast-function-type',
  '-Wno-strict-aliasing'
]
add_project_arguments(
  cc.get_supported_arguments(cc_flags),
  language: 'c'
)

prog_python = import('python').find_installation('python3')
message('python.language_version:', prog_python.language_version())

xnvmelib_libdir = get_option('libdir')

#
# The project itself
#
project_version_major = meson.project_version().split('.')[0]
project_version_minor = meson.project_version().split('.')[1]
project_version_patch = meson.project_version().split('.')[2]

#
# Configuration
#
conf_data = configuration_data()
conf_data.set('XNVME_VERSION_MAJOR', project_version_major)
conf_data.set('XNVME_VERSION_MINOR', project_version_minor)
conf_data.set('XNVME_VERSION_PATCH', project_version_patch)

conf_data.set('XNVME_DEBUG_ENABLED', get_option('buildtype') == 'debug')

conf_data.set('XNVME_BE_CBI_ADMIN_SHIM_ENABLED', get_option('cbi_admin_shim'))
conf_data.set('XNVME_BE_CBI_ASYNC_EMU_ENABLED', get_option('cbi_async_emu'))
conf_data.set('XNVME_BE_CBI_ASYNC_NIL_ENABLED', get_option('cbi_async_nil'))
conf_data.set('XNVME_BE_CBI_ASYNC_POSIX_ENABLED', get_option('cbi_async_posix') and (is_linux or is_freebsd or is_darwin))

thread_dep = dependency(
  'threads',
  required: get_option('cbi_async_thrpool'),
)
conf_data.set('XNVME_BE_CBI_ASYNC_THRPOOL_ENABLED', thread_dep.found())

conf_data.set('XNVME_BE_CBI_MEM_POSIX_ENABLED', get_option('cbi_mem_posix') and (is_linux or is_freebsd or is_darwin))
conf_data.set('XNVME_BE_CBI_SYNC_PSYNC_ENABLED', get_option('cbi_sync_psync') and (is_linux or is_freebsd or is_darwin))

conf_data.set('XNVME_BE_RAMDISK_ENABLED', get_option('be_ramdisk'))

setupapi_dep = cc.find_library(
  'setupapi',
  has_headers: ['setupapi.h'],
  required: is_windows,
)
conf_data.set('XNVME_BE_WINDOWS_ENABLED', setupapi_dep.found())
conf_data.set('XNVME_BE_WINDOWS_FS_ENABLED', conf_data.get('XNVME_BE_WINDOWS_ENABLED'))
conf_data.set('XNVME_BE_WINDOWS_ASYNC_ENABLED', conf_data.get('XNVME_BE_WINDOWS_ENABLED'))

conf_data.set('XNVME_BE_LINUX_ENABLED', is_linux)
conf_data.set('XNVME_BE_LINUX_BLOCK_ENABLED', is_linux ? cc.has_header('linux/blkzoned.h') : false)
conf_data.set('XNVME_BE_LINUX_BLOCK_ZONED_ENABLED', is_linux ? cc.has_header('linux/blkzoned.h') : false)

aio_dep = cc.find_library(
  'aio',
  has_headers: ['libaio.h'],
  required: get_option('with-libaio'),
)
conf_data.set('XNVME_BE_LINUX_LIBAIO_ENABLED', aio_dep.found())

uring_dep = dependency(
  'liburing',
  version: '>=2.2',
  required: get_option('with-liburing'),
)
conf_data.set('XNVME_BE_LINUX_LIBURING_ENABLED', uring_dep.found())

libvfn_dep = dependency(
  'libvfn',
  version: '>=2.0.2',
  required: get_option('with-libvfn')
)
conf_data.set('XNVME_BE_LINUX_VFIO_ENABLED', libvfn_dep.found())

corefoundation_dep = dependency(
  'appleframeworks',
  modules: 'CoreFoundation',
  required: is_darwin
)
iokit_dep = dependency(
  'appleframeworks',
  modules: 'IOKit',
  required: is_darwin
)
conf_data.set('XNVME_BE_MACOS_ENABLED', corefoundation_dep.found() and iokit_dep.found())

conf_data.set('XNVME_BE_FBSD_ENABLED', is_freebsd)



spdk_nvme = dependency(
  'spdk_nvme',
  required: get_option('with-spdk')
)

spdk_env_dpdk = dependency(
  'spdk_env_dpdk',
  required: get_option('with-spdk')
)

spdk_syslibs = dependency(
  'spdk_syslibs',
  required: get_option('with-spdk')
)

conf_data.set('XNVME_BE_SPDK_ENABLED', spdk_nvme.found() and spdk_env_dpdk.found() and spdk_env_dpdk.found())
conf_data.set('XNVME_BE_SPDK_TRANSPORT_PCIE_ENABLED', conf_data.get('XNVME_BE_SPDK_ENABLED'))
conf_data.set('XNVME_BE_SPDK_TRANSPORT_TCP_ENABLED', conf_data.get('XNVME_BE_SPDK_ENABLED'))
conf_data.set('XNVME_BE_SPDK_TRANSPORT_RDMA_ENABLED', conf_data.get('XNVME_BE_SPDK_ENABLED'))
conf_data.set('XNVME_BE_SPDK_TRANSPORT_FC_ENABLED', conf_data.get('XNVME_BE_SPDK_ENABLED'))

conf_data.set('XNVME_BE_ASYNC_ENABLED', true)

conf = configure_file(
  configuration : conf_data,
  output : 'xnvme_config.h',
)
conf_inc = [ include_directories('.') ]
if is_windows
  conf_inc += [ include_directories('toolbox' / 'third-party' / 'windows') ]
endif

rpm_conf = configuration_data()
rpm_conf.set('NAME',    meson.project_name())
rpm_conf.set('VERSION', meson.project_version())
rpm_conf.set('LICENSE', meson.project_license()[0])
configure_file(
    input:         'xnvme.spec.in',
    output:        'xnvme.spec',
    configuration: rpm_conf,
)

# Provide configuration via header-file 'xnvme_config.h'
add_global_arguments(['-include', 'xnvme_config.h'], language: 'c')

# bash-completions: run the Makefile target 'make gen-man-pages' to update the completions
# TODO: run the generator-script as part of the meson-build-process for the individual cli-tools to
# avoid manual build of man-pages and to avoid storing autogenerated stuff in the repository
bash_completion_dep = dependency(
  'bash-completion',
  version: '>=2.0',
  required: get_option('bash_completion')
)
if bash_completion_dep.found()
  bash_completions_dir = bash_completion_dep.get_variable('completionsdir')
  message('Bash-completions: ' + bash_completions_dir)
  subdir('toolbox'/'bash_completion.d')
endif

# man-pages: run the Makefile target 'make gen-man-pages' to update the man-pages'XNVME_BE_SPDK_ENABLED'
# TODO: run the generator-script as part of the meson-build-process for the individual cli-tools to
# avoid manual build of man-pages and to avoid storing autogenerated stuff in the repository
if get_option('manpages')
  subdir('man')
endif

# Scripts
subdir('toolbox')

# Misc. third-party dependencies

rt_dep = cc.find_library('rt', required: is_darwin ? false : (is_linux or is_freebsd))
if not cc.has_function('clock_gettime', prefix: '#include <time.h>', dependencies: rt_dep)
  error('clock_gettime not found')
endif

# SPDK dependencies used for creating the xNVMe libraries - but doesn't work for generating pkg-config
if conf_data.get('XNVME_BE_SPDK_ENABLED')
  math_dep = cc.find_library(
    'm',
    has_headers: ['math.h'],
  )

  ssl_dep = dependency(
    'openssl',
    version: '>=1.1.1',
  )

  dlfcn_dep = cc.find_library(
    'dl',
    has_headers: ['dlfcn.h'],
  )

  uuid_dep = cc.find_library(
    'uuid',
    dirs: is_freebsd ? [ '/usr/local/lib' ] : [],
    header_include_directories: is_freebsd ? [ include_directories('/usr/local/include') ] : [],
    has_headers: ['uuid/uuid.h'],
  )
  numa_dep = cc.find_library(
    'numa',
    has_headers: ['numaif.h'],
    required: is_linux,
  )
  execinfo_dep = cc.find_library(
    'execinfo',
    has_headers: ['execinfo.h'],
    required: is_freebsd,
  )
  elf_dep = cc.find_library(
    'elf',
    has_headers: ['sys/elf_common.h'],
    required: is_freebsd,
  )

  spdk_deps = [dlfcn_dep, math_dep, numa_dep, uuid_dep, ssl_dep, ssl_dep, execinfo_dep, elf_dep,]
else
  spdk_deps = []
endif

# Headers
subdir('include')

# Library source
subdir('lib')

# Tests, tools, and code examples
if get_option('tests')
  subdir('tests')
endif
if get_option('tools')
  subdir('tools')
endif
if get_option('examples')
  subdir('examples')
endif
