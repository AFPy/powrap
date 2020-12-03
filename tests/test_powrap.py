from pathlib import Path

import pytest
import os

from powrap import powrap

FIXTURE_DIR = Path(__file__).resolve().parent


@pytest.mark.parametrize("po_file", (FIXTURE_DIR / "bad" / "glossary.po",))
def test_fail_on_bad_wrapping(po_file, capsys):
    assert powrap.check_style([po_file]) == 1
    assert str(po_file) in capsys.readouterr().err


@pytest.mark.parametrize("po_file", (FIXTURE_DIR / "good").glob("*.po"))
def test_succees_on_good_wrapping(po_file, capsys):
    assert powrap.check_style([po_file]) == 0
    assert str(po_file) not in capsys.readouterr().err


@pytest.mark.parametrize("po_file", (FIXTURE_DIR / "bad" / "invalid_po_file.po",))
def test_msgcat_error(po_file, capsys):
    assert powrap.check_style([po_file]) == 0
    assert str(po_file) not in capsys.readouterr().err


@pytest.mark.parametrize("po_file", ("non_existent_file.po",))
def test_fileread_error(po_file, capsys):
    assert powrap.check_style([po_file]) == 0
    assert str(po_file) not in capsys.readouterr().err


@pytest.mark.parametrize("po_file", (FIXTURE_DIR / "good").glob("*.po"))
def test_wrong_msgcat(po_file):
    """Test if msgcat is not available"""
    environ_saved = os.environ["PATH"]
    os.environ["PATH"] = ""
    with pytest.raises(SystemExit) as sysexit:
        powrap.check_style([po_file])
    os.environ["PATH"] = environ_saved
    assert sysexit.type == SystemExit
    assert sysexit.value.code == 127
