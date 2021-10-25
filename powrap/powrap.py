"""Fix style of uncommited po files, or all if --all is given."""

import argparse
import sys
import os
from typing import Iterable
import difflib
from pathlib import Path
from subprocess import check_output, run, CalledProcessError
from tempfile import NamedTemporaryFile

from tqdm import tqdm

from powrap import __version__


def check_style(po_files: Iterable[str], no_wrap=False, quiet=False, diff=False) -> int:
    """Check style of given po_files.

    Prints errors on stderr and returns the number of errors found.
    """
    errors = 0
    for po_path in tqdm(po_files, desc="Checking wrapping of po files", disable=quiet):
        try:
            with open(po_path, encoding="UTF-8") as po_file:
                po_content = po_file.read()
        except OSError as open_error:
            tqdm.write(f"Error opening '{po_path}': {open_error}")
            continue
        with NamedTemporaryFile("w+") as tmpfile:
            args = ["msgcat", "-", "-o", tmpfile.name]
            if no_wrap:
                args[1:1] = ["--no-wrap"]
            try:
                run(args, encoding="utf-8", check=True, input=po_content)
            except CalledProcessError as run_error:
                tqdm.write(f"Error processing '{po_path}': {run_error}")
                continue
            except FileNotFoundError as run_error:
                tqdm.write("Error running " + " ".join(args) + f": {run_error}")
                sys.exit(127)
            new_po_content = tmpfile.read()
            if po_content != new_po_content:
                errors += 1
                print("Would rewrap:", po_path, file=sys.stderr)
                if diff:
                    for line in difflib.unified_diff(
                        po_content.splitlines(keepends=True),
                        new_po_content.splitlines(keepends=True),
                    ):
                        print(line, end="", file=sys.stderr)
    return errors


def fix_style(po_files, no_wrap=False, quiet=False):
    """Fix style of given po_files."""
    for po_path in tqdm(po_files, desc="Fixing wrapping of po files", disable=quiet):
        with open(po_path, encoding="UTF-8") as po_file:
            po_content = po_file.read()
        args = ["msgcat", "-", "-o", po_path]
        if no_wrap:
            args[1:1] = ["--no-wrap"]
        try:
            run(args, encoding="utf-8", check=True, input=po_content)
        except CalledProcessError as run_error:
            tqdm.write(f"Error processing '{po_path}': {run_error}")
        except FileNotFoundError as run_error:
            tqdm.write("Error running " + " ".join(args) + f": {run_error}")
            sys.exit(127)


def parse_args():
    """Parse powrap command line arguments."""

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
        except PermissionError as read_error:
            raise argparse.ArgumentTypeError(
                "{!r}: Permission denied.".format(path_str)
            ) from read_error
        return path_obj

    parser = argparse.ArgumentParser(
        prog="powrap",
        description="Ensure po files are using the standard gettext format",
        epilog="""exit code:
    0:nothing to do
    1:would rewrap
  127:error running msgcat""",
    )
    parser.add_argument(
        "--modified",
        "-m",
        action="store_true",
        help="Use git to find modified files instead of passing them as arguments.",
    )
    parser.add_argument(
        "-C",
        help="To use with --modified to tell where the git "
        "repo is, in case it's not in the current working directory.",
        type=Path,
        dest="git_root",
        default=Path.cwd(),
    )
    parser.add_argument(
        "--quiet", "-q", action="store_true", help="Do not show progress bar."
    )
    parser.add_argument(
        "--diff",
        "-d",
        action="store_true",
        help="Don't write the files back, just output a diff for each file on stdout "
        "(implies --check).",
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
        sys.exit(1)
    if args.po_files and args.modified:
        parser.print_help()
        sys.exit(1)
    return args


def main():
    """Powrap main entrypoint (parsing command line and all)."""
    args = parse_args()
    if args.git_root:
        os.chdir(args.git_root)
    if args.modified:
        git_status = check_output(
            ["git", "status", "--porcelain", "--no-renames"],
            encoding="utf-8",
        )
        git_status_lines = [
            line.split(maxsplit=2) for line in git_status.split("\n") if line
        ]
        args.po_files.extend(
            Path(filename)
            for status, filename in git_status_lines
            if filename.endswith(".po") and status != "D"
        )
    if not args.po_files:
        print("Nothing to do, exiting.")
        sys.exit(0)
    if args.check or args.diff:
        errors = check_style(args.po_files, args.no_wrap, args.quiet, args.diff)
        sys.exit(errors > 0)
    else:
        fix_style(args.po_files, args.no_wrap, args.quiet)
