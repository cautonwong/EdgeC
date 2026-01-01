package protocols

import (
	"contract-compiler/model"
	"strings"
)

// 协议处理器接口
type ProtocolHandler interface {
	Name() string
	Resolve(t model.TagDef, b model.Binding) ResolvedBinding
}

var registry = make(map[string]ProtocolHandler)

func Register(h ProtocolHandler) {
	registry[strings.ToUpper(h.Name())] = h
}

func GetHandler(name string) ProtocolHandler {
	return registry[strings.ToUpper(name)]
}
