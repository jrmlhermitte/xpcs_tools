#!/usr/bin/env python
# to install:
# python setup.py install
from setuptools import setup
import setuptools
import numpy as np

setup(
    name='xpcs_tools',
    author='Julien Lhermitte',
    packages=setuptools.find_packages(exclude=['doc']),
    include_dirs=[np.get_include()],
    )

