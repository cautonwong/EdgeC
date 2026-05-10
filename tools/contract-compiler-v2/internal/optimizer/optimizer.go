package optimizer

import (
	"contract-compiler-v2/internal/frontend"
	"contract-compiler-v2/internal/ir"
	"fmt"
	"sort"
	"strconv"
	"strings"
	"hash/crc32"
)

type Optimizer struct{}

func (o *Optimizer) Optimize(ast *frontend.ContractAST) (*ir.ContractIR, error) {
	// 1. Semantic Expansion: Transform Models/Instances into flattened Tags/Devices
	o.expandSemanticModel(ast)

	res := &ir.ContractIR{
		ProductName:   ast.ProductName,
		Models:        []ir.ModelRecord{},
		Instances:     []ir.InstanceRecord{},
		LookupEngines: make(map[string]*ir.PerfectHashEngine),
		ReverseMaps:   make(map[string][]ir.ReverseMapEntry),
		Tags:          []ir.TagRecord{},
		Connectivity:  []ir.ConnectivityRecord{},
		Devices:       []ir.DeviceRecord{},
		Pipelines:     []ir.PipelineRecord{},
		UsedStages:    []string{},
	}

	// 2. Populate Semantic Records in IR
	for _, m := range ast.Models {
		mr := ir.ModelRecord{
			Name: m.Name, Protocol: m.Protocol, Version: m.Version,
			Points: []ir.ModelPointRecord{},
		}
		for _, p := range m.Points {
			mr.Points = append(mr.Points, ir.ModelPointRecord{
				Name: p.Name, Address: p.Address, Type: p.Type, Unit: p.Unit,
				Category: p.Category, Bits: p.Bits, Scale: p.Scale, Attrs: p.Attrs,
			})
		}
		res.Models = append(res.Models, mr)
	}
	for _, i := range ast.Instances {
		ir_inst := ir.InstanceRecord{
			Name: i.Name, ModelName: i.ModelName, BusID: i.BusID, Address: i.Address,
			Bindings: []ir.InstanceTagBinding{},
		}
		for _, b := range i.Bindings {
			ir_inst.Bindings = append(ir_inst.Bindings, ir.InstanceTagBinding{
				PointName: b.Point, TagID: b.TagID,
			})
		}
		res.Instances = append(res.Instances, ir_inst)
	}

	// 3. Existing Legacy Optimization Logic
	sort.Slice(ast.Tags, func(i, j int) bool { return ast.Tags[i].ID < ast.Tags[j].ID })

	nameToTagID := make(map[string]uint32)
	nameToIndex := make(map[string]uint32)
	tagHashEntries := []ir.HashSlot{}
	var sig uint32
	
	for i, t := range ast.Tags {
		category := "general"
		if c, ok := t.Attributes["category"].(string); ok { category = c }
		if t.Category != "" { category = t.Category }
		
		tagScaler := 1.0
		if len(t.Bindings) > 0 {
			tagScaler = float64(o.getScaler(t.Bindings[0].Attributes))
		}

		record := ir.TagRecord{
			ID: t.ID, Name: t.Name, DataType: t.DataType, Unit: t.Unit, Category: category,
			LogicalIndex: uint32(i), Attributes: t.Attributes, Bindings: []ir.BindingRecord{},
		}
		if record.Attributes == nil { record.Attributes = make(map[string]interface{}) }
		
		if _, ok := record.Attributes["scaler"]; !ok {
			record.Attributes["scaler"] = tagScaler
		}

		for _, b := range t.Bindings {
			record.Bindings = append(record.Bindings, ir.BindingRecord{Protocol: b.Protocol, Attributes: b.Attributes})
		}
		res.Tags = append(res.Tags, record)
		nameToTagID[t.Name] = t.ID
		nameToIndex[t.Name] = uint32(i)
		sig = crc32.Update(sig, crc32.IEEETable, []byte(fmt.Sprintf("%d", t.ID)))
		
		tagHashEntries = append(tagHashEntries, ir.HashSlot{Key: uint64(t.ID), TagID: t.ID, LogicalIndex: uint32(i)})
	}
	res.Signature = sig
	res.TagEngine = o.generatePHash("TAG", tagHashEntries)

	count := len(res.Tags)
	size := 0
	if count > 0 { size = (count-1)/32 + 1 }
	res.Router = ir.RouterInfo{
		TriggerMask: make([]uint32, size),
		WatcherMask: make([]uint32, size),
		MaxIndex:    count - 1,
	}

	protoMap := make(map[string][]ir.HashSlot)
	for i, t := range ast.Tags {
		for _, b := range t.Bindings {
			if b.Protocol == "" { continue }
			p := strings.ToUpper(b.Protocol)
			key := o.parseKey(p, b.Attributes)
			scaler := o.getScaler(b.Attributes)
			length := uint16(getUint64(b.Attributes, "length", 2))
			access := uint32(getUint64(b.Attributes, "access", 1))

			slot := ir.HashSlot{
				TagID: t.ID, LogicalIndex: uint32(i), Key: key, 
				Scale: scaler,
				ByteLen: length, Access: access,
			}
			protoMap[p] = append(protoMap[p], slot)
			res.ReverseMaps[p] = append(res.ReverseMaps[p], ir.ReverseMapEntry{
				TagID: t.ID, LogicalIndex: uint32(i), Key: key, Scale: slot.Scale,
				ByteLen: length, Access: access,
			})
		}
	}

	for proto, entries := range protoMap {
		engine := o.generatePHash(proto, entries)
		if engine != nil { res.LookupEngines[proto] = engine }
	}

	usedStageMap := make(map[string]bool)
	for _, p := range ast.Pipelines {
		rec := ir.PipelineRecord{ID: p.ID, TriggerTag: nameToTagID[p.Trigger], Stages: []ir.StageRecord{}}
		if idx, ok := nameToIndex[p.Trigger]; ok { 
			res.Router.TriggerMask[idx/32] |= (1 << (idx % 32)) 
		}

		for j, s := range p.Stages {
			st := ir.StageRecord{ 
				Name: s.Name, Type: s.Type, 
				NextIndices: [4]int{-1, -1, -1, -1},
			}
			usedStageMap[strings.ToLower(s.Type)] = true
			cfg := s.Config
			st.F1 = getFloat64(cfg, "threshold", getFloat64(cfg, "alpha", getFloat64(cfg, "min", 0.0)))
			if gt, ok := cfg["greater_than"].(bool); ok && gt { st.F2 = 1.0 } else { st.F2 = getFloat64(cfg, "beta", getFloat64(cfg, "max", 0.0)) }
			st.F3 = getFloat64(cfg, "action_value", getFloat64(cfg, "action_val", 0.0))
			st.U1 = uint32(getUint64(cfg, "target_tag", getUint64(cfg, "state_tag", getUint64(cfg, "bus_id", 0))))
			st.S1 = getString(cfg, "protocol", getString(cfg, "script", ""))
			
			// Resolve branches to indices
			if len(s.Branches) > 0 {
				for bName, targetName := range s.Branches {
					bIdx := -1
					switch strings.ToLower(bName) {
					case "ok", "next", "true": bIdx = 0
					case "error", "fail", "false": bIdx = 1
					case "branch2": bIdx = 2
					case "branch3": bIdx = 3
					}
					
					if bIdx >= 0 {
						targetIdx := -1
						for k, ts := range p.Stages {
							if ts.Name == targetName {
								targetIdx = k; break
							}
						}
						st.NextIndices[bIdx] = targetIdx
					}
				}
			} else {
				// Default linear flow
				if j + 1 < len(p.Stages) {
					st.NextIndices[0] = j + 1
				}
			}

			rec.Stages = append(rec.Stages, st)

			for _, val := range cfg {
				if sVal, ok := val.(string); ok {
					if idx, ok := nameToIndex[sVal]; ok {
						res.Router.WatcherMask[idx/32] |= (1 << (idx % 32))
					}
				}
			}
		}
		res.Pipelines = append(res.Pipelines, rec)
	}
	for s := range usedStageMap { res.UsedStages = append(res.UsedStages, s) }
	sort.Strings(res.UsedStages)

	for _, p := range ast.Policies {
		rec := ir.PolicyRecord{ Name: p.Name, Type: p.Type, Tags: []uint32{} }
		rec.Interval = uint32(getUint64(p.Attributes, "interval", 0))
		if tags, ok := p.Attributes["tags"].([]interface{}); ok {
			for _, t := range tags { if s, ok := t.(string); ok { rec.Tags = append(rec.Tags, nameToTagID[s]) } }
		}
		res.Policies = append(res.Policies, rec)
	}

	for _, c := range ast.Connectivity {
		simPath := c.SimulationPath
		if strings.HasPrefix(simPath, "UDS:") {
			simPath = strings.TrimPrefix(simPath, "UDS:")
		}
		res.Connectivity = append(res.Connectivity, ir.ConnectivityRecord{
			ID: c.ID, Label: c.Label, Category: c.Category, Type: c.Type,
			SimulationPath: simPath, ProductionPath: c.ProductionPath,
			Simulation:     c.Simulation,
			Config:         c.Config,
		})
	}
	for _, d := range ast.Devices {
		simData := d.Simulation
		if simData == nil { simData = make(map[string]interface{}) }
		
		protoName := d.Protocol
		if strings.EqualFold(protoName, "modbusrtu") { protoName = "ModbusRTU" }
		
		if _, ok := simData["actor"]; !ok { simData["actor"] = strings.ToLower(protoName) }
		if _, ok := simData["id"]; !ok { simData["id"] = d.Address }

		res.Devices = append(res.Devices, ir.DeviceRecord{
			ID: d.ID, BusID: d.BusID, Protocol: protoName, Address: d.Address, Model: d.Model,
			Simulation: simData,
		})
	}

	return res, nil
}

