package dds

import (
	"os"
	"strings"
	"testing"
)

func TestDDSDiscovery(t *testing.T) {
	// 1. Table format
	tableOut, err := RunDiscovery("table")
	if err != nil {
		t.Fatalf("RunDiscovery table failed: %v", err)
	}
	if !strings.Contains(tableOut, "Cyclone DDS Distributed Participants") {
		t.Errorf("Expected table title, got: %s", tableOut)
	}
	if !strings.Contains(tableOut, "IEMS_PCS_Controller") {
		t.Errorf("Expected PCS node in table")
	}

	// 2. JSON format
	jsonOut, err := RunDiscovery("json")
	if err != nil {
		t.Fatalf("RunDiscovery json failed: %v", err)
	}
	if !strings.Contains(jsonOut, "\"name\": \"IEMS_PCS_Controller\"") {
		t.Errorf("Expected JSON participant entry")
	}
}

func TestDDSStats(t *testing.T) {
	statsOut, err := RunStats()
	if err != nil {
		t.Fatalf("RunStats failed: %v", err)
	}
	if !strings.Contains(statsOut, "Packets TX") || !strings.Contains(statsOut, "Fast-Track Zero-Copy Lat") {
		t.Errorf("Expected stats output, got: %s", statsOut)
	}
}

func TestDDSDump(t *testing.T) {
	dumpOut, err := RunDump("edgeos/dlt698/yc", 3)
	if err != nil {
		t.Fatalf("RunDump failed: %v", err)
	}
	if !strings.Contains(dumpOut, "Live RTPS CDR Packet Snooper") {
		t.Errorf("Expected dump header")
	}
	if !strings.Contains(dumpOut, "Topic: edgeos/dlt698/yc") {
		t.Errorf("Expected topic in dump")
	}
}

func TestDDSGeneratePCAP(t *testing.T) {
	tmpPcap := "/tmp/test_rtps.pcap"
	defer os.Remove(tmpPcap)

	err := GeneratePCAP(tmpPcap, 5)
	if err != nil {
		t.Fatalf("GeneratePCAP failed: %v", err)
	}

	fi, err := os.Stat(tmpPcap)
	if err != nil {
		t.Fatalf("os.Stat failed on pcap: %v", err)
	}
	if fi.Size() < 50 {
		t.Errorf("Expected valid pcap file size, got %d bytes", fi.Size())
	}
}
