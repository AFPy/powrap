from pathlib import Path
from tempfile import NamedTemporaryFile

from powrap import powrap

FIXTURE_DIR = Path(__file__).resolve().parent
BAD_FILEPATH = FIXTURE_DIR / "bad" / "glossary.po"
GOOD_FILEPATH = FIXTURE_DIR / "good" / "glossary.po"


def test_fix_style():
    with open(BAD_FILEPATH, "r") as bad_f, open(GOOD_FILEPATH) as good_f:
        bad_po_content, good_po_content = (bad_f.read(), good_f.read())
    with NamedTemporaryFile("w+") as tmpfile:
        tmpfile.write(bad_po_content)
        tmpfile.seek(0)
        powrap.fix_style([tmpfile.name])
        assert tmpfile.read() == good_po_content
