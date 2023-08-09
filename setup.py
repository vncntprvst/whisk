# Create setup.py to enable pip install

import os

# Get a list of all binary files 
bin_files = [os.path.join('whisk', 'bin', f) for f in os.listdir(os.path.join('whisk', 'bin')) if os.path.isfile(os.path.join('whisk', 'bin', f))]

from setuptools import setup

with open('README.md', 'r') as f:
    long_description = f.read()

setup(
    name='whisk-janelia',
    version='1.1.4',
    author='Nathan Clack',
    maintainer=', '.join(['clackn','cxrodgers','mitchclough','vncntprvst']),
    url='https://github.com/nclack/whisk/',
    license='LICENSE.txt',
    description='Fully automated tracking of single rows of whiskers in high-speed video.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    install_requires=[],
    packages=['whisk'],
    package_data={'whisk': ['bin/*']},
    include_package_data=True,
)