func (o *Optimizer) expandSemanticModel(ast *frontend.ContractAST) {
	modelMap := make(map[string]*frontend.ModelNode)
	for _, m := range ast.Models { modelMap[m.Name] = m }

	for _, inst := range ast.Instances {
		model, ok := modelMap[inst.ModelName]
		if !ok { continue }

		// 1. Synthesize DeviceRecord from Instance
		ast.Devices = append(ast.Devices, &frontend.DeviceNode{
			ID:       inst.Name,
			BusID:    inst.BusID,
			Protocol: model.Protocol,
			Address:  inst.Address,
			Model:    model.Name,
		})

		// 2. Expand Bindings into Tags
		for _, b := range inst.Bindings {
			var point *frontend.PointNode
			for _, p := range model.Points {
				if p.Name == b.Point { point = p; break }
			}
			if point == nil { continue }

			var target *frontend.TagNode
			for _, t := range ast.Tags {
				if t.ID == b.TagID { target = t; break }
			}
			if target == nil {
				target = &frontend.TagNode{
					ID: b.TagID,
					Name: fmt.Sprintf("%s_%s", inst.Name, point.Name),
					DataType: point.Type,
					Unit: point.Unit,
					Attributes: make(map[string]interface{}),
				}
				ast.Tags = append(ast.Tags, target)
			}

			// Semantic Metadata Injection
			if target.Attributes == nil { target.Attributes = make(map[string]interface{}) }
			target.Attributes["category"] = point.Category
			if point.Bits != "" { target.Attributes["bits"] = point.Bits }
			if point.Scale != 0 { target.Attributes["scaler"] = point.Scale }
			for k, v := range point.Attrs { target.Attributes[k] = v }

			// Protocol Binding (Master Plane context)
			attr := map[string]interface{}{
				"addr": point.Address,
				"type": point.Type,
			}
			if point.Bits != "" { attr["bits"] = point.Bits }
			if point.Scale != 0 { attr["scaler"] = point.Scale }
			
			// [Important] Store the device specific address (SlaveID) for multi-instance support
			attr["device_address"] = inst.Address
			fmt.Printf("  [Optimizer] Expanded Binding: Tag %d, DeviceAddr %v, RegAddr %d\n", b.TagID, inst.Address, point.Address)

			target.Bindings = append(target.Bindings, &frontend.BindingNode{
				Protocol: model.Protocol,
				Attributes: attr,
			})
		}
	}
}

