#include "table.h"

Table::Table() {
    whole_estimate.clear();
}

Table::Table(const Table & rhs) {
    *this = rhs;
}

Table & Table::operator=(const Table & rhs) {
    /* For now,  Change if you add more data members to the class */
    whole_estimate = rhs.whole_estimate;
	dv = rhs.dv;
	neighbor_lats = rhs.neighbor_lats;

    return *this;
}

#if defined(GENERIC)
ostream & Table::Print(ostream &os) const
{
    os << "Generic Table()";
    return os;
}
#endif

#if defined(LINKSTATE)
ostream & Table::Print(ostream &os) const
{
    os << "LinkState Table()";
    return os;
}
#endif

#if defined(DISTANCEVECTOR)
ostream & Table::Print(ostream &os) const
{
    os << "DistanceVector Table()";

    map<unsigned, map<unsigned, TopoLink> >::const_iterator row = whole_estimate.begin();

    while (row != whole_estimate.end()) {
        map<unsigned, TopoLink>::const_iterator col = (*row).second.begin();
        os << "Row=" << (*row).first;

        while (col != (*row).second.end()) {
            os << " Col=" << (*col).first << " cost=" << (*col).second.cost << endl;
            col++;
        }
        row++;
    }

    return os;
}

#endif
