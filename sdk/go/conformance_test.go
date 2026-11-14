package edgeappsdk_test

import (
	"bufio"
	"encoding/binary"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"reflect"
	"syscall"
	"testing"
	"time"

	sdk "edgeos/sdk/go"
)

type stepExpect struct {
	OK     bool           `json:"ok"`
	Result map[string]any `json:"result"`
	Code   string         `json:"code"`
}

type step struct {
	Fn     string         `json:"fn"`
	Args   map[string]any `json:"args"`
	Expect stepExpect     `json:"expect"`
}

type vector struct {
	Name      string         `json:"name"`
	Event     map[string]any `json:"event"`
	Steps     []step         `json:"steps"`
	ExpectLog []string       `json:"expect_log"`
}

type suite struct {
	Vectors []vector `json:"vectors"`
}

func loadSuite(t *testing.T) suite {
	t.Helper()
	for _, prefix := range []string{"", "../", "../../"} {
		path := filepath.Join(prefix, "sdk/conformance/vectors.json")
		data, err := os.ReadFile(path)
		if err != nil {
			continue
		}
		var parsed suite
		if err := json.Unmarshal(data, &parsed); err != nil {
			t.Fatalf("%s: %v", path, err)
		}
		return parsed
	}
	t.Fatal("cannot find sdk/conformance/vectors.json")
	return suite{}
}

func socketpair(t *testing.T) (*os.File, *os.File) {
	t.Helper()
	fds, err := syscall.Socketpair(syscall.AF_UNIX, syscall.SOCK_STREAM, 0)
	if err != nil {
		t.Fatalf("socketpair: %v", err)
	}
	return os.NewFile(uintptr(fds[0]), "app"), os.NewFile(uintptr(fds[1]), "host")
}

func readFrame(reader *bufio.Reader) (map[string]any, error) {
	var prefix [4]byte
	if _, err := io.ReadFull(reader, prefix[:]); err != nil {
		return nil, err
	}
	size := binary.BigEndian.Uint32(prefix[:])
	payload := make([]byte, size)
	if _, err := io.ReadFull(reader, payload); err != nil {
		return nil, err
	}
	var message map[string]any
	if err := json.Unmarshal(payload, &message); err != nil {
		return nil, err
	}
	return message, nil
}

func writeFrame(writer io.Writer, message map[string]any) error {
	body, err := json.Marshal(message)
	if err != nil {
		return err
	}
	var prefix [4]byte
	binary.BigEndian.PutUint32(prefix[:], uint32(len(body)))
	_, err = writer.Write(append(prefix[:], body...))
	return err
}

func serve(host io.ReadWriter, steps []step, logs *[]string) <-chan error {
	done := make(chan error, 1)
	go func() {
		reader := bufio.NewReader(host)
		for index, want := range steps {
			request, err := readFrame(reader)
			if err != nil {
				done <- fmt.Errorf("step %d read: %w", index, err)
				return
			}
			fn, _ := request["fn"].(string)
			if fn != want.Fn {
				done <- fmt.Errorf("step %d fn got %s want %s", index, fn, want.Fn)
				return
			}
			if !reflect.DeepEqual(request["args"], want.Args) {
				done <- fmt.Errorf("step %d args got %#v want %#v", index, request["args"], want.Args)
				return
			}
			if want.Fn == "log" {
				message, _ := want.Args["message"].(string)
				*logs = append(*logs, message)
			}
			response := map[string]any{"id": request["id"]}
			if want.Expect.OK {
				response["ok"] = true
				response["result"] = want.Expect.Result
			} else {
				response["ok"] = false
				response["code"] = want.Expect.Code
			}
			if err := writeFrame(host, response); err != nil {
				done <- fmt.Errorf("step %d write: %w", index, err)
				return
			}
		}
		done <- nil
	}()
	return done
}

func expectError(s step, err error) error {
	if s.Expect.OK {
		return fmt.Errorf("%s: unexpected error %v", s.Fn, err)
	}
	var sdkErr *sdk.Error
	if !errors.As(err, &sdkErr) {
		return fmt.Errorf("%s: expected %s, got %v", s.Fn, s.Expect.Code, err)
	}
	if sdkErr.Code != s.Expect.Code {
		return fmt.Errorf("%s: got %s want %s", s.Fn, sdkErr.Code, s.Expect.Code)
	}
	return nil
}

func decodeVectorValue(encoded map[string]any) (any, error) {
	for _, kind := range []string{"float", "integer", "boolean", "text"} {
		value, ok := encoded[kind]
		if !ok {
			continue
		}
		if kind == "integer" {
			return int64(value.(float64)), nil
		}
		return value, nil
	}
	return nil, fmt.Errorf("unknown value tag %#v", encoded)
}

