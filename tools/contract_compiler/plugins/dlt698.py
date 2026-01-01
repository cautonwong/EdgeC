from ..core.interface import ProtocolGenerator

class Dlt698Generator(ProtocolGenerator):
    @property
    def protocol_name(self): return "DLT698"
    def get_c_struct_def(self): return "typedef struct { const char* oad; edge_tag_id_t id; float scale; } dlt698_map_t;"
    def prepare_master_data(self, b): return {"entries": b, "count": len(b)}
    def prepare_slave_data(self, b): return {"oads": b}
