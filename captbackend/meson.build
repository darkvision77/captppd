cups_dep = dependency('cups')
libusb_dep = dependency('libusb-1.0')
libcapt_dep = dependency('libcapt', fallback: ['libcapt', 'libcapt_dep'], static: true)

sources = files(
    'PrinterInfo.cpp',
    'CmdClean.cpp',
    'CmdPrint.cpp',
    'main.cpp',
)

subdir('Core')
subdir('UsbBackend')

executable(
    get_option('backendname'),
    sources,
    dependencies: [cups_dep, libcapt_dep, libusb_dep],

    cpp_args: [
        '-DCAPTBACKEND_NAME="@0@"'.format(get_option('backendname')),
        '-DCAPTBACKEND_VERSION="@0@"'.format(meson.project_version()),
        '-DLIBCAPT_VERSION="@0@"'.format(libcapt_dep.version()),
    ],

    install: true,
    install_mode: ['r-x------', 'root', 'root'],
    install_dir: cups_backend_dir,
)
