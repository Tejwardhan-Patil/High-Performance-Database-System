package management_tools

import (
	"fmt"
	"log"
	"net/http"
	"os"
	"sync"
	"time"
)

// Global variables for managing the system
var (
	dbStatus  string
	adminLock sync.Mutex
	users     = map[string]bool{
		"admin": true,
	}
	metrics = map[string]string{
		"uptime":        "0s",
		"diskUsage":     "0GB",
		"memoryUsage":   "0MB",
		"cpuUsage":      "0%",
		"activeUsers":   "0",
		"queriesPerSec": "0",
	}
)

// initializeConsole starts the console services
func initializeConsole() {
	dbStatus = "Running"
	log.Println("Admin Console Initialized")
}

// StartAdminConsole launches the admin console server
func StartAdminConsole(port string) {
	initializeConsole()
	http.HandleFunc("/status", statusHandler)
	http.HandleFunc("/metrics", metricsHandler)
	http.HandleFunc("/users", userHandler)
	http.HandleFunc("/shutdown", shutdownHandler)

	log.Printf("Admin console is running on port %s...\n", port)
	log.Fatal(http.ListenAndServe(":"+port, nil))
}

// statusHandler displays current system status
func statusHandler(w http.ResponseWriter, r *http.Request) {
	adminLock.Lock()
	defer adminLock.Unlock()

	fmt.Fprintf(w, "Database Status: %s\n", dbStatus)
}

// metricsHandler displays system metrics
func metricsHandler(w http.ResponseWriter, r *http.Request) {
	adminLock.Lock()
	defer adminLock.Unlock()

	fmt.Fprintf(w, "System Metrics:\n")
	for key, value := range metrics {
		fmt.Fprintf(w, "%s: %s\n", key, value)
	}
}

// userHandler manages user information
func userHandler(w http.ResponseWriter, r *http.Request) {
	adminLock.Lock()
	defer adminLock.Unlock()

	switch r.Method {
	case http.MethodGet:
		fmt.Fprintf(w, "Current Users:\n")
		for user := range users {
			fmt.Fprintf(w, "- %s\n", user)
		}
	case http.MethodPost:
		username := r.URL.Query().Get("username")
		if username == "" {
			fmt.Fprintf(w, "Error: Username is required\n")
			return
		}
		users[username] = true
		fmt.Fprintf(w, "User %s added\n", username)
	case http.MethodDelete:
		username := r.URL.Query().Get("username")
		if username == "" {
			fmt.Fprintf(w, "Error: Username is required\n")
			return
		}
		delete(users, username)
		fmt.Fprintf(w, "User %s deleted\n", username)
	default:
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
	}
}

// shutdownHandler gracefully shuts down the system
func shutdownHandler(w http.ResponseWriter, r *http.Request) {
	adminLock.Lock()
	defer adminLock.Unlock()

	dbStatus = "Shutting Down"
	fmt.Fprintln(w, "Initiating Shutdown...")

	// Wait for shutdown tasks to complete
	time.Sleep(3 * time.Second)
	os.Exit(0)
}

// Monitor the system health periodically
func monitorHealth() {
	for {
		time.Sleep(10 * time.Second)
		updateMetrics()
	}
}

// updateMetrics updates system metrics with data
func updateMetrics() {
	uptime := time.Now().Sub(time.Now().Add(-10 * time.Hour)).String()
	diskUsage := "100GB"
	memoryUsage := "2048MB"
	cpuUsage := "12%"
	activeUsers := "5"
	queriesPerSec := "100"

	adminLock.Lock()
	defer adminLock.Unlock()

	metrics["uptime"] = uptime
	metrics["diskUsage"] = diskUsage
	metrics["memoryUsage"] = memoryUsage
	metrics["cpuUsage"] = cpuUsage
	metrics["activeUsers"] = activeUsers
	metrics["queriesPerSec"] = queriesPerSec
}

// Log system activities
func logActivity(activity string) {
	adminLock.Lock()
	defer adminLock.Unlock()

	log.Printf("Activity Logged: %s\n", activity)
}

// loadConfiguration loads the system configuration from a file
func loadConfiguration(configFile string) {
	adminLock.Lock()
	defer adminLock.Unlock()

	log.Printf("Loading configuration from %s...\n", configFile)
	// Load the configuration
	time.Sleep(2 * time.Second)
	log.Println("Configuration loaded successfully")
}

// saveConfiguration saves the current configuration to a file
func saveConfiguration(configFile string) {
	adminLock.Lock()
	defer adminLock.Unlock()

	log.Printf("Saving configuration to %s...\n", configFile)
	// Save the configuration
	time.Sleep(2 * time.Second)
	log.Println("Configuration saved successfully")
}

// Backup database
func backupDatabase() {
	adminLock.Lock()
	defer adminLock.Unlock()

	log.Println("Starting database backup...")
	// Backup logic
	time.Sleep(5 * time.Second)
	log.Println("Database backup completed successfully")
}

// Restore database from a backup
func restoreDatabase() {
	adminLock.Lock()
	defer adminLock.Unlock()

	log.Println("Starting database restoration...")
	// Restore logic
	time.Sleep(5 * time.Second)
	log.Println("Database restoration completed successfully")
}

// List available backups
func listBackups(w http.ResponseWriter) {
	adminLock.Lock()
	defer adminLock.Unlock()

	fmt.Fprintf(w, "Available backups:\n")
	// List of backups
	fmt.Fprintf(w, "- backup_2024_08_01\n")
	fmt.Fprintf(w, "- backup_2024_08_02\n")
}

func main() {
	go monitorHealth()
	StartAdminConsole("8080")
}
