#!/usr/bin/env python

"""Setup configuration."""

from setuptools import setup, find_packages

setup(
    author="Impinj",
    author_email="support@impinj.com",
    classifiers=[
        "Intended Audience :: Developers",
        "Programming Language :: Python :: 3",
    ],
    description="ex10 development kit",
    entry_points={
        "console_scripts": [
            "r807=utils.r807:main",
            "beagle_sdd_capture=utils.beagle_sdd_capture:main",
        ],
    },
    include_package_data=True,
    install_requires=[
        'crcmod',
        'numpy',
        'pyyaml',
        'requests'
    ],
    license='Proprietary',
    name='ex10_dev_kit',
    version='2.00.00',
    packages=find_packages(
        where=".", exclude=["tests*"]
    ),
    package_data={
        "py2c_interface": [
            "lib_py2c.so",
            "lib_py2c-coverage.so",
            "yk_app_yx4.yk_image",
        ]
    },    scripts=[],
    setup_requires=[],
    zip_safe=False,
)
