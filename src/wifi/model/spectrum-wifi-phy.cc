/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Ghada Badawy <gbadawy@gmail.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 *
 * Ported from yans-wifi-phy.cc by several contributors starting
 * with Nicola Baldo and Dean Armstrong
 */

#include "spectrum-wifi-phy.h"
#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-value.h"
#include "wifi-channel.h"
#include "wifi-mode.h"
#include "wifi-preamble.h"
#include "wifi-phy-state-helper.h"
#include "error-rate-model.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/pointer.h"
#include "ns3/net-device.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/boolean.h"
#include "ns3/node.h"
#include "ampdu-tag.h"
#include "wifi-spectrum-signal-parameters.h"
#include "wifi-phy-tag.h"
#include "wifi-spectrum-helper.h"
#include "ns3/antenna-model.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SpectrumWifiPhy");

NS_OBJECT_ENSURE_REGISTERED (SpectrumWifiPhy);

// XXX refactor to use C++-11 initialization list once bug 2270 patched
std::map<uint32_t, double>
SpectrumWifiPhy::CreateChannelMap ()
{
  std::map<uint32_t, double> m;
  m[8] = 5040; m[12] = 5060; m[16] = 5080;
  m[36] = 5180; m[38] = 5190; m[40] = 5200; m[42] = 5210; m[44] = 5220;
  m[46] = 5230; m[48] = 5240; m[50] = 5250; m[52] = 5260; m[54] = 5270;
  m[56] = 5280; m[58] = 5290; m[60] = 5300; m[62] = 5310; m[64] = 5320;
  m[100] = 5500; m[102] = 5510; m[104] = 5520; m[106] = 5530; m[108] = 5540;
  m[110] = 5550; m[112] = 5560; m[114] = 5570; m[116] = 5580; m[118] = 5590;
  m[120] = 5600; m[122] = 5610; m[124] = 5620; m[126] = 5630; m[128] = 5640;
  m[132] = 5660; m[134] = 5670; m[136] = 5680; m[138] = 5690; m[140] = 5700;
  m[142] = 5710; m[144] = 5720; 
  m[149] = 5745; m[151] = 5755; m[153] = 5765; m[155] = 5775; m[157] = 5785;
  m[159] = 5795; m[161] = 5805; m[165] = 5825;
  m[184] = 4920; m[188] = 4940; m[192] = 4960; m[196] = 4980;
  // 802.11p
  m[175] = 5875; m[181] = 5905;
  return m;
}
std::map<uint32_t, double> SpectrumWifiPhy::m_channelMap =  SpectrumWifiPhy::CreateChannelMap ();

// XXX refactor to use C++-11 initialization list once bug 2270 patched
std::map<uint32_t, double>
SpectrumWifiPhy::CreateChannelMap10Mhz ()
{
  std::map<uint32_t, double> m;
  m[7] = 5035; m[9] = 5045; m[11] = 5055; m[183] = 4915; m[185] = 4925;
  m[187] = 4935; m[189] = 4945;
  // 802.11p
  m[172] = 5860; m[174] = 5870; m[176] = 5880; m[178] = 5890;
  m[180] = 5900; m[182] = 5910; m[184] = 5920;
  return m;
}
std::map<uint32_t, double> SpectrumWifiPhy::m_channelMap10Mhz =  SpectrumWifiPhy::CreateChannelMap10Mhz ();

TypeId
SpectrumWifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SpectrumWifiPhy")
    .SetParent<WifiPhy> ()
    .SetGroupName ("Wifi")
    .AddConstructor<SpectrumWifiPhy> ()
    .AddAttribute ("EnergyDetectionThreshold",
                   "The energy of a received signal should be higher than "
                   "this threshold (dbm) to allow the PHY layer to detect the signal.",
                   DoubleValue (-96.0),
                   MakeDoubleAccessor (&SpectrumWifiPhy::SetEdThreshold,
                                       &SpectrumWifiPhy::GetEdThreshold),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("CcaMode1Threshold",
                   "The energy of a received signal should be higher than "
                   "this threshold (dbm) to allow the PHY layer to declare CCA BUSY state.",
                   DoubleValue (-99.0),
                   MakeDoubleAccessor (&SpectrumWifiPhy::SetCcaMode1Threshold,
                                       &SpectrumWifiPhy::GetCcaMode1Threshold),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("DisableWifiReception", "Prevent Wi-Fi frame sync from ever happening",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SpectrumWifiPhy::m_disableWifiReception),
                   MakeBooleanChecker ())
    .AddAttribute ("TxGain",
                   "Transmission gain (dB).",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&SpectrumWifiPhy::SetTxGain,
                                       &SpectrumWifiPhy::GetTxGain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxGain",
                   "Reception gain (dB).",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&SpectrumWifiPhy::SetRxGain,
                                       &SpectrumWifiPhy::GetRxGain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxPowerLevels",
                   "Number of transmission power levels available between "
                   "TxPowerStart and TxPowerEnd included.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&SpectrumWifiPhy::m_nTxPower),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TxPowerEnd",
                   "Maximum available transmission level (dbm).",
                   DoubleValue (16.0206),
                   MakeDoubleAccessor (&SpectrumWifiPhy::SetTxPowerEnd,
                                       &SpectrumWifiPhy::GetTxPowerEnd),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxPowerStart",
                   "Minimum available transmission level (dbm).",
                   DoubleValue (16.0206),
                   MakeDoubleAccessor (&SpectrumWifiPhy::SetTxPowerStart,
                                       &SpectrumWifiPhy::GetTxPowerStart),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxNoiseFigure",
                   "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                   " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                   "\"the difference in decibels (dB) between"
                   " the noise output of the actual receiver to the noise output of an "
                   " ideal receiver with the same overall gain and bandwidth when the receivers "
                   " are connected to sources at the standard noise temperature T0 (usually 290 K)\".",
                   DoubleValue (7),
                   MakeDoubleAccessor (&SpectrumWifiPhy::SetRxNoiseFigure,
                                       &SpectrumWifiPhy::GetRxNoiseFigure),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("State",
                   "The state of the PHY layer.",
                   PointerValue (),
                   MakePointerAccessor (&SpectrumWifiPhy::m_state),
                   MakePointerChecker<WifiPhyStateHelper> ())
    .AddAttribute ("ChannelSwitchDelay",
                   "Delay between two short frames transmitted on different frequencies.",
                   TimeValue (MicroSeconds (250)),
                   MakeTimeAccessor (&SpectrumWifiPhy::m_channelSwitchDelay),
                   MakeTimeChecker ())
    .AddAttribute ("ChannelNumber",
                   "Channel center frequency = Channel starting frequency + 5 MHz * nch.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&SpectrumWifiPhy::SetChannelNumber,
                                         &SpectrumWifiPhy::GetChannelNumber),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Frequency",
                   "The center frequency of the currently configured channel, in MHz",
                    TypeId::ATTR_GET,
                    UintegerValue (2412), // This value is ignored since there is no setter
                   MakeUintegerAccessor (&SpectrumWifiPhy::GetFrequency),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TxAntennas",
                   "The number of supported Tx antennas.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&SpectrumWifiPhy::GetNumberOfTransmitAntennas,
                                         &SpectrumWifiPhy::SetNumberOfTransmitAntennas),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("RxAntennas",
                   "The number of supported Rx antennas.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&SpectrumWifiPhy::GetNumberOfReceiveAntennas,
                                         &SpectrumWifiPhy::SetNumberOfReceiveAntennas),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ShortGuardEnabled",
                   "Whether or not short guard interval is enabled."
                   "This parameter is only valuable for 802.11n/ac STAs and APs.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SpectrumWifiPhy::GetGuardInterval,
                                        &SpectrumWifiPhy::SetGuardInterval),
                   MakeBooleanChecker ())
    .AddAttribute ("LdpcEnabled",
                   "Whether or not LDPC is enabled.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SpectrumWifiPhy::GetLdpc,
                                        &SpectrumWifiPhy::SetLdpc),
                   MakeBooleanChecker ())
    .AddAttribute ("STBCEnabled",
                   "Whether or not STBC is enabled.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SpectrumWifiPhy::GetStbc,
                                        &SpectrumWifiPhy::SetStbc),
                   MakeBooleanChecker ())
    .AddAttribute ("GreenfieldEnabled",
                   "Whether or not Greenfield is enabled."
                   "This parameter is only valuable for 802.11n STAs and APs.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SpectrumWifiPhy::GetGreenfield,
                                        &SpectrumWifiPhy::SetGreenfield),
                   MakeBooleanChecker ())
    .AddAttribute ("ShortPlcpPreambleSupported",
                   "Whether or not short PLCP preamble is supported."
                   "This parameter is only valuable for 802.11b STAs and APs."
                   "Note: 802.11g APs and STAs always support short PLCP preamble.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SpectrumWifiPhy::GetShortPlcpPreambleSupported,
                                        &SpectrumWifiPhy::SetShortPlcpPreambleSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("ChannelWidth",
                   "Whether 5MHz, 10MHz, 20MHz, 22MHz, 40MHz, 80 MHz or 160 MHz.",
                   UintegerValue (20),
                   MakeUintegerAccessor (&SpectrumWifiPhy::GetChannelWidth,
                                         &SpectrumWifiPhy::SetChannelWidth),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("SignalArrival",
                     "Signal arrival",
                     MakeTraceSourceAccessor (&SpectrumWifiPhy::m_signalCb),
                     "ns3::SpectrumWifiPhy::SignalArrivalCallback")
  ;
  return tid;
}

SpectrumWifiPhy::SpectrumWifiPhy ()
  : m_initialized (false),
    m_channelNumber (1),
    m_endRxEvent (),
    m_endPlcpRxEvent (),
    m_channelCenterFrequency (0),
    m_standard (WIFI_PHY_STANDARD_80211a),
    m_mpdusNum (0),
    m_plcpSuccess (false),
    m_txMpduReferenceNumber (0xffffffff),
    m_rxMpduReferenceNumber (0xffffffff)
{
  NS_LOG_FUNCTION (this);
  m_random = CreateObject<UniformRandomVariable> ();
  m_state = CreateObject<WifiPhyStateHelper> ();
}

SpectrumWifiPhy::~SpectrumWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
SpectrumWifiPhy::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_deviceRateSet.clear ();
  m_deviceMcsSet.clear ();
  m_device = 0;
  m_mobility = 0;
  m_state = 0;
}

