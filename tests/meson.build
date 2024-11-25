tests_sources = files(
  'common.c',
  'msg-dummy-authorizer.c',
)

common_deps = [
  libmsgraph_dep,
  libuhttpmock_dep,
  libxml_dep,
]

# The empty objects may be used in future for keeping custom
# arguments for each test
tests = {
  'authorization': {},
  'drive': {},
  'mail': {},
  'user': {},
  'service': {},
}

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

foreach test_name, extra_args: tests
  should_fail = extra_args.get('should_fail', false)
  exe = executable(
    test_name,
    test_name + '.c',
    c_args: ['-DG_LOG_DOMAIN="@0@"'.format(meson.project_name())],
    include_directories: top_inc,
    dependencies: common_deps + extra_args.get('dependencies', []),
    sources: tests_sources,
    install: false,
  )

  test(
    test_name,
    exe,
    env: test_env,
    should_fail: should_fail,
  )
endforeach