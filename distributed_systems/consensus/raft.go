package consensus

import (
	"log"
	"math/rand"
	"sync"
	"time"
)

// Role of the Raft node
type Role int

const (
	Follower Role = iota
	Candidate
	Leader
)

const (
	HeartbeatTimeout  = 50 * time.Millisecond
	ElectionTimeout   = 150 * time.Millisecond
	BroadcastInterval = 100 * time.Millisecond
)

type LogEntry struct {
	Term    int
	Command interface{}
}

// Raft node structure
type Raft struct {
	mu          sync.Mutex
	id          int
	peers       []int
	role        Role
	term        int
	votedFor    int
	log         []LogEntry
	commitIdx   int
	lastApplied int
	nextIndex   map[int]int
	matchIndex  map[int]int
	applyCh     chan ApplyMsg
	timeoutCh   chan bool
	heartbeatCh chan bool
	voteCount   int
	stopCh      chan struct{}
}

type ApplyMsg struct {
	CommandValid bool
	Command      interface{}
	CommandIndex int
}

func NewRaft(id int, peers []int, applyCh chan ApplyMsg) *Raft {
	raft := &Raft{
		id:          id,
		peers:       peers,
		role:        Follower,
		term:        0,
		votedFor:    -1,
		log:         []LogEntry{},
		commitIdx:   0,
		lastApplied: 0,
		nextIndex:   make(map[int]int),
		matchIndex:  make(map[int]int),
		applyCh:     applyCh,
		timeoutCh:   make(chan bool),
		heartbeatCh: make(chan bool),
		voteCount:   0,
		stopCh:      make(chan struct{}),
	}
	go raft.run()
	return raft
}

func (rf *Raft) run() {
	for {
		switch rf.role {
		case Follower:
			select {
			case <-rf.timeoutCh:
				rf.startElection()
			case <-rf.heartbeatCh:
				rf.resetTimeout()
			}
		case Candidate:
			rf.startElection()
		case Leader:
			rf.broadcastHeartbeat()
			time.Sleep(BroadcastInterval)
		}
	}
}

func (rf *Raft) resetTimeout() {
	timeout := time.Duration(rand.Intn(150)+150) * time.Millisecond
	time.AfterFunc(timeout, func() { rf.timeoutCh <- true })
}

func (rf *Raft) startElection() {
	rf.mu.Lock()
	rf.term++
	rf.votedFor = rf.id
	rf.voteCount = 1
	rf.role = Candidate
	rf.mu.Unlock()

	for _, peer := range rf.peers {
		go rf.sendRequestVote(peer)
	}

	select {
	case <-rf.heartbeatCh:
		rf.mu.Lock()
		rf.role = Follower
		rf.mu.Unlock()
	case <-time.After(ElectionTimeout):
		rf.mu.Lock()
		if rf.voteCount > len(rf.peers)/2 {
			rf.role = Leader
			rf.initializeLeaderState()
		} else {
			rf.role = Follower
		}
		rf.mu.Unlock()
	}
}

func (rf *Raft) initializeLeaderState() {
	for _, peer := range rf.peers {
		rf.nextIndex[peer] = len(rf.log)
		rf.matchIndex[peer] = 0
	}
}

func (rf *Raft) broadcastHeartbeat() {
	for _, peer := range rf.peers {
		go rf.sendAppendEntries(peer)
	}
}

func (rf *Raft) sendRequestVote(peer int) {
	rf.mu.Lock()
	args := RequestVoteArgs{
		Term:        rf.term,
		CandidateID: rf.id,
		LastLogIdx:  len(rf.log) - 1,
		LastLogTerm: rf.getLastLogTerm(),
	}
	rf.mu.Unlock()

	var reply RequestVoteReply
	if rf.sendRPC(peer, "Raft.RequestVote", args, &reply) {
		rf.mu.Lock()
		defer rf.mu.Unlock()
		if reply.VoteGranted {
			rf.voteCount++
			if rf.voteCount > len(rf.peers)/2 {
				rf.role = Leader
				rf.initializeLeaderState()
			}
		} else if reply.Term > rf.term {
			rf.term = reply.Term
			rf.role = Follower
			rf.votedFor = -1
		}
	}
}

func (rf *Raft) sendAppendEntries(peer int) {
	rf.mu.Lock()
	args := AppendEntriesArgs{
		Term:         rf.term,
		LeaderID:     rf.id,
		PrevLogIdx:   rf.nextIndex[peer] - 1,
		PrevLogTerm:  rf.getLogTerm(rf.nextIndex[peer] - 1),
		Entries:      rf.log[rf.nextIndex[peer]:],
		LeaderCommit: rf.commitIdx,
	}
	rf.mu.Unlock()

	var reply AppendEntriesReply
	if rf.sendRPC(peer, "Raft.AppendEntries", args, &reply) {
		rf.mu.Lock()
		defer rf.mu.Unlock()
		if reply.Success {
			rf.nextIndex[peer] = len(rf.log)
			rf.matchIndex[peer] = rf.nextIndex[peer] - 1
		} else if reply.Term > rf.term {
			rf.term = reply.Term
			rf.role = Follower
			rf.votedFor = -1
		} else {
			rf.nextIndex[peer]--
		}
	}
}

func (rf *Raft) getLastLogTerm() int {
	if len(rf.log) == 0 {
		return -1
	}
	return rf.log[len(rf.log)-1].Term
}

func (rf *Raft) getLogTerm(index int) int {
	if index < 0 || index >= len(rf.log) {
		return -1
	}
	return rf.log[index].Term
}

func (rf *Raft) sendRPC(peer int, method string, args interface{}, reply interface{}) bool {
	log.Printf("Sending RPC to peer %d, method: %s, args: %+v, reply: %+v", peer, method, args, reply)
	return true
}

type RequestVoteArgs struct {
	Term        int
	CandidateID int
	LastLogIdx  int
	LastLogTerm int
}

type RequestVoteReply struct {
	Term        int
	VoteGranted bool
}

type AppendEntriesArgs struct {
	Term         int
	LeaderID     int
	PrevLogIdx   int
	PrevLogTerm  int
	Entries      []LogEntry
	LeaderCommit int
}

type AppendEntriesReply struct {
	Term    int
	Success bool
}
