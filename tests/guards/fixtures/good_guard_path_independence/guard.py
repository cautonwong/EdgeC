import os

from c_source_scan import skip_subtree


def scan(target_dir):
    for root, dirs, files in os.walk(target_dir):
        if skip_subtree(root, target_dir):
            continue
        for name in files:
            yield os.path.join(root, name)
