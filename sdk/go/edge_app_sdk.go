// Package edgeappsdk is a thin client for the frozen EdgeC app host ABI v1.
//
// Wire format is one request/response per message: a 4-byte big-endian length
// prefix followed by UTF-8 JSON. See sdk/README.md for the contract.
package edgeappsdk

import (
	"bufio"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"io"
)

const ABIVersion = "1.0.0"

// Error keeps the original host error code, including DENIED_* values.
type Error struct {
	Code string
}

func (e *Error) Error() string { return e.Code }

// Sample carries a decoded point value and its quality.
type Sample struct {
	Value    any
	Quality  string
	SourceTS uint64
}

// Exports are the three functions the app registers with the future app-host.
type Exports struct {
	Init    func()
	OnEvent func(event json.RawMessage)
	Stop    func()
}

type Client struct {
	r       *bufio.Reader
	w       io.Writer
	nextID  uint64
	exports Exports
}

func New(r io.Reader, w io.Writer) *Client {
	return &Client{r: bufio.NewReader(r), w: w, nextID: 1}
}

func (c *Client) SetExports(exports Exports) {
	c.exports = exports
}

// Dispatch handles one host-to-app request (app_init / app_on_event / app_stop)
// and returns the response message.
func (c *Client) Dispatch(request map[string]any) map[string]any {
	id := request["id"]
	fn, _ := request["fn"].(string)
	switch fn {
	case "app_init":
		if c.exports.Init != nil {
			c.exports.Init()
		}
	case "app_on_event":
		if c.exports.OnEvent != nil {
			event, err := json.Marshal(request["args"])
			if err != nil {
				return map[string]any{"id": id, "ok": false, "code": "protocol"}
			}
			c.exports.OnEvent(event)
		}
	case "app_stop":
		if c.exports.Stop != nil {
			c.exports.Stop()
		}
	default:
		return map[string]any{"id": id, "ok": false, "code": "not_found"}
	}
	return map[string]any{"id": id, "ok": true, "result": map[string]any{}}
}

func (c *Client) call(fn string, args any) (json.RawMessage, error) {
	id := c.nextID
	c.nextID++
	body, err := json.Marshal(map[string]any{"id": id, "fn": fn, "args": args})
	if err != nil {
		return nil, err
	}
	var prefix [4]byte
	binary.BigEndian.PutUint32(prefix[:], uint32(len(body)))
	if _, err := c.w.Write(append(prefix[:], body...)); err != nil {
		return nil, err
	}
	if _, err := io.ReadFull(c.r, prefix[:]); err != nil {
		return nil, err
	}
	size := binary.BigEndian.Uint32(prefix[:])
	payload := make([]byte, size)
	if _, err := io.ReadFull(c.r, payload); err != nil {
		return nil, err
	}
	var response struct {
		ID     uint64          `json:"id"`
		OK     bool            `json:"ok"`
		Result json.RawMessage `json:"result"`
		Code   string          `json:"code"`
	}
	if err := json.Unmarshal(payload, &response); err != nil {
		return nil, err
	}
	if response.ID != id {
		return nil, &Error{Code: "protocol"}
	}
	if !response.OK {
		return nil, &Error{Code: response.Code}
	}
	return response.Result, nil
}

func (c *Client) PointResolve(name string) (uint32, string, error) {
	raw, err := c.call("point_resolve", map[string]any{"name": name})
	if err != nil {
		return 0, "", err
	}
	var result struct {
		Handle uint32 `json:"handle"`
		Access string `json:"access"`
	}
	if err := json.Unmarshal(raw, &result); err != nil {
		return 0, "", &Error{Code: "protocol"}
	}
	return result.Handle, result.Access, nil
}

func (c *Client) PointRead(handle uint32) (Sample, error) {
	raw, err := c.call("point_read", map[string]any{"handle": handle})
	if err != nil {
		return Sample{}, err
	}
	var wire struct {
		Value    map[string]json.RawMessage `json:"value"`
		Quality  string                     `json:"quality"`
		SourceTS uint64                     `json:"source_ts"`
	}
	if err := json.Unmarshal(raw, &wire); err != nil {
		return Sample{}, &Error{Code: "protocol"}
	}
	value, err := decodeValue(wire.Value)
	if err != nil {
		return Sample{}, err
	}
	return Sample{Value: value, Quality: wire.Quality, SourceTS: wire.SourceTS}, nil
}

func (c *Client) PointSubscribe(handle uint32, deadband float64, minIntervalMS, maxIntervalMS uint32) (uint32, error) {
	raw, err := c.call("point_subscribe", map[string]any{
		"handle":          handle,
		"deadband":        deadband,
		"min_interval_ms": minIntervalMS,
		"max_interval_ms": maxIntervalMS,
	})
	if err != nil {
		return 0, err
	}
	var result struct {
		Subscription uint32 `json:"subscription"`
	}
	if err := json.Unmarshal(raw, &result); err != nil {
		return 0, &Error{Code: "protocol"}
	}
	return result.Subscription, nil
}

func (c *Client) PointPublish(handle uint32, sample Sample) error {
	value, err := encodeValue(sample.Value)
	if err != nil {
		return err
	}
	quality := sample.Quality
	if quality == "" {
		quality = "good"
	}
	_, err = c.call("point_publish", map[string]any{
		"handle": handle,
		"sample": map[string]any{
			"value":     value,
			"quality":   quality,
			"source_ts": sample.SourceTS,
		},
	})
	return err
}

func (c *Client) Log(level, message string) error {
	_, err := c.call("log", map[string]any{"level": level, "message": message})
	return err
}

func (c *Client) Now() (uint64, error) {
	raw, err := c.call("now", map[string]any{})
	if err != nil {
		return 0, err
	}
	var result struct {
		NowMS uint64 `json:"now_ms"`
	}
	if err := json.Unmarshal(raw, &result); err != nil {
		return 0, &Error{Code: "protocol"}
	}
	return result.NowMS, nil
}

func decodeValue(encoded map[string]json.RawMessage) (any, error) {
	if raw, ok := encoded["float"]; ok {
		var value float64
		if err := json.Unmarshal(raw, &value); err != nil {
			return nil, &Error{Code: "protocol"}
		}
		return value, nil
	}
	if raw, ok := encoded["integer"]; ok {
		var value int64
		if err := json.Unmarshal(raw, &value); err != nil {
			return nil, &Error{Code: "protocol"}
		}
		return value, nil
	}
	if raw, ok := encoded["boolean"]; ok {
		var value bool
		if err := json.Unmarshal(raw, &value); err != nil {
			return nil, &Error{Code: "protocol"}
		}
		return value, nil
	}
	if raw, ok := encoded["text"]; ok {
		var value string
		if err := json.Unmarshal(raw, &value); err != nil {
			return nil, &Error{Code: "protocol"}
		}
		return value, nil
	}
	return nil, &Error{Code: "protocol"}
}

func encodeValue(value any) (map[string]any, error) {
	switch v := value.(type) {
	case bool:
		return map[string]any{"boolean": v}, nil
	case int:
		return map[string]any{"integer": v}, nil
	case int64:
		return map[string]any{"integer": v}, nil
	case float64:
		return map[string]any{"float": v}, nil
	case string:
		return map[string]any{"text": v}, nil
	}
	return nil, fmt.Errorf("unsupported point value %T", value)
}
