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

#include "simple-aodv-trust-handler.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("SimpleAodvTrustHandler");

SimpleAodvTrustHandler::SimpleAodvTrustHandler ()
{
}

SimpleAodvTrustHandler::~SimpleAodvTrustHandler ()
{
}

bool SimpleAodvTrustHandler::OnReceivePromiscuousCallback (Ptr<NetDevice> device,
                                                           Ptr<const Packet> packet,
                                                           uint16_t protocol,
                                                           const Address &from,
                                                           const Address &to,
                                                           NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION(device << packet << protocol << &from << &to << packetType);
  bool found = false;

  return found;
}

int32_t SimpleAodvTrustHandler::calculateTrust (Ipv4Address address)
{
  AodvTrustEntry m_aodvTrustEntry = m_trustParameters[address];
  double trustDouble = m_aodvTrustEntry.GetRply() / m_aodvTrustEntry.GetRreq();
  // Update the value in Trust Table here
  std::cout<<trustDouble + 1<<std::endl; // to avoid unused variable compilation warning
  return 1;
}

}
