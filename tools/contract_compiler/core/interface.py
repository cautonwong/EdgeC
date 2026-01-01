from abc import ABC, abstractmethod

class ProtocolGenerator(ABC):
    @property
    @abstractmethod
    def protocol_name(self):
        """返回协议名，如 'MODBUS'"""
        pass

    @abstractmethod
    def get_c_struct_def(self):
        """返回 C 语言结构体定义"""
        pass

    @abstractmethod
    def prepare_master_data(self, bindings):
        """为网关端准备数据"""
        pass

    @abstractmethod
    def prepare_slave_data(self, bindings):
        """为从机端准备数据"""
        pass
