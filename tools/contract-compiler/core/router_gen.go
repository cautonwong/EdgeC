package core

import (
	"contract-compiler/model"
	"fmt"
)

type RouterInfo struct {
	TriggerMask []uint32
	WatcherMask []uint32
	MaxActiveIndex int
}

func GenerateRouterInfo(contract *model.EffectiveContract) RouterInfo {
	// 基于 ActiveTags 的数量（稠密索引）来生成位图
	count := len(contract.ActiveTags)
	if count == 0 {
		return RouterInfo{TriggerMask: []uint32{0}, WatcherMask: []uint32{0}}
	}

	size := (count / 32) + 1
	info := RouterInfo{
		TriggerMask: make([]uint32, size),
		WatcherMask: make([]uint32, size),
		MaxActiveIndex: count - 1,
	}

	// 建立 TagName -> CompactIndex (0..N-1) 的映射
	nameToIndex := make(map[string]int)
	for i, t := range contract.ActiveTags {
		nameToIndex[t.Name] = i
	}

	setBit := func(mask []uint32, idx int) {
		if idx < 0 { return }
		mask[idx/32] |= (1 << (uint32(idx) % 32))
	}

	// 处理 Pipelines
	for _, p := range contract.Pipelines {
		if idx, ok := nameToIndex[p.Trigger]; ok {
			setBit(info.TriggerMask, idx)
		} else if p.Trigger != "" {
			// 兜底：尝试按 ID 匹配 (如果 Trigger 存的是 ID 字符串)
			for i, t := range contract.ActiveTags {
				if fmt.Sprintf("%d", t.ID) == p.Trigger {
					setBit(info.TriggerMask, i)
					break
				}
			}
		}

		// [补强] 处理多触发源 (triggers 列表)
		for _, trig := range p.Triggers {
			trigName, _ := trig["tag"].(string)
			if trigName == "" {
				trigName, _ = trig["event"].(string)
			}
			if idx, ok := nameToIndex[trigName]; ok {
				setBit(info.TriggerMask, idx)
			}
		}

		for _, s := range p.Stages {
			for k, v := range s {
				// 识别 Watcher 测点 (这里可以根据业务需求扩展识别逻辑)
				if k == "target_tag" { continue } // 排除输出点
				if str, ok := v.(string); ok {
					if idx, ok := nameToIndex[str]; ok {
						setBit(info.WatcherMask, idx)
					}
				}
			}
		}
	}

	return info
}