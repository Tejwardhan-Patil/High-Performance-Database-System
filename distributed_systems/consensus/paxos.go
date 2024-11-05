package consensus

import (
	"errors"
	"fmt"
	"math/rand"
	"sync"
	"time"
)

type ProposalID struct {
	Number int
	NodeID int
}

type Proposer struct {
	ID        int
	ProposeID ProposalID
	Value     interface{}
	Acceptors []*Acceptor
	Majority  int
	PromiseID ProposalID
	accepted  bool
	mu        sync.Mutex
}

type Acceptor struct {
	ID          int
	PromisedID  ProposalID
	AcceptedID  ProposalID
	AcceptedVal interface{}
	mu          sync.Mutex
}

type Learner struct {
	ID           int
	AcceptedVals map[ProposalID]interface{}
	mu           sync.Mutex
}

type PaxosSystem struct {
	Proposers []*Proposer
	Acceptors []*Acceptor
	Learners  []*Learner
}

func NewPaxosSystem(numProposers, numAcceptors, numLearners int) *PaxosSystem {
	p := &PaxosSystem{}
	for i := 0; i < numProposers; i++ {
		p.Proposers = append(p.Proposers, &Proposer{ID: i, Majority: (numAcceptors/2 + 1)})
	}
	for i := 0; i < numAcceptors; i++ {
		p.Acceptors = append(p.Acceptors, &Acceptor{ID: i})
	}
	for i := 0; i < numLearners; i++ {
		p.Learners = append(p.Learners, &Learner{ID: i, AcceptedVals: make(map[ProposalID]interface{})})
	}
	return p
}

func (p *Proposer) Propose(value interface{}) error {
	p.mu.Lock()
	defer p.mu.Unlock()

	p.ProposeID.Number++
	p.Value = value
	fmt.Printf("Proposer %d: Proposing value %v with ID %d\n", p.ID, value, p.ProposeID.Number)

	promises := 0
	for _, a := range p.Acceptors {
		if p.SendPrepare(a) {
			promises++
		}
	}

	if promises < p.Majority {
		return errors.New("failed to get majority for prepare phase")
	}

	accepted := 0
	for _, a := range p.Acceptors {
		if p.SendAccept(a) {
			accepted++
		}
	}

	if accepted < p.Majority {
		return errors.New("failed to get majority for accept phase")
	}

	p.NotifyLearners()
	return nil
}

func (p *Proposer) SendPrepare(a *Acceptor) bool {
	a.mu.Lock()
	defer a.mu.Unlock()

	if a.PromisedID.Number >= p.ProposeID.Number {
		return false
	}

	a.PromisedID = p.ProposeID
	fmt.Printf("Proposer %d: Acceptor %d promises for proposal %d\n", p.ID, a.ID, p.ProposeID.Number)
	return true
}

func (p *Proposer) SendAccept(a *Acceptor) bool {
	a.mu.Lock()
	defer a.mu.Unlock()

	if a.PromisedID.Number > p.ProposeID.Number {
		return false
	}

	a.AcceptedID = p.ProposeID
	a.AcceptedVal = p.Value
	fmt.Printf("Proposer %d: Acceptor %d accepts proposal %d\n", p.ID, a.ID, p.ProposeID.Number)
	return true
}

func (p *Proposer) NotifyLearners() {
	for range p.Acceptors {
		fmt.Printf("Proposer %d: Notify Learners about accepted value %v\n", p.ID, p.Value)
	}
}

func (a *Acceptor) ReceivePrepare(proposalID ProposalID) bool {
	a.mu.Lock()
	defer a.mu.Unlock()

	if a.PromisedID.Number > proposalID.Number {
		return false
	}

	a.PromisedID = proposalID
	return true
}

func (a *Acceptor) ReceiveAccept(proposalID ProposalID, value interface{}) bool {
	a.mu.Lock()
	defer a.mu.Unlock()

	if a.PromisedID.Number > proposalID.Number {
		return false
	}

	a.AcceptedID = proposalID
	a.AcceptedVal = value
	return true
}

func (l *Learner) Learn(proposalID ProposalID, value interface{}) {
	l.mu.Lock()
	defer l.mu.Unlock()

	l.AcceptedVals[proposalID] = value
	fmt.Printf("Learner %d learned value %v from proposal %d\n", l.ID, value, proposalID.Number)
}

func (p *PaxosSystem) RunElection() {
	rand.Seed(time.Now().UnixNano())
	for _, proposer := range p.Proposers {
		go func(proposer *Proposer) {
			value := rand.Intn(100)
			err := proposer.Propose(value)
			if err != nil {
				fmt.Printf("Proposer %d failed to propose value: %v\n", proposer.ID, err)
			}
		}(proposer)
	}
}

func (p *PaxosSystem) RunConsensus() {
	for _, proposer := range p.Proposers {
		go func(proposer *Proposer) {
			value := rand.Intn(100)
			if err := proposer.Propose(value); err == nil {
				for _, learner := range p.Learners {
					for _, acceptor := range p.Acceptors {
						learner.Learn(acceptor.AcceptedID, acceptor.AcceptedVal)
					}
				}
			}
		}(proposer)
	}
}

func main() {
	ps := NewPaxosSystem(3, 5, 2)
	ps.RunElection()
	time.Sleep(2 * time.Second)

	fmt.Println("Running Paxos consensus")
	ps.RunConsensus()
	time.Sleep(2 * time.Second)
}
