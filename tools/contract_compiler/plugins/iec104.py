from ..core.interface import ProtocolGenerator

class Iec104Generator(ProtocolGenerator):
    @property
    def protocol_name(self): return "IEC104"
    def get_c_struct_def(self): return ""
    def prepare_master_data(self, b): return {"entries": sorted(b, key=lambda x: x['ioa']), "count": len(b)}
    def prepare_slave_data(self, b): return {"points": b}