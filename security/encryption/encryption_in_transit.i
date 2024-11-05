%module encryption_in_transit

%{
#include "encryption_in_transit.go"
%}

// Expose functions to C/C++
extern tls.Config *LoadServerTLSConfig(const char *certFile, const char *keyFile, const char *caCertFile);
extern void SecureServer(const char *addr, const char *certFile, const char *keyFile, const char *caCertFile);
extern void SecureClient(const char *serverAddr, const char *caCertFile);