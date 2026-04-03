#include <Arduino.h>
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "clock.h"

struct HeapLogEntry {
    unsigned long timestamp; // millis() when logged
    size_t freeHeap;
    size_t minFreeHeap;
    size_t largestBlock;
};

// Keep last N entries for trend (e.g., 24 entries = last 24 hours if interval = 1h)
#define MAX_HEAP_LOG 24
HeapLogEntry heapLog[MAX_HEAP_LOG];
uint8_t heapLogIndex = 0;

// Logging interval in milliseconds (e.g., 1h = 3600000 ms)
const unsigned long HEAP_LOG_INTERVAL = 15 * 60 * 1000UL; // 15 min for demo
unsigned long lastHeapLogTime = 0;

// Call this inside loop() or a dedicated task
void logHeapDrift() {
    unsigned long now = millis();
    if (now - lastHeapLogTime < HEAP_LOG_INTERVAL) return; // wait interval
    lastHeapLogTime = now;

    size_t freeHeap = ESP.getFreeHeap();
    size_t minFreeHeap = ESP.getMinFreeHeap();
    size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    // Only log if meaningful change (threshold)
    static size_t lastMinFree = 0;
    if (abs((int)minFreeHeap - (int)lastMinFree) < 32) return;
    lastMinFree = minFreeHeap;

    // Save into circular log
    heapLog[heapLogIndex] = { now, freeHeap, minFreeHeap, largestBlock };
    heapLogIndex = (heapLogIndex + 1) % MAX_HEAP_LOG;

    // Print summary
    Serial.printf("[Heap Trend] t=%lu ms | Free: %u | Min: %u | Largest: %u\n",
                  now, freeHeap, minFreeHeap, largestBlock);
}

// Optional: dump full trend (e.g., every 24 entries or via serial command)
void dumpHeapTrend() {
    Serial.println(F("\n[Heap Trend Dump]"));
    for (uint8_t i = 0; i < MAX_HEAP_LOG; i++) {
        HeapLogEntry &e = heapLog[i];
        if (e.timestamp == 0) continue; // skip empty
        Serial.printf("%2u) t=%lu ms | Free: %u | Min: %u | Largest: %u\n",
                      i, e.timestamp, e.freeHeap, e.minFreeHeap, e.largestBlock);
    }
}


void check_memory_safety() {
    // Get the largest contiguous block of internal memory & DMA
    size_t free_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    size_t free_dma = heap_caps_get_free_size(MALLOC_CAP_DMA);
    
    // Reboot if the largest block is under 15KB or DMA is under 5KB
    if (free_block < 15000 || free_dma < 5000) { 
        logStatusMessage("SafetyRestart");
        printf("Memory too fragmented (%u bytes). Restarting...\n", free_block);
        vTaskDelay(pdMS_TO_TICKS(3000)); 
        esp_restart();
    }
}

void memory_monitor_task(void *pvParameters) {
    while (1) {
        check_memory_safety();
        vTaskDelay(pdMS_TO_TICKS(600000)); // Check every 600 seconds
    }
}