project('msgraph', 'c',
          version: '0.3.3',
    meson_version: '>= 0.63.0',
  default_options: [ 'warning_level=2',
                     'werror=false',
                     'c_std=gnu11',
                   ],
)

# Versioning
msgraph_api_version = '1'

cc = meson.get_compiler('c')

msgraph_prefix = get_option('prefix')
msgraph_pkgincludedir = join_paths(msgraph_prefix, get_option('includedir'))
msgraph_include_dir = join_paths(msgraph_pkgincludedir, 'msgraph')
msgraph_basename = 'msgraph-@0@'.format(msgraph_api_version)
pkglibdir = join_paths(msgraph_prefix, get_option('libdir'), 'libmsgraph')
msg_name = meson.project_name()
msg_libexecdir = get_option('libexecdir')

build_root = meson.current_build_dir()
top_inc = include_directories('.')
src_inc = include_directories('src')

glib_dep = dependency('glib-2.0', version: '>= 2.28')
gio_dep = dependency('gio-2.0')
json_dep = dependency('json-glib-1.0')
libsoup_dep = dependency('libsoup-3.0')
goa_dep = dependency('goa-1.0')

libuhttpmock_dep = dependency('libuhttpmock-1.0', version: '>= 0.11.0', required: get_option('tests'))
libxml_dep = dependency('libxml-2.0', required: get_option('tests'))

gnome = import('gnome')

enable_gir = get_option('introspection')

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('G_LOG_DOMAIN', meson.project_name())
config_h.set('GOA_API_IS_SUBJECT_TO_CHANGE', true)
configure_file(
  output: 'config.h',
  configuration: config_h
)

add_project_arguments([
  '-I' + meson.project_build_root(),
  '-DMSG_COMPILATION',
], language: 'c')


subdir('src')

if get_option('tests')
  subdir('tests')
endif

if get_option('gtk_doc')
  if not get_option('introspection')
    error('Can\'t generate docs without introspection enabled!')
  endif

  subdir('doc')
endif
