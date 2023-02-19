"""Fix style of uncommitted po files, or all if --all is given."""

import argparse
import sys
import os
import difflib
from pathlib import Path
from subprocess import check_output

from polib import pofile
from tqdm import tqdm

from powrap import __version__


def process(args: argparse.Namespace) -> int:
    """Process the files. Returns an exit code."""
    exit_with_error = False
    exit_with_modified_files = False
    if args.check:
        desc = "Checking wrapping of po files"
    else:
        desc = "Fixing wrapping of po files"
    for po_path in tqdm(args.po_files, desc=desc, disable=args.quiet):
        try:
            catalog = pofile(str(po_path), wrapwidth=args.width)
        except OSError as error:
            tqdm.write(f"Error reading PO file '{po_path}': {error}", file=sys.stderr)
            exit_with_error = True
            continue
        new_contents = str(catalog)
        if args.check:
            # Use the catalog's specified encoding detected by polib
            old_contents = Path(po_path).read_text(encoding=catalog.encoding)
            if new_contents != old_contents:
                exit_with_modified_files = True
                tqdm.write(f"Would rewrap: {po_path}", file=sys.stderr)
                if args.diff:
                    for line in difflib.unified_diff(
                        old_contents.splitlines(keepends=True),
                        new_contents.splitlines(keepends=True),
                    ):
                        tqdm.write(line, end="", file=sys.stderr)
        else:
            try:
                po_path.write_text(new_contents, encoding=catalog.encoding)
            except OSError as error:
                tqdm.write(f"Error writing '{po_path}': {error}")
                exit_with_error = True
    if exit_with_error:
        return 127
    if exit_with_modified_files:
        return 1
    return 0


def parse_args(raw_args):
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
  127:encountered error
  """,
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
        "--width",
        type=int,
        default=80,  # TODO check
        help="Line width",
    )
    parser.add_argument(
        "--no-wrap",
        help="remove any wrapping, no matter the line length; same as --width=0",
        action="store_const",
        dest="width",
        const=0,
    )
    parser.add_argument("po_files", nargs="*", help="po files.", type=path)
    args = parser.parse_args(raw_args)
    if not args.po_files and not args.modified:
        parser.print_help()
        sys.exit(1)
    if args.po_files and args.modified:
        parser.print_help()
        sys.exit(1)
    if args.diff:
        args.check = True
    return args


def main(raw_args=sys.argv[1:]) -> int:
    """Powrap main entrypoint (parsing command line and all). Returns an exit code."""
    args = parse_args(raw_args)
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
        return 0
    return process(args)
