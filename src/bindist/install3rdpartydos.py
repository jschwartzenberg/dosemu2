from consolemenu import *
from consolemenu.items import *
import subprocess

menu = ConsoleMenu("DOSEMU2 DOS installer script", "Pick the DOS to install")

for dosline in subprocess.getoutput(['downloaddos -l']).splitlines():
    dos = dosline.split(' ', 1)
    menu.append_item(CommandItem(dos[1],  "downloaddos -d " + dos[0] + " /tmp/ins && installdos /tmp/ins ${HOME}/.dosemu/drive_c", should_exit=True))

menu.show()
