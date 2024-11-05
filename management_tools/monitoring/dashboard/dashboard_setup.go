package dashboard

import (
	"fmt"
	"html/template"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"sync"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

// Dashboard holds the configuration and HTTP server for the monitoring dashboard
type Dashboard struct {
	metricsRegistry *prometheus.Registry
	server          *http.Server
	mu              sync.Mutex
	templates       *template.Template
}

// NewDashboard creates a new dashboard instance with default settings
func NewDashboard() *Dashboard {
	d := &Dashboard{
		metricsRegistry: prometheus.NewRegistry(),
	}
	d.setupDefaultMetrics()
	return d
}

// Start starts the HTTP server for the dashboard
func (d *Dashboard) Start(port int) error {
	d.mu.Lock()
	defer d.mu.Unlock()

	mux := http.NewServeMux()
	mux.Handle("/metrics", promhttp.HandlerFor(d.metricsRegistry, promhttp.HandlerOpts{}))
	mux.HandleFunc("/", d.dashboardHandler)

	d.server = &http.Server{
		Addr:    fmt.Sprintf(":%d", port),
		Handler: mux,
	}

	log.Printf("Starting dashboard server on port %d", port)
	return d.server.ListenAndServe()
}

// Stop stops the HTTP server gracefully
func (d *Dashboard) Stop() error {
	d.mu.Lock()
	defer d.mu.Unlock()

	if d.server == nil {
		return nil
	}
	log.Println("Stopping dashboard server")
	return d.server.Shutdown(nil)
}

// setupDefaultMetrics sets up basic Prometheus metrics for monitoring
func (d *Dashboard) setupDefaultMetrics() {
	goRequestsTotal := prometheus.NewCounterVec(
		prometheus.CounterOpts{
			Name: "go_http_requests_total",
			Help: "Number of HTTP requests processed by the Go application",
		},
		[]string{"path"},
	)
	d.metricsRegistry.MustRegister(goRequestsTotal)

	goRequestDuration := prometheus.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "go_http_request_duration_seconds",
			Help:    "Duration of HTTP requests in seconds",
			Buckets: prometheus.DefBuckets,
		},
		[]string{"path"},
	)
	d.metricsRegistry.MustRegister(goRequestDuration)

	http.DefaultServeMux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		goRequestsTotal.WithLabelValues(r.URL.Path).Inc()
		timer := prometheus.NewTimer(goRequestDuration.WithLabelValues(r.URL.Path))
		defer timer.ObserveDuration()

		http.ServeFile(w, r, "index.html")
	})
}

// dashboardHandler renders the HTML dashboard page
func (d *Dashboard) dashboardHandler(w http.ResponseWriter, r *http.Request) {
	tmplPath := filepath.Join("templates", "dashboard.html")
	if _, err := os.Stat(tmplPath); os.IsNotExist(err) {
		http.Error(w, "Dashboard template not found", http.StatusInternalServerError)
		return
	}

	if err := d.templates.ExecuteTemplate(w, "dashboard.html", nil); err != nil {
		http.Error(w, "Failed to render template", http.StatusInternalServerError)
	}
}

// RenderDashboard renders the dashboard HTML and starts the dashboard server
func (d *Dashboard) RenderDashboard() error {
	tmpl, err := template.ParseFiles(filepath.Join("templates", "dashboard.html"))
	if err != nil {
		log.Fatal("Error loading template files:", err)
		return err
	}

	d.mu.Lock()
	d.templates = tmpl
	d.mu.Unlock()

	// Start the dashboard server
	err = d.Start(8080)
	if err != nil {
		log.Fatal("Failed to start dashboard server:", err)
		return err
	}

	return nil
}

func main() {
	// Create a new dashboard instance
	dashboard := NewDashboard()

	// Serve the dashboard on port 8080
	if err := dashboard.RenderDashboard(); err != nil {
		log.Fatal(err)
	}

	// Wait for a termination signal and shutdown the dashboard
	stop := make(chan os.Signal, 1)
	<-stop
	if err := dashboard.Stop(); err != nil {
		log.Fatal(err)
	}
}
