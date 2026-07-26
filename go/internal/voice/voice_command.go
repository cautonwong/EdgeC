package voice

import (
	"errors"
	"strings"
)

// VoiceCommandType represents parsed offline voice command actions.
type VoiceCommandType string

const (
	CmdUnknown            VoiceCommandType = "CMD_UNKNOWN"
	CmdPause              VoiceCommandType = "CMD_PAUSE"
	CmdResume             VoiceCommandType = "CMD_RESUME"
	CmdEmergencySOS       VoiceCommandType = "CMD_SOS"
	CmdIncreaseResistance VoiceCommandType = "CMD_INCREASE_RESISTANCE"
	CmdDecreaseResistance VoiceCommandType = "CMD_DECREASE_RESISTANCE"
)

// VoiceParsedResult holds transcription result and identified command metadata.
type VoiceParsedResult struct {
	RawTranscript string           `json:"raw_transcript"`
	Command       VoiceCommandType `json:"command"`
	Confidence    float32          `json:"confidence"`
	IsActionable  bool             `json:"is_actionable"`
}

// VoiceCommandParser handles offline voice recognition output parsing and intent mapping.
type VoiceCommandParser struct {
	minConfidence float32
}

// NewVoiceCommandParser creates a new voice command parser.
func NewVoiceCommandParser(minConfidence float32) *VoiceCommandParser {
	if minConfidence <= 0 {
		minConfidence = 0.75
	}
	return &VoiceCommandParser{
		minConfidence: minConfidence,
	}
}

// ParseTranscript analyzes text transcript from Whisper.cpp and maps it to a control command.
func (p *VoiceCommandParser) ParseTranscript(text string, whisperConfidence float32) (*VoiceParsedResult, error) {
	cleanText := strings.TrimSpace(text)
	if cleanText == "" {
		return nil, errors.New("empty transcript text")
	}

	result := &VoiceParsedResult{
		RawTranscript: cleanText,
		Command:       CmdUnknown,
		Confidence:    whisperConfidence,
		IsActionable:  false,
	}

	// Keywords mapping
	switch {
	case strings.Contains(cleanText, "呼叫") || strings.Contains(cleanText, "急救") || strings.Contains(cleanText, "救命"):
		result.Command = CmdEmergencySOS
	case strings.Contains(cleanText, "暂停") || strings.Contains(cleanText, "停一下"):
		result.Command = CmdPause
	case strings.Contains(cleanText, "恢复") || strings.Contains(cleanText, "继续"):
		result.Command = CmdResume
	case strings.Contains(cleanText, "增加阻力") || strings.Contains(cleanText, "加大阻力") || strings.Contains(cleanText, "重一点"):
		result.Command = CmdIncreaseResistance
	case strings.Contains(cleanText, "减小阻力") || strings.Contains(cleanText, "减轻阻力") || strings.Contains(cleanText, "轻一点"):
		result.Command = CmdDecreaseResistance
	}

	if result.Command != CmdUnknown && whisperConfidence >= p.minConfidence {
		result.IsActionable = true
	}

	return result, nil
}
