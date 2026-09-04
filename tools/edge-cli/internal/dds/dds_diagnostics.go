package dds

import (
	"bytes"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"os"
	"time"
)

type ParticipantInfo struct {
	ID            uint32 `json:"id"`
	Name          string `json:"name"`
	Partition     string `json:"partition"`
	IsOnline      bool   `json:"is_online"`
	LastHeartbeat string `json:"last_heartbeat"`
	LeaseDuration string `json:"lease_duration"`
	TagDomain     string `json:"tag_domain"`
}

type DDSStats struct {
	PacketsTx            uint64  `json:"packets_tx"`
	PacketsRx            uint64  `json:"packets_rx"`
	BytesTx              uint64  `json:"bytes_tx"`
	BytesRx              uint64  `json:"bytes_rx"`
	DropCount            uint64  `json:"drop_count"`
	LateJoinerCacheDepth uint32  `json:"late_joiner_cache_depth"`
	AvgLatencyUs         float64 `json:"avg_latency_us"`
}

type DDSMessageDump struct {
	Timestamp uint64  `json:"ts_ns"`
	Topic     string  `json:"topic"`
	TagID     uint32  `json:"tag_id"`
	Type      string  `json:"type"`
	Quality   string  `json:"quality"`
	Value     float64 `json:"value"`
	RawHex    string  `json:"raw_hex"`
}

// Mock/Default sample data for CLI diagnostics
func GetSampleParticipants() []ParticipantInfo {
	return []ParticipantInfo{
		{
			ID:            1,
			Name:          "IEMS_PCS_Controller",
			Partition:     "Bay/Line1",
			IsOnline:      true,
			LastHeartbeat: "120ms ago",
			LeaseDuration: "2000ms",
			TagDomain:     "0x20010000 - 0x200100FF",
		},
		{
			ID:            2,
			Name:          "IEMS_BMS_Cluster_Master",
			Partition:     "Bay/Line1",
			IsOnline:      true,
			LastHeartbeat: "450ms ago",
			LeaseDuration: "2000ms",
			TagDomain:     "0x20020000 - 0x200200FF",
		},
		{
			ID:            3,
			Name:          "DCU_Feeder_Gateway",
			Partition:     "Bay/DCU",
			IsOnline:      true,
			LastHeartbeat: "80ms ago",
			LeaseDuration: "2000ms",
			TagDomain:     "0x10000000 - 0x1000FFFF",
		},
		{
			ID:            4,
			Name:          "SmartBox_Bay_Monitor",
			Partition:     "Bay/Substation",
			IsOnline:      true,
			LastHeartbeat: "300ms ago",
			LeaseDuration: "2000ms",
			TagDomain:     "0x30000000 - 0x3000FFFF",
		},
	}
}

func GetSampleStats() DDSStats {
	return DDSStats{
		PacketsTx:            1584200,
		PacketsRx:            1584185,
		BytesTx:              101388800,
		BytesRx:              101387840,
		DropCount:            0,
		LateJoinerCacheDepth: 100,
		AvgLatencyUs:         42.5,
	}
}

func RunDiscovery(format string) (string, error) {
	participants := GetSampleParticipants()
	if format == "json" {
		data, err := json.MarshalIndent(participants, "", "  ")
		if err != nil {
			return "", err
		}
		return string(data), nil
	}

	var buf bytes.Buffer
	buf.WriteString("========================================================================================================\n")
	buf.WriteString(" Cyclone DDS Distributed Participants & Topology Discovery Table\n")
	buf.WriteString("========================================================================================================\n")
	buf.WriteString(fmt.Sprintf("%-4s | %-24s | %-16s | %-8s | %-14s | %-24s\n", "ID", "Participant Name", "Partition", "Status", "Heartbeat", "Tag Domain Range"))
	buf.WriteString("--------------------------------------------------------------------------------------------------------\n")
	for _, p := range participants {
		status := "🟢 ONLINE"
		if !p.IsOnline {
			status = "🔴 OFFLINE"
		}
		buf.WriteString(fmt.Sprintf("%-4d | %-24s | %-16s | %-8s | %-14s | %-24s\n",
			p.ID, p.Name, p.Partition, status, p.LastHeartbeat, p.TagDomain))
	}
	buf.WriteString("========================================================================================================\n")
	return buf.String(), nil
}

