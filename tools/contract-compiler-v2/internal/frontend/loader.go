package frontend

import (
	"os"
	"path/filepath"
	"strings"
	"gopkg.in/yaml.v3"
)

type Loader struct {
	BaseDir string
	seen    map[string]bool
}

func NewLoader(baseDir string) *Loader {
	return &Loader{
		BaseDir: baseDir,
		seen:    make(map[string]bool),
	}
}

func (l *Loader) Load(productPath, localPath string) (*ContractAST, error) {
	ast := &ContractAST{}

	// 1. Load Product YAML
	pData, err := os.ReadFile(productPath)
	if err != nil { return nil, err }
	
	var rawProduct map[string]interface{}
	if err := yaml.Unmarshal(pData, &rawProduct); err != nil { return nil, err }
	
	ast.ProductName, _ = rawProduct["product"].(string)

	// 2. Recursive Load Includes
	l.processIncludes(rawProduct, ast)

	// 3. Load Local Manifest Overrides (Highest priority)
	if localPath != "" {
		lData, err := os.ReadFile(localPath)
		if err == nil {
			var rawLocal map[string]interface{}
			yaml.Unmarshal(lData, &rawLocal)
			l.processIncludes(rawLocal, ast)
			l.processManifest(rawLocal, ast)
		}
	}

	return ast, nil
}

func (l *Loader) processIncludes(raw map[string]interface{}, ast *ContractAST) {
	// A. Recurse first (depth-first to allow parent overrides)
	var includes []string
	if incs, ok := raw["includes"].([]interface{}); ok {
		for _, v := range incs { if s, ok := v.(string); ok { includes = append(includes, s) } }
	}
	if incs, ok := raw["include"].([]interface{}); ok {
		for _, v := range incs { if s, ok := v.(string); ok { includes = append(includes, s) } }
	}
	for _, incPath := range includes {
		l.loadInclude(incPath, ast)
	}

	// B. Process Models (蓝图库)
	if models, ok := raw["models"].([]interface{}); ok {
		for _, m := range models {
			if mm, ok := m.(map[string]interface{}); ok {
				l.parseModel(mm, ast)
			}
		}
	}

	// C. Process Instances (物理实例)
	if instances, ok := raw["instances"].([]interface{}); ok {
		for _, i := range instances {
			if im, ok := i.(map[string]interface{}); ok {
				l.parseInstance(im, ast)
			}
		}
	}

	// D. Process Tags in current level (Legacy Support)
	if tags, ok := raw["tags"].([]interface{}); ok {
		for _, t := range tags {
			if tm, ok := t.(map[string]interface{}); ok {
				l.parseTag(tm, ast)
			}
		}
	}
	
	// E. Process Pipelines
	if pipes, ok := raw["pipelines"].([]interface{}); ok {
		for _, p := range pipes {
			if pm, ok := p.(map[string]interface{}); ok {
				id, _ := pm["id"].(string)
				trigger, _ := pm["trigger"].(string)
				stagesRaw, _ := pm["stages"].([]interface{})
				node := &PipelineNode{ID: id, Trigger: trigger}
				for _, s := range stagesRaw {
					if sm, ok := s.(map[string]interface{}); ok {
						st := StageNode{ 
							Name: getString(sm, "name"), 
							Type: getString(sm, "type"),
							Branches: make(map[string]string),
						}
						if cfg, ok := sm["config"].(map[string]interface{}); ok { st.Config = cfg } else { st.Config = sm }
						
						// Parse Branches (Topology)
						if branches, ok := sm["branches"].(map[string]interface{}); ok {
							for k, v := range branches {
								if vs, ok := v.(string); ok { st.Branches[k] = vs }
							}
						}
						
						node.Stages = append(node.Stages, st)
					}
				}
				ast.Pipelines = append(ast.Pipelines, node)
			}
		}
	}

	// F. Process Mappings (Binding overrides)
	proto, _ := raw["protocol"].(string)
	mappings, _ := raw["mappings"].([]interface{})
	if len(mappings) == 0 { mappings, _ = raw["map"].([]interface{}) }
	for _, m := range mappings {
		if mm, ok := m.(map[string]interface{}); ok {
			tagName, _ := mm["tag"].(string)
			if tagName == "" { continue }
			l.upsertBinding(ast, tagName, proto, mm)
		}
	}

	// [Nexus Model Auto-extraction] Support single-file Model extraction from flat Binding structure
	if modelName, ok := raw["model"].(string); ok && proto != "" {
		node := &ModelNode{
			Name:     modelName,
			Protocol: proto,
			Version:  getString(raw, "version"),
		}
		for _, m := range mappings {
			if mm, ok := m.(map[string]interface{}); ok {
				pnode := &PointNode{
					Name:     getString(mm, "tag"),
					Type:     getString(mm, "type"),
					Unit:     getString(mm, "unit"),
					Category: getString(mm, "category"),
					Bits:     getString(mm, "bits"),
				}
				if addr, ok := mm["addr"].(int); ok { pnode.Address = uint32(addr) }
				if scale, ok := mm["scale"].(float64); ok { pnode.Scale = scale }
				if attrs, ok := mm["attributes"].(map[string]interface{}); ok {
					pnode.Attrs = attrs
				}
				node.Points = append(node.Points, pnode)
			}
		}
		// Avoid duplicates
		exists := false
		for _, exm := range ast.Models {
			if exm.Name == modelName {
				exists = true
				break
			}
		}
		if !exists {
			ast.Models = append(ast.Models, node)
		}
	}
}

