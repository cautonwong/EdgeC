package shm

import (
	"fmt"
	"os"
	"testing"
)

func TestOpenLegacyHeaderlessShadow(t *testing.T) {
	name := fmt.Sprintf("go_legacy_%d", os.Getpid())
	path := "/dev/shm/edgeos_shadow_" + name
	os.Remove(path)
	f, err := os.OpenFile(path, os.O_CREATE|os.O_RDWR, 0600)
	if err != nil {
		t.Fatal(err)
	}
	if err := f.Truncate(int64(4 * PipeMsgSize)); err != nil {
		f.Close()
		t.Fatal(err)
	}
	f.Close()

	m, err := OpenShadowMatrix(name, 4)
	if err != nil {
		t.Fatal(err)
	}
	if err := m.WriteTag(1, 123.5, 1000); err != nil {
		t.Fatal(err)
	}
	got, err := m.ReadTag(1)
	if err != nil {
		t.Fatal(err)
	}
	if v, _ := got.Scalar(); v != 123.5 {
		t.Fatalf("value = %v, want 123.5", v)
	}
	m.Close()
	os.Remove(path)
}
