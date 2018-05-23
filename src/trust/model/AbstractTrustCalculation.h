/*
 * AbstractTrustCalculation.h
 *
 *  Created on: May 24, 2018
 *      Author: jude
 */

#ifndef ABSTRACTTRUSTCALCULATION_H_
#define ABSTRACTTRUSTCALCULATION_H_

#include "ns3/ipv4-address.h"

namespace ns3 {

class AbstractTrustCalculation {
public:
	AbstractTrustCalculation();
	virtual ~AbstractTrustCalculation();

	/**
	 * This method will be  override to facilate the different
	 * trust calculation logics depending on the protocol
	 */
	int32_t calculateTrust(Ipv4Address address);
};

} /* namespace ns3 */

#endif /* ABSTRACTTRUSTCALCULATION_H_ */
