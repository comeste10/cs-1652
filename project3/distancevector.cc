#include "distancevector.h"
#include "context.h"

DistanceVector::DistanceVector(unsigned n, SimulationContext* c, double b, double l) :
	Node(n, c, b, l)
{}

DistanceVector::DistanceVector(const DistanceVector & rhs) :
	Node(rhs)
{
	*this = rhs;
}

DistanceVector & DistanceVector::operator=(const DistanceVector & rhs) {
	Node::operator=(rhs);
	return *this;
}

DistanceVector::~DistanceVector() {}


/** Write the following functions.  They currently have dummy implementations **/
void DistanceVector::LinkHasBeenUpdated(Link* l) {
	cerr << *this << ": Link Update: " << *l << endl;
	
	// directly update link
	this->routing_table.neighbor_lats[l->GetDest()].cost = l->GetLatency();
	this->routing_table.dv[l->GetDest()].cost = -1;
	this->routing_table.whole_estimate[l->GetDest()][l->GetDest()].cost = 0;

	// bellman-ford update table
	bool my_dv_has_changed = false;
	unsigned currSrc;
	unsigned currDest;	
	double currCost = -1;	
	double newCost = -1;
	double tempCost = -1;
	double tempCost1 = -1;
	double tempCost2 = -1;
	map<unsigned, map<unsigned, TopoLink> > my_whole_estimate;
	map<unsigned, TopoLink> my_dv;
	map<unsigned, TopoLink> my_neighbor_lats;
	unsigned best_hop;
	
	currSrc = this->number;
	newCost = -1;
	my_whole_estimate = this->routing_table.whole_estimate;
	my_dv = my_whole_estimate[this->number];
	my_neighbor_lats = this->routing_table.neighbor_lats;

	deque<Node*> my_neighbors = *(this->GetNeighbors());
	deque<Node*>::const_iterator n_iter;

	map<unsigned, TopoLink>::const_iterator dv_iter;
	for(dv_iter = my_dv.begin(); dv_iter != my_dv.end(); dv_iter++) {
		currDest = dv_iter->first;
		currCost = dv_iter->second.cost;

		if(this->number == currDest) {
			this->routing_table.dv[currDest].cost = 0;
			this->routing_table.next_hops[currDest] = currDest;
		}

		//	iterate over all neighbors to determine next hop
		my_neighbors = *(this->GetNeighbors());
		for(n_iter = my_neighbors.begin(); n_iter != my_neighbors.end(); n_iter++) {
			tempCost1 = my_neighbor_lats[(**n_iter).GetNumber()].cost;
			tempCost2 = (*(**n_iter).GetRoutingTable()).whole_estimate[(**n_iter).GetNumber()][currDest].cost;
			if(tempCost1 == -1 || tempCost2 == -1) {
				continue;
			}
			tempCost =  tempCost1 + tempCost2;
			if(tempCost < newCost) {
				newCost = tempCost;
				best_hop = (**n_iter).GetNumber();
			}
		}
		
		// compare my current cost to best neighbor's cost
		if(newCost < currCost && newCost != -1) {
			my_dv_has_changed = true;
			this->routing_table.dv[currDest].cost = newCost;
			this->routing_table.next_hops[currDest] = best_hop;
		}
	}
	
	if(my_dv_has_changed) {
		
		// update my whole estimate with the my new dv		
		this->routing_table.whole_estimate[this->number] = this->routing_table.dv;
		
		// create message( this.number, this.routing_table.dv )
		RoutingMessage * outgoing_msg = new RoutingMessage();
		outgoing_msg->src = this->number;
		outgoing_msg->src_dv = this->routing_table.dv;
		
		// send message to all neighbors
		this->SendToNeighbors(outgoing_msg);
	}

}

