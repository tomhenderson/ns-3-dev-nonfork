#ifndef ABSTRACTTRUSTENTRY_H_
#define ABSTRACTTRUSTENTRY_H_

#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"

namespace ns3
{
class AbstractTrustEntry
{
  private:
    Ipv4Address neighbourAddress;
    int32_t trustValue;
    Time timestamp;

  public:
    AbstractTrustEntry();
    virtual ~AbstractTrustEntry();

    Ipv4Address getNeighbourAddress();
    void setNeighbourAddress(Ipv4Address neighbourAddress);

    const Time& getTimestamp();
    void setTimestamp(const Time& timestamp);

    int32_t getTrustValue();
    void setTrustValue(int32_t trustValue);
};
} // namespace ns3

#endif
