sizes = [16, 22, 24, 32, 48]
names = [
  'stock_xfburn-audio-cd',
  'stock_xfburn-audio-copy',
  'stock_xfburn-blank-cdrw',
  'stock_xfburn-burn-cd',
  'stock_xfburn-data-copy',
  'stock_xfburn-data-copy',
  'stock_xfburn-format-dvdrw',
  'stock_xfburn-import-session',
  'stock_xfburn-new-data-composition',
  'stock_xfburn',
]

foreach name : names
  foreach size : sizes
    filename = '@0@x@0@'.format(size) / '@0@.png'.format(name)
    if fs.is_file(filename)
      install_data(
        filename,
        install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'stock' / 'media',
      )
    endif
  endforeach

  filename = 'scalable' / '@0@.svg'.format(name)
  if fs.is_file(filename)
    install_data(
      filename,
      install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
    )
  endif
endforeach
