package encryption

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
)

// Export LoadServerTLSConfig
func LoadServerTLSConfig(certFile, keyFile, caCertFile string) (*tls.Config, error) {
	cert, err := tls.LoadX509KeyPair(certFile, keyFile)
	if err != nil {
		return nil, fmt.Errorf("failed to load server certificate and key: %v", err)
	}

	caCert, err := ioutil.ReadFile(caCertFile)
	if err != nil {
		return nil, fmt.Errorf("failed to read CA cert: %v", err)
	}

	caCertPool := x509.NewCertPool()
	caCertPool.AppendCertsFromPEM(caCert)

	tlsConfig := &tls.Config{
		Certificates: []tls.Certificate{cert},
		ClientCAs:    caCertPool,
		ClientAuth:   tls.RequireAndVerifyClientCert,
	}

	return tlsConfig, nil
}

// Export SecureServer
func SecureServer(addr, certFile, keyFile, caCertFile string) {
	tlsConfig, err := LoadServerTLSConfig(certFile, keyFile, caCertFile)
	if err != nil {
		log.Fatalf("unable to configure TLS: %v", err)
	}

	server := &http.Server{
		Addr:      addr,
		TLSConfig: tlsConfig,
	}

	http.HandleFunc("/secure", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprintf(w, "This is a secure endpoint")
	})

	log.Printf("Starting secure server at %s\n", addr)
	log.Fatal(server.ListenAndServeTLS(certFile, keyFile))
}

// Export SecureClient
func SecureClient(serverAddr, caCertFile string) {
	tlsConfig, err := LoadClientTLSConfig(caCertFile)
	if err != nil {
		log.Fatalf("unable to configure TLS for client: %v", err)
	}

	client := &http.Client{
		Transport: &http.Transport{
			TLSClientConfig: tlsConfig,
		},
	}

	resp, err := client.Get(fmt.Sprintf("https://%s/secure", serverAddr))
	if err != nil {
		log.Fatalf("failed to perform GET request: %v", err)
	}
	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		log.Fatalf("failed to read response body: %v", err)
	}

	log.Printf("Server Response: %s\n", string(body))
}

func LoadClientTLSConfig(caCertFile string) (*tls.Config, error) {
	caCert, err := ioutil.ReadFile(caCertFile)
	if err != nil {
		return nil, fmt.Errorf("failed to read CA cert: %v", err)
	}

	caCertPool := x509.NewCertPool()
	caCertPool.AppendCertsFromPEM(caCert)

	tlsConfig := &tls.Config{
		RootCAs: caCertPool,
	}

	return tlsConfig, nil
}
