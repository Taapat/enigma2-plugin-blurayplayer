from distutils.core import setup, Extension
import setup_translate


plugin = 'Extensions.BlurayPlayer'

module = Extension(plugin + '.blurayinfo',
		libraries=['bluray', 'udfread'],
		sources=['src/blurayinfo.c'])

setup(name='enigma2-plugin-extensions-blurayplayer',
		version='1.0',
		author='Taapat',
		author_email='taapat@gmail.com',
		package_dir={plugin: 'src'},
		packages=[plugin],
		package_data={plugin: ['*.png', 'locale/*/LC_MESSAGES/*.mo']},
		description='Play Blu-ray videos',
		cmdclass=setup_translate.cmdclass,
		ext_modules=[module])
