package core

import (
	"contract-compiler/model"
)

type RouterInfo struct {
	TriggerMask []uint32
	WatcherMask []uint32
	ActiveCount uint32
}

func GenerateRouterInfo(contract *model.EffectiveContract) RouterInfo {
	activeCount := uint32(len(contract.ActiveTags))

	// 位图大小 (以 32 位为单位)
	size := (activeCount / 32) + 1
	info := RouterInfo{
		TriggerMask: make([]uint32, size),
		WatcherMask: make([]uint32, size),
		ActiveCount: activeCount,
	}

	// 辅助函数：根据名称找 Index
	findIdx := func(name string) int {
		for i, t := range contract.ActiveTags {
			if t.Name == name {
				return i
			}
		}
		return -1
	}

	setBit := func(mask []uint32, idx int) {
		if idx < 0 {
			return
		}
		mask[idx/32] |= (1 << (uint32(idx) % 32))
	}

	// 1. 处理 Pipelines
	for _, p := range contract.Pipelines {
		// Trigger 是显式定义的
		idx := findIdx(p.Trigger)
		setBit(info.TriggerMask, idx)

		// 检查 Stages 中是否有被动引用的测点 (Watcher)
		for _, s := range p.Stages {
			for _, v := range s {
				if str, ok := v.(string); ok {
					if idx := findIdx(str); idx != -1 {
						setBit(info.WatcherMask, idx)
					}
				}
				if list, ok := v.([]interface{}); ok {
					for _, item := range list {
						if str, ok := item.(string); ok {
							if idx := findIdx(str); idx != -1 {
								setBit(info.WatcherMask, idx)
							}
						}
					}
				}
			}
		}
	}

	return info
}