func (l *Loader) loadInclude(path string, ast *ContractAST) {
	absPath := path
	if !filepath.IsAbs(path) { absPath = filepath.Join(l.BaseDir, path) }
	if l.seen[absPath] { return }
	l.seen[absPath] = true
	data, err := os.ReadFile(absPath); if err != nil { return }
	var raw map[string]interface{}
	if err := yaml.Unmarshal(data, &raw); err != nil { return }
	l.processIncludes(raw, ast)
}

func (l *Loader) parseModel(mm map[string]interface{}, ast *ContractAST) {
	name := getString(mm, "name")
	if name == "" { return }
	
	node := &ModelNode{
		Name:     name,
		Protocol: getString(mm, "protocol"),
		Version:  getString(mm, "version"),
	}
	
	if points, ok := mm["points"].([]interface{}); ok {
		for _, p := range points {
			if pm, ok := p.(map[string]interface{}); ok {
				pnode := &PointNode{
					Name:     getString(pm, "name"),
					Type:     getString(pm, "type"),
					Unit:     getString(pm, "unit"),
					Category: getString(pm, "category"),
					Bits:     getString(pm, "bits"),
				}
				if addr, ok := pm["address"].(int); ok { pnode.Address = uint32(addr) }
				if scale, ok := pm["scale"].(float64); ok { pnode.Scale = scale }
				
				if attrs, ok := pm["attributes"].(map[string]interface{}); ok {
					pnode.Attrs = attrs
				} else if attrs, ok := pm["attrs"].(map[string]interface{}); ok {
					pnode.Attrs = attrs
				}
				node.Points = append(node.Points, pnode)
			}
		}
	}
	ast.Models = append(ast.Models, node)
}

func (l *Loader) parseInstance(im map[string]interface{}, ast *ContractAST) {
	name := getString(im, "name")
	if name == "" { return }
	
	node := &InstanceNode{
		Name:      name,
		ModelName: getString(im, "model"),
	}
	if busID, ok := im["bus_id"].(int); ok { node.BusID = busID }
	if addr, ok := im["address"]; ok { node.Address = addr }
	
	if bindings, ok := im["bindings"].([]interface{}); ok {
		for _, b := range bindings {
			if bm, ok := b.(map[string]interface{}); ok {
				bnode := &BindingMapNode{
					Point: getString(bm, "point"),
				}
				if tagID, ok := bm["tag_id"].(int); ok { bnode.TagID = uint32(tagID) }
				node.Bindings = append(node.Bindings, bnode)
			}
		}
	}
	ast.Instances = append(ast.Instances, node)
}

