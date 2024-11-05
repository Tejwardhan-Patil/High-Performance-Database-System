package rpc

import (
	"context"
	"fmt"
	"io"
	"log"
	"rpc/protos" // Import the generated protobuf code
	"time"

	"google.golang.org/grpc"
)

// RpcClient struct holds the connection and client
type RpcClient struct {
	connection *grpc.ClientConn
	client     protos.RPCServiceClient
}

// NewRpcClient initializes a new RpcClient
func NewRpcClient(serverAddress string) *RpcClient {
	// Set up a connection to the server
	conn, err := grpc.Dial(serverAddress, grpc.WithInsecure(), grpc.WithBlock())
	if err != nil {
		log.Fatalf("Failed to connect to server: %v", err)
	}

	client := protos.NewRPCServiceClient(conn)

	return &RpcClient{
		connection: conn,
		client:     client,
	}
}

// Close the connection when done
func (c *RpcClient) Close() {
	err := c.connection.Close()
	if err != nil {
		log.Fatalf("Error closing the connection: %v", err)
	}
}

// UnaryCall sends a unary request to the server
func (c *RpcClient) UnaryCall(ctx context.Context, request *protos.Request) (*protos.Response, error) {
	response, err := c.client.UnaryCall(ctx, request)
	if err != nil {
		return nil, fmt.Errorf("UnaryCall failed: %v", err)
	}
	return response, nil
}

// ServerStreamingCall initiates a server-side streaming call
func (c *RpcClient) ServerStreamingCall(ctx context.Context, request *protos.Request) error {
	stream, err := c.client.ServerStreamingCall(ctx, request)
	if err != nil {
		return fmt.Errorf("ServerStreamingCall failed: %v", err)
	}

	for {
		response, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			return fmt.Errorf("Error receiving stream: %v", err)
		}
		log.Printf("Received: %v", response)
	}

	return nil
}

// ClientStreamingCall sends a stream of requests to the server
func (c *RpcClient) ClientStreamingCall(ctx context.Context, requests []*protos.Request) (*protos.Response, error) {
	stream, err := c.client.ClientStreamingCall(ctx)
	if err != nil {
		return nil, fmt.Errorf("ClientStreamingCall failed: %v", err)
	}

	for _, req := range requests {
		if err := stream.Send(req); err != nil {
			return nil, fmt.Errorf("Error sending stream: %v", err)
		}
	}

	response, err := stream.CloseAndRecv()
	if err != nil {
		return nil, fmt.Errorf("Error receiving response: %v", err)
	}

	return response, nil
}

// BidirectionalStreamingCall handles bidirectional streaming between client and server
func (c *RpcClient) BidirectionalStreamingCall(ctx context.Context, requests []*protos.Request) error {
	stream, err := c.client.BidirectionalStreamingCall(ctx)
	if err != nil {
		return fmt.Errorf("BidirectionalStreamingCall failed: %v", err)
	}

	done := make(chan bool)

	// Goroutine to send requests
	go func() {
		for _, req := range requests {
			if err := stream.Send(req); err != nil {
				log.Printf("Error sending request: %v", err)
				return
			}
		}
		err := stream.CloseSend()
		if err != nil {
			log.Printf("Error closing send: %v", err)
		}
	}()

	// Goroutine to receive responses
	go func() {
		for {
			response, err := stream.Recv()
			if err == io.EOF {
				done <- true
				return
			}
			if err != nil {
				log.Printf("Error receiving response: %v", err)
				done <- true
				return
			}
			log.Printf("Received: %v", response)
		}
	}()

	<-done
	return nil
}

// RetryUnaryCall attempts a retry of UnaryCall on failure
func (c *RpcClient) RetryUnaryCall(ctx context.Context, request *protos.Request, retries int) (*protos.Response, error) {
	var lastErr error
	for attempt := 0; attempt < retries; attempt++ {
		response, err := c.UnaryCall(ctx, request)
		if err == nil {
			return response, nil
		}
		lastErr = err
		log.Printf("Retry attempt %d failed: %v", attempt+1, err)
		time.Sleep(2 * time.Second) // Exponential backoff can be applied here
	}
	return nil, fmt.Errorf("RetryUnaryCall failed after %d attempts: %v", retries, lastErr)
}

// StreamInterceptor can be used to wrap streaming RPC calls with additional functionality like logging
func (c *RpcClient) StreamInterceptor(
	ctx context.Context,
	method string,
	req, resp interface{},
	cc *grpc.ClientConn,
	invoker grpc.Streamer) error {

	log.Printf("Invoking method: %s with request: %v", method, req)
	err := invoker(ctx, method, req, resp, cc, invoker)
	if err != nil {
		log.Printf("StreamInterceptor: error invoking method: %s, err: %v", method, err)
		return err
	}
	log.Printf("Method %s invoked successfully with response: %v", method, resp)
	return nil
}

func main() {
	serverAddress := "localhost:50051"

	client := NewRpcClient(serverAddress)
	defer client.Close()

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*5)
	defer cancel()

	request := &protos.Request{
		Message: "Hello from client",
	}

	// Unary Call
	response, err := client.UnaryCall(ctx, request)
	if err != nil {
		log.Fatalf("Error in UnaryCall: %v", err)
	}
	log.Printf("UnaryCall response: %v", response)

	// Server streaming call
	err = client.ServerStreamingCall(ctx, request)
	if err != nil {
		log.Fatalf("Error in ServerStreamingCall: %v", err)
	}

	// Client streaming call
	requests := []*protos.Request{
		{Message: "First message"},
		{Message: "Second message"},
		{Message: "Third message"},
	}

	response, err = client.ClientStreamingCall(ctx, requests)
	if err != nil {
		log.Fatalf("Error in ClientStreamingCall: %v", err)
	}
	log.Printf("ClientStreamingCall response: %v", response)

	// Bidirectional streaming call
	err = client.BidirectionalStreamingCall(ctx, requests)
	if err != nil {
		log.Fatalf("Error in BidirectionalStreamingCall: %v", err)
	}
}
