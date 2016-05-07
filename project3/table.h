#ifndef _table
#define _table

#include <iostream>
#include <map>

using namespace std;

struct TopoLink {
    TopoLink(): cost(-1), age(0) {}

    TopoLink(const TopoLink & rhs) {
        *this = rhs;
    }

    TopoLink & operator=(const TopoLink & rhs) {
        this->cost = rhs.cost;
        this->age = rhs.age;

        return *this;
    }

    double cost;
    int age;
};

// Students should write this class
class Table {
    private:
        
    public:
        Table();
        Table(const Table &);
        Table & operator=(const Table &);

        ostream & Print(ostream &os) const;

        // Anything else you need

        #if defined(LINKSTATE)
        #endif

        #if defined(DISTANCEVECTOR)
			map<unsigned, map<unsigned, TopoLink> > whole_estimate;
			map<unsigned, TopoLink> dv;
			map<unsigned, TopoLink> neighbor_lats;
			map<unsigned, unsigned> next_hops;
        #endif
};

inline ostream & operator<<(ostream &os, const Table & t) { return t.Print(os);}


#endif
