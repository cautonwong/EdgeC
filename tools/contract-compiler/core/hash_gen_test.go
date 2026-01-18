package core

import (
	"fmt"
	"testing"
)

// 测试完美哈希算法在不同规模下的成功率
func TestGeneratePerfectHash_Scaling(t *testing.T) {
	sizes := []int{10, 100, 500, 1000}
	for _, n := range sizes {
		t.Run(fmt.Sprintf("Size_%d", n), func(t *testing.T) {
			entries := make([]HashEntry, n)
			for i := 0; i < n; i++ {
				// 模拟工业点位：带有大量低位 0 的特征
				entries[i] = HashEntry{
					RawKey:  uint64(i << 16), 
					TagName: fmt.Sprintf("TAG_%d", i),
					TagID:   uint32(i + 1000),
				}
			}

			info := GeneratePerfectHash("TEST", entries)
			// check if we have a valid table (size >= n)
			if info.TableSize < uint32(n) {
				t.Errorf("Table too small: %d < %d", info.TableSize, n)
			}
			t.Logf("Success! Seed: %d, TableSize: %d", info.Seed, info.TableSize)

			// 验证查找一致性
			foundCount := 0
			for _, e := range entries {
				h := computeHash(e.RawKey, info.Seed, info.TableSize)
				if info.Entries[h].TagID != e.TagID {
					t.Errorf("Hash mismatch for %s at index %d (Expected ID %d, Got %d)", e.TagName, h, e.TagID, info.Entries[h].TagID)
				} else {
					foundCount++
				}
			}
			if foundCount != n {
				t.Errorf("Expected %d found entries, got %d", n, foundCount)
			}
		})
	}
}

// 验证物理键碰撞检测
func TestGeneratePerfectHash_Collision(t *testing.T) {
	defer func() {
		if r := recover(); r == nil {
			t.Errorf("The code did not panic on physical key collision")
		}
	}()

	entries := []HashEntry{
		{RawKey: 100, TagName: "TAG_A", TagID: 1},
		{RawKey: 100, TagName: "TAG_B", TagID: 2}, // 物理地址相同，Tag 不同
	}
	GeneratePerfectHash("TEST_PROTO", entries)
}
