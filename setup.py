from setuptools import setup

with open("README.rst") as readme_file:
    readme = readme_file.read()

setup(
    name="poindent",
    version="0.1.1",
    description="Find an properly reindent .po files.",
    long_description=readme,
    author="Julien Palard",
    author_email="julien@palard.fr",
    url="https://github.com/JulienPalard/poindent",
    packages=["poindent"],
    package_dir={"poindent": "poindent"},
    entry_points={"console_scripts": ["poindent=poindent.poindent:main"]},
    include_package_data=True,
    install_requires=["tqdm"],
    license="MIT license",
    zip_safe=False,
    keywords="poindent",
    classifiers=[
        "Development Status :: 2 - Pre-Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Natural Language :: English",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
    ],
)
