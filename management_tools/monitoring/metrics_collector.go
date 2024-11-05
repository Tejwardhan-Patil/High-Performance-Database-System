package monitoring

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"sync"
	"time"

	"github.com/shirou/gopsutil/cpu"
	"github.com/shirou/gopsutil/disk"
	"github.com/shirou/gopsutil/mem"
	"github.com/shirou/gopsutil/net"
)

// MetricsData stores all the metrics
type MetricsData struct {
	CPUUsage     []float64            `json:"cpu_usage"`
	MemoryUsage  uint64               `json:"memory_usage"`
	DiskUsage    uint64               `json:"disk_usage"`
	NetworkStats []net.IOCountersStat `json:"network_stats"`
	Timestamp    time.Time            `json:"timestamp"`
}

// MetricsCollector collects and stores system performance metrics
type MetricsCollector struct {
	data       []MetricsData
	dataLock   sync.Mutex
	interval   time.Duration
	dataLimit  int
	stopSignal chan struct{}
}

// NewMetricsCollector creates a new MetricsCollector
func NewMetricsCollector(interval time.Duration, dataLimit int) *MetricsCollector {
	return &MetricsCollector{
		data:       make([]MetricsData, 0),
		interval:   interval,
		dataLimit:  dataLimit,
		stopSignal: make(chan struct{}),
	}
}

// Start begins the metric collection process
func (mc *MetricsCollector) Start() {
	ticker := time.NewTicker(mc.interval)
	go func() {
		for {
			select {
			case <-ticker.C:
				mc.collect()
			case <-mc.stopSignal:
				ticker.Stop()
				return
			}
		}
	}()
}

// Stop stops the metric collection process
func (mc *MetricsCollector) Stop() {
	close(mc.stopSignal)
}

// collect collects system metrics and stores them in the MetricsCollector
func (mc *MetricsCollector) collect() {
	cpuUsage, err := mc.collectCPUUsage()
	if err != nil {
		log.Printf("Error collecting CPU usage: %v", err)
		return
	}

	memUsage, err := mc.collectMemoryUsage()
	if err != nil {
		log.Printf("Error collecting memory usage: %v", err)
		return
	}

	diskUsage, err := mc.collectDiskUsage()
	if err != nil {
		log.Printf("Error collecting disk usage: %v", err)
		return
	}

	netStats, err := mc.collectNetworkStats()
	if err != nil {
		log.Printf("Error collecting network stats: %v", err)
		return
	}

	mc.dataLock.Lock()
	defer mc.dataLock.Unlock()

	// Limit the number of stored data points
	if len(mc.data) >= mc.dataLimit {
		mc.data = mc.data[1:]
	}

	mc.data = append(mc.data, MetricsData{
		CPUUsage:     cpuUsage,
		MemoryUsage:  memUsage,
		DiskUsage:    diskUsage,
		NetworkStats: netStats,
		Timestamp:    time.Now(),
	})
}

// collectCPUUsage collects CPU usage data
func (mc *MetricsCollector) collectCPUUsage() ([]float64, error) {
	percentages, err := cpu.Percent(0, false)
	if err != nil {
		return nil, err
	}
	return percentages, nil
}

// collectMemoryUsage collects memory usage data
func (mc *MetricsCollector) collectMemoryUsage() (uint64, error) {
	v, err := mem.VirtualMemory()
	if err != nil {
		return 0, err
	}
	return v.Used, nil
}

// collectDiskUsage collects disk usage data
func (mc *MetricsCollector) collectDiskUsage() (uint64, error) {
	d, err := disk.Usage("/")
	if err != nil {
		return 0, err
	}
	return d.Used, nil
}

// collectNetworkStats collects network usage data
func (mc *MetricsCollector) collectNetworkStats() ([]net.IOCountersStat, error) {
	stats, err := net.IOCounters(false)
	if err != nil {
		return nil, err
	}
	return stats, nil
}

// ServeMetrics serves the collected metrics as JSON over HTTP
func (mc *MetricsCollector) ServeMetrics(w http.ResponseWriter, r *http.Request) {
	mc.dataLock.Lock()
	defer mc.dataLock.Unlock()

	jsonData, err := json.Marshal(mc.data)
	if err != nil {
		http.Error(w, "Failed to serialize data", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.Write(jsonData)
}

// SaveMetricsToFile saves the collected metrics to a file
func (mc *MetricsCollector) SaveMetricsToFile(filePath string) error {
	mc.dataLock.Lock()
	defer mc.dataLock.Unlock()

	jsonData, err := json.Marshal(mc.data)
	if err != nil {
		return fmt.Errorf("failed to serialize data: %w", err)
	}

	err = ioutil.WriteFile(filePath, jsonData, 0644)
	if err != nil {
		return fmt.Errorf("failed to write file: %w", err)
	}

	return nil
}

// LoadMetricsFromFile loads the collected metrics from a file
func (mc *MetricsCollector) LoadMetricsFromFile(filePath string) error {
	mc.dataLock.Lock()
	defer mc.dataLock.Unlock()

	jsonData, err := ioutil.ReadFile(filePath)
	if err != nil {
		return fmt.Errorf("failed to read file: %w", err)
	}

	err = json.Unmarshal(jsonData, &mc.data)
	if err != nil {
		return fmt.Errorf("failed to deserialize data: %w", err)
	}

	return nil
}

// StartHTTPServer starts an HTTP server to expose metrics
func StartHTTPServer(mc *MetricsCollector, port int) {
	http.HandleFunc("/metrics", mc.ServeMetrics)
	log.Printf("Starting HTTP server on port %d", port)
	if err := http.ListenAndServe(fmt.Sprintf(":%d", port), nil); err != nil {
		log.Fatalf("Failed to start HTTP server: %v", err)
	}
}

func main() {
	// Interval for collecting metrics (every 10 seconds)
	interval := 10 * time.Second
	// Maximum number of metrics data points to store
	dataLimit := 100

	collector := NewMetricsCollector(interval, dataLimit)
	collector.Start()

	// Start the HTTP server for serving metrics on port 8080
	go StartHTTPServer(collector, 8080)

	// Allow the collector to run for a specified time before stopping
	time.Sleep(10 * time.Minute)
	collector.Stop()

	// Save the collected metrics to a file
	if err := collector.SaveMetricsToFile("metrics.json"); err != nil {
		log.Fatalf("Failed to save metrics to file: %v", err)
	}

	// Load the metrics from a file later
	if err := collector.LoadMetricsFromFile("metrics.json"); err != nil {
		log.Fatalf("Failed to load metrics from file: %v", err)
	}
}
