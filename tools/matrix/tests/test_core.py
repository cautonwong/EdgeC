import pytest
import json
import os
from tools.matrix.engine.loader import SchemaLoader
from tools.matrix.engine.model import VBus, BaseVDevice

# --- Fixtures ---

@pytest.fixture
def sample_manifest(tmp_path):
    manifest_data = {
        "product": "TestProd",
        "tags": [
            {"id": 1, "name": "TestTag1", "type": "float", "bindings": [
                {"protocol": "MODBUS", "attributes": {"addr": 10, "scale": 0.1}}
            ]},
            {"id": 2, "name": "TestTag2", "type": "int", "bindings": []}
        ],
        "topology": [
            {
                "bus_id": 0,
                "transport": {"type": "uds", "path": "/tmp/test.sock"},
                "devices": [{"protocol": "ModbusRTU", "count": 1, "start_address": 1}]
            }
        ]
    }
    
    f = tmp_path / "omnis_manifest.json"
    f.write_text(json.dumps(manifest_data))
    return str(f)

# --- Loader Tests ---

def test_loader_parses_manifest(sample_manifest):
    loader = SchemaLoader(sample_manifest)
    
    # Check tags
    assert len(loader.tags) == 2
    assert loader.name_to_id["TestTag1"] == 1
    
    # Check Modbus mappings extraction
    assert 1 in loader.modbus_map
    binding = loader.modbus_map[1]
    assert binding["attributes"]["addr"] == 10
    
    # Check Topology
    assert "topology" in loader.world_config
    topo = loader.world_config["topology"]
    assert len(topo) == 1
    assert topo[0]["bus_id"] == 0
    assert topo[0]["devices"][0]["protocol"] == "ModbusRTU"

# --- Orchestrator/Model Tests ---

class MockTransport:
    def __init__(self, path=None):
        self.path = path
        self.on_received = None
    async def start(self): pass

def test_vbus_device_attachment():
    bus = VBus(0, MockTransport())
    dev = BaseVDevice("Dev1", 1)
    
    bus.attach_device(dev)
    assert dev.name in [d.name for d in bus.devices] # Fixed: devices is a list, not dict
    assert bus.devices[0] == dev

def test_device_injection(sample_manifest):
    loader = SchemaLoader(sample_manifest)
    dev = BaseVDevice("Dev1", 1)
    
    # Inject value using Tag Name "TestTag1" which resolves to ID 1
    # BaseVDevice.inject writes to self.datastore keyed by TagID
    dev.inject("TestTag1", 123.45, loader)
    
    # Verify internal datastore
    assert dev.datastore.get(1) == 123.45

# --- Integration Test: Orchestrator Topology Build ---

from tools.matrix.engine.orchestrator import MatrixOrchestrator

def test_orchestrator_builds_topology(sample_manifest):
    orch = MatrixOrchestrator(sample_manifest)
    
    with pytest.MonkeyPatch.context() as m:
        # Return a proper Mock object, not a string
        m.setattr("tools.matrix.engine.orchestrator.UDSTransport", lambda path: MockTransport(path))
        
        if not orch.loader.world_config:
            orch.loader.world_config = {"topology": []}
            
        orch.build_from_world()
        
        assert 0 in orch.buses
        bus0 = orch.buses[0]
        assert len(bus0.devices) == 1
        dev_name = "Bus0_ModbusRTU_1"
        assert dev_name in orch.devices
        
        device = orch.devices[dev_name]
        assert device.address == 1
        
        from tools.matrix.actors.modbus import ModbusRTUProtocol
        assert isinstance(device.protocol, ModbusRTUProtocol)
