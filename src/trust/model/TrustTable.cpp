/*
 * TrustTable.cpp
 *
 *  Created on: May 22, 2018
 *      Author: jude
 */

#include "TrustTable.h"

namespace ns3 {
namespace trust {

TrustTable::TrustTable()
{
}

void TrustTable::addRecord(AbstractTrustEntry entry)
{
	tableRecords.push_back(entry);
}

void TrustTable::removeRecord(AbstractTrustEntry entry)
{
	tableRecords.pop_back(); //need to change this
}

void updateRecord(AbstractTrustEntry entry)
{
	// TODO write update trust record logic
}

TrustTable::~TrustTable()
{
}

}

} /* namespace ns3 */
