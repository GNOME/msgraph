msgraph_content_files = [
]

msg_toml = configure_file(
  input: 'msg.toml.in',
  output: '@BASENAME@',
  configuration: {
    'VERSION': msgraph_api_version,
  },
)

gidocgen = find_program('gi-docgen')

msgraph_docs = custom_target(
  'msgraph-docs',
  input: [msg_toml, msgraph_gir[0]],
  output: msgraph_basename,
  command: [
    gidocgen,
    'generate',
    '--quiet',
    '--add-include-path=@0@'.format(meson.current_source_dir() / 'src'),
    '--config=@INPUT0@',
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT1@',
  ],
  depend_files: [ msgraph_content_files ],
  build_by_default: true,
  install: true,
  install_dir: get_option('datadir') / 'doc',
)