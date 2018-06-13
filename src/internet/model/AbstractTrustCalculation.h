/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Sri Lanka Institute of Information Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jude Niroshan <jude.niroshan11@gmail.com>
 */

#ifndef ABSTRACTTRUSTCALCULATION_H_
#define ABSTRACTTRUSTCALCULATION_H_

#include "ns3/ipv4-address.h"

namespace ns3 {

class AbstractTrustCalculation
{

  /**
   * \ingroup internet
   * \defgroup trust Trust management framework.
   *
   * The Trust Management Framework is built-in support to implement custom
   * trust based protocols in ns-3.
   */

  /**
   * \ingroup trust
   * \brief The abstract trust calculation for trust framework.
   * This class should be extended in order to define a custom trust
   * calculation algorithm.
   */
public:
  AbstractTrustCalculation ();
  virtual ~AbstractTrustCalculation ();

  /**
   * \brief Calculate the trust value for a given destination node
   * \param [in] address target node IPv4 address to calculate the trust
   * \returns int32_t trust value
   */
  int32_t calculateTrust (Ipv4Address address);
};

} // namespace ns3

#endif
