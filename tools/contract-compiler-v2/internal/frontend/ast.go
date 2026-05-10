package frontend

type ContractAST struct {
	ProductName string
	Models      []*ModelNode
	Instances   []*InstanceNode
	Tags        []*TagNode
	Connectivity []*ConnectivityNode
	Devices      []*DeviceNode
	Pipelines    []*PipelineNode
	Policies     []*PolicyNode
	Configs      []*ConfigNode
}

type ModelNode struct {
	Name     string
	Protocol string
	Version  string
	Points   []*PointNode
}

type PointNode struct {
	Name     string
	Address  uint32
	Type     string
	Unit     string
	Category string
	Bits     string
	Scale    float64
	Attrs    map[string]interface{}
}

type InstanceNode struct {
	Name      string
	ModelName string
	BusID     int
	Address   interface{}
	Bindings  []*BindingMapNode
}

type BindingMapNode struct {
	Point string
	TagID uint32
}

type TagNode struct {
	ID         uint32
	Name       string
	DataType   string
	Unit       string
	Category   string
	Attributes map[string]interface{}
	Bindings   []*BindingNode
}

type BindingNode struct {
	Protocol   string
	Attributes map[string]interface{}
}

type ConnectivityNode struct {
	ID              int
	Label           string
	Category        string
	Type            string
	Config          map[string]interface{}
	Simulation      map[string]interface{}
	SimulationPath  string
	ProductionPath  string
}

type DeviceNode struct {
	ID         string
	BusID      int
	Protocol   string
	Address    interface{}
	Model      string
	Simulation map[string]interface{}
}

type PipelineNode struct {
	ID      string
	Trigger string
	Stages  []StageNode
}

type StageNode struct {
	Name     string
	Type     string
	Config   map[string]interface{}
	Branches map[string]string
}

type PolicyNode struct {
	Name       string
	Type       string
	Attributes map[string]interface{}
}

type ConfigNode struct {
	Name  string
	Value interface{}
}
