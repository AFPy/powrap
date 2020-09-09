from pathlib import Path

import pytest

from powrap import powrap

FIXTURE_DIR = Path(__file__).resolve().parent


@pytest.mark.parametrize("po_file", (FIXTURE_DIR / "bad" / "glossary.po",))
def test_fail_on_bad_wrapping(po_file):
    assert powrap.check_style([po_file]) == [po_file]


@pytest.mark.parametrize("po_file", (FIXTURE_DIR / "good").glob("*.po"))
def test_succees_on_good_wrapping(po_file):
    assert powrap.check_style([po_file]) == []


@pytest.mark.parametrize("po_file", (FIXTURE_DIR / "bad" / "invalid_po_file.po",))
def test_msgcat_error(po_file):
    assert powrap.check_style([po_file]) == []


@pytest.mark.parametrize("po_file", ("non_existent_file.po",))
def test_fileread_error(po_file):
    assert powrap.check_style([po_file]) == []