func runStep(client *sdk.Client, s step) error {
	switch s.Fn {
	case "point_resolve":
		handle, access, err := client.PointResolve(s.Args["name"].(string))
		if err != nil {
			return expectError(s, err)
		}
		if !s.Expect.OK {
			return fmt.Errorf("point_resolve unexpectedly succeeded")
		}
		wantHandle := uint32(s.Expect.Result["handle"].(float64))
		wantAccess := s.Expect.Result["access"].(string)
		if handle != wantHandle || access != wantAccess {
			return fmt.Errorf("resolve got (%d,%s) want (%d,%s)", handle, access, wantHandle, wantAccess)
		}

	case "point_read":
		sample, err := client.PointRead(uint32(s.Args["handle"].(float64)))
		if err != nil {
			return expectError(s, err)
		}
		if !s.Expect.OK {
			return fmt.Errorf("point_read unexpectedly succeeded")
		}
		wantValue, err := decodeVectorValue(s.Expect.Result["value"].(map[string]any))
		if err != nil {
			return err
		}
		wantQuality := s.Expect.Result["quality"].(string)
		wantSourceTS := uint64(s.Expect.Result["source_ts"].(float64))
		if !reflect.DeepEqual(sample.Value, wantValue) || sample.Quality != wantQuality || sample.SourceTS != wantSourceTS {
			return fmt.Errorf("read got %#v want %#v", sample, s.Expect.Result)
		}

	case "point_subscribe":
		subscription, err := client.PointSubscribe(
			uint32(s.Args["handle"].(float64)),
			s.Args["deadband"].(float64),
			uint32(s.Args["min_interval_ms"].(float64)),
			uint32(s.Args["max_interval_ms"].(float64)),
		)
		if err != nil {
			return expectError(s, err)
		}
		if !s.Expect.OK {
			return fmt.Errorf("point_subscribe unexpectedly succeeded")
		}
		want := uint32(s.Expect.Result["subscription"].(float64))
		if subscription != want {
			return fmt.Errorf("subscription got %d want %d", subscription, want)
		}

	case "point_publish":
		wire := s.Args["sample"].(map[string]any)
		value, err := decodeVectorValue(wire["value"].(map[string]any))
		if err != nil {
			return err
		}
		err = client.PointPublish(uint32(s.Args["handle"].(float64)), sdk.Sample{
			Value:    value,
			Quality:  wire["quality"].(string),
			SourceTS: uint64(wire["source_ts"].(float64)),
		})
		if err != nil {
			return expectError(s, err)
		}
		if !s.Expect.OK {
			return fmt.Errorf("point_publish unexpectedly succeeded")
		}

	case "log":
		err := client.Log(s.Args["level"].(string), s.Args["message"].(string))
		if err != nil {
			return expectError(s, err)
		}
		if !s.Expect.OK {
			return fmt.Errorf("log unexpectedly succeeded")
		}

	case "now":
		now, err := client.Now()
		if err != nil {
			return expectError(s, err)
		}
		if !s.Expect.OK {
			return fmt.Errorf("now unexpectedly succeeded")
		}
		want := uint64(s.Expect.Result["now_ms"].(float64))
		if now != want {
			return fmt.Errorf("now got %d want %d", now, want)
		}

	default:
		return fmt.Errorf("unknown fn %s", s.Fn)
	}
	return nil
}

func runVector(t *testing.T, v vector) {
	t.Helper()
	appFile, hostFile := socketpair(t)
	defer appFile.Close()
	defer hostFile.Close()

	logs := []string{}
	done := serve(hostFile, v.Steps, &logs)
	client := sdk.New(appFile, appFile)

	var appErr error
	client.SetExports(sdk.Exports{
		Init: func() {},
		OnEvent: func(event json.RawMessage) {
			for _, s := range v.Steps {
				if err := runStep(client, s); err != nil {
					appErr = err
					return
				}
			}
		},
		Stop: func() {},
	})
	client.Dispatch(map[string]any{"id": float64(1), "fn": "app_init", "args": map[string]any{}})
	client.Dispatch(map[string]any{"id": float64(2), "fn": "app_on_event", "args": v.Event})
	client.Dispatch(map[string]any{"id": float64(3), "fn": "app_stop", "args": map[string]any{}})

	select {
	case err := <-done:
		if err != nil {
			t.Fatal(err)
		}
	case <-time.After(3 * time.Second):
		t.Fatal("fake host did not finish")
	}
	if appErr != nil {
		t.Fatal(appErr)
	}
	if !reflect.DeepEqual(logs, v.ExpectLog) {
		t.Fatalf("logs got %v want %v", logs, v.ExpectLog)
	}
}

func TestConformanceVectors(t *testing.T) {
	parsed := loadSuite(t)
	if len(parsed.Vectors) == 0 {
		t.Fatal("no vectors found")
	}
	for _, v := range parsed.Vectors {
		v := v
		t.Run(v.Name, func(t *testing.T) {
			runVector(t, v)
		})
	}
}
