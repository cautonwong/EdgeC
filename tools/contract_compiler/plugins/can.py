from ..core.interface import ProtocolGenerator

class CanGenerator(ProtocolGenerator):
    @property
    def protocol_name(self): return "CAN"

    def get_c_struct_def(self):
        return "typedef struct { uint32_t can_id; edge_tag_id_t id; int start; int len; float factor; float offset; } can_map_t;"

    def prepare_master_data(self, bindings):
        return {"entries": bindings, "count": len(bindings)}

    def prepare_slave_data(self, bindings):
        return {"frames": bindings}
