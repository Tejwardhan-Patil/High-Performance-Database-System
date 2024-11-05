package main

import (
	"fmt"
	"os"
	"time"
)

type Firewall struct {
	whitelistedIPs   map[string]bool
	whitelistedPorts map[int]bool
	blockedIPs       map[string]bool
	logFile          *os.File
}

// Initialize a new firewall with whitelist IPs and Ports
func NewFirewall(whitelistedIPs []string, whitelistedPorts []int, logFilePath string) *Firewall {
	fw := &Firewall{
		whitelistedIPs:   make(map[string]bool),
		whitelistedPorts: make(map[int]bool),
		blockedIPs:       make(map[string]bool),
	}
	// Add whitelisted IPs
	for _, ip := range whitelistedIPs {
		fw.whitelistedIPs[ip] = true
	}
	// Add whitelisted Ports
	for _, port := range whitelistedPorts {
		fw.whitelistedPorts[port] = true
	}
	// Set up log file
	var err error
	fw.logFile, err = os.OpenFile(logFilePath, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		fmt.Println("Error opening log file:", err)
		return nil
	}

	return fw
}

// Log unauthorized access attempts
func (fw *Firewall) logUnauthorizedAccess(ip string, port int, reason string) {
	timestamp := time.Now().Format(time.RFC3339)
	logEntry := fmt.Sprintf("%s - Unauthorized access from %s on port %d: %s\n", timestamp, ip, port, reason)
	fw.logFile.WriteString(logEntry)
}

// Check if IP is whitelisted
func (fw *Firewall) isIPWhitelisted(ip string) bool {
	return fw.whitelistedIPs[ip]
}

// Check if Port is whitelisted
func (fw *Firewall) isPortWhitelisted(port int) bool {
	return fw.whitelistedPorts[port]
}

// Block IP manually
func (fw *Firewall) BlockIP(ip string) {
	fw.blockedIPs[ip] = true
}

// Unblock IP manually
func (fw *Firewall) UnblockIP(ip string) {
	delete(fw.blockedIPs, ip)
}

// Check if IP is blocked
func (fw *Firewall) isIPBlocked(ip string) bool {
	return fw.blockedIPs[ip]
}

// Simulate incoming network connections
func (fw *Firewall) SimulateConnection(ip string, port int) {
	if fw.isIPBlocked(ip) {
		fmt.Printf("Connection from %s blocked.\n", ip)
		fw.logUnauthorizedAccess(ip, port, "Blocked IP")
		return
	}

	if !fw.isIPWhitelisted(ip) {
		fmt.Printf("Connection from %s not whitelisted.\n", ip)
		fw.logUnauthorizedAccess(ip, port, "IP not whitelisted")
		return
	}

	if !fw.isPortWhitelisted(port) {
		fmt.Printf("Connection on port %d from %s is not whitelisted.\n", port, ip)
		fw.logUnauthorizedAccess(ip, port, "Port not whitelisted")
		return
	}

	fmt.Printf("Connection from %s on port %d is allowed.\n", ip, port)
}

// Close the log file
func (fw *Firewall) Close() {
	fw.logFile.Close()
}

func main() {
	// Whitelisted IPs and Ports
	whitelistedIPs := []string{"192.168.1.100", "10.0.0.5", "172.16.0.1"}
	whitelistedPorts := []int{80, 443, 22}

	// Initialize firewall
	firewall := NewFirewall(whitelistedIPs, whitelistedPorts, "firewall_log.txt")
	defer firewall.Close()

	// Simulate network connections
	firewall.SimulateConnection("192.168.1.100", 80)
	firewall.SimulateConnection("192.168.1.101", 80)
	firewall.SimulateConnection("192.168.1.100", 8080)
	firewall.SimulateConnection("10.0.0.5", 443)

	// Block and test blocking functionality
	firewall.BlockIP("192.168.1.101")
	firewall.SimulateConnection("192.168.1.101", 80)
	firewall.SimulateConnection("192.168.1.100", 443)

	// Manually unblock and recheck connection
	firewall.UnblockIP("192.168.1.101")
	firewall.SimulateConnection("192.168.1.101", 80)
}