func (l *Loader) parseTag(tm map[string]interface{}, ast *ContractAST) {
	name := getString(tm, "name")
	idVal, hasID := tm["id"].(int)
	
	if name == "" && !hasID { return }

	// Search existing by Name OR ID
	var target *TagNode
	for _, t := range ast.Tags {
		if (name != "" && t.Name == name) || (hasID && t.ID == uint32(idVal)) {
			target = t; break
		}
	}

	if target == nil {
		target = &TagNode{Name: name, Attributes: make(map[string]interface{})}
		ast.Tags = append(ast.Tags, target)
	}

	if name != "" { target.Name = name }
	if hasID { target.ID = uint32(idVal) }
	if u := getString(tm, "unit"); u != "" { target.Unit = u }
	if d := getString(tm, "type"); d != "" { target.DataType = d }
	if attr, ok := tm["attributes"].(map[string]interface{}); ok {
		for k, v := range attr { target.Attributes[k] = v }
	}
}

func (l *Loader) upsertBinding(ast *ContractAST, tagName, proto string, attr map[string]interface{}) {
	var targetTag *TagNode
	for _, t := range ast.Tags { if t.Name == tagName { targetTag = t; break } }
	if targetTag == nil { return }

	// [Nexus Fix] 严格区分协议别名 (e.g. Modbus vs ModbusRTU)
	upProto := strings.ToUpper(proto)

	// Override existing binding for the same protocol
	found := false
	for _, b := range targetTag.Bindings {
		if strings.ToUpper(b.Protocol) == upProto {
			// 深度覆盖属性
			for k, v := range attr { b.Attributes[k] = v }
			found = true; break
		}
	}
	if !found {
		targetTag.Bindings = append(targetTag.Bindings, &BindingNode{
			Protocol: proto, Attributes: attr,
		})
	}
}

func (l *Loader) processManifest(raw map[string]interface{}, ast *ContractAST) {
	if conns, ok := raw["connectivity"].([]interface{}); ok {
		for _, c := range conns {
			if cm, ok := c.(map[string]interface{}); ok {
				node := &ConnectivityNode{
					ID: cm["id"].(int), Label: getString(cm, "label"),
					Category: getString(cm, "category"), Type: getString(cm, "type"),
					Config:     make(map[string]interface{}),
					Simulation: make(map[string]interface{}),
				}
				if sim, ok := cm["simulation"].(map[string]interface{}); ok {
					node.Simulation = sim
					node.SimulationPath = getString(sim, "endpoint")
					if node.SimulationPath == "" { node.SimulationPath = getString(sim, "control_sock") }
				}
				if cfg, ok := cm["config"].(map[string]interface{}); ok { 
					node.Config = cfg
					node.ProductionPath = getString(cfg, "path") 
				}
				ast.Connectivity = append(ast.Connectivity, node)
			}
		}
	}

	// [Nexus] 同时支持 top-level instances 定义
	if instances, ok := raw["instances"].([]interface{}); ok {
		for _, i := range instances {
			if im, ok := i.(map[string]interface{}); ok {
				l.parseInstance(im, ast)
			}
		}
	}

	if devices, ok := raw["devices"].([]interface{}); ok {
		for _, d := range devices {
			if dm, ok := d.(map[string]interface{}); ok {
				node := &DeviceNode{
					ID: getString(dm, "id"), BusID: dm["bus_id"].(int),
					Protocol: getString(dm, "protocol"), Model: getString(dm, "model"),
					Simulation: make(map[string]interface{}),
				}
				if sim, ok := dm["simulation"].(map[string]interface{}); ok {
					node.Simulation = sim
				}
				ast.Devices = append(ast.Devices, node)
			}
		}
	}
}

func getString(m map[string]interface{}, key string) string {
	if v, ok := m[key].(string); ok { return v }
	return ""
}
