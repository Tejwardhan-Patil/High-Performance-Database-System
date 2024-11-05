package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"time"

	"github.com/golang-jwt/jwt"
	"golang.org/x/crypto/bcrypt"
)

// Secret key used for signing JWT tokens
var jwtKey = []byte("my_secret_key")

// User struct for storing user details
type User struct {
	Username string `json:"username"`
	Password string `json:"password"`
}

// Claims struct for JWT payload
type Claims struct {
	Username string `json:"username"`
	jwt.StandardClaims
}

// User database simulation
var userDB = map[string]string{}

// Helper function to create a password hash
func hashPassword(password string) (string, error) {
	bytes, err := bcrypt.GenerateFromPassword([]byte(password), 14)
	return string(bytes), err
}

// Helper function to check password hash
func checkPasswordHash(password, hash string) bool {
	err := bcrypt.CompareHashAndPassword([]byte(hash), []byte(password))
	return err == nil
}

// Register endpoint to add new users
func registerHandler(w http.ResponseWriter, r *http.Request) {
	var user User
	err := json.NewDecoder(r.Body).Decode(&user)
	if err != nil {
		http.Error(w, "Invalid request payload", http.StatusBadRequest)
		return
	}
	if _, exists := userDB[user.Username]; exists {
		http.Error(w, "User already exists", http.StatusConflict)
		return
	}
	hashedPassword, err := hashPassword(user.Password)
	if err != nil {
		http.Error(w, "Error hashing password", http.StatusInternalServerError)
		return
	}
	userDB[user.Username] = hashedPassword
	w.WriteHeader(http.StatusCreated)
}

// Login endpoint to authenticate users and return JWT
func loginHandler(w http.ResponseWriter, r *http.Request) {
	var user User
	err := json.NewDecoder(r.Body).Decode(&user)
	if err != nil {
		http.Error(w, "Invalid request payload", http.StatusBadRequest)
		return
	}
	storedPassword, ok := userDB[user.Username]
	if !ok || !checkPasswordHash(user.Password, storedPassword) {
		http.Error(w, "Invalid credentials", http.StatusUnauthorized)
		return
	}
	// Create JWT token
	expirationTime := time.Now().Add(5 * time.Minute)
	claims := &Claims{
		Username: user.Username,
		StandardClaims: jwt.StandardClaims{
			ExpiresAt: expirationTime.Unix(),
		},
	}
	token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
	tokenString, err := token.SignedString(jwtKey)
	if err != nil {
		http.Error(w, "Error generating token", http.StatusInternalServerError)
		return
	}
	http.SetCookie(w, &http.Cookie{
		Name:    "token",
		Value:   tokenString,
		Expires: expirationTime,
	})
}

// Middleware to authenticate and authorize users
func authMiddleware(next http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		c, err := r.Cookie("token")
		if err != nil {
			if err == http.ErrNoCookie {
				http.Error(w, "Not authenticated", http.StatusUnauthorized)
				return
			}
			http.Error(w, "Bad request", http.StatusBadRequest)
			return
		}
		tokenStr := c.Value
		claims := &Claims{}
		tkn, err := jwt.ParseWithClaims(tokenStr, claims, func(token *jwt.Token) (interface{}, error) {
			return jwtKey, nil
		})
		if err != nil || !tkn.Valid {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return
		}
		next(w, r)
	}
}

// Protected endpoint that requires JWT authentication
func protectedHandler(w http.ResponseWriter, r *http.Request) {
	w.Write([]byte("Protected content"))
}

// Refresh token endpoint
func refreshHandler(w http.ResponseWriter, r *http.Request) {
	c, err := r.Cookie("token")
	if err != nil {
		if err == http.ErrNoCookie {
			http.Error(w, "Not authenticated", http.StatusUnauthorized)
			return
		}
		http.Error(w, "Bad request", http.StatusBadRequest)
		return
	}
	tokenStr := c.Value
	claims := &Claims{}
	tkn, err := jwt.ParseWithClaims(tokenStr, claims, func(token *jwt.Token) (interface{}, error) {
		return jwtKey, nil
	})
	if err != nil || !tkn.Valid {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return
	}
	// Check token expiration
	if time.Unix(claims.ExpiresAt, 0).Sub(time.Now()) > 30*time.Second {
		http.Error(w, "Token not expired yet", http.StatusBadRequest)
		return
	}
	expirationTime := time.Now().Add(5 * time.Minute)
	claims.ExpiresAt = expirationTime.Unix()
	token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
	tokenString, err := token.SignedString(jwtKey)
	if err != nil {
		http.Error(w, "Error generating token", http.StatusInternalServerError)
		return
	}
	http.SetCookie(w, &http.Cookie{
		Name:    "token",
		Value:   tokenString,
		Expires: expirationTime,
	})
}

// Logout endpoint to clear JWT token
func logoutHandler(w http.ResponseWriter, r *http.Request) {
	http.SetCookie(w, &http.Cookie{
		Name:    "token",
		Value:   "",
		Expires: time.Now(),
	})
	w.Write([]byte("Logged out"))
}

// Home handler
func homeHandler(w http.ResponseWriter, r *http.Request) {
	w.Write([]byte("Welcome to the Authentication Server"))
}

func main() {
	// Setting up routes
	http.HandleFunc("/register", registerHandler)
	http.HandleFunc("/login", loginHandler)
	http.HandleFunc("/logout", authMiddleware(logoutHandler))
	http.HandleFunc("/refresh", authMiddleware(refreshHandler))
	http.HandleFunc("/protected", authMiddleware(protectedHandler))
	http.HandleFunc("/", homeHandler)

	// Starting server
	port := "8080"
	if os.Getenv("PORT") != "" {
		port = os.Getenv("PORT")
	}
	fmt.Printf("Authentication server running on port %s\n", port)
	log.Fatal(http.ListenAndServe(":"+port, nil))
}