func (o *Optimizer) parseKey(proto string, attr map[string]interface{}) uint64 {
	p := strings.ToUpper(proto)
	
	// Composite Key Support: (DeviceAddr << 32) | RegAddr
	deviceAddr := getUint64(attr, "device_address", 0)
	regAddr := getUint64(attr, "addr", getUint64(attr, "address", getUint64(attr, "ioa", 0)))
	
	compositeKey := (deviceAddr << 32) | regAddr

	switch p {
	case "MODBUS", "IEC104", "MODBUSRTU": 
		// If multiple instances exist, use composite key to prevent collisions in lookup engine
		if deviceAddr != 0 { return compositeKey }
		return regAddr
	case "DLT645", "DLT698":
		s := getString(attr, "di", getString(attr, "oad", "0"))
		v, _ := strconv.ParseUint(strings.ReplaceAll(s, "0x", ""), 16, 64)
		if deviceAddr != 0 { return (deviceAddr << 32) | v }
		return v
	case "CAN": return getUint64(attr, "can_id", 0)
	default: return 0
	}
}

func getUint64(m map[string]interface{}, key string, def uint64) uint64 {
	if v, ok := m[key]; ok {
		switch val := v.(type) {
		case int: return uint64(val)
		case int32: return uint64(val)
		case int64: return uint64(val)
		case uint: return uint64(val)
		case uint32: return uint64(val)
		case uint64: return val
		case float64: return uint64(val)
		case string:
			res, _ := strconv.ParseUint(strings.ReplaceAll(val, "0x", ""), 16, 64)
			return res
		}
	}
	return def
}

