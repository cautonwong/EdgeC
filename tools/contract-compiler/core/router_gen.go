package core

import (
	"contract-compiler/model"
)

type RouterInfo struct {
	TriggerMask []uint32
	WatcherMask []uint32
	MaxTagID    uint32
}

func GenerateRouterInfo(contract *model.EffectiveContract) RouterInfo {
	var maxID uint32 = 0
	for _, t := range contract.ActiveTags {
		if t.ID > maxID {
			maxID = t.ID
		}
	}

	// 位图大小 (以 32 位为单位)
	size := (maxID / 32) + 1
	info := RouterInfo{
		TriggerMask: make([]uint32, size),
		WatcherMask: make([]uint32, size),
		MaxTagID:    maxID,
	}

	// 辅助函数：根据名称找 ID
	findID := func(name string) uint32 {
		for _, t := range contract.ActiveTags {
			if t.Name == name {
				return t.ID
			}
		}
		return 0
	}

	setBit := func(mask []uint32, id uint32) {
		if id == 0 {
			return
		}
		mask[id/32] |= (1 << (id % 32))
	}

	// 1. 处理 Pipelines
	for _, p := range contract.Pipelines {
		// Trigger 是显式定义的
		tid := findID(p.Trigger)
		setBit(info.TriggerMask, tid)

		// 检查 Stages 中是否有被动引用的测点 (Watcher)
		for _, s := range p.Stages {
			for _, v := range s {
				if str, ok := v.(string); ok {
					if tid := findID(str); tid != 0 {
						setBit(info.WatcherMask, tid)
					}
				}
				if list, ok := v.([]interface{}); ok {
					for _, item := range list {
						if str, ok := item.(string); ok {
							if tid := findID(str); tid != 0 {
								setBit(info.WatcherMask, tid)
							}
						}
					}
				}
			}
		}
	}

	return info
}