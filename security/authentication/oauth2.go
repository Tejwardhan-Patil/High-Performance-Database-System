package main

import (
	"crypto/rand"
	"crypto/sha256"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"log"
	"math/big"
	"net/http"
	"strings"
	"sync"
)

// OAuth2Config holds the OAuth 2.0 configuration
type OAuth2Config struct {
	ClientID     string
	ClientSecret string
	RedirectURI  string
	AuthURL      string
	TokenURL     string
	Scope        string
}

// OAuth2Token represents the access and refresh tokens
type OAuth2Token struct {
	AccessToken  string
	RefreshToken string
	ExpiresIn    int64
	TokenType    string
}

// sessionStore stores OAuth2 states for CSRF protection (in-memory for simplicity)
var sessionStore = struct {
	sync.Mutex
	state map[string]bool
}{state: make(map[string]bool)}

// GenerateState generates a random state for OAuth2 flow and stores it
func GenerateState() string {
	const letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
	var result strings.Builder
	for i := 0; i < 16; i++ {
		num, _ := rand.Int(rand.Reader, big.NewInt(int64(len(letters))))
		result.WriteByte(letters[num.Int64()])
	}
	state := result.String()

	// Store state in sessionStore for later validation
	sessionStore.Lock()
	sessionStore.state[state] = true
	sessionStore.Unlock()

	return state
}

// OAuth2AuthorizeHandler handles the authorization request
func OAuth2AuthorizeHandler(config OAuth2Config) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		state := GenerateState()
		authURL := fmt.Sprintf("%s?response_type=code&client_id=%s&redirect_uri=%s&scope=%s&state=%s",
			config.AuthURL, config.ClientID, config.RedirectURI, config.Scope, state)
		http.Redirect(w, r, authURL, http.StatusFound)
	}
}

// OAuth2TokenHandler exchanges authorization code for an access token
func OAuth2TokenHandler(config OAuth2Config) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		code := r.URL.Query().Get("code")
		state := r.URL.Query().Get("state")

		// Validate the state to prevent CSRF attacks
		sessionStore.Lock()
		_, validState := sessionStore.state[state]
		if !validState {
			sessionStore.Unlock()
			http.Error(w, "Invalid state", http.StatusBadRequest)
			return
		}
		delete(sessionStore.state, state) // Remove the state after validation
		sessionStore.Unlock()

		token, err := exchangeCodeForToken(config, code)
		if err != nil {
			http.Error(w, "Failed to exchange token", http.StatusInternalServerError)
			return
		}

		json.NewEncoder(w).Encode(token)
	}
}

// exchangeCodeForToken exchanges the authorization code for a token
func exchangeCodeForToken(config OAuth2Config, code string) (OAuth2Token, error) {
	data := fmt.Sprintf("grant_type=authorization_code&code=%s&redirect_uri=%s&client_id=%s&client_secret=%s",
		code, config.RedirectURI, config.ClientID, config.ClientSecret)

	req, err := http.NewRequest("POST", config.TokenURL, strings.NewReader(data))
	if err != nil {
		return OAuth2Token{}, err
	}
	req.Header.Set("Content-Type", "application/x-www-form-urlencoded")

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return OAuth2Token{}, err
	}
	defer resp.Body.Close()

	var token OAuth2Token
	if err := json.NewDecoder(resp.Body).Decode(&token); err != nil {
		return OAuth2Token{}, err
	}

	return token, nil
}

// OAuth2TokenValidationHandler validates the access token
func OAuth2TokenValidationHandler() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		authHeader := r.Header.Get("Authorization")
		if !strings.HasPrefix(authHeader, "Bearer ") {
			http.Error(w, "Invalid token", http.StatusUnauthorized)
			return
		}

		accessToken := strings.TrimPrefix(authHeader, "Bearer ")
		if !validateToken(accessToken) {
			http.Error(w, "Invalid token", http.StatusUnauthorized)
			return
		}

		w.WriteHeader(http.StatusOK)
		w.Write([]byte("Token is valid"))
	}
}

// validateToken simulates token validation (for simplicity)
func validateToken(token string) bool {
	return token != "" // Validation logic
}

// OAuth2TokenRevocationHandler handles token revocation
func OAuth2TokenRevocationHandler() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		authHeader := r.Header.Get("Authorization")
		if !strings.HasPrefix(authHeader, "Bearer ") {
			http.Error(w, "Invalid token", http.StatusUnauthorized)
			return
		}

		accessToken := strings.TrimPrefix(authHeader, "Bearer ")
		if err := revokeToken(accessToken); err != nil {
			http.Error(w, "Failed to revoke token", http.StatusInternalServerError)
			return
		}

		w.WriteHeader(http.StatusOK)
		w.Write([]byte("Token revoked"))
	}
}

// revokeToken simulates token revocation
func revokeToken(token string) error {
	log.Printf("Token revoked: %s", token)
	return nil
}

// OAuth2RefreshTokenHandler handles token refreshing
func OAuth2RefreshTokenHandler(config OAuth2Config) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		refreshToken := r.FormValue("refresh_token")
		if refreshToken == "" {
			http.Error(w, "Missing refresh token", http.StatusBadRequest)
			return
		}

		token, err := refreshAccessToken(config, refreshToken)
		if err != nil {
			http.Error(w, "Failed to refresh token", http.StatusInternalServerError)
			return
		}

		json.NewEncoder(w).Encode(token)
	}
}

// refreshAccessToken exchanges the refresh token for a new access token
func refreshAccessToken(config OAuth2Config, refreshToken string) (OAuth2Token, error) {
	data := fmt.Sprintf("grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s",
		refreshToken, config.ClientID, config.ClientSecret)

	req, err := http.NewRequest("POST", config.TokenURL, strings.NewReader(data))
	if err != nil {
		return OAuth2Token{}, err
	}
	req.Header.Set("Content-Type", "application/x-www-form-urlencoded")

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return OAuth2Token{}, err
	}
	defer resp.Body.Close()

	var token OAuth2Token
	if err := json.NewDecoder(resp.Body).Decode(&token); err != nil {
		return OAuth2Token{}, err
	}

	return token, nil
}

// HashClientSecret hashes the client secret using SHA-256
func HashClientSecret(secret string) string {
	hash := sha256.New()
	hash.Write([]byte(secret))
	return base64.URLEncoding.EncodeToString(hash.Sum(nil))
}

/* func main() {
	config := OAuth2Config{
		ClientID:     "client-id",
		ClientSecret: "client-secret",
		RedirectURI:  "http://localhost:8080/callback",
		AuthURL:      "http://localhost:8080/oauth2/authorize",
		TokenURL:     "http://localhost:8080/oauth2/token",
		Scope:        "read write",
	}

	http.HandleFunc("/oauth2/authorize", OAuth2AuthorizeHandler(config))
	http.HandleFunc("/oauth2/token", OAuth2TokenHandler(config))
	http.HandleFunc("/oauth2/validate", OAuth2TokenValidationHandler())
	http.HandleFunc("/oauth2/revoke", OAuth2TokenRevocationHandler())
	http.HandleFunc("/oauth2/refresh", OAuth2RefreshTokenHandler(config))

	log.Println("OAuth 2.0 server started on :8080")
	log.Fatal(http.ListenAndServe(":8080", nil))
} */
