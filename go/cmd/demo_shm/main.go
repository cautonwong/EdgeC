package main

import (
	"fmt"
	"log"
	"time"

	"edgeos.go/pkg/shm"
)

func main() {
	// The C tests create shadow tables like test_hub, test_seal, etc.
	// But those are transient. We'll wait for one or assume one is running.
	// Wait, we can write a C program that creates a DataHub, writes to it, and spins.
	// Or we can just build a demo C program.
	
	fmt.Println("This is a demo. Run the C test first, then run this to read it.")
	matrix, err := shm.OpenShadowMatrix("demo", 1024)
	if err != nil {
		log.Fatalf("Failed to open: %v", err)
	}
	defer matrix.Close()

	for i := 0; i < 5; i++ {
		msg, err := matrix.ReadTag(1001) // Example ID
		if err != nil {
			log.Printf("Error reading: %v", err)
		} else {
			f, _ := msg.Scalar()
			fmt.Printf("Tag 1001 value: %f\n", f)
		}
		time.Sleep(1 * time.Second)
	}
}
