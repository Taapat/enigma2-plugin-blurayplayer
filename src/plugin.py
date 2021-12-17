from __future__ import print_function

import os

from Screens.MovieSelection import MovieSelection
from Plugins.Plugin import PluginDescriptor
from enigma import getDesktop

from . import _


isMovieSelection = False
orig_gotFilename = None
orig_itemSelectedCheckTimeshiftCallback = None
try:
	orig_gotFilename = MovieSelection.gotFilename
	orig_itemSelectedCheckTimeshiftCallback = MovieSelection.itemSelectedCheckTimeshiftCallback
	isMovieSelection = True
except Exception:
	print('[BlurayPlayer] Plugin can not be used in MovieSelection')


# Replaces the original gotFilename to add bluray folder test at the beginning
# If test fails call original gotFilename as orig_gotFilename to to keep the code unchanged
def gotFilename(self, res, selItem=None):
	global orig_gotFilename
	if res and os.path.isdir(res):
		if os.path.isdir(os.path.join(res, 'BDMV/STREAM/')):
			try:
				from Plugins.Extensions.BlurayPlayer import BlurayUi
				self.session.open(BlurayUi.BlurayMain, res)
			except Exception as e:
				print('[BlurayPlayer] Cannot open BlurayPlayer:', e)
			else:
				return
	# Call the private copy of the original.
	orig_gotFilename(self, res, selItem)


# Replaces the original itemSelectedCheckTimeshiftCallback to add iso mount at the beginning
# If mount fails call original as orig_itemSelectedCheckTimeshiftCallback to to keep code unchanged
def itemSelectedCheckTimeshiftCallback(self, ext, path, answer):
	global orig_itemSelectedCheckTimeshiftCallback
	if answer:
		if ext == '.iso':
			try:
				from Plugins.Extensions.BlurayPlayer import blurayinfo
				if blurayinfo.isBluray(path) == 1:
					from Plugins.Extensions.BlurayPlayer import BlurayUi
					self.session.open(BlurayUi.BlurayMain, path)
					return True
			except Exception as e:
				print("[BlurayPlayer] CheckTimeshift error:", e)
		# Call the private copy of the original
		orig_itemSelectedCheckTimeshiftCallback(self, ext, path, answer)


# If we can work, put our new definitions into place, overriding the
# originals. We've already remembered what they were.
if isMovieSelection:
	MovieSelection.gotFilename = gotFilename
	MovieSelection.itemSelectedCheckTimeshiftCallback = itemSelectedCheckTimeshiftCallback


def dirBrowser(session, **kwargs):
	from .BlurayPlayerDirBrowser import BlurayPlayerDirBrowser
	session.open(BlurayPlayerDirBrowser)


def Plugins(**kwargs):
	screenwidth = getDesktop(0).size().width()
	if screenwidth and screenwidth == 1920:
		icon = 'BlurayPlayer_FHD.png'
	else:
		icon = 'BlurayPlayer_HD.png'
	return [PluginDescriptor(
			name=_('Blu-ray player'),
			description=_('Watch blu-ray discs in folder or iso'),
			where=[PluginDescriptor.WHERE_PLUGINMENU],
			icon=icon,
			fnc=dirBrowser)]
