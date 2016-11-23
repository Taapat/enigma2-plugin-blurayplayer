from Plugins.Plugin import PluginDescriptor
from enigma import getDesktop

from . import _
import ChangeFunctions


def dirBrowser(session, **kwargs):
	from BlurayPlayerDirBrowser import BlurayPlayerDirBrowser
	session.open(BlurayPlayerDirBrowser)


def Plugins(**kwargs):
	screenwidth = getDesktop(0).size().width()
	if screenwidth and screenwidth == 1920:
		icon = 'BlurayPlayer_FHD.png'
	else:
		icon = 'BlurayPlayer_HD.png'
	return [PluginDescriptor(
			name = _('Blu-ray player'),
			description = _('Watch blu-ray discs in folder or iso'),
			where = [PluginDescriptor.WHERE_PLUGINMENU,],
			icon = icon,
			fnc = dirBrowser
			)]