void
SpectrumWifiPhy::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  m_initialized = true;
}

void
SpectrumWifiPhy::ConfigureStandard (enum WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  switch (standard)
    {
    case WIFI_PHY_STANDARD_80211a:
      Configure80211a ();
      break;
    case WIFI_PHY_STANDARD_80211b:
      Configure80211b ();
      break;
    case WIFI_PHY_STANDARD_80211g:
      Configure80211g ();
      break;
    case WIFI_PHY_STANDARD_80211_10MHZ:
      Configure80211_10Mhz ();
      break;
    case WIFI_PHY_STANDARD_80211_5MHZ:
      Configure80211_5Mhz ();
      break;
    case WIFI_PHY_STANDARD_holland:
      ConfigureHolland ();
      break;
    case WIFI_PHY_STANDARD_80211n_2_4GHZ:
      Configure80211n_2_4Ghz ();
      break;
    case WIFI_PHY_STANDARD_80211n_5GHZ:
      Configure80211n_5Ghz ();
      break;
    case WIFI_PHY_STANDARD_80211ac:
      Configure80211ac ();
      break;
    default:
      NS_ASSERT (false);
      break;
    }
  m_standard = standard;
}

void
SpectrumWifiPhy::SetRxNoiseFigure (double noiseFigureDb)
{
  NS_LOG_FUNCTION (this << noiseFigureDb);
  m_interference.SetNoiseFigure (DbToRatio (noiseFigureDb));
}

void
SpectrumWifiPhy::SetTxPowerStart (double start)
{
  NS_LOG_FUNCTION (this << start);
  m_txPowerBaseDbm = start;
}

void
SpectrumWifiPhy::SetTxPowerEnd (double end)
{
  NS_LOG_FUNCTION (this << end);
  m_txPowerEndDbm = end;
}

void
SpectrumWifiPhy::SetNTxPower (uint32_t n)
{
  NS_LOG_FUNCTION (this << n);
  m_nTxPower = n;
}

void
SpectrumWifiPhy::SetTxGain (double gain)
{
  NS_LOG_FUNCTION (this << gain);
  m_txGainDb = gain;
}

void
SpectrumWifiPhy::SetRxGain (double gain)
{
  NS_LOG_FUNCTION (this << gain);
  m_rxGainDb = gain;
}

void
SpectrumWifiPhy::SetEdThreshold (double threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_edThresholdW = DbmToW (threshold);
}

void
SpectrumWifiPhy::SetCcaMode1Threshold (double threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_ccaMode1ThresholdW = DbmToW (threshold);
}

void
SpectrumWifiPhy::SetErrorRateModel (Ptr<ErrorRateModel> rate)
{
  m_interference.SetErrorRateModel (rate);
}

void
SpectrumWifiPhy::SetDevice (Ptr<NetDevice> device)
{
  m_device = device;
}

void
SpectrumWifiPhy::SetMobility (Ptr<MobilityModel> mobility)
{
  m_mobility = mobility;
}

double
SpectrumWifiPhy::GetRxNoiseFigure (void) const
{
  return RatioToDb (m_interference.GetNoiseFigure ());
}

double
SpectrumWifiPhy::GetTxPowerStart (void) const
{
  return m_txPowerBaseDbm;
}

double
SpectrumWifiPhy::GetTxPowerEnd (void) const
{
  return m_txPowerEndDbm;
}

double
SpectrumWifiPhy::GetTxGain (void) const
{
  return m_txGainDb;
}