func getFloat64(m map[string]interface{}, key string, def float64) float64 {
	if v, ok := m[key]; ok {
		switch val := v.(type) {
		case int: return float64(val)
		case int32: return float64(val)
		case int64: return float64(val)
		case uint32: return float64(val)
		case uint64: return float64(val)
		case float64: return val
		case float32: return float64(val)
		case string:
			res, _ := strconv.ParseFloat(val, 64)
			return res
		}
	}
	return def
}

func getString(m map[string]interface{}, key string, def string) string {
	if v, ok := m[key].(string); ok { return v }
	return def
}

func toFloat32(v interface{}) float32 {
	switch val := v.(type) {
	case float64: return float32(val)
	case int: return float32(val)
	case float32: return val
	}
	return 1.0
}

func (o *Optimizer) getScaler(attr map[string]interface{}) float32 {
	if v, ok := attr["scaler"]; ok { return toFloat32(v) }
	if v, ok := attr["scale"]; ok { return toFloat32(v) }
	return 1.0
}

func (o *Optimizer) generatePHash(proto string, entries []ir.HashSlot) *ir.PerfectHashEngine {
	if len(entries) == 0 { return nil }
	
	size := uint32(1)
	for size < uint32(len(entries)*2) { size <<= 1 }
	if size < 4 { size = 4 }

	fmt.Printf("  [Optimizer] Generating PHash for %s with %d entries...\n", proto, len(entries))
	for i, e := range entries {
		fmt.Printf("    Entry %d: Key=0x%016X (Device=%d, Reg=%d), TagID=%d\n", i, e.Key, uint32(e.Key>>32), uint32(e.Key), e.TagID)
	}

	for attempt := 0; attempt < 8; attempt++ {
		for seed := uint32(0); seed < 100000; seed++ {
			occupied := make(map[uint32]bool)
			collision := false
			table := make([]ir.HashSlot, size)
			for _, e := range entries {
				h := o.computeHash(e.Key, seed, size)
				if occupied[h] { 
					collision = true
					break 
				}
				occupied[h] = true
				table[h] = e
			}
			if !collision { 
				fmt.Printf("  [Optimizer] PHash for %s: Size=%d, Seed=%d, Entries=%d\n", proto, size, seed, len(entries))
				return &ir.PerfectHashEngine{ Protocol: proto, Seed: seed, TableSize: size, Slots: table } 
			}
		}
		size <<= 1 
	}
	fmt.Printf("  [Optimizer] FATAL: Failed to generate PHash for %s after 8 attempts\n", proto)
	return nil
}

func (o *Optimizer) computeHash(key uint64, seed uint32, size uint32) uint32 {
	// Mix 64-bit key into 32-bit hash space
	h := uint32(key) ^ uint32(key >> 32) ^ seed
	h ^= h >> 16
	h *= 0x85ebca6b
	h ^= h >> 13
	h *= 0xc2b2ae35
	h ^= h >> 16
	return h & (size - 1)
}
