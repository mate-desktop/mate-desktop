icons = [
  '16x16/apps/mate-desktop.png',
  '22x22/apps/mate-desktop.png',
  '24x24/apps/mate-desktop.png',
  '32x32/apps/mate-desktop.png',
  '48x48/apps/mate-desktop.png',
  '128x128/apps/mate-desktop.png',
  'scalable/apps/mate-desktop.svg',
  'scalable/apps/mate-desktop-symbolic.svg',
]

foreach i : icons
  dir = i.split('/')
  install_data(i,
    install_dir: join_paths('share/icons/hicolor', dir[0], dir[1]),
  )
endforeach
