custom_target(
  'xfburn.1',
  input: 'xfburn.xml',
  output: 'xfburn.1',
  command: [xsltproc, '--nonet', '--output', '@OUTPUT@', 'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl', '@INPUT@'],
  install: true,
  install_dir: get_option('prefix') / get_option('mandir') / 'man1',
)
