import yaml
import re
import os

def parse_markdown_table(md_content):
    """
    模拟从 Markdown 中提取表格数据。
    在实际生产中，这里可以接入 LLM API 对非标准表格进行语义清洗。
    """
    lines = md_content.strip().split('\n')
    table_data = []
    # 简单的正则匹配 Markdown 表格行
    for line in lines:
        if '|' in line and '---' not in line and '寄存器' not in line:
            cols = [c.strip() for c in line.split('|') if c.strip()]
            if len(cols) >= 4:
                table_data.append({
                    'addr': cols[0],
                    'name': cols[1],
                    'type': cols[2],
                    'scale': float(cols[3]) if cols[3] != '-' else 1.0,
                    'unit': cols[4] if len(cols) > 4 else '-'
                })
    return table_data

def generate_edgec_contract(product_name, table_data):
    """
    生成 EdgeC 格式的 tags 和 bindings
    """
    tags = []
    bindings = {
        'protocol': 'MODBUS_RTU',
        'mappings': []
    }

    # 起始 ID，实际应从全局库分配
    start_id = 10000 

    for i, row in enumerate(table_data):
        tag_id = start_id + i
        # 1. 生成 Tag 定义
        tag_name = f"{product_name.upper()}_{i:03d}" # 临时名称，实际由 LLM 翻译为英文语义
        tags.append({
            'id': tag_id,
            'name': tag_name,
            'description': row['name'],
            'type': 'float', # 边缘侧统一转为 float 处理
            'unit': row['unit']
        })

        # 2. 生成 Modbus 绑定
        addr_int = int(row['addr'], 16) if '0x' in row['addr'] else int(row['addr'])
        bindings['mappings'].append({
            'tag': tag_name,
            'addr': addr_int,
            'scaler': row['scale'],
            'length': 2 if 'uint32' in row['type'] else 1
        })

    return tags, bindings

if __name__ == "__main__":
    # 模拟输入数据
    mock_md = """
| 0x0100 | A相电压 | uint16 | 0.1 | V | 精度 0.1V |
| 0x0102 | B相电压 | uint16 | 0.1 | V | |
| 0x0104 | A相电流 | uint32 | 0.001 | A | 精度 1mA |
| 0x0200 | 运行状态 | uint16 | 1 | - | 位拨码表示 |
    """
    
    product = "SmartMeter_X"
    print(f"--- Processing PDF extraction for {product} ---")
    
    table = parse_markdown_table(mock_md)
    tags, bindings = generate_edgec_contract(product, table)
    
    # 输出结果
    print("\n[Generated Tags Segment]")
    print(yaml.dump(tags, allow_unicode=True))
    
    print("\n[Generated Modbus Bindings]")
    print(yaml.dump(bindings, allow_unicode=True))
