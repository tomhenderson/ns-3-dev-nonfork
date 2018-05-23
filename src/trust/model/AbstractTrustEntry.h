/*
 * AbstractTrustEntry.h
 *
 *  Created on: May 21, 2018
 *      Author: jude
 */
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"

#ifndef ABSTRACTTRUSTENTRY_H_
#define ABSTRACTTRUSTENTRY_H_
namespace ns3 {
namespace trust {
class AbstractTrustEntry {

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
}
}

#endif /* ABSTRACTTRUSTENTRY_H_ */
