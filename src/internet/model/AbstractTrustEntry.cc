#include "AbstractTrustEntry.h"

namespace ns3
{

AbstractTrustEntry::AbstractTrustEntry ()
{}

AbstractTrustEntry::~AbstractTrustEntry ()
{}

Ipv4Address AbstractTrustEntry::getNeighbourAddress ()
{
  return this->neighbourAddress;
}

void AbstractTrustEntry::setNeighbourAddress (Ipv4Address neighbourAddress)
{
  this->neighbourAddress = neighbourAddress;
}

const Time& AbstractTrustEntry::getTimestamp ()
{
  return this->timestamp;
}

void AbstractTrustEntry::setTimestamp (const Time& timestamp)
{
  this->timestamp = timestamp;
}

int32_t AbstractTrustEntry::getTrustValue ()
{
  return this->trustValue;
}

void AbstractTrustEntry::setTrustValue (int32_t trustValue)
{
  this->trustValue = trustValue;
}

} // namespace ns3

