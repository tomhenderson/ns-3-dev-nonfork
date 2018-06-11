#include "trust-table.h"

namespace ns3
{

TrustTable::TrustTable ()
{}

void TrustTable::AddRecord (AbstractTrustEntry entry)
{
  m_tableRecords.push_back(entry);
}

void TrustTable::RemoveRecord (AbstractTrustEntry entry)
{
  m_tableRecords.pop_back(); //need to change this
}

void updateRecord (AbstractTrustEntry entry)
{
  // TODO write update trust record logic
}

TrustTable::~TrustTable()
{}

} // namespace ns3
