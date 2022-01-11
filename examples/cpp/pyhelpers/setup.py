from setuptools import setup, Extension
from Cython.Build import cythonize
import numpy as np

setup(ext_modules=cythonize(
    language_level='3',
    module_list=[
        Extension(
            'pyhelpers.cointegration',
            ['pyhelpers/cointegration.pyx'],
            include_dirs=[np.get_include()],
            language='c++'
        ),
        Extension(
            'pyhelpers.stationarity',
            ['pyhelpers/stationarity.pyx'],
            include_dirs=[np.get_include()],
            language='c++'
        ),
    ]
))
