package networking_tests

import (
	"context"
	"fmt"
	"net"
	"testing"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
	pb "website.com/networking/protocols/proto"
)

// Mock implementation of an RPC server
type MockRPCServer struct {
	pb.UnimplementedRPCServiceServer
}

func (s *MockRPCServer) Ping(ctx context.Context, req *pb.PingRequest) (*pb.PingResponse, error) {
	return &pb.PingResponse{Message: fmt.Sprintf("Pong: %s", req.Message)}, nil
}

// Mock implementation of a gRPC server
type MockGRPCServer struct {
	pb.UnimplementedGRPCServiceServer
}

func (s *MockGRPCServer) SendMessage(ctx context.Context, req *pb.MessageRequest) (*pb.MessageResponse, error) {
	return &pb.MessageResponse{Reply: "Message received: " + req.Text}, nil
}

// Updated to accept testing.TB for flexibility
func startMockRPCServer(tb testing.TB, port string) {
	lis, err := net.Listen("tcp", port)
	if err != nil {
		tb.Fatalf("Failed to listen on port %s: %v", port, err)
	}

	grpcServer := grpc.NewServer()
	pb.RegisterRPCServiceServer(grpcServer, &MockRPCServer{})
	reflection.Register(grpcServer)

	go func() {
		if err := grpcServer.Serve(lis); err != nil {
			tb.Fatalf("Failed to serve: %v", err)
		}
	}()

	time.Sleep(2 * time.Second)
}

func startMockGRPCServer(tb testing.TB, port string) {
	lis, err := net.Listen("tcp", port)
	if err != nil {
		tb.Fatalf("Failed to listen on port %s: %v", port, err)
	}

	grpcServer := grpc.NewServer()
	pb.RegisterGRPCServiceServer(grpcServer, &MockGRPCServer{})
	reflection.Register(grpcServer)

	go func() {
		if err := grpcServer.Serve(lis); err != nil {
			tb.Fatalf("Failed to serve: %v", err)
		}
	}()

	time.Sleep(2 * time.Second)
}

// Tests
func TestRPCPing(t *testing.T) {
	port := ":50051"
	startMockRPCServer(t, port)

	conn, err := grpc.Dial("localhost"+port, grpc.WithInsecure())
	if err != nil {
		t.Fatalf("Failed to connect: %v", err)
	}
	defer conn.Close()

	client := pb.NewRPCServiceClient(conn)
	resp, err := client.Ping(context.Background(), &pb.PingRequest{Message: "Ping"})
	if err != nil {
		t.Fatalf("Failed to ping: %v", err)
	}

	if resp.Message != "Pong: Ping" {
		t.Errorf("Expected 'Pong: Ping', got '%s'", resp.Message)
	}
}

func TestGRPCSendMessage(t *testing.T) {
	port := ":50052"
	startMockGRPCServer(t, port)

	conn, err := grpc.Dial("localhost"+port, grpc.WithInsecure())
	if err != nil {
		t.Fatalf("Failed to connect: %v", err)
	}
	defer conn.Close()

	client := pb.NewGRPCServiceClient(conn)
	resp, err := client.SendMessage(context.Background(), &pb.MessageRequest{Text: "Hello"})
	if err != nil {
		t.Fatalf("Failed to send message: %v", err)
	}

	if resp.Reply != "Message received: Hello" {
		t.Errorf("Expected 'Message received: Hello', got '%s'", resp.Reply)
	}
}

// Benchmark tests

func BenchmarkRPCPing(b *testing.B) {
	port := ":50051"
	startMockRPCServer(b, port)

	conn, err := grpc.Dial("localhost"+port, grpc.WithInsecure())
	if err != nil {
		b.Fatalf("Failed to connect: %v", err)
	}
	defer conn.Close()

	client := pb.NewRPCServiceClient(conn)

	for i := 0; i < b.N; i++ {
		_, err := client.Ping(context.Background(), &pb.PingRequest{Message: "Ping"})
		if err != nil {
			b.Fatalf("Failed to ping: %v", err)
		}
	}
}

func BenchmarkGRPCSendMessage(b *testing.B) {
	port := ":50052"
	startMockGRPCServer(b, port)

	conn, err := grpc.Dial("localhost"+port, grpc.WithInsecure())
	if err != nil {
		b.Fatalf("Failed to connect: %v", err)
	}
	defer conn.Close()

	client := pb.NewGRPCServiceClient(conn)

	for i := 0; i < b.N; i++ {
		_, err := client.SendMessage(context.Background(), &pb.MessageRequest{Text: "Hello"})
		if err != nil {
			b.Fatalf("Failed to send message: %v", err)
		}
	}
}