double
SpectrumWifiPhy::GetRxGain (void) const
{
  return m_rxGainDb;
}

double
SpectrumWifiPhy::GetEdThreshold (void) const
{
  return WToDbm (m_edThresholdW);
}

double
SpectrumWifiPhy::GetCcaMode1Threshold (void) const
{
  return WToDbm (m_ccaMode1ThresholdW);
}

Ptr<ErrorRateModel>
SpectrumWifiPhy::GetErrorRateModel (void) const
{
  return m_interference.GetErrorRateModel ();
}

Ptr<NetDevice>
SpectrumWifiPhy::GetDevice (void) const
{
  return m_device;
}

Ptr<MobilityModel>
SpectrumWifiPhy::GetMobility (void)
{
  if (m_mobility != 0)
    {
      return m_mobility;
    }
  else
    {
      return m_device->GetNode ()->GetObject<MobilityModel> ();
    }
}

Ptr<const SpectrumModel>
SpectrumWifiPhy::GetRxSpectrumModel () const
{
  return m_rxSpectrumModel;
}

void
SpectrumWifiPhy::SetNoisePowerSpectralDensity (Ptr<const SpectrumValue> noisePsd)
{
  NS_LOG_FUNCTION (this << noisePsd);
  NS_ASSERT (noisePsd);
  m_rxSpectrumModel = noisePsd->GetSpectrumModel ();
  NS_ASSERT (m_rxSpectrumModel);
}

double
SpectrumWifiPhy::CalculateSnr (WifiTxVector txVector, double ber) const
{
  return m_interference.GetErrorRateModel ()->CalculateSnr (txVector, ber);
}

Ptr<WifiChannel>
SpectrumWifiPhy::GetChannel (void) const
{
  return Ptr<WifiChannel> (0);
}

void
SpectrumWifiPhy::SetChannel (Ptr<SpectrumChannel> channel)
{
  m_channel = channel;
}

void
SpectrumWifiPhy::SetChannelNumber (uint16_t nch)
{
  if (!m_initialized)
    {
      //this is not channel switch, this is initialization
      NS_LOG_DEBUG ("start at channel " << nch);
      m_channelNumber = nch;
      return;
    }

  NS_ASSERT (!IsStateSwitching ());
  switch (m_state->GetState ())
    {
    case SpectrumWifiPhy::RX:
      NS_LOG_DEBUG ("drop packet because of channel switching while reception");
      m_endPlcpRxEvent.Cancel ();
      m_endRxEvent.Cancel ();
      goto switchChannel;
      break;
    case SpectrumWifiPhy::TX:
      NS_LOG_DEBUG ("channel switching postponed until end of current transmission");
      Simulator::Schedule (GetDelayUntilIdle (), &SpectrumWifiPhy::SetChannelNumber, this, nch);
      break;
    case SpectrumWifiPhy::CCA_BUSY:
    case SpectrumWifiPhy::IDLE:
      goto switchChannel;
      break;
    case SpectrumWifiPhy::SLEEP:
      NS_LOG_DEBUG ("channel switching ignored in sleep mode");
      break;
    default:
      NS_ASSERT (false);
      break;
    }

  return;

switchChannel:

  NS_LOG_DEBUG ("switching channel " << m_channelNumber << " -> " << nch);
  m_state->SwitchToChannelSwitching (m_channelSwitchDelay);
  m_interference.EraseEvents ();
  /*
   * Needed here to be able to correctly sensed the medium for the first
   * time after the switching. The actual switching is not performed until
   * after m_channelSwitchDelay. Packets received during the switching
   * state are added to the event list and are employed later to figure
   * out the state of the medium after the switching.
   */
  m_channelNumber = nch;
}

void
SpectrumWifiPhy::SetPacketReceivedCallback (RxCallback callback)
{
  m_rxCallback = callback;
}

uint16_t
SpectrumWifiPhy::GetChannelNumber (void) const
{
  return m_channelNumber;
}

Time
SpectrumWifiPhy::GetChannelSwitchDelay (void) const
{
  return m_channelSwitchDelay;
}

void
SpectrumWifiPhy::AddOperationalChannel (uint16_t channelNumber)
{
  m_operationalChannelList.push_back (channelNumber);
}

std::vector<uint16_t>
SpectrumWifiPhy::GetOperationalChannelList () const
{
  std::vector<uint16_t> channelList;
  channelList.push_back (m_channelNumber);
  for (std::vector<uint16_t>::size_type i = 0; i != m_operationalChannelList.size (); i++)
    {
      if (m_operationalChannelList[i] != m_channelNumber)
        { 
          channelList.push_back (m_channelNumber);
        }
    }
  return channelList;
}

void
SpectrumWifiPhy::ClearOperationalChannelList ()
{
  m_operationalChannelList.clear ();
}

void
SpectrumWifiPhy::SetSleepMode (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state->GetState ())
    {
    case SpectrumWifiPhy::TX:
      NS_LOG_DEBUG ("setting sleep mode postponed until end of current transmission");
      Simulator::Schedule (GetDelayUntilIdle (), &SpectrumWifiPhy::SetSleepMode, this);
      break;
    case SpectrumWifiPhy::RX:
      NS_LOG_DEBUG ("setting sleep mode postponed until end of current reception");
      Simulator::Schedule (GetDelayUntilIdle (), &SpectrumWifiPhy::SetSleepMode, this);
      break;
    case SpectrumWifiPhy::SWITCHING:
      NS_LOG_DEBUG ("setting sleep mode postponed until end of channel switching");
      Simulator::Schedule (GetDelayUntilIdle (), &SpectrumWifiPhy::SetSleepMode, this);
      break;
    case SpectrumWifiPhy::CCA_BUSY:
    case SpectrumWifiPhy::IDLE:
      NS_LOG_DEBUG ("setting sleep mode");
      m_state->SwitchToSleep ();
      break;
    case SpectrumWifiPhy::SLEEP:
      NS_LOG_DEBUG ("already in sleep mode");
      break;
    default:
      NS_ASSERT (false);
      break;
    }
}

void
SpectrumWifiPhy::ResumeFromSleep (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state->GetState ())
    {
    case SpectrumWifiPhy::TX:
    case SpectrumWifiPhy::RX:
    case SpectrumWifiPhy::IDLE:
    case SpectrumWifiPhy::CCA_BUSY:
    case SpectrumWifiPhy::SWITCHING:
      {
        NS_LOG_DEBUG ("not in sleep mode, there is nothing to resume");
        break;
      }
    case SpectrumWifiPhy::SLEEP:
      {
        NS_LOG_DEBUG ("resuming from sleep mode");
        Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaMode1ThresholdW);
        m_state->SwitchFromSleep (delayUntilCcaEnd);
        break;
      }
    default:
      {
        NS_ASSERT (false);
        break;
      }
    }
}

void
SpectrumWifiPhy::SetReceiveOkCallback (RxOkCallback callback)
{
  m_state->SetReceiveOkCallback (callback);
}

