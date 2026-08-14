import re
import os

def create_dlt698():
    content = """// DLT698 Objects IDL
module DLT698 {
    struct TagData {
        @key string oad;
        string description;
    };
    struct ClassData {
        @key unsigned short class_id;
        string name;
    };
};
"""
    with open('/workspaces/edgeos/EdgeC/contracts/idl/dlt698_objects.idl', 'w') as f:
        f.write(content)

def create_dlms():
    content = """// DLMS Blue Book Objects IDL
module DLMS {
    struct CosemAttributeData {
        @key string obis_code;
        @key unsigned short class_id;
        string description;
    };
};
"""
    with open('/workspaces/edgeos/EdgeC/contracts/idl/dlms_bluebook_objects.idl', 'w') as f:
        f.write(content)

def create_idis():
    content = """// IDIS Pack3 Objects IDL
module IDIS {
    struct ProfileData {
        @key string obis_code;
        string description;
    };
};
"""
    with open('/workspaces/edgeos/EdgeC/contracts/idl/idis_pack3_objects.idl', 'w') as f:
        f.write(content)

os.makedirs('/workspaces/edgeos/EdgeC/contracts/idl/', exist_ok=True)
create_dlt698()
create_dlms()
create_idis()
print("IDL files generated.")
