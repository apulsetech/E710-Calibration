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
    description="ex10 calgen",
    install_requires=[
        'pyyaml',
        'jinja2==3.0.1',
        'six==1.16.0',
    ],
    license='Proprietary',
    name='ex10_calgen',
    version='1.2.14',
    packages = ["calibration_parser", "parsing_utils"],
    package_dir={
        "calibration_parser": "./python_tools/ex10_api_yaml_parsers",
        "parsing_utils": "./python_tools/parsing"
    },
    scripts=[],
    setup_requires=[],
    zip_safe=False,
)