void
SpectrumWifiPhy::SetReceiveErrorCallback (RxErrorCallback callback)
{
  m_state->SetReceiveErrorCallback (callback);
}

void
SpectrumWifiPhy::SwitchMaybeToCcaBusy (void)
{
  //We are here because we have received the first bit of a packet and we are
  //not going to be able to synchronize on it
  //In this model, CCA becomes busy when the aggregation of all signals as
  //tracked by the InterferenceHelper class is higher than the CcaBusyThreshold

  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaMode1ThresholdW);
  if (!delayUntilCcaEnd.IsZero ())
    {
      m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
}

void
SpectrumWifiPhy::StartRx (Ptr<SpectrumSignalParameters> rxParams)
{
  NS_LOG_FUNCTION (this << rxParams);
  Time rxDuration = rxParams->duration;
  Ptr<SpectrumValue> receivedSignalPsd = rxParams->psd;
  uint32_t senderNodeId = 0;
  if (rxParams->txPhy)
    {
      senderNodeId = rxParams->txPhy->GetDevice ()->GetNode ()->GetId ();
    }
  NS_LOG_DEBUG ("Received signal from " << senderNodeId << " with unfiltered power " << WToDbm (Integral (*receivedSignalPsd)) << " dBm");
  // Integrate over our receive bandwidth (i.e., all that the receive
  // spectral mask representing our filtering allows) to find the
  // total energy apparent to the "demodulator".
  Ptr<SpectrumValue> filter = WifiSpectrumHelper::CreateRfFilter (m_channelNumber);
  SpectrumValue filteredSignal = (*filter) * (*receivedSignalPsd);
  // Add receiver antenna gain
  double rxPowerW = Integral (filteredSignal) * DbToRatio (m_rxGainDb);
  double rxPowerDbm = WToDbm (rxPowerW);
  NS_LOG_DEBUG ("Signal power received: " << rxPowerDbm << " dBm");

  Ptr<WifiSpectrumSignalParameters> wifiRxParams = DynamicCast<WifiSpectrumSignalParameters> (rxParams);

  // Log the signal arrival to the trace source
  m_signalCb (wifiRxParams ? true : false, senderNodeId, WToDbm (rxPowerW), rxDuration);
  if (wifiRxParams == 0)
    {
      NS_LOG_INFO ("Received non Wi-Fi signal");
      m_interference.AddForeignSignal (rxDuration, rxPowerW);
      SwitchMaybeToCcaBusy ();
      return;
    }
  if (wifiRxParams && m_disableWifiReception)
    {
      NS_LOG_INFO ("Received Wi-Fi signal but blocked from syncing");
      m_interference.AddForeignSignal (rxDuration, rxPowerW);
      SwitchMaybeToCcaBusy ();
      return;
    }

  Ptr<Packet> packet = wifiRxParams->packet;
  WifiPhyTag tag;
  bool found = packet->RemovePacketTag (tag);
  if (!found)
    {
      NS_FATAL_ERROR ("Received Wi-Fi Spectrum Signal with no WifiPhyTag");
      return;
    }

  WifiTxVector txVector = tag.GetWifiTxVector ();
  if (txVector.GetNss () > GetNumberOfReceiveAntennas ())
    {
      /* failure. */
      NotifyRxDrop (packet);
      NS_LOG_INFO ("Reception ends in failure because less RX antennas than number of spatial streams");
      SwitchMaybeToCcaBusy ();
      return;
    }

  enum WifiPreamble preamble = tag.GetWifiPreamble ();
  enum mpduType mpdutype = tag.GetMpduType ();

  // At this point forward, processing parallels that of 
  // YansWifiPhy::StartReceivePreambleAndHeader ()

  AmpduTag ampduTag;
  Time endRx = Simulator::Now () + rxDuration;
  Time preambleAndHeaderDuration = CalculatePlcpPreambleAndHeaderDuration (txVector, preamble);

  Ptr<InterferenceHelper::Event> event;
  event = m_interference.Add (packet->GetSize (),
                              txVector,
                              preamble,
                              rxDuration,
                              rxPowerW);

  switch (m_state->GetState ())
    {
    case SpectrumWifiPhy::SWITCHING:
      NS_LOG_DEBUG ("drop packet because of channel switching");
      NotifyRxDrop (packet);
      m_plcpSuccess = false;
      /*
       * Packets received on the upcoming channel are added to the event list
       * during the switching state. This way the medium can be correctly sensed
       * when the device listens to the channel for the first time after the
       * switching e.g. after channel switching, the channel may be sensed as
       * busy due to other devices' tramissions started before the end of
       * the switching.
       */
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the completion of the
          //channel switching.
          SwitchMaybeToCcaBusy ();
          return;
        }
      break;
    case SpectrumWifiPhy::RX:
      NS_LOG_DEBUG ("drop packet because already in Rx (power=" <<
                    rxPowerW << "W)");
      NotifyRxDrop (packet);
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the reception of the
          //currently-received packet.
          SwitchMaybeToCcaBusy ();
          return;
        }
      break;
    case SpectrumWifiPhy::TX:
      NS_LOG_DEBUG ("drop packet because already in Tx (power=" <<
                    rxPowerW << "W)");
      NotifyRxDrop (packet);
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the transmission of the
          //currently-transmitted packet.
          SwitchMaybeToCcaBusy ();
          return;
        }
      break;
    case SpectrumWifiPhy::CCA_BUSY:
    case SpectrumWifiPhy::IDLE:
      if (rxPowerW > m_edThresholdW) //checked here, no need to check in the payload reception (current implementation assumes constant rx power over the packet duration)
        {
          if (preamble == WIFI_PREAMBLE_NONE && m_mpdusNum == 0)
            {
              NS_LOG_DEBUG ("drop packet because no preamble has been received");
              NotifyRxDrop (packet);
              SwitchMaybeToCcaBusy ();
              return;
            }
          else if (preamble == WIFI_PREAMBLE_NONE && m_plcpSuccess == false) //A-MPDU reception fails
            {
              NS_LOG_DEBUG ("Drop MPDU because no plcp has been received");
              NotifyRxDrop (packet);
              SwitchMaybeToCcaBusy ();
              return;
            }
          else if (preamble != WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum == 0)
            {
              //received the first MPDU in an MPDU
              m_mpdusNum = ampduTag.GetNoOfMpdus () - 1;
              m_rxMpduReferenceNumber++;
            }
          else if (preamble == WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum > 0)
            {
              //received the other MPDUs that are part of the A-MPDU
              if (ampduTag.GetNoOfMpdus () < m_mpdusNum)
                {
                  NS_LOG_DEBUG ("Missing MPDU from the A-MPDU " << m_mpdusNum - ampduTag.GetNoOfMpdus ());
                  m_mpdusNum = ampduTag.GetNoOfMpdus ();
                }
              else
                {
                  m_mpdusNum--;
                }
            }
          else if (preamble != WIFI_PREAMBLE_NONE && m_mpdusNum > 0 )
            {
              NS_LOG_DEBUG ("Didn't receive the last MPDUs from an A-MPDU " << m_mpdusNum);
              m_mpdusNum = 0;
            }

          NS_LOG_DEBUG ("sync to signal (power=" << rxPowerW << "W)");
          //sync to signal
          m_state->SwitchToRx (rxDuration);
          NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
          NotifyRxBegin (packet);
          m_interference.NotifyRxStart ();

          if (preamble != WIFI_PREAMBLE_NONE)
            {
              NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
              m_endPlcpRxEvent = Simulator::Schedule (preambleAndHeaderDuration, &SpectrumWifiPhy::StartReceivePacket, this,
                                                      packet, txVector, preamble, mpdutype, event);
            }

          NS_ASSERT (m_endRxEvent.IsExpired ());
          m_endRxEvent = Simulator::Schedule (rxDuration, &SpectrumWifiPhy::EndReceive, this,
                                              packet, preamble, mpdutype, event);
        }
      else
        {
          NS_LOG_DEBUG ("drop packet because signal power too Small (" <<
                        rxPowerW << "<" << m_edThresholdW << ")");
          NotifyRxDrop (packet);
          m_plcpSuccess = false;
          SwitchMaybeToCcaBusy ();
          return;
        }
      break;
    case SpectrumWifiPhy::SLEEP:
      NS_LOG_DEBUG ("drop packet because in sleep mode");
      NotifyRxDrop (packet);
      m_plcpSuccess = false;
      break;
    }

  return;
}

