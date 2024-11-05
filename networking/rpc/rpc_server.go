package rpc

import (
	pb "/proto"
	"context"
	"fmt"
	"log"
	"sync"
	"time"
)

// Define a server struct that implements the RPC methods
type server struct {
	pb.UnimplementedServiceServer
	data sync.Map // thread-safe map for storing key-value pairs
}

// RPC method to greet a client
func (s *server) Greet(ctx context.Context, req *pb.GreetRequest) (*pb.GreetResponse, error) {
	name := req.GetName()
	if name == "" {
		return nil, fmt.Errorf("name cannot be empty")
	}
	return &pb.GreetResponse{
		Message: fmt.Sprintf("Hello, %s!", name),
	}, nil
}

// RPC method for setting key-value pairs
func (s *server) SetData(ctx context.Context, req *pb.SetDataRequest) (*pb.SetDataResponse, error) {
	s.data.Store(req.GetKey(), req.GetValue())
	return &pb.SetDataResponse{
		Status: "Success",
	}, nil
}

// RPC method for retrieving key-value pairs
func (s *server) GetData(ctx context.Context, req *pb.GetDataRequest) (*pb.GetDataResponse, error) {
	value, ok := s.data.Load(req.GetKey())
	if !ok {
		return nil, fmt.Errorf("key not found")
	}
	return &pb.GetDataResponse{
		Value: value.(string),
	}, nil
}

// Streaming RPC for sending time updates to the client
func (s *server) TimeUpdates(req *pb.TimeRequest, stream pb.Service_TimeUpdatesServer) error {
	for {
		select {
		case <-stream.Context().Done():
			log.Println("Client disconnected")
			return nil
		default:
			currentTime := time.Now().Format(time.RFC3339)
			if err := stream.Send(&pb.TimeResponse{CurrentTime: currentTime}); err != nil {
				return err
			}
			time.Sleep(1 * time.Second)
		}
	}
}

// Server-side streaming RPC for sending a list of data items
func (s *server) ListData(req *pb.ListDataRequest, stream pb.Service_ListDataServer) error {
	count := 0
	s.data.Range(func(key, value interface{}) bool {
		if err := stream.Send(&pb.ListDataResponse{
			Key:   key.(string),
			Value: value.(string),
		}); err != nil {
			return false
		}
		count++
		if count >= int(req.GetLimit()) {
			return false
		}
		return true
	})
	return nil
}

// A unary RPC that simulates a long-running task
func (s *server) LongRunningTask(ctx context.Context, req *pb.TaskRequest) (*pb.TaskResponse, error) {
	taskID := req.GetTaskId()
	log.Printf("Started long-running task: %s", taskID)
	time.Sleep(10 * time.Second) // Simulate long task
	log.Printf("Completed long-running task: %s", taskID)
	return &pb.TaskResponse{Status: "Completed"}, nil
}
