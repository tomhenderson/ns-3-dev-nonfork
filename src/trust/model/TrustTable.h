/*
 * TrustTable.h
 *
 *  Created on: May 22, 2018
 *      Author: jude
 */

#ifndef TRUSTTABLE_H_
#define TRUSTTABLE_H_

namespace ns3 {
namespace trust {

class TrustTable {
private:
	std::vector<AbstractTrustEntry> tableRecords;
public:
	TrustTable();
	virtual ~TrustTable();
	void addRecord(AbstractTrustEntry entry);
	void removeRecord(AbstractTrustEntry entry);
	void updateRecord(AbstractTrustEntry entry);
};
}

} /* namespace ns3 */

#endif /* TRUSTTABLE_H_ */
