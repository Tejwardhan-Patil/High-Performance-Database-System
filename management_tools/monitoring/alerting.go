package monitoring

import (
	"fmt"
	"log"
	"net/smtp"
	"sync"
	"time"
)

type MetricType string

const (
	MetricTypeCPUUsage     MetricType = "CPU_USAGE"
	MetricTypeMemoryUsage  MetricType = "MEMORY_USAGE"
	MetricTypeDiskSpace    MetricType = "DISK_SPACE"
	MetricTypeResponseTime MetricType = "RESPONSE_TIME"
)

type AlertLevel string

const (
	AlertLevelWarning  AlertLevel = "WARNING"
	AlertLevelCritical AlertLevel = "CRITICAL"
)

type Alert struct {
	Metric    MetricType
	Level     AlertLevel
	Message   string
	Timestamp time.Time
}

type Threshold struct {
	Warning  float64
	Critical float64
}

type Metric struct {
	Type      MetricType
	Value     float64
	Timestamp time.Time
}

type AlertingSystem struct {
	mu          sync.Mutex
	thresholds  map[MetricType]Threshold
	alerts      []Alert
	emailConfig EmailConfig
}

type EmailConfig struct {
	SMTPServer string
	Port       int
	Username   string
	Password   string
	From       string
	To         []string
}

func NewAlertingSystem(emailConfig EmailConfig) *AlertingSystem {
	return &AlertingSystem{
		thresholds: map[MetricType]Threshold{
			MetricTypeCPUUsage:     {Warning: 70.0, Critical: 90.0},
			MetricTypeMemoryUsage:  {Warning: 75.0, Critical: 95.0},
			MetricTypeDiskSpace:    {Warning: 80.0, Critical: 95.0},
			MetricTypeResponseTime: {Warning: 200.0, Critical: 500.0},
		},
		emailConfig: emailConfig,
	}
}

func (as *AlertingSystem) CollectMetric(metric Metric) {
	as.mu.Lock()
	defer as.mu.Unlock()

	threshold, exists := as.thresholds[metric.Type]
	if !exists {
		log.Printf("No thresholds set for metric type: %s\n", metric.Type)
		return
	}

	alert := as.evaluateThresholds(metric, threshold)
	if alert != nil {
		as.alerts = append(as.alerts, *alert)
		as.sendAlert(alert)
	}
}

func (as *AlertingSystem) evaluateThresholds(metric Metric, threshold Threshold) *Alert {
	if metric.Value >= threshold.Critical {
		return &Alert{
			Metric:    metric.Type,
			Level:     AlertLevelCritical,
			Message:   fmt.Sprintf("%s has reached a critical value of %.2f", metric.Type, metric.Value),
			Timestamp: metric.Timestamp,
		}
	} else if metric.Value >= threshold.Warning {
		return &Alert{
			Metric:    metric.Type,
			Level:     AlertLevelWarning,
			Message:   fmt.Sprintf("%s has reached a warning value of %.2f", metric.Type, metric.Value),
			Timestamp: metric.Timestamp,
		}
	}
	return nil
}

func (as *AlertingSystem) sendAlert(alert *Alert) {
	message := fmt.Sprintf(
		"To: %s\r\nSubject: %s Alert: %s\r\n\r\n%s occurred at %s with the message: %s\r\n",
		as.emailConfig.To,
		alert.Level,
		alert.Metric,
		alert.Level,
		alert.Timestamp.Format(time.RFC822),
		alert.Message,
	)

	auth := smtp.PlainAuth("", as.emailConfig.Username, as.emailConfig.Password, as.emailConfig.SMTPServer)

	err := smtp.SendMail(
		fmt.Sprintf("%s:%d", as.emailConfig.SMTPServer, as.emailConfig.Port),
		auth,
		as.emailConfig.From,
		as.emailConfig.To,
		[]byte(message),
	)
	if err != nil {
		log.Printf("Failed to send alert email: %v", err)
	} else {
		log.Printf("Alert email sent for %s alert on %s", alert.Level, alert.Metric)
	}
}

func (as *AlertingSystem) GetRecentAlerts() []Alert {
	as.mu.Lock()
	defer as.mu.Unlock()

	return as.alerts
}

func (as *AlertingSystem) ConfigureThresholds(metricType MetricType, warning float64, critical float64) {
	as.mu.Lock()
	defer as.mu.Unlock()

	as.thresholds[metricType] = Threshold{
		Warning:  warning,
		Critical: critical,
	}

	log.Printf("Thresholds updated for %s: Warning = %.2f, Critical = %.2f", metricType, warning, critical)
}
