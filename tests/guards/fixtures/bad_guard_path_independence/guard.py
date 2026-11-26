import os


def scan(target_dir):
    for root, dirs, files in os.walk(target_dir):
        if "tests" in root and "guards" not in root:
            continue
        for name in files:
            yield os.path.join(root, name)
