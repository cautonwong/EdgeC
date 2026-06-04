import yaml

manifest = {
    "connectivity": [],
    "tags": [
        {"id": 0x20000200, "attributes": {"poll_interval_ms": 1000, "persist": True}},
        {"id": 0x20010200, "attributes": {"poll_interval_ms": 1000, "persist": True}}
    ]
}

for i in range(1, 33):
    manifest["connectivity"].append({
        "id": i,
        "label": f"RS485_BUS_{i:02d}",
        "category": "SOUTHBOUND",
        "type": "UART",
        "config": {"path": f"/tmp/rs485_{i}.sock", "baud": 9600},
        "simulation": {"transport": "UDS", "endpoint": f"UDS:/tmp/rs485_{i}.sock"}
    })

with open("/workspaces/edgeos/EdgeC/cmd/dcu/manifest.yaml", "w") as f:
    yaml.dump(manifest, f)
print("✅ [MANIFEST] 32-Channel deployment manifest generated.")