void
SpectrumWifiPhy::StartReceivePacket (Ptr<Packet> packet,
                                 WifiTxVector txVector,
                                 enum WifiPreamble preamble,
                                 enum mpduType mpdutype,
                                 Ptr<InterferenceHelper::Event> event)
{
  NS_LOG_FUNCTION (this << packet << txVector.GetMode () << preamble << (uint32_t)mpdutype);
  NS_ASSERT (IsStateRx ());
  NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
  AmpduTag ampduTag;
  WifiMode txMode = txVector.GetMode ();

  struct InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculatePlcpHeaderSnrPer (event);

  NS_LOG_DEBUG ("snr(dB)=" << RatioToDb (snrPer.snr) << ", per=" << snrPer.per);

  if (m_random->GetValue () > snrPer.per) //plcp reception succeeded
    {
      if (IsModeSupported (txMode) || IsMcsSupported (txMode))
        {
          NS_LOG_DEBUG ("receiving plcp payload"); //endReceive is already scheduled
          m_plcpSuccess = true;
        }
      else //mode is not allowed
        {
          NS_LOG_DEBUG ("drop packet because it was sent using an unsupported mode (" << txMode << ")");
          NotifyRxDrop (packet);
          m_plcpSuccess = false;
        }
    }
  else //plcp reception failed
    {
      NS_LOG_DEBUG ("drop packet because plcp preamble/header reception failed");
      NotifyRxDrop (packet);
      m_plcpSuccess = false;
    }
}

Ptr<WifiSpectrumPhyInterface> 
SpectrumWifiPhy::GetSpectrumPhy (void) const
{
  return m_wifiSpectrumPhyInterface;
}

Ptr<AntennaModel>
SpectrumWifiPhy::GetRxAntenna (void) const
{
  return m_antenna;
}

void
SpectrumWifiPhy::SetAntenna (Ptr<AntennaModel> a)
{
  NS_LOG_FUNCTION (this << a);
  m_antenna = a;
}

void
SpectrumWifiPhy::CreateWifiSpectrumPhyInterface (Ptr<NetDevice> device)
{
  m_wifiSpectrumPhyInterface = CreateObject<WifiSpectrumPhyInterface> ();
  m_wifiSpectrumPhyInterface->SetSpectrumWifiPhy (this);
  m_wifiSpectrumPhyInterface->SetDevice (device);
}

void
SpectrumWifiPhy::SendPacket (Ptr<const Packet> packet, WifiTxVector txVector, WifiPreamble preamble)
{
  SendPacket (packet, txVector, preamble, NORMAL_MPDU);
}

void
SpectrumWifiPhy::SendPacket (Ptr<const Packet> packet, WifiTxVector txVector, WifiPreamble preamble, enum mpduType mpdutype)
{
  NS_LOG_FUNCTION (this << packet << txVector.GetMode () 
    << txVector.GetMode ().GetDataRate (txVector)
    << preamble << (uint32_t)txVector.GetTxPowerLevel () << (uint32_t)mpdutype);
  /* Transmission can happen if:
   *  - we are syncing on a packet. It is the responsability of the
   *    MAC layer to avoid doing this but the PHY does nothing to
   *    prevent it.
   *  - we are idle
   */
  NS_ASSERT (!m_state->IsStateTx () && !m_state->IsStateSwitching ());

  if (m_state->IsStateSleep ())
    {
      NS_LOG_DEBUG ("Dropping packet because in sleep mode");
      NotifyTxDrop (packet);
      return;
    }

  Time txDuration = CalculateTxDuration (packet->GetSize (), txVector, preamble, GetFrequency (), mpdutype, 1);
  NS_ASSERT (txDuration > NanoSeconds (0));

  if (m_state->IsStateRx ())
    {
      m_endPlcpRxEvent.Cancel ();
      m_endRxEvent.Cancel ();
      m_interference.NotifyRxEnd ();
    }
  NotifyTxBegin (packet);
  uint32_t dataRate500KbpsUnits;
  if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HT || txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_VHT)
    {
      dataRate500KbpsUnits = 128 + txVector.GetMode ().GetMcsValue ();
    }
  else
    {
      dataRate500KbpsUnits = txVector.GetMode ().GetDataRate (txVector.GetChannelWidth (), txVector.IsShortGuardInterval (), 1) * txVector.GetNss () / 500000;
    }
  if (mpdutype == MPDU_IN_AGGREGATE && preamble != WIFI_PREAMBLE_NONE)
    {
      //send the first MPDU in an MPDU
      m_txMpduReferenceNumber++;
    }
  struct mpduInfo aMpdu;
  aMpdu.type = mpdutype;
  aMpdu.mpduRefNumber = m_txMpduReferenceNumber;
  NotifyMonitorSniffTx (packet, (uint16_t)GetFrequency (), GetChannelNumber (), dataRate500KbpsUnits, preamble, txVector, aMpdu);
  m_state->SwitchToTx (txDuration, packet, GetPowerDbm (txVector.GetTxPowerLevel ()), txVector, preamble);
  //
  // Spectrum elements added here
  //
  Ptr<Packet> newPacket = packet->Copy (); // obtain non-const Packet
  WifiPhyTag oldtag;
  newPacket->RemovePacketTag (oldtag);
  WifiPhyTag tag (txVector, preamble, mpdutype);
  newPacket->AddPacketTag (tag);

  double txPowerWatts = DbmToW (GetPowerDbm (txVector.GetTxPowerLevel ()) + m_txGainDb);

  Ptr<SpectrumValue> txPowerSpectrum =
    WifiSpectrumHelper::CreateTxPowerSpectralDensity (txPowerWatts, txVector.GetMode (), m_channelNumber);

  Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters> ();
  txParams->duration = txDuration;
  txParams->psd = txPowerSpectrum;
  NS_ASSERT_MSG (m_wifiSpectrumPhyInterface, "SpectrumPhy() is not set; maybe forgot to call CreateWifiSpectrumPhyInterface?");
  txParams->txPhy = m_wifiSpectrumPhyInterface->GetObject<SpectrumPhy> ();
  txParams->txAntenna = m_antenna;
  txParams->packet = newPacket;
  NS_LOG_DEBUG ("Starting transmission with wifi power " << WToDbm (txPowerWatts) << " dBm on channel " << m_channelNumber);
  NS_LOG_DEBUG ("Starting transmission with spectrum power " << WToDbm (Integral (*txPowerSpectrum)) << " dBm");
  m_channel->StartTx (txParams);
}

