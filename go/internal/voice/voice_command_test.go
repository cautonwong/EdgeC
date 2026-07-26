package voice

import (
	"testing"
)

func TestVoiceCommandParser(t *testing.T) {
	parser := NewVoiceCommandParser(0.8)

	tests := []struct {
		input        string
		confidence   float32
		expectedCmd  VoiceCommandType
		isActionable bool
	}{
		{"医生帮我，呼叫急救！", 0.95, CmdEmergencySOS, true},
		{"麻烦暂停训练", 0.85, CmdPause, true},
		{"恢复训练吧", 0.82, CmdResume, true},
		{"太轻了，增加阻力", 0.88, CmdIncreaseResistance, true},
		{"太重了，轻一点", 0.90, CmdDecreaseResistance, true},
		{"今天天气不错", 0.99, CmdUnknown, false},
		{"暂停", 0.50, CmdPause, false}, // confidence below threshold
	}

	for _, tt := range tests {
		res, err := parser.ParseTranscript(tt.input, tt.confidence)
		if err != nil {
			t.Fatalf("unexpected error for input %q: %v", tt.input, err)
		}

		if res.Command != tt.expectedCmd {
			t.Errorf("input %q: expected cmd %s, got %s", tt.input, tt.expectedCmd, res.Command)
		}
		if res.IsActionable != tt.isActionable {
			t.Errorf("input %q: expected actionable=%v, got %v", tt.input, tt.isActionable, res.IsActionable)
		}
	}
}
