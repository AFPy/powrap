#!/usr/bin/env python3

"""Fix style of uncommited po files, or all if --all is given.

The "tac|tac" trick is an equivalent to "sponge" but as tac is in
coreutils we're avoiding you to install a new packet.  Without
`tac|tac` or `sponge`, `msgcat` may start to write in the po file
before having finised to read it, yielding unpredictible behavior.
(Yep we also can write to another file and mv it, like sed -i does.)
"""

from shlex import quote
from subprocess import check_output

from tqdm import tqdm


def fix_style(po_files, modified=False, no_wrap=False):
    """Fix style of unversionned ``.po`` files, or or all f
    """
    if modified:
        git_status = check_output(["git", "status", "--porcelain"],
                                  universal_newlines=True)
        git_status_lines = [line.split(maxsplit=2) for line in
                            git_status.split('\n')
                            if line]
        po_files.extend(filename for status, filename in git_status_lines
                        if filename.endswith('.po'))
    for po_file in tqdm(po_files, desc="Fixing indentation in po files"):
        check_output('tac {} | tac | msgcat - -o {} {}'.format(
            quote(po_file), quote(po_file), '--no-wrap' if no_wrap else ''),
                     shell=True)


def main():
    import argparse
    parser = argparse.ArgumentParser(
        description='Ensure po files are using the standard gettext format')
    parser.add_argument('--modified', '-m', action='store_true',
                        help='Use git to find modified files.')
    parser.add_argument('--no-wrap', action='store_true',
                        help='see `man msgcat`, usefull to sed right after.')
    parser.add_argument('po_files', nargs='*', help='po files.')
    args = parser.parse_args()
    if not args.po_files and not args.modified:
        parser.print_help()
        exit(1)
    fix_style(args.po_files, args.modified, args.no_wrap)
