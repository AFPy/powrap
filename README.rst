powrap
======

|build| |pypi|

.. |build| image:: https://travis-ci.org/JulienPalard/powrap.svg?branch=master
   :target: https://travis-ci.org/JulienPalard/powrap

.. |pypi| image:: https://img.shields.io/pypi/v/powrap.svg
   :target: https://pypi.python.org/pypi/powrap

Script to fix indentation of given ``.po`` files. If ``--modified`` is
given, it will only fix modified files according to git (usefull if
your ``.po`` files are versionned).

if ``--quiet`` is given, the progress bar will not be shown

This package only runs ``msgcat`` from the ``gettext`` package, so if
your distribution don't have it, it just won't work.


Contributing
============

Start by creating a venv and installing `requirements-dev.in`.

To run the tests, use `tox -p auto`.
