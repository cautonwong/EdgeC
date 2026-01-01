from ..core.interface import ProtocolGenerator

class Dlt645Generator(ProtocolGenerator):
    @property
    def protocol_name(self): return "DLT645"
    def get_c_struct_def(self): return "typedef struct { const char* di; edge_tag_id_t id; float scale; } dlt645_map_t;"
    def prepare_master_data(self, b): return {"entries": b, "count": len(b)}
    def prepare_slave_data(self, b): return {"dis": b}
