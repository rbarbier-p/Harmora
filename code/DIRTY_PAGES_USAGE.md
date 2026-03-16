# Dirty Pages Optimization Guide

## How Dirty Pages Mode Works

When `DISPLAY_MODE_DIRTYPAGES` is enabled:
- `display_update()` only sends pages marked as "dirty" (changed)
- Each drawing operation marks affected pages as dirty
- `display_clear()` marks ALL 8 pages dirty (defeats the optimization!)

## Performance Comparison

| Scenario | Pages Sent | Expected Time |
|----------|-----------|---------------|
| Full screen update (8 pages) | 8 | ~2175µs |
| Single page (y=0-7) | 1 | ~270µs |
| Three pages (e.g., pages 0,6,7) | 3 | ~815µs |

---

## Approach 1: Clear Only Text Regions ⭐ **RECOMMENDED**

Instead of `display_clear()`, clear only the rectangular areas where text changes.

### Example: Your Encoder Display

```c
// BEFORE (slow - updates all 8 pages):
display_clear();  // Marks all pages dirty!
display_draw_string(0, 0, "E1:1234 E2:5678");
display_draw_string(0, 48, "Scan: 123us");
display_draw_string(0, 57, "update: 2175us");
display_update();  // Sends 8 pages = ~2175µs

// AFTER (fast - updates only 3 pages):
display_clear_rect(0, 0, 128, 8);   // Clear page 0 (y=0-7)
display_draw_string(0, 0, "E1:1234 E2:5678");

display_clear_rect(0, 48, 128, 8);  // Clear page 6 (y=48-55)
display_draw_string(0, 48, "Scan: 123us");

display_clear_rect(0, 57, 128, 7);  // Clear page 7 (y=57-63)
display_draw_string(0, 57, "update: 2175us");

display_update();  // Sends only pages 0,6,7 = ~815µs (2.7x faster!)
```

### When to Use
- **Best for:** Updating specific regions of the screen
- **Perfect for:** Your encoder counter display
- **Benefit:** 2-8x faster depending on how many pages you update

---

## Approach 2: Overwrite with Spaces (No Extra Code)

If text is always the same length, you can overwrite without clearing.

```c
// Text at same position always has same length
display_draw_string(0, 0, "E1:   0 E2:   0");  // First time
// ... later ...
display_draw_string(0, 0, "E1:1234 E2:5678");  // Overwrites old text
display_update();
```

### Pros & Cons
- ✅ No need to clear
- ✅ Simple
- ❌ Only works if text length is fixed
- ❌ Leaves artifacts if new text is shorter

---

## Approach 3: Track Previous Values (Most Efficient)

Only redraw when values actually change.

```c
static int16_t last_encoder_values[6] = {0};
static uint8_t force_redraw = 1;

// In loop:
uint8_t changed = 0;
for (uint8_t i = 0; i < 6; i++) {
    if (encoder_values[i] != last_encoder_values[i]) {
        last_encoder_values[i] = encoder_values[i];
        changed = 1;
    }
}

if (changed || force_redraw) {
    // Clear and redraw only changed lines
    display_clear_rect(0, 0, 128, 8);
    snprintf(buf, sizeof(buf), "E1:%4d E2:%4d", encoder_values[0], encoder_values[1]);
    display_draw_string(0, 0, buf);
    display_update();
    force_redraw = 0;
}
```

### Pros & Cons
- ✅ Minimal updates
- ✅ Best performance
- ❌ More complex code
- ❌ Requires storing previous state (12-24 bytes RAM)

---

## Approach 4: Page-Aligned Layout (Advanced)

Design your UI so elements align to page boundaries (every 8 pixels).

```c
// Layout at page boundaries:
// Page 0 (y=0-7):   "E1:1234 E2:5678"
// Page 1 (y=8-15):  "E3:1234 E4:5678"
// Page 6 (y=48-55): "Scan: 123us"
// Page 7 (y=56-63): "update: 815us"

// Only update pages that changed:
if (encoders_1_2_changed) {
    display_clear_rect(0, 0, 128, 8);
    display_draw_string(0, 0, buf_line0);
}

if (stats_changed) {
    display_clear_rect(0, 56, 128, 8);
    display_draw_string(0, 56, buf_stats);
}

display_update();  // Only sends changed pages
```

---

## Recommended Implementation for Your Encoder Display

Based on your current code (atmega328p.c:182-205), here's the optimized version:

```c
// Instead of display_clear(), clear only the 3 text regions:

if (any_change || display_update_needed) {
    // Clear only the regions we're updating
    display_clear_rect(0, 0, 128, 8);    // Page 0: encoder line 1
    display_clear_rect(0, 48, 128, 8);   // Page 6: encoder line 2  
    display_clear_rect(0, 57, 128, 7);   // Page 7: stats line

    // Format strings
    snprintf(buf[0], sizeof(buf[0]), "E1:%4d E2:%4d", click_counter[0], click_counter[1]);
    snprintf(buf[1], sizeof(buf[1]), "EN5:%4d EN6:%4d", click_counter[4], click_counter[5]);
    snprintf(buf[3], sizeof(buf[3]), "update: %3dus", update_time_us);
    
    // Draw text (marks pages 0,6,7 as dirty)
    display_draw_string(0, 0, buf[0]);
    display_draw_string(0, 48, buf[1]);
    display_draw_string(0, 57, buf[3]);
    
    // Measure update time
    uint16_t update_start = stopwatch_read();
    display_update();  // Only sends pages 0,6,7 (~815µs instead of 2175µs)
    uint16_t update_end = stopwatch_read();
    
    display_update_needed = 0;
    update_time_us = stopwatch_ticks_to_us(update_end - update_start);
}
```

**Expected results:**
- Before: `display_update()` = ~2175µs (8 pages)
- After: `display_update()` = ~815µs (3 pages) ✨ **2.7x faster!**

---

## API Reference

### New Functions

```c
// Clear a rectangular region (set pixels to 0)
void display_clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

// Fill a rectangular region (set pixels to 1)
void display_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
```

### Existing Functions

```c
// Clear entire screen (marks ALL pages dirty - avoid in dirty pages mode!)
void display_clear(void);

// Send dirty pages to display
void display_update(void);
```

---

## Summary

| Approach | Speed Gain | RAM Cost | Complexity | Recommended? |
|----------|-----------|----------|------------|--------------|
| Clear regions only | 2-8x | 0 bytes | Low | ✅ **YES** |
| Overwrite with spaces | 2-8x | 0 bytes | Very Low | If fixed-width |
| Track previous values | 2-10x | 12-50 bytes | Medium | If RAM available |
| Page-aligned layout | 2-8x | 0 bytes | Medium | For new UIs |

**Bottom line:** Use `display_clear_rect()` instead of `display_clear()` for 2-3x faster updates with zero RAM cost!
