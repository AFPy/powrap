#!/usr/bin/env python3

"""Fix style of uncommited po files, or all if --all is given.
"""

from shlex import quote
from subprocess import check_output, run

from tqdm import tqdm


def fix_style(po_files, modified=False, no_wrap=False):
    """Fix style of unversionned ``.po`` files, or or all f
    """
    if modified:
        git_status = check_output(["git", "status", "--porcelain"], encoding="utf-8")
        git_status_lines = [
            line.split(maxsplit=2) for line in git_status.split("\n") if line
        ]
        po_files.extend(
            filename
            for status, filename in git_status_lines
            if filename.endswith(".po")
        )
    for po_path in tqdm(po_files, desc="Fixing indentation in po files"):
        with open(po_path, encoding="UTF-8") as po_file:
            po_content = po_file.read()
        args = ["msgcat", "-", "-o", po_path]
        if no_wrap:
            args[1:1] = ["--no-wrap"]
        run(args, encoding="utf-8", check=True, input=po_content)


def main():
    import argparse

    parser = argparse.ArgumentParser(
        prog="powrap",
        description="Ensure po files are using the standard gettext format",
    )
    parser.add_argument(
        "--modified", "-m", action="store_true", help="Use git to find modified files."
    )
    parser.add_argument(
        "--no-wrap",
        action="store_true",
        help="see `man msgcat`, usefull to sed right after.",
    )
    parser.add_argument("po_files", nargs="*", help="po files.")
    args = parser.parse_args()
    if not args.po_files and not args.modified:
        parser.print_help()
        exit(1)
    fix_style(args.po_files, args.modified, args.no_wrap)
