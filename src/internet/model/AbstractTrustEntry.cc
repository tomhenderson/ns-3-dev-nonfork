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

#include "AbstractTrustEntry.h"

namespace ns3 {

AbstractTrustEntry::AbstractTrustEntry ()
{
}

AbstractTrustEntry::~AbstractTrustEntry ()
{
}

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

