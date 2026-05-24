import re

with open('docs/698_parsed.md', 'r', encoding='utf-8') as f:
    lines = f.readlines()

categories = {
    '0': ('0000-0FFF', '电能与最大需量类 (Energy & Demand)'),
    '1': ('1000-1FFF', '不常见电量类或扩展变量'),
    '2': ('2000-2FFF', '实时变量与瞬时值类 (Variables/Instantaneous: 电压/电流/功率)'),
    '3': ('3000-3FFF', '事件记录类 (Event Records: 停电/错相/异常等)'),
    '4': ('4000-4FFF', '参数设置类 (Parameters: 变比/限值/费率/档案)'),
    '5': ('5000-5FFF', '冻结与负荷类 (Freeze/Load Profile: 日冻结/月冻结/曲线)'),
    '6': ('6000-6FFF', '控制与显示类 (Control & Display)'),
    '8': ('8000-8FFF', '执行操作与特殊类 (Action/Methods)'),
}

summary = {k: [] for k in categories.keys()}
other = []

# Pattern to capture standard OIs (4 hex digits) at the beginning of a line
pattern = re.compile(r'^([0-9A-F]{4})\s+(\d+)\s+([^\s]+.*?)\s+(数据类型：)?([a-zA-Z\-]+)')

for line in lines:
    m = pattern.search(line.strip())
    if m:
        oi = m.group(1)
        desc = m.group(3).strip()
        dtype = m.group(5)
        prefix = oi[0]
        entry = f"- OI: {oi}, 描述: {desc}, 数据类型: {dtype}"
        if prefix in summary:
            if len(summary[prefix]) < 20:  # just sample up to 20 for notes
                summary[prefix].append(entry)
        else:
            if len(other) < 10:
                other.append(entry)

with open('docs/698_OAD_STUDY_NOTES.md', 'w', encoding='utf-8') as f:
    f.write("# DL/T 698.45 OAD 深度研读笔记\n\n")
    f.write("通过对 `698_parsed.md` 的深度解析，我梳理了协议中各大数据点域的核心业务作用：\n\n")
    
    for prefix, (range_str, name) in categories.items():
        f.write(f"## {name} (OI Range: {range_str})\n")
        if summary[prefix]:
            for entry in summary[prefix]:
                f.write(f"{entry}\n")
            f.write("... (还有更多同类点位)\n")
        else:
            f.write("无标准解析项或格式不同。\n")
        f.write("\n")
