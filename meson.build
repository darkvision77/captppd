project(
    'captppd',
    'cpp',
    version: '0.1.0',
    license: 'BSD-2-Clause',
    license_files: ['LICENSE'],
    default_options: [
        'buildtype=debugoptimized',
        'b_ndebug=if-release',
        'warning_level=3',
        'cpp_std=c++23',
        'cpp_rtti=false',
        'b_lundef=false',
    ],
    meson_version: '>=1.5.0',
)

cpp = meson.get_compiler('cpp')

cpp.compiles('''
#include <stop_token>
int main() { std::stop_token token; return 0; }
''', name: 'std::stop_token support', required: true)

cups_config = find_program('cups-config')

cups_serverbin = get_option('cupsserverbin')
if cups_serverbin == ''
    cups_serverbin = run_command(cups_config, '--serverbin', check: true).stdout().strip()
endif

cups_datadir = get_option('cupsdatadir')
if cups_datadir == ''
    cups_datadir = run_command(cups_config, '--datadir', check: true).stdout().strip()
endif

cups_dep = dependency('cups')
libusb_dep = dependency('libusb-1.0')
libcapt_dep = dependency('libcapt', fallback: ['libcapt', 'libcapt_dep'], static: true)

add_project_arguments(
    '-DCAPTBACKEND_NAME="@0@"'.format(get_option('backendname')),
    '-DCAPTBACKEND_VERSION="@0@"'.format(meson.project_version()),
    '-DLIBCAPT_VERSION="@0@"'.format(libcapt_dep.version()),
    language: ['cpp'],
)

summary({
    'Backend name': get_option('backendname'),
    'CUPS bin directory': cups_serverbin,
    'CUPS data directory': cups_datadir,
}, section: 'Configuration')

subdir('captbackend')
subdir('ppd')
subdir('dist')
subdir('tests')
