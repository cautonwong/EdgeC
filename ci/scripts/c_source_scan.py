"""Strip C comments while keeping line numbers stable.

A guard that scans raw source text lets a comment satisfy or break a rule:
`argv[3]` in a comment is not a use, and a commented-out literal is not a
violation. Comments become spaces, newlines stay, and string/char literals are
kept so `"http://..."` does not open a comment.
"""

import os


# Harnesses under formal/ use CBMC-only intrinsics and are not product source.
SHARED_PRUNE_DIRS = {"formal"}


def rel_scan_parts(path, target_dir):
    """Path segments of `path` below the scan target; empty for the target itself.

    Guard exemptions key off the tree being scanned, never the checkout path:
    a worktree at /tmp/guards/edgec must scan the same files as one at /tmp/x.
    """
    rel = os.path.relpath(path, target_dir).replace("\\", "/")
    return [] if rel == "." else rel.split("/")


def skip_subtree(root, target_dir):
    """True for the fixture and test subtrees guards must not scan.

    Fixtures are validated by the refutation suite, not as repo code; a target
    that is itself a fixture has no `fixtures` segment below it and is scanned.
    Tests are out of scope except under `guards` (the guard fixtures).
    """
    parts = rel_scan_parts(root, target_dir)
    if "fixtures" in parts:
        return True
    return "tests" in parts and "guards" not in parts


def strip_comments(text):
    out = []
    i = 0
    size = len(text)
    while i < size:
        char = text[i]
        if text.startswith("//", i):
            while i < size and text[i] != "\n":
                out.append(" ")
                i += 1
        elif text.startswith("/*", i):
            out.append("  ")
            i += 2
            while i < size and not text.startswith("*/", i):
                out.append("\n" if text[i] == "\n" else " ")
                i += 1
            if i < size:
                out.append("  ")
                i += 2
        elif char in "\"'":
            quote = char
            out.append(char)
            i += 1
            while i < size:
                out.append(text[i])
                if text[i] == "\\" and i + 1 < size:
                    out.append(text[i + 1])
                    i += 2
                    continue
                i += 1
                if text[i - 1] == quote or text[i - 1] == "\n":
                    break
        else:
            out.append(char)
            i += 1
    return "".join(out)
