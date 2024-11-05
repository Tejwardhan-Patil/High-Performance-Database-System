package grpc_protocol

import (
	pb "/protofile" // Import the compiled protocol buffer
	"context"
	"log"
	"net"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

// Server is used to implement the gRPC server
type Server struct {
	pb.UnimplementedServiceServer
}

// Unary call
func (s *Server) Unary(ctx context.Context, in *pb.RequestMessage) (*pb.ResponseMessage, error) {
	log.Printf("Received request: %s", in.GetMessage())
	response := &pb.ResponseMessage{Message: "Unary response: " + in.GetMessage()}
	return response, nil
}

// Server-side streaming
func (s *Server) ServerStreaming(in *pb.RequestMessage, stream pb.Service_ServerStreamingServer) error {
	log.Printf("Received stream request: %s", in.GetMessage())
	for i := 0; i < 10; i++ {
		res := &pb.ResponseMessage{Message: "Streaming message " + time.Now().String()}
		if err := stream.Send(res); err != nil {
			return err
		}
		time.Sleep(1 * time.Second)
	}
	return nil
}

// Client-side streaming
func (s *Server) ClientStreaming(stream pb.Service_ClientStreamingServer) error {
	var messages []string
	for {
		req, err := stream.Recv()
		if err == grpc.ErrServerStopped {
			break
		}
		if err != nil {
			return err
		}
		log.Printf("Received stream request: %s", req.GetMessage())
		messages = append(messages, req.GetMessage())
	}
	return stream.SendAndClose(&pb.ResponseMessage{Message: "All messages received."})
}

// Bidirectional streaming
func (s *Server) BidirectionalStreaming(stream pb.Service_BidirectionalStreamingServer) error {
	for {
		req, err := stream.Recv()
		if err != nil {
			break
		}
		log.Printf("Received stream request: %s", req.GetMessage())
		res := &pb.ResponseMessage{Message: "Replying: " + req.GetMessage()}
		if err := stream.Send(res); err != nil {
			return err
		}
	}
	return nil
}

// Start the gRPC server
func StartGRPCServer() {
	lis, err := net.Listen("tcp", ":50051")
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}

	grpcServer := grpc.NewServer()
	pb.RegisterServiceServer(grpcServer, &Server{})
	reflection.Register(grpcServer) // For easier inspection

	log.Printf("gRPC server is running on port 50051")
	if err := grpcServer.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}

// Client implementation
func StartGRPCClient() {
	conn, err := grpc.Dial("localhost:50051", grpc.WithInsecure())
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()

	c := pb.NewServiceClient(conn)

	// Unary request
	sendUnaryRequest(c)

	// Server-streaming request
	sendServerStreamingRequest(c)

	// Client-streaming request
	sendClientStreamingRequest(c)

	// Bidirectional streaming request
	sendBidirectionalStreamingRequest(c)
}

// Helper function for sending unary request
func sendUnaryRequest(c pb.ServiceClient) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()

	req := &pb.RequestMessage{Message: "Hello from client"}
	res, err := c.Unary(ctx, req)
	if err != nil {
		log.Fatalf("could not send unary request: %v", err)
	}
	log.Printf("Unary response: %s", res.GetMessage())
}

// Helper function for sending server-streaming request
func sendServerStreamingRequest(c pb.ServiceClient) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Second*5)
	defer cancel()

	req := &pb.RequestMessage{Message: "Requesting stream"}
	stream, err := c.ServerStreaming(ctx, req)
	if err != nil {
		log.Fatalf("could not stream: %v", err)
	}

	for {
		res, err := stream.Recv()
		if err != nil {
			break
		}
		log.Printf("Stream response: %s", res.GetMessage())
	}
}

// Helper function for sending client-streaming request
func sendClientStreamingRequest(c pb.ServiceClient) {
	stream, err := c.ClientStreaming(context.Background())
	if err != nil {
		log.Fatalf("could not send stream: %v", err)
	}

	for i := 0; i < 5; i++ {
		req := &pb.RequestMessage{Message: "Stream message " + time.Now().String()}
		if err := stream.Send(req); err != nil {
			log.Fatalf("failed to send: %v", err)
		}
	}

	res, err := stream.CloseAndRecv()
	if err != nil {
		log.Fatalf("failed to receive response: %v", err)
	}
	log.Printf("Client streaming response: %s", res.GetMessage())
}

// Helper function for bidirectional streaming
func sendBidirectionalStreamingRequest(c pb.ServiceClient) {
	stream, err := c.BidirectionalStreaming(context.Background())
	if err != nil {
		log.Fatalf("could not send bidirectional stream: %v", err)
	}

	done := make(chan struct{})
	go func() {
		for {
			res, err := stream.Recv()
			if err != nil {
				break
			}
			log.Printf("Bidirectional stream response: %s", res.GetMessage())
		}
		close(done)
	}()

	for i := 0; i < 5; i++ {
		req := &pb.RequestMessage{Message: "Client streaming message " + time.Now().String()}
		if err := stream.Send(req); err != nil {
			log.Fatalf("failed to send message: %v", err)
		}
		time.Sleep(1 * time.Second)
	}

	stream.CloseSend()
	<-done
}
