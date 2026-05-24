import re

with open('docs/698_parsed.md', 'r', encoding='utf-8') as f:
    lines = f.readlines()

classes = []
pattern = re.compile(r'接口类[（\(]class_id\s*=\s*(\d+)[）\)]')

for i, line in enumerate(lines):
    m = pattern.search(line)
    if m:
        class_id = m.group(1)
        # try to get the class name from the current line or previous line
        name = line.replace(f"（class_id={class_id}）", "").replace(f"(class_id={class_id})", "").strip()
        name = re.sub(r'^[0-9\.]+\s+', '', name)
        
        # Avoid exact duplicates
        if not any(c['id'] == class_id for c in classes):
            classes.append({'id': class_id, 'name': name, 'line': i})

print(f"Found {len(classes)} classes")
for c in sorted(classes, key=lambda x: int(x['id'])):
    print(f"Class {c['id']}: {c['name']}")
