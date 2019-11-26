#!/usr/bin/env python3

"""Fix style of uncommited po files, or all if --all is given.
"""

import sys
from shlex import quote
from pathlib import Path
from subprocess import check_output, run
from tempfile import NamedTemporaryFile

from tqdm import tqdm

from powrap import __version__


def check_style(po_files, no_wrap=False, quiet=False):
    """Check style of given po_files
    """
    to_fix = []
    for po_path in tqdm(po_files, desc="Checking wrapping of po files", disable=quiet):
        with open(po_path, encoding="UTF-8") as po_file:
            po_content = po_file.read()
        with NamedTemporaryFile("w+") as tmpfile:
            args = ["msgcat", "-", "-o", tmpfile.name]
            if no_wrap:
                args[1:1] = ["--no-wrap"]
            run(args, encoding="utf-8", check=True, input=po_content)
            new_po_content = tmpfile.read()
            if po_content != new_po_content:
                to_fix.append(po_path)
    return to_fix


def fix_style(po_files, no_wrap=False, quiet=False):
    """Fix style of given po_files.
    """
    fixed = []
    for po_path in tqdm(po_files, desc="Fixing wrapping of po files", disable=quiet):
        with open(po_path, encoding="UTF-8") as po_file:
            po_content = po_file.read()
        args = ["msgcat", "-", "-o", po_path]
        if no_wrap:
            args[1:1] = ["--no-wrap"]
        run(args, encoding="utf-8", check=True, input=po_content)
        with open(po_path, encoding="UTF-8") as po_file:
            new_po_content = po_file.read()
        if po_content != new_po_content:
            fixed.append(po_path)
    return fixed


def parse_args():
    import argparse

    def path(path_str):
        path_obj = Path(path_str)
        if not path_obj.exists():
            raise argparse.ArgumentTypeError(
                "File {!r} does not exists.".format(path_str)
            )
        if not path_obj.is_file():
            raise argparse.ArgumentTypeError("{!r} is not a file.".format(path_str))
        try:
            path_obj.read_text()
        except PermissionError:
            raise argparse.ArgumentTypeError(
                "{!r}: Permission denied.".format(path_str)
            )
        return path_obj

    parser = argparse.ArgumentParser(
        prog="powrap",
        description="Ensure po files are using the standard gettext format",
    )
    parser.add_argument(
        "--modified", "-m", action="store_true", help="Use git to find modified files."
    )
    parser.add_argument(
        "--quiet", "-q", action="store_true", help="Do not show progress bar."
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Don't write the files back, just return the status. "
        "Return code 0 means nothing would change.  "
        "Return code 1 means some files would be reformatted.",
    )

    parser.add_argument(
        "--version", action="version", version="%(prog)s " + __version__
    )
    parser.add_argument(
        "--no-wrap",
        action="store_true",
        help="see `man msgcat`, usefull to sed right after.",
    )
    parser.add_argument("po_files", nargs="*", help="po files.", type=path)
    args = parser.parse_args()
    if not args.po_files and not args.modified:
        parser.print_help()
        exit(1)
    return args


def main():
    args = parse_args()
    if args.modified:
        git_status = check_output(["git", "status", "--porcelain"], encoding="utf-8")
        git_status_lines = [
            line.split(maxsplit=2) for line in git_status.split("\n") if line
        ]
        args.po_files.extend(
            Path(filename)
            for status, filename in git_status_lines
            if filename.endswith(".po")
        )
    if not args.po_files:
        print("Nothing to do, exiting.")
        sys.exit(0)
    if args.check:
        to_fix = check_style(args.po_files, args.no_wrap, args.quiet)
        if to_fix:
            print("Would rewrap:", *to_fix, sep="\n- ")
        sys.exit(1 if to_fix else 0)
    else:
        fixed = fix_style(args.po_files, args.no_wrap, args.quiet)
        sys.exit(1 if fixed else 0)
