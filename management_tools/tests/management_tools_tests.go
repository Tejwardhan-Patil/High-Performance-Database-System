package management_tools_tests

import (
	"bytes"
	"encoding/json"
	"management_tools"
	"net/http"
	"net/http/httptest"
	"testing"
)

// Test case for Admin Console initialization
func TestAdminConsoleInitialization(t *testing.T) {
	admin := management_tools.NewAdminConsole()

	if admin == nil {
		t.Fatalf("Expected AdminConsole instance, got nil")
	}

	if admin.Port != 8080 {
		t.Errorf("Expected default port 8080, got %d", admin.Port)
	}
}

// Test case for starting Admin Console
func TestAdminConsoleStart(t *testing.T) {
	admin := management_tools.NewAdminConsole()

	server := httptest.NewServer(http.HandlerFunc(admin.Handler))
	defer server.Close()

	resp, err := http.Get(server.URL)
	if err != nil {
		t.Fatalf("Failed to start AdminConsole: %v", err)
	}

	if resp.StatusCode != http.StatusOK {
		t.Errorf("Expected status OK, got %v", resp.StatusCode)
	}
}

// Test case for handling metrics collection
func TestMetricsCollector(t *testing.T) {
	collector := management_tools.NewMetricsCollector()
	collector.CollectMetrics()

	metrics := collector.GetMetrics()

	if metrics.CPUUsage <= 0 {
		t.Errorf("Expected positive CPU usage, got %v", metrics.CPUUsage)
	}

	if metrics.MemoryUsage <= 0 {
		t.Errorf("Expected positive Memory usage, got %v", metrics.MemoryUsage)
	}
}

// Test case for alert generation
func TestAlerting(t *testing.T) {
	alert := management_tools.NewAlertingSystem()

	alert.Trigger("High CPU Usage", 90)

	alerts := alert.GetActiveAlerts()
	if len(alerts) != 1 {
		t.Fatalf("Expected 1 active alert, got %v", len(alerts))
	}

	if alerts[0].Message != "High CPU Usage" {
		t.Errorf("Expected alert message 'High CPU Usage', got %v", alerts[0].Message)
	}

	if alerts[0].Severity != "Critical" {
		t.Errorf("Expected 'Critical' severity, got %v", alerts[0].Severity)
	}
}

// Test case for dashboard JSON response
func TestDashboardMetrics(t *testing.T) {
	dashboard := management_tools.NewDashboard()
	server := httptest.NewServer(http.HandlerFunc(dashboard.MetricsHandler))
	defer server.Close()

	resp, err := http.Get(server.URL)
	if err != nil {
		t.Fatalf("Failed to get response from dashboard: %v", err)
	}

	if resp.StatusCode != http.StatusOK {
		t.Fatalf("Expected status OK, got %v", resp.StatusCode)
	}

	var metrics map[string]interface{}
	err = json.NewDecoder(resp.Body).Decode(&metrics)
	if err != nil {
		t.Fatalf("Failed to decode metrics: %v", err)
	}

	if metrics["cpu_usage"] == nil {
		t.Errorf("Expected cpu_usage in metrics, got nil")
	}
}

// Test for Admin Console POST method
func TestAdminConsolePost(t *testing.T) {
	admin := management_tools.NewAdminConsole()
	server := httptest.NewServer(http.HandlerFunc(admin.Handler))
	defer server.Close()

	payload := map[string]string{"action": "restart"}
	body, _ := json.Marshal(payload)

	req, err := http.NewRequest("POST", server.URL, bytes.NewBuffer(body))
	if err != nil {
		t.Fatalf("Failed to create POST request: %v", err)
	}

	req.Header.Set("Content-Type", "application/json")
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		t.Fatalf("POST request failed: %v", err)
	}

	if resp.StatusCode != http.StatusOK {
		t.Errorf("Expected status OK, got %v", resp.StatusCode)
	}
}

// Test for handling large number of metrics
func TestMetricsCollectorLoad(t *testing.T) {
	collector := management_tools.NewMetricsCollector()
	for i := 0; i < 1000; i++ {
		collector.CollectMetrics()
	}

	metrics := collector.GetMetrics()

	if metrics.CPUUsage <= 0 {
		t.Errorf("Expected positive CPU usage, got %v", metrics.CPUUsage)
	}
}

// Test case for alert resolution
func TestAlertResolution(t *testing.T) {
	alert := management_tools.NewAlertingSystem()

	alert.Trigger("Low Disk Space", 5)
	alert.Resolve("Low Disk Space")

	activeAlerts := alert.GetActiveAlerts()
	if len(activeAlerts) != 0 {
		t.Fatalf("Expected 0 active alerts after resolution, got %v", len(activeAlerts))
	}
}

// Test for Dashboard performance under high load
func TestDashboardLoad(t *testing.T) {
	dashboard := management_tools.NewDashboard()
	server := httptest.NewServer(http.HandlerFunc(dashboard.MetricsHandler))
	defer server.Close()

	var err error
	for i := 0; i < 1000; i++ {
		_, err = http.Get(server.URL)
		if err != nil {
			t.Fatalf("Failed to handle request %v: %v", i, err)
		}
	}
}

// Test case for handling high latency alerts
func TestHighLatencyAlert(t *testing.T) {
	alert := management_tools.NewAlertingSystem()

	alert.Trigger("High Latency", 200)

	alerts := alert.GetActiveAlerts()
	if len(alerts) == 0 {
		t.Fatalf("Expected active alerts for high latency, found none")
	}

	if alerts[0].Message != "High Latency" {
		t.Errorf("Expected alert message 'High Latency', got %v", alerts[0].Message)
	}
}

// Test case for validating correct alert priority
func TestAlertPriority(t *testing.T) {
	alert := management_tools.NewAlertingSystem()

	alert.Trigger("Disk Failure", 100)

	alerts := alert.GetActiveAlerts()
	if alerts[0].Severity != "Critical" {
		t.Errorf("Expected 'Critical' severity, got %v", alerts[0].Severity)
	}

	alert.Trigger("High CPU Usage", 80)
	if alerts[1].Severity != "Warning" {
		t.Errorf("Expected 'Warning' severity, got %v", alerts[1].Severity)
	}
}

// Test case for ensuring alert deduplication
func TestAlertDeduplication(t *testing.T) {
	alert := management_tools.NewAlertingSystem()

	alert.Trigger("Memory Leak", 95)
	alert.Trigger("Memory Leak", 95) // Duplicate alert

	alerts := alert.GetActiveAlerts()
	if len(alerts) != 1 {
		t.Errorf("Expected 1 alert after deduplication, got %v", len(alerts))
	}
}
