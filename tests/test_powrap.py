from pathlib import Path

import pytest

from powrap import powrap

FIXTURE_DIR = Path(__file__).resolve().parent


@pytest.mark.parametrize("po_file", (FIXTURE_DIR / "bad" / "glossary.po",))
def test_fail_on_bad_wrapping(po_file, capsys):
    assert powrap.main(["--check", str(po_file)]) == 1
    assert str(po_file) in capsys.readouterr().err


@pytest.mark.parametrize("po_file", (FIXTURE_DIR / "good").glob("*.po"))
def test_success_on_good_wrapping(po_file, capsys):
    assert powrap.main(["--check", str(po_file)]) == 0
    assert str(po_file) not in capsys.readouterr().err


@pytest.mark.parametrize("po_file", (FIXTURE_DIR / "bad" / "invalid_po_file.po",))
def test_po_error(po_file, capsys):
    assert powrap.main(["--check", str(po_file)]) == 127
    assert str(po_file) in capsys.readouterr().err


@pytest.mark.parametrize("po_file", ("non_existent_file.po",))
def test_fileread_error(po_file, capsys):
    with pytest.raises(SystemExit):
        powrap.main(["--check", str(po_file)])
    assert str(po_file) in capsys.readouterr().err
