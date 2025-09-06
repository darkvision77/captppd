sources = files(
    'CmdClean.cpp',
    'CmdPrint.cpp',
)

main_sources = files(
    'main.cpp',
)

subdir('Core')
subdir('UsbBackend')

captbackend_lib = static_library(
    get_option('backendname'),
    sources,
    dependencies: [cups_dep, libcapt_dep, libusb_dep],
    install: false,
)
captbackend_dep = declare_dependency(
    link_with: [captbackend_lib],
    dependencies: [cups_dep, libcapt_dep, libusb_dep],
    include_directories: include_directories('.'),
)

executable(
    get_option('backendname'),
    main_sources,
    dependencies: [captbackend_dep],
    install: true,
    install_mode: ['r-x------', 'root', 'root'],
    install_dir: cups_backend_dir,
)
