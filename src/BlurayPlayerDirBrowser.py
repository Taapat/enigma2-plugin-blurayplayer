import os

from Components.ActionMap import ActionMap
from Components.config import config
from Components.FileList import FileList
from Components.Sources.StaticText import StaticText
from Screens.Screen import Screen

from . import _
from BlurayUi import BlurayMain


class BlurayPlayerDirBrowser(Screen):
	def __init__(self, session):
		Screen.__init__(self, session)
		self.session = session
		self.skinName = ['BlurayPlayerDirBrowser', 'FileBrowser']
		self['key_red'] = StaticText(_('Exit'))
		self['key_green'] = StaticText(_('Ok'))
		self['filelist'] = FileList(directory=config.usage.default_path.value,
				matchingPattern='(?i)^.*\.(iso)', enableWrapAround=True)
		self['FilelistActions'] = ActionMap(['SetupActions', 'ColorActions'],
				{
					'cancel': self.close,
					'red': self.close,
					'ok': self.ok,
					'green': self.ok
				})
		self.setTitle(_('Please select the blu-ray disc'))

	def ok(self):
		fileName = self['filelist'].getFilename()
		if fileName and fileName[-1:] == '/' and \
				os.path.isdir(os.path.join(fileName, 'BDMV/STREAM/')):
			self.session.open(BlurayMain, fileName)
		elif self['filelist'].canDescent():
			self['filelist'].descent()
		elif fileName and fileName[-1:] != '/':
			currentDir = self['filelist'].getCurrentDirectory()
			iso_path = os.path.join(currentDir, fileName)
			try:
				from Plugins.Extensions.BlurayPlayer import blurayinfo
				if blurayinfo.isBluray(iso_path) == 1:
					self.session.open(BlurayMain, iso_path)
			except Exception as e:
				print "[BlurayPlayer] Error on open iso:", e
