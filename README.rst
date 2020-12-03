powrap
======

|pypi|

.. |pypi| image:: https://img.shields.io/pypi/v/powrap.svg
   :target: https://pypi.python.org/pypi/powrap

Script to fix indentation of given ``.po`` files. If ``--modified`` is
given, it will only fix modified files according to git (usefull if
your ``.po`` files are versionned).

if ``--quiet`` is given, the progress bar will not be shown


Powrap is part of poutils!
==========================

`Poutils <https://pypi.org/project/poutils>`_ (``.po`` utils) is a metapackage to easily install useful Python tools to use with po files
and ``powrap`` is a part of it! Go check out `Poutils <https://pypi.org/project/poutils>`_ to discover the other tools!


Dependencies
============

``powrap`` relies on ``msgcat`` from ``gettext`` so you'll have to
install ``gettext`` first, for example on Debian run::

  apt install gettext


Contributing
============

Start by creating a venv and ``pip install -r requirements-dev.txt`` in
it.

To run the tests, use ``tox -p auto``.

To install ``powrap`` in the current venv run ``pip install -e .``.


Dependencies
------------

We're using ``pip-tools`` to pin our dependencies, but in the
``setup.cfg`` our dependencies are *not* pinned, the goal is to ensure
``powrap`` can easily be installed along with other tools.

Dependencies pinning is only done to have a reproducible development
environment and corresponding env in the CI::

  pip-compile setup.py  # generates requirements.txt
  pip-compile requirements-dev.in  # generates requirements-dev.txt

It's possible to upgrade pinned dependencies with the ``--upgrade``
flag of ``pip-compile``.
