ppdc = find_program('ppdc')

custom_target(
    input: 'captppd.drv',
    output: [
        'LBP800CAPTPPD.ppd',
        'LBP810CAPTPPD.ppd',
        'LBP1120CAPTPPD.ppd',
        'LBP1210CAPTPPD.ppd',
        'LBP3200CAPTPPD.ppd',
    ],
    command: [
        ppdc,
        '-D', 'CAPTBACKEND_VERSION=@0@'.format(meson.project_version()),
        '-D', 'HAVE_DITHERING_TYPE=@0@'.format(get_option('ditheringopt').enabled() ? 1 : 0),
        '-d', '@OUTDIR@',
        '@INPUT@'
    ],
    install: true,
    install_dir: cups_ppd_dir,
)