uint32_t
SpectrumWifiPhy::GetNModes (void) const
{
  return m_deviceRateSet.size ();
}

WifiMode
SpectrumWifiPhy::GetMode (uint32_t mode) const
{
  return m_deviceRateSet[mode];
}

bool
SpectrumWifiPhy::IsModeSupported (WifiMode mode) const
{
  for (uint32_t i = 0; i < GetNModes (); i++)
    {
      if (mode == GetMode (i))
        {
          return true;
        }
    }
  return false;
}
bool
SpectrumWifiPhy::IsMcsSupported (WifiMode mcs)
{
  for (uint32_t i = 0; i < GetNMcs (); i++)
    {
      if (mcs == GetMcs (i))
        {
          return true;
        }
    }
  return false;
}

uint32_t
SpectrumWifiPhy::GetNTxPower (void) const
{
  return m_nTxPower;
}

void
SpectrumWifiPhy::Configure80211a (void)
{
  NS_LOG_FUNCTION (this);
  m_channelCenterFrequency = 5180; //MHz
  m_channelNumber = 36;
  SetChannelWidth (20); //MHz

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate48Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate54Mbps ());
}

void
SpectrumWifiPhy::Configure80211b (void)
{
  NS_LOG_FUNCTION (this);
  m_channelCenterFrequency = 2412; //MHz
  m_channelNumber = 1;
  SetChannelWidth (22); //MHz

  m_deviceRateSet.push_back (WifiPhy::GetDsssRate1Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate2Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate5_5Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate11Mbps ());
}

void
SpectrumWifiPhy::Configure80211g (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211b ();
  SetChannelWidth (20); //20 MHz

  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate9Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate24Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate48Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate54Mbps ());
}

void
SpectrumWifiPhy::Configure80211_10Mhz (void)
{
  NS_LOG_FUNCTION (this);
  m_channelCenterFrequency = 5035; //MHz
  m_channelNumber = 7;
  SetChannelWidth (10); //MHz

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate3MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate4_5MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate27MbpsBW10MHz ());
}

void
SpectrumWifiPhy::Configure80211_5Mhz (void)
{
  NS_LOG_FUNCTION (this);
  m_channelCenterFrequency = 5180; //MHz
  m_channelNumber = 36;
  SetChannelWidth (5); //MHz

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate1_5MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate2_25MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate3MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate4_5MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate13_5MbpsBW5MHz ());
}

void
SpectrumWifiPhy::ConfigureHolland (void)
{
  NS_LOG_FUNCTION (this);
  m_channelCenterFrequency = 5180; //MHz
  m_channelNumber = 36;
  SetChannelWidth (20); //MHz

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate54Mbps ());
}

void
SpectrumWifiPhy::ConfigureHtDeviceMcsSet (void)
{
  NS_LOG_FUNCTION (this);

  bool htFound = false;
  for (std::vector<uint32_t>::size_type i = 0; i < m_bssMembershipSelectorSet.size (); i++)
    {
      if (m_bssMembershipSelectorSet[i] == HT_PHY)
        {
          htFound = true;
          break;
        }
    }
  if (htFound)
    {
      // erase all HtMcs modes from deviceMcsSet
      size_t index = m_deviceMcsSet.size () - 1;
      for (std::vector<WifiMode>::reverse_iterator rit = m_deviceMcsSet.rbegin (); rit != m_deviceMcsSet.rend(); ++rit, --index)
        {
          if (m_deviceMcsSet[index].GetModulationClass ()== WIFI_MOD_CLASS_HT)
            {
              m_deviceMcsSet.erase (m_deviceMcsSet.begin () + index);
            }
        }
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs0 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs1 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs2 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs3 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs4 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs5 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs6 ());
      m_deviceMcsSet.push_back (WifiPhy::GetHtMcs7 ());
      if (GetSupportedTxSpatialStreams () > 1)
        {
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs8 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs9 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs10 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs11 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs12 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs13 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs14 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs15 ());
        }
      if (GetSupportedTxSpatialStreams () > 2)
        {
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs16 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs17 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs18 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs19 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs20 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs21 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs22 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs23 ());
        }
      if (GetSupportedTxSpatialStreams () > 3)
        {
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs24 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs25 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs26 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs27 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs28 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs29 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs30 ());
          m_deviceMcsSet.push_back (WifiPhy::GetHtMcs31 ());
        }
    }
}

void
SpectrumWifiPhy::Configure80211n_2_4Ghz (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211b ();
  Configure80211g ();
  m_channelCenterFrequency = 2412; //MHz
  m_channelNumber = 1;
  SetChannelWidth (20); //20 MHz
  m_bssMembershipSelectorSet.push_back (HT_PHY);
  ConfigureHtDeviceMcsSet ();
}

void
SpectrumWifiPhy::Configure80211n_5Ghz (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211a ();
  m_channelCenterFrequency = 5180; //MHz
  m_channelNumber = 36;
  SetChannelWidth (20); //MHz
  m_bssMembershipSelectorSet.push_back (HT_PHY);
  ConfigureHtDeviceMcsSet ();
}

void
SpectrumWifiPhy::Configure80211ac (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211n_5Ghz ();
  m_channelCenterFrequency = 5210; //MHz
  m_channelNumber = 42;
  SetChannelWidth (80); //80 MHz

  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs0 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs1 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs2 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs3 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs4 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs5 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs6 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs7 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs8 ());
  m_deviceMcsSet.push_back (WifiPhy::GetVhtMcs9 ());

  m_bssMembershipSelectorSet.push_back (VHT_PHY);
}

void
SpectrumWifiPhy::RegisterListener (WifiPhyListener *listener)
{
  m_state->RegisterListener (listener);
}

void
SpectrumWifiPhy::UnregisterListener (WifiPhyListener *listener)
{
  m_state->UnregisterListener (listener);
}

bool
SpectrumWifiPhy::IsStateCcaBusy (void)
{
  return m_state->IsStateCcaBusy ();
}

bool
SpectrumWifiPhy::IsStateIdle (void)
{
  return m_state->IsStateIdle ();
}

