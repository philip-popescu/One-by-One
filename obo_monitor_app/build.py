import PyInstaller.__main__

PyInstaller.__main__.run([
    'main.py',
    "--add-data=data:data",
    "--add-data=config:config",
    "--add-data=udp_server.py:.",
    '--windowed'
])
