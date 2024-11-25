msgraph_inc_dir = include_directories('.')
msgraph_built_headers = []

msgraph_sources = files(
  'drive/msg-drive.c',
  'drive/msg-drive-item.c',
  'drive/msg-drive-item-file.c',
  'drive/msg-drive-item-folder.c',
  'drive/msg-drive-service.c',
  'mail/msg-mail-folder.c',
  'mail/msg-mail-message.c',
  'mail/msg-mail-service.c',
  'user/msg-user.c',
  'user/msg-user-contact-folder.c',
  'user/msg-user-service.c',
  'msg-authorizer.c',
  'msg-error.c',
  'msg-goa-authorizer.c',
  'msg-input-stream.c',
  'msg-json-utils.c',
  'msg-oauth2-authorizer.c',
  'msg-service.c',
)

msgraph_headers = files(
  'drive/msg-drive.h',
  'drive/msg-drive-item.h',
  'drive/msg-drive-item-file.h',
  'drive/msg-drive-item-folder.h',
  'drive/msg-drive-service.h',
  'mail/msg-mail-folder.h',
  'mail/msg-mail-message.h',
  'mail/msg-mail-service.h',
  'user/msg-user.h',
  'user/msg-user-contact-folder.h',
  'user/msg-user-service.h',
  'msg.h',
  'msg-authorizer.h',
  'msg-error.h',
  'msg-goa-authorizer.h',
  'msg-input-stream.h',
  'msg-json-utils.h',
  'msg-oauth2-authorizer.h',
  'msg-private.h',
  'msg-service.h',
)

version_split = meson.project_version().split('.')
MAJOR_VERSION = version_split[0]
MINOR_VERSION = version_split[1]
MICRO_VERSION = version_split[2]

version_conf = configuration_data()
version_conf.set('VERSION', meson.project_version())
version_conf.set('MAJOR_VERSION', MAJOR_VERSION)
version_conf.set('MINOR_VERSION', MINOR_VERSION)
version_conf.set('MICRO_VERSION', MICRO_VERSION)

configure_file(
  input: 'msg-version.h.in',
  output: 'msg-version.h',
  configuration: version_conf,
  install: true,
  install_dir: join_paths(get_option('includedir'), 'msg')
)

msgraph_deps = [
  glib_dep,
  gio_dep,
  json_dep,
  libsoup_dep,
  goa_dep,
]

mapfile = 'libmsgraph.map'
vscript = '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(), mapfile)
vflag = []
if cc.has_multi_link_arguments(vscript)
  vflag += vscript
endif

msgraph_lib = shared_library(
  msgraph_basename,
  msgraph_sources,
  dependencies: msgraph_deps,
  link_args : vflag,
  link_depends : mapfile,
  include_directories: [top_inc],
  version: meson.project_version(),
  soversion: '1',
  install: true,
  install_rpath: pkglibdir,
)

if get_option('introspection')
  msgraph_gir_extra_args = [
    '--c-include=msg.h',
    # '--quiet',
    '-DMSGRAPH_COMPILATION',
  ]
  msgraph_gir = gnome.generate_gir(
    msgraph_lib,
    sources: [ msgraph_sources, msgraph_headers ],
    nsversion: msgraph_api_version,
    namespace: 'Msg',
    symbol_prefix: 'msg',
    export_packages: msgraph_basename,
    link_with: msgraph_lib,
    includes: [
      'GObject-2.0',
      'Gio-2.0',
      'GLib-2.0',
      'Goa-1.0',
      'Json-1.0',
      'Soup-3.0',
    ],
    install: true,
    extra_args:  msgraph_gir_extra_args,
  )
endif

libmsgraph_dep = declare_dependency(
  link_with : msgraph_lib,
  include_directories : msgraph_inc_dir,
  dependencies : msgraph_deps
)

install_headers(msgraph_headers, subdir: 'msg', preserve_path : true)

pkg = import('pkgconfig')

pkg.generate(
  description: 'A shared library for accessing MS Graph API',
    libraries: msgraph_lib,
         name: 'msgraph',
     filebase: 'msgraph-' + msgraph_api_version,
      version: meson.project_version(),
      subdirs: 'msg',
     requires: msgraph_deps,
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)
