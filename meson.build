project(
  'xfburn',
  'c',
  version: '0.7.2-dev',
  license: 'GPL-2.0-or-later',
  meson_version: '>= 0.56.0',
  default_options: [
    'c_std=gnu11',
    'buildtype=debugoptimized',
    'warning_level=2',
  ]
)

project_namespace = 'apps'
pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()
feature_cflags = []

cc = meson.get_compiler('c')
fs = import('fs')
gnome = import('gnome')
i18n = import('i18n')

dependency_versions = {
  'gudev': '>= 145',
  'gst': '>= 1.0.0',
  'libburn': '>= 0.4.2',
  'libisofs': '>= 0.6.2',
  'glib': '>= 2.66.0',
  'gtk': '>= 3.24.0',
  'xfce4': '>= 4.18.0',
}

gio = dependency('gio-2.0', version: dependency_versions['glib'])
giounix = dependency('gio-unix-2.0', version: dependency_versions['glib'])
glib = dependency('glib-2.0', version: dependency_versions['glib'])
gthread = dependency('gthread-2.0', version: dependency_versions['glib'])
gtk = dependency('gtk+-3.0', version: dependency_versions['gtk'])
libburn = dependency('libburn-1', version: dependency_versions['libburn'])
libisofs = dependency('libisofs-1', version: dependency_versions['libisofs'])
libxfce4util = dependency('libxfce4util-1.0', version: dependency_versions['xfce4'])
libxfce4ui = dependency('libxfce4ui-2', version: dependency_versions['xfce4'])
if libxfce4ui.version().version_compare('< 4.21.0')
  exo = dependency('exo-2', version: dependency_versions['xfce4'])
else
  exo = dependency('', required: false)
endif

libm = cc.find_library('m', required: true)
xsltproc = find_program('xsltproc', required: true)

# optional dependencies
gudev = dependency('gudev-1.0', version: dependency_versions['gudev'], required: false)
if gudev.found()
  feature_cflags += '-DHAVE_GUDEV=1'
endif

gst = dependency('gstreamer-1.0', version: dependency_versions['gst'], required: false)
if gst.found()
  feature_cflags += '-DHAVE_GST=1'
  gstpbutils = dependency('gstreamer-pbutils-1.0', version: dependency_versions['gst'])
else
  gstpbutils = dependency('', required: false)
endif

extra_cflags = []
extra_cflags_check = [
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wold-style-definition',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Winit-self',
  '-Wshadow',
  '-Wmissing-include-dirs',
  '-Wundef',
  '-Wformat',
  '-Wformat-security',
  '-Wformat-y2k',
  '-Wnested-externs',
  '-Wno-unused-parameter',
  '-Wno-declaration-after-statement',
  '-Wno-missing-field-initializers',
  '-Werror=implicit-function-declaration',
  '-Wno-error=deprecated-declarations',
]

optimization = get_option('optimization')
if get_option('debug') and optimization in ['0', 'g']
  extra_cflags_check += '-fstack-protector-strong'
  extra_cflags += [
    '-DDEBUG=1',
    '-DDEBUG_TRACE=1',
    '-DG_ENABLE_DEBUG',
  ]
elif optimization in ['3', 'minsize']
  extra_cflags += [
    '-DNDEBUG',
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_ASSERT',
  ]
endif

if dependency_versions.has_key('glib')
  glib_version_parts = dependency_versions['glib'].split(' ')
  glib_min_version_parts = glib_version_parts[1].split('.')
  glib_min_version_define = 'GLIB_VERSION_@0@_@1@'.format(glib_min_version_parts[0], glib_min_version_parts[1])
  extra_cflags += [
    '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_min_version_define),
    '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_min_version_define),
    '-DG_LOG_DOMAIN="@0@"'.format(meson.project_name()),
    '-DG_LOG_USE_STRUCTURED=1',
  ]
endif

version_parts = meson.project_version().split('-dev')[0].split('.')
version_short = '@0@.@1@'.format(version_parts[0], version_parts[1])

extra_cflags += [
  '-DPACKAGE="@0@"'.format(meson.project_name()),
  '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DVERSION_SHORT="@0@"'.format(version_short),
  '-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(pkgdatadir),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/@0@/@1@/-/issues"'.format(project_namespace, meson.project_name()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DPREFIX="@0@"'.format(get_option('prefix')),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
  '-DDATADIR="@0@"'.format(get_option('prefix') / get_option('datadir')),
  '-DINCLUDEDIR="@0@"'.format(get_option('prefix') / get_option('includedir')),
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DLIBEXECDIR="@0@"'.format(get_option('prefix') / get_option('libexecdir')),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DLOCALSTATEDIR="@0@"'.format(get_option('prefix') / get_option('localstatedir')),
  '-DSBINDIR="@0@"'.format(get_option('prefix') / get_option('sbindir')),
  '-DSYSCONFDIR="@0@"'.format(get_option('prefix') / get_option('sysconfdir')),
  '-DHAVE_XFCE_REVISION_H=1',
]

add_project_arguments(cc.get_supported_arguments(extra_cflags_check), language: 'c')
add_project_arguments(extra_cflags, language: 'c')
add_project_arguments(feature_cflags, language: 'c')

xfce_revision_h = vcs_tag(
  command: ['git', 'rev-parse', '--short', 'HEAD'],
  fallback: 'UNKNOWN',
  input: 'xfce-revision.h.in',
  output: 'xfce-revision.h',
  replace_string: '@REVISION@',
)

i18n.merge_file(
    input: 'org.xfce.xfburn.appdata.xml.in',
    output: 'org.xfce.xfburn.appdata.xml',
    po_dir: 'po',
    type: 'xml',
    install: true,
    install_dir: get_option('prefix') / get_option('datadir') / 'metainfo',
)

i18n.merge_file(
    input: 'xfburn.desktop.in',
    output: 'xfburn.desktop',
    po_dir: 'po',
    type: 'desktop',
    install: true,
    install_dir: get_option('prefix') / get_option('datadir') / 'applications',
)

install_data(
  ['xfburn-popup-menus.ui', 'xfburn-toolbars.ui', 'xfburn.ui'],
  install_dir: get_option('prefix') / get_option('datadir') / 'xfburn',
)

subdir('desktop-integration')
subdir('docs')
subdir('icons')
subdir('po')
subdir('xfburn')