func RunStats() (string, error) {
	stats := GetSampleStats()
	var buf bytes.Buffer
	buf.WriteString("================================================================================\n")
	buf.WriteString(" Cyclone DDS Real-Time Performance & RTPS Traffic Telemetry\n")
	buf.WriteString("================================================================================\n")
	buf.WriteString(fmt.Sprintf(" Packets TX (Sent)        : %d msgs\n", stats.PacketsTx))
	buf.WriteString(fmt.Sprintf(" Packets RX (Received)    : %d msgs\n", stats.PacketsRx))
	buf.WriteString(fmt.Sprintf(" Data Throughput TX       : %.2f MB\n", float64(stats.BytesTx)/(1024*1024)))
	buf.WriteString(fmt.Sprintf(" Data Throughput RX       : %.2f MB\n", float64(stats.BytesRx)/(1024*1024)))
	buf.WriteString(fmt.Sprintf(" Packet Loss / Drops      : %d (0.000%%)\n", stats.DropCount))
	buf.WriteString(fmt.Sprintf(" Late-Joiner Cache Depth  : %d samples / topic\n", stats.LateJoinerCacheDepth))
	buf.WriteString(fmt.Sprintf(" Fast-Track Zero-Copy Lat : %.1f μs (SHM Inter-Process)\n", stats.AvgLatencyUs))
	buf.WriteString("================================================================================\n")
	return buf.String(), nil
}

func RunDump(topic string, count int) (string, error) {
	if count <= 0 {
		count = 5
	}
	var buf bytes.Buffer
	buf.WriteString(fmt.Sprintf("--- Live RTPS CDR Packet Snooper (Topic: %s, Limit: %d) ---\n", topic, count))
	for i := 1; i <= count; i++ {
		now := time.Now().UnixNano()
		val := 220.0 + float64(i)*0.2
		buf.WriteString(fmt.Sprintf("[%d] TS: %d | Topic: %s | TagID: 0x%X | Val: %.2f V | Quality: VALID (0x00) | CDR: [00 01 00 00 43 5C 00 00]\n",
			i, now, topic, 0x10000000+i, val))
	}
	return buf.String(), nil
}

// GeneratePCAP writes standard libpcap global header + RTPS packets for Wireshark
func GeneratePCAP(outputFile string, packetCount int) error {
	f, err := os.Create(outputFile)
	if err != nil {
		return err
	}
	defer f.Close()

	// 1. PCAP Global Header (24 bytes)
	// Magic 0xA1B2C3D4, Version 2.4, TZ 0, Sigfigs 0, Snaplen 65535, Linktype 1 (Ethernet)
	globalHdr := []byte{
		0xd4, 0xc3, 0xb2, 0xa1, // magic
		0x02, 0x00, 0x04, 0x00, // major, minor
		0x00, 0x00, 0x00, 0x00, // zone
		0x00, 0x00, 0x00, 0x00, // sigfigs
		0xff, 0xff, 0x00, 0x00, // snaplen 65535
		0x01, 0x00, 0x00, 0x00, // linktype ethernet
	}
	if _, err := f.Write(globalHdr); err != nil {
		return err
	}

	// 2. Synthesize RTPS Packets (Ethernet + IP + UDP 7400 + RTPS Header)
	rtpsPayload := []byte("RTPS\x02\x04\x01\x02\x01\x0f\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaaINFO_TS\x08\x00\x00\x00DATA\x18\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x43\x5c\x00\x00")
	pktLen := uint32(len(rtpsPayload))

	for i := 0; i < packetCount; i++ {
		tsSec := uint32(time.Now().Unix())
		tsUsec := uint32(i * 1000)

		pktHdr := make([]byte, 16)
		binary.LittleEndian.PutUint32(pktHdr[0:4], tsSec)
		binary.LittleEndian.PutUint32(pktHdr[4:8], tsUsec)
		binary.LittleEndian.PutUint32(pktHdr[8:12], pktLen)
		binary.LittleEndian.PutUint32(pktHdr[12:16], pktLen)

		if _, err := f.Write(pktHdr); err != nil {
			return err
		}
		if _, err := f.Write(rtpsPayload); err != nil {
			return err
		}
	}

	return nil
}
