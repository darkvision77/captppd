if get_option('tests').disabled()
    subdir_done()
endif

tests = [
    'PrinterInfoTest',
    'StatusMessageTest',
    'StateReporterTest',
]

gtest_dep = dependency('gtest_main')
gmock_dep = dependency('gmock')

foreach t : tests
    exe = executable(
        t,
        files('@0@.cpp'.format(t)),
        dependencies: [gtest_dep, gmock_dep, captbackend_dep],
        install: false,
    )
    test(t, exe, protocol: 'gtest', suite: ['core'])
endforeach
