from ..core.interface import ProtocolGenerator

class DlmsGenerator(ProtocolGenerator):
    @property
    def protocol_name(self): return "DLMS"
    def get_c_struct_def(self): return ""
    def prepare_master_data(self, b): return {"entries": b, "count": len(b)}
    def prepare_slave_data(self, b): return {"objects": b}