package config

import (
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"path/filepath"
	"strings"
	"time"
	"yaml"

	"gopkg.in/yaml.v2"
)

// Config represents the configuration structure
type Config struct {
	Server   ServerConfig   `yaml:"server" json:"server"`
	Database DatabaseConfig `yaml:"database" json:"database"`
	Logging  LoggingConfig  `yaml:"logging" json:"logging"`
	Security SecurityConfig `yaml:"security" json:"security"`
}

// ServerConfig holds server-specific configuration
type ServerConfig struct {
	Host         string        `yaml:"host" json:"host"`
	Port         int           `yaml:"port" json:"port"`
	Timeout      time.Duration `yaml:"timeout" json:"timeout"`
	ReadTimeout  time.Duration `yaml:"read_timeout" json:"read_timeout"`
	WriteTimeout time.Duration `yaml:"write_timeout" json:"write_timeout"`
}

// DatabaseConfig holds database-specific configuration
type DatabaseConfig struct {
	Driver   string `yaml:"driver" json:"driver"`
	Host     string `yaml:"host" json:"host"`
	Port     int    `yaml:"port" json:"port"`
	Username string `yaml:"username" json:"username"`
	Password string `yaml:"password" json:"password"`
	Name     string `yaml:"name" json:"name"`
}

// LoggingConfig holds logging configuration
type LoggingConfig struct {
	Level  string `yaml:"level" json:"level"`
	Format string `yaml:"format" json:"format"`
	Output string `yaml:"output" json:"output"`
}

// SecurityConfig holds security-related settings
type SecurityConfig struct {
	EnableTLS      bool   `yaml:"enable_tls" json:"enable_tls"`
	TLSCertPath    string `yaml:"tls_cert_path" json:"tls_cert_path"`
	TLSKeyPath     string `yaml:"tls_key_path" json:"tls_key_path"`
	AllowedOrigins string `yaml:"allowed_origins" json:"allowed_origins"`
}

// LoadConfig loads configuration from a given file path
func LoadConfig(configPath string) (*Config, error) {
	ext := strings.ToLower(filepath.Ext(configPath))

	if ext == ".yaml" || ext == ".yml" {
		return loadYAMLConfig(configPath)
	} else if ext == ".json" {
		return loadJSONConfig(configPath)
	} else {
		return nil, errors.New("unsupported config file format")
	}
}

// loadYAMLConfig loads configuration from a YAML file
func loadYAMLConfig(path string) (*Config, error) {
	fileData, err := ioutil.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("failed to read config file: %w", err)
	}

	var config Config
	if err := yaml.Unmarshal(fileData, &config); err != nil {
		return nil, fmt.Errorf("failed to unmarshal yaml config: %w", err)
	}

	return &config, nil
}

// loadJSONConfig loads configuration from a JSON file
func loadJSONConfig(path string) (*Config, error) {
	fileData, err := ioutil.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("failed to read config file: %w", err)
	}

	var config Config
	if err := json.Unmarshal(fileData, &config); err != nil {
		return nil, fmt.Errorf("failed to unmarshal json config: %w", err)
	}

	return &config, nil
}

// Validate validates the loaded configuration
func (c *Config) Validate() error {
	if c.Server.Host == "" {
		return errors.New("server host cannot be empty")
	}
	if c.Server.Port == 0 {
		return errors.New("server port cannot be zero")
	}
	if c.Database.Driver == "" {
		return errors.New("database driver cannot be empty")
	}
	if c.Database.Host == "" {
		return errors.New("database host cannot be empty")
	}
	if c.Logging.Level == "" {
		return errors.New("logging level cannot be empty")
	}
	if c.Security.EnableTLS && (c.Security.TLSCertPath == "" || c.Security.TLSKeyPath == "") {
		return errors.New("TLS is enabled, but certificate and key paths are not provided")
	}
	return nil
}

// SaveConfig saves the configuration to a specified file format
func SaveConfig(config *Config, configPath string) error {
	ext := strings.ToLower(filepath.Ext(configPath))

	if ext == ".yaml" || ext == ".yml" {
		return saveYAMLConfig(config, configPath)
	} else if ext == ".json" {
		return saveJSONConfig(config, configPath)
	} else {
		return errors.New("unsupported config file format")
	}
}

// saveYAMLConfig saves the configuration in YAML format
func saveYAMLConfig(config *Config, path string) error {
	fileData, err := yaml.Marshal(config)
	if err != nil {
		return fmt.Errorf("failed to marshal yaml config: %w", err)
	}

	if err := ioutil.WriteFile(path, fileData, 0644); err != nil {
		return fmt.Errorf("failed to write config file: %w", err)
	}

	return nil
}

// saveJSONConfig saves the configuration in JSON format
func saveJSONConfig(config *Config, path string) error {
	fileData, err := json.MarshalIndent(config, "", "  ")
	if err != nil {
		return fmt.Errorf("failed to marshal json config: %w", err)
	}

	if err := ioutil.WriteFile(path, fileData, 0644); err != nil {
		return fmt.Errorf("failed to write config file: %w", err)
	}

	return nil
}

// GetConfigValue retrieves a value from the config based on dot notation ("server.host")
func (c *Config) GetConfigValue(key string) (interface{}, error) {
	parts := strings.Split(key, ".")
	var value interface{} = c

	for _, part := range parts {
		switch v := value.(type) {
		case *Config:
			switch part {
			case "server":
				value = &v.Server
			case "database":
				value = &v.Database
			case "logging":
				value = &v.Logging
			case "security":
				value = &v.Security
			default:
				return nil, fmt.Errorf("unknown config key: %s", part)
			}
		case *ServerConfig:
			switch part {
			case "host":
				value = v.Host
			case "port":
				value = v.Port
			case "timeout":
				value = v.Timeout
			case "read_timeout":
				value = v.ReadTimeout
			case "write_timeout":
				value = v.WriteTimeout
			default:
				return nil, fmt.Errorf("unknown server config key: %s", part)
			}
		case *DatabaseConfig:
			switch part {
			case "driver":
				value = v.Driver
			case "host":
				value = v.Host
			case "port":
				value = v.Port
			case "username":
				value = v.Username
			case "password":
				value = v.Password
			case "name":
				value = v.Name
			default:
				return nil, fmt.Errorf("unknown database config key: %s", part)
			}
		case *LoggingConfig:
			switch part {
			case "level":
				value = v.Level
			case "format":
				value = v.Format
			case "output":
				value = v.Output
			default:
				return nil, fmt.Errorf("unknown logging config key: %s", part)
			}
		case *SecurityConfig:
			switch part {
			case "enable_tls":
				value = v.EnableTLS
			case "tls_cert_path":
				value = v.TLSCertPath
			case "tls_key_path":
				value = v.TLSKeyPath
			case "allowed_origins":
				value = v.AllowedOrigins
			default:
				return nil, fmt.Errorf("unknown security config key: %s", part)
			}
		default:
			return nil, fmt.Errorf("invalid config key path: %s", key)
		}
	}
	return value, nil
}