bool
SpectrumWifiPhy::IsStateBusy (void)
{
  return m_state->IsStateBusy ();
}

bool
SpectrumWifiPhy::IsStateRx (void)
{
  return m_state->IsStateRx ();
}

bool
SpectrumWifiPhy::IsStateTx (void)
{
  return m_state->IsStateTx ();
}

bool
SpectrumWifiPhy::IsStateSwitching (void)
{
  return m_state->IsStateSwitching ();
}

bool
SpectrumWifiPhy::IsStateSleep (void)
{
  return m_state->IsStateSleep ();
}

Time
SpectrumWifiPhy::GetStateDuration (void)
{
  return m_state->GetStateDuration ();
}

Time
SpectrumWifiPhy::GetDelayUntilIdle (void)
{
  return m_state->GetDelayUntilIdle ();
}

Time
SpectrumWifiPhy::GetLastRxStartTime (void) const
{
  return m_state->GetLastRxStartTime ();
}

double
SpectrumWifiPhy::DbToRatio (double dB) const
{
  double ratio = std::pow (10.0, dB / 10.0);
  return ratio;
}

double
SpectrumWifiPhy::DbmToW (double dBm) const
{
  double mW = std::pow (10.0, dBm / 10.0);
  return mW / 1000.0;
}

double
SpectrumWifiPhy::WToDbm (double w) const
{
  return 10.0 * std::log10 (w * 1000.0);
}

double
SpectrumWifiPhy::RatioToDb (double ratio) const
{
  return 10.0 * std::log10 (ratio);
}

double
SpectrumWifiPhy::GetEdThresholdW (void) const
{
  return m_edThresholdW;
}

double
SpectrumWifiPhy::GetPowerDbm (uint8_t power) const
{
  NS_ASSERT (m_txPowerBaseDbm <= m_txPowerEndDbm);
  NS_ASSERT (m_nTxPower > 0);
  double dbm;
  if (m_nTxPower > 1)
    {
      dbm = m_txPowerBaseDbm + power * (m_txPowerEndDbm - m_txPowerBaseDbm) / (m_nTxPower - 1);
    }
  else
    {
      NS_ASSERT_MSG (m_txPowerBaseDbm == m_txPowerEndDbm, "cannot have TxPowerEnd != TxPowerStart with TxPowerLevels == 1");
      dbm = m_txPowerBaseDbm;
    }
  return dbm;
}

void
SpectrumWifiPhy::EndReceive (Ptr<Packet> packet, enum WifiPreamble preamble, enum mpduType mpdutype, Ptr<InterferenceHelper::Event> event)
{
  NS_LOG_FUNCTION (this << packet << event);
  NS_ASSERT (IsStateRx ());
  NS_ASSERT (event->GetEndTime () == Simulator::Now ());

  struct InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculatePlcpPayloadSnrPer (event);
  m_interference.NotifyRxEnd ();
  bool rxSucceeded;

  if (m_plcpSuccess == true)
    {
      NS_LOG_DEBUG ("mode=" << (event->GetPayloadMode ().GetDataRate (event->GetTxVector ())) <<
                    ", snr(dB)=" << RatioToDb (snrPer.snr) << ", per=" << snrPer.per << ", size=" << packet->GetSize ());

      if (m_random->GetValue () > snrPer.per)
        {
          NotifyRxEnd (packet);
          uint32_t dataRate500KbpsUnits;
          if ((event->GetPayloadMode ().GetModulationClass () == WIFI_MOD_CLASS_HT) || (event->GetPayloadMode ().GetModulationClass () == WIFI_MOD_CLASS_VHT))
            {
              dataRate500KbpsUnits = 128 + event->GetPayloadMode ().GetMcsValue ();
            }
          else
            {
              dataRate500KbpsUnits = event->GetPayloadMode ().GetDataRate (event->GetTxVector ().GetChannelWidth (), event->GetTxVector ().IsShortGuardInterval (), 1) * event->GetTxVector ().GetNss () / 500000;
            }
          struct signalNoiseDbm signalNoise;
          signalNoise.signal = RatioToDb (event->GetRxPowerW ()) + 30;
          signalNoise.noise = RatioToDb (event->GetRxPowerW () / snrPer.snr) - GetRxNoiseFigure () + 30;
          struct mpduInfo aMpdu;
          aMpdu.type = mpdutype;
          aMpdu.mpduRefNumber = m_rxMpduReferenceNumber;
          NotifyMonitorSniffRx (packet, (uint16_t)GetFrequency (), GetChannelNumber (), dataRate500KbpsUnits, event->GetPreambleType (), event->GetTxVector (), aMpdu, signalNoise);
          m_state->SwitchFromRxEndOk (packet, snrPer.snr, event->GetTxVector (), event->GetPreambleType ());
          rxSucceeded = true;
        }
      else
        {
          /* failure. */
          NotifyRxDrop (packet);
          bool isEndOfFrame = ((mpdutype == NORMAL_MPDU && preamble != WIFI_PREAMBLE_NONE) || (mpdutype == LAST_MPDU_IN_AGGREGATE && preamble == WIFI_PREAMBLE_NONE));
          m_state->SwitchFromRxEndError (packet, snrPer.snr, isEndOfFrame);
          rxSucceeded = false;
        }
      if (!m_rxCallback.IsNull ())
        {
         m_rxCallback(rxSucceeded);
        }
    }
  else
    {
      bool isEndOfFrame = ((mpdutype == NORMAL_MPDU && preamble != WIFI_PREAMBLE_NONE) || (mpdutype == LAST_MPDU_IN_AGGREGATE && preamble == WIFI_PREAMBLE_NONE));
      m_state->SwitchFromRxEndError (packet, snrPer.snr, isEndOfFrame);
      rxSucceeded = false;
      if (!m_rxCallback.IsNull () && isEndOfFrame)
        {
          m_rxCallback(rxSucceeded);
        }
    }

  if (preamble == WIFI_PREAMBLE_NONE && mpdutype == LAST_MPDU_IN_AGGREGATE)
    {
      m_plcpSuccess = false;
    }

}

int64_t
SpectrumWifiPhy::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_random->SetStream (stream);
  return 1;
}

void
SpectrumWifiPhy::SetNumberOfTransmitAntennas (uint32_t tx)
{
  m_numberOfTransmitters = tx;
  ConfigureHtDeviceMcsSet ();
}

void
SpectrumWifiPhy::SetNumberOfReceiveAntennas (uint32_t rx)
{
  m_numberOfReceivers = rx;
}

void
SpectrumWifiPhy::SetLdpc (bool Ldpc)
{
  m_ldpc = Ldpc;
}

void
SpectrumWifiPhy::SetStbc (bool stbc)
{
  m_stbc = stbc;
}

void
SpectrumWifiPhy::SetGreenfield (bool greenfield)
{
  m_greenfield = greenfield;
}

bool
SpectrumWifiPhy::GetGuardInterval (void) const
{
  return m_guardInterval;
}

