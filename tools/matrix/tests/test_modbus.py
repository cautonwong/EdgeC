import pytest
import asyncio
from tools.matrix.actors.modbus import ModbusRTUProtocol
from tools.matrix.engine.model import DataStore

# Modbus PDU Constants
FUNC_READ_HOLDING = 0x03
FUNC_WRITE_SINGLE = 0x06

class MockDataStore(DataStore):
    pass # Use real DataStore logic

@pytest.fixture
def modbus_actor():
    # Setup: Create a protocol handler for Address 1
    # Bind TagID 100 -> Register 0 (Scale 1)
    # Bind TagID 101 -> Register 1 (Scale 0.1)
    bindings = {
        100: {"attributes": {"addr": 0, "scaler": 1}},
        101: {"attributes": {"addr": 1, "scaler": 0.1}}
    }
    return ModbusRTUProtocol(slave_id=1, bindings=bindings)

@pytest.mark.asyncio
async def test_modbus_read_holding(modbus_actor):
    store = MockDataStore()
    store.set(100, 123)  # Reg 0 = 123
    store.set(101, 45.6) # Reg 1 = 456 (45.6 / 0.1)

    # Request: Addr=1, Func=3, Start=0, Count=2
    # CRC is checked by pymodbus? The actor seems to use pymodbus.ServerDecoder?
    # Let's check source code of ModbusRTUProtocol.
    # Assuming it handles raw bytes. If it relies on pymodbus internal frames, we need to send full RTU frame.
    
    # RTU Frame: 01 03 00 00 00 02 C4 0B
    req = b'\x01\x03\x00\x00\x00\x02\xC4\x0B'
    
    resp = await modbus_actor.process(req, store)
    
    # Expected: 01 03 04 00 7B 01 C8 [CRC_L] [CRC_H]
    # 00 7B = 123
    # 01 C8 = 456
    
    assert resp is not None
    assert resp[0] == 1 # Slave ID
    assert resp[1] == 3 # Func Code
    assert resp[2] == 4 # Byte Count
    # Reg 0
    val0 = (resp[3] << 8) | resp[4]
    assert val0 == 123
    # Reg 1
    val1 = (resp[5] << 8) | resp[6]
    assert val1 == 456

@pytest.mark.asyncio
async def test_modbus_write_single(modbus_actor):
    store = MockDataStore()
    
    # Request: Write Reg 0 -> 555
    # 01 06 00 00 02 2B [CRC]
    # 0x022B = 555
    # CRC for 01 06 00 00 02 2B is 09 62 (Little Endian?) -> Let's ignore CRC check on input if actor allows, 
    # but ModbusRTUProtocol likely checks CRC.
    # Let's generate valid CRC or use a tool.
    
    # Simple CRC16 for [01 06 00 00 02 2B]
    # 01 06 00 00 02 2B -> CRC=0x398B (Low first? Modbus is Low Byte First) -> 8B 39
    req = b'\x01\x06\x00\x00\x02\x2B\x8B\x39'
    
    resp = await modbus_actor.process(req, store)
    
    assert resp is not None
    assert resp[1] == 0x06
    
    # Verify DataStore updated
    # Tag 100 bound to Reg 0 with Scale 1
    assert store.get(100) == 555

@pytest.mark.asyncio
async def test_modbus_ignore_other_slave(modbus_actor):
    store = MockDataStore()
    # Request for Slave 2
    req = b'\x02\x03\x00\x00\x00\x01\x00\x00' # CRC invalid but addr mismatch first
    resp = await modbus_actor.process(req, store)
    assert resp is None
