from ..core.interface import ProtocolGenerator

class ModbusGenerator(ProtocolGenerator):
    @property
    def protocol_name(self): return "MODBUS"
    def get_c_struct_def(self): return ""
    def prepare_master_data(self, b): return {"entries": sorted(b, key=lambda x: x['addr']), "count": len(b)}
    def prepare_slave_data(self, b): return {"registers": sorted(b, key=lambda x: x['addr'])}