void DistanceVector::ProcessIncomingRoutingMessage(RoutingMessage *m) {
	cerr << *this << " got a routing message: " << *m << " (ignored)" << endl;

	// directly update my table with neighbor dv	
	unsigned neighbor_int = m->src;
	map<unsigned, TopoLink> neighbor_dv = m->src_dv;
	map<unsigned, map<unsigned, TopoLink> > my_whole_estimate2;
	this->routing_table.whole_estimate[neighbor_int] = neighbor_dv;


	map<unsigned, map<unsigned, TopoLink> > my_whole_estimate;
	map<unsigned, TopoLink> my_dv;	
	my_whole_estimate = this->routing_table.whole_estimate;
	my_dv = my_whole_estimate[this->number];	

	// for loop check
	map<unsigned, TopoLink>::const_iterator dv_iter;
	for(dv_iter = my_dv.begin(); dv_iter != my_dv.end(); dv_iter++) {
		if(my_dv[dv_iter->first].cost == -1) {}
	}

	// bellman-ford update table
	bool my_dv_has_changed = false;
	unsigned currSrc;
	unsigned currDest;	
	double currCost = -1;	
	double newCost = -1;
	double tempCost = -1;
	double tempCost1 = -1;
	double tempCost2 = -1;
	map<unsigned, TopoLink> my_neighbor_lats;
	unsigned best_hop;
	
	currSrc = this->number;
	newCost = -1;
	
	my_neighbor_lats = this->routing_table.neighbor_lats;

	deque<Node*> my_neighbors;
	deque<Node*>::const_iterator n_iter;

	for(dv_iter = my_dv.begin(); dv_iter != my_dv.end(); dv_iter++) {
		currDest = dv_iter->first;
		currCost = dv_iter->second.cost;

		if(this->number == currDest) {
			this->routing_table.dv[currDest].cost = 0;
			this->routing_table.next_hops[currDest] = currDest;
		}

		//	iterate over all neighbors to determine next hop
		my_neighbors = *(this->GetNeighbors());
		for(n_iter = my_neighbors.begin(); n_iter != my_neighbors.end(); n_iter++) {
			tempCost1 = my_neighbor_lats[(**n_iter).GetNumber()].cost;
			tempCost2 = (*(**n_iter).GetRoutingTable()).whole_estimate[(**n_iter).GetNumber()][currDest].cost;
			if(tempCost1 == -1 || tempCost2 == -1) {
				continue;
			}
			tempCost =  tempCost1 + tempCost2;
			if(tempCost < newCost) {
				newCost = tempCost;
				best_hop = (**n_iter).GetNumber();
			}
		}
		
		// compare my current cost to best neighbor's cost
		if(newCost < currCost && newCost != -1) {
			my_dv_has_changed = true;
			this->routing_table.dv[currDest].cost = newCost;
			this->routing_table.next_hops[currDest] = best_hop;
		}
	
	}
	
	if(my_dv_has_changed) {
		
		// update my whole estimate with the my new dv		
		this->routing_table.whole_estimate[this->number] = this->routing_table.dv;
		
		// create message( this.number, this.routing_table.dv )
		RoutingMessage * outgoing_msg = new RoutingMessage();
		outgoing_msg->src = this->number;
		outgoing_msg->src_dv = this->routing_table.dv;
		
		// send message to all neighbors
		this->SendToNeighbors(outgoing_msg);
	}
	
}

void DistanceVector::TimeOut() {
	cerr << *this << " got a timeout: (ignored)" << endl;
	// not used in DistanceVector
}



Node* DistanceVector::GetNextHop(Node *destination) { 
	
	unsigned next_hop_number = this->routing_table.next_hops[destination->GetNumber()];
	Node * temp_hop = new Node(next_hop_number, NULL, 0, 0);
	Node * next_hop = context->FindMatchingNode(const_cast<Node *>(temp_hop));
	temp_hop->~Node();
	return next_hop;
}

Table* DistanceVector::GetRoutingTable() {
	return &routing_table;
}

ostream & DistanceVector::Print(ostream &os) const { 
	Node::Print(os);
	return os;
}