void
SpectrumWifiPhy::SetGuardInterval (bool guardInterval)
{
  m_guardInterval = guardInterval;
}

uint32_t
SpectrumWifiPhy::GetFrequency (void) const
{
  double frequency;
  switch (m_standard)
    {
    case WIFI_PHY_STANDARD_80211b:
    case WIFI_PHY_STANDARD_80211g:
    case WIFI_PHY_STANDARD_80211n_2_4GHZ:
      NS_ASSERT_MSG (m_channelNumber >= 1 && m_channelNumber <= 14, "Invalid channel number " << m_channelNumber);
      if (m_channelNumber < 14)
        {
          return (2407 + 5 * m_channelNumber);
        }
      else
        {
          return 2484; // channel 14
        }
      NS_ASSERT (false);
      break;
    case WIFI_PHY_STANDARD_80211a:
    case WIFI_PHY_STANDARD_80211n_5GHZ:
    case WIFI_PHY_STANDARD_80211ac:
    case WIFI_PHY_STANDARD_holland:
      NS_ASSERT_MSG (m_channelNumber >= 7 && m_channelNumber <= 196, "Invalid channel number " << m_channelNumber);
      frequency = m_channelMap [m_channelNumber];
      NS_ASSERT (frequency); // not found in map
      return (frequency);
      break;
    case WIFI_PHY_STANDARD_80211_10MHZ:
      // 10 MHz channels are defined in some regions; 
      frequency = m_channelMap10Mhz [m_channelNumber];
      if (frequency == 0)
        {
          // Users may instead choose a 20/40/80/160 MHz channel number;
          // in this case, reuse those center frequencies
          frequency = m_channelMap [m_channelNumber];
        }
      NS_ASSERT (frequency); // not found in map
      return frequency;
      break;
    case WIFI_PHY_STANDARD_80211_5MHZ:
      // Since no standard channel numbers exist for 5MHz channels, we
      // reuse the 20/40/80/160 channel center frequencies here
      frequency = m_channelMap [m_channelNumber];
      NS_ASSERT (frequency); // not found in map
      break;
    default:
      NS_ASSERT_MSG (false, "Standard " << m_standard << " not found");
      break;
    }
  return 0;
}

uint32_t
SpectrumWifiPhy::GetNumberOfTransmitAntennas (void) const
{
  return m_numberOfTransmitters;
}

uint32_t
SpectrumWifiPhy::GetNumberOfReceiveAntennas (void) const
{
  return m_numberOfReceivers;
}

bool
SpectrumWifiPhy::GetLdpc (void) const
{
  return m_ldpc;
}

bool
SpectrumWifiPhy::GetStbc (void) const
{
  return m_stbc;
}

bool
SpectrumWifiPhy::GetGreenfield (void) const
{
  return m_greenfield;
}

bool
SpectrumWifiPhy::GetShortPlcpPreambleSupported (void) const
{
  return m_shortPreamble;
}

void
SpectrumWifiPhy::SetShortPlcpPreambleSupported (bool enable)
{
  m_shortPreamble = enable;
}

void
SpectrumWifiPhy::SetChannelWidth (uint32_t channelwidth)
{
  NS_ASSERT_MSG (channelwidth == 5 || channelwidth == 10 || channelwidth == 20 || channelwidth == 22 || channelwidth == 40 || channelwidth == 80 || channelwidth == 160, "wrong channel width value");
  m_channelWidth = channelwidth;
  AddSupportedChannelWidth (channelwidth);
}

uint32_t
SpectrumWifiPhy::GetChannelWidth (void) const
{
  return m_channelWidth;
}

uint8_t 
SpectrumWifiPhy::GetSupportedRxSpatialStreams (void) const
{
  return (static_cast<uint8_t> (GetNumberOfReceiveAntennas ()));
}

uint8_t 
SpectrumWifiPhy::GetSupportedTxSpatialStreams (void) const
{
  return (static_cast<uint8_t> (GetNumberOfTransmitAntennas ()));
}

void
SpectrumWifiPhy::AddSupportedChannelWidth (uint32_t width)
{
  NS_LOG_FUNCTION (this << width);
  for (std::vector<uint32_t>::size_type i = 0; i != m_supportedChannelWidthSet.size (); i++)
    {
      if (m_supportedChannelWidthSet[i] == width)
        {
          return;
        }
    }
  m_supportedChannelWidthSet.push_back (width);
}

std::vector<uint32_t> 
SpectrumWifiPhy::GetSupportedChannelWidthSet (void) const
{
  return m_supportedChannelWidthSet;
}

uint32_t
SpectrumWifiPhy::GetNBssMembershipSelectors (void) const
{
  return m_bssMembershipSelectorSet.size ();
}

uint32_t
SpectrumWifiPhy::GetBssMembershipSelector (uint32_t selector) const
{
  return m_bssMembershipSelectorSet[selector];
}

WifiModeList
SpectrumWifiPhy::GetMembershipSelectorModes (uint32_t selector)
{
  uint32_t id = GetBssMembershipSelector (selector);
  WifiModeList supportedmodes;
  if (id == HT_PHY || id == VHT_PHY)
    {
      //mandatory MCS 0 to 7
      supportedmodes.push_back (WifiPhy::GetHtMcs0 ());
      supportedmodes.push_back (WifiPhy::GetHtMcs1 ());
      supportedmodes.push_back (WifiPhy::GetHtMcs2 ());
      supportedmodes.push_back (WifiPhy::GetHtMcs3 ());
      supportedmodes.push_back (WifiPhy::GetHtMcs4 ());
      supportedmodes.push_back (WifiPhy::GetHtMcs5 ());
      supportedmodes.push_back (WifiPhy::GetHtMcs6 ());
      supportedmodes.push_back (WifiPhy::GetHtMcs7 ());
    }
  if (id == VHT_PHY)
    {
      //mandatory MCS 0 to 9
      supportedmodes.push_back (WifiPhy::GetVhtMcs0 ());
      supportedmodes.push_back (WifiPhy::GetVhtMcs1 ());
      supportedmodes.push_back (WifiPhy::GetVhtMcs2 ());
      supportedmodes.push_back (WifiPhy::GetVhtMcs3 ());
      supportedmodes.push_back (WifiPhy::GetVhtMcs4 ());
      supportedmodes.push_back (WifiPhy::GetVhtMcs5 ());
      supportedmodes.push_back (WifiPhy::GetVhtMcs6 ());
      supportedmodes.push_back (WifiPhy::GetVhtMcs7 ());
      supportedmodes.push_back (WifiPhy::GetVhtMcs8 ());
      supportedmodes.push_back (WifiPhy::GetVhtMcs9 ());
    }
  return supportedmodes;
}

uint8_t
SpectrumWifiPhy::GetNMcs (void) const
{
  return m_deviceMcsSet.size ();
}

WifiMode
SpectrumWifiPhy::GetMcs (uint8_t mcs) const
{
  return m_deviceMcsSet[mcs];
}

} //namespace ns3
