#include <iostream>
#include <stdio.h>
#include <regex>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <ctime>
#include <sstream>
#include <sys/stat.h>
#include <cassert>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/config-store-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SocketBoundTcpRoutingExample");

std::string dir = "examples/results/hetero2-100mb-1mb/";
bool thEnabled = false;
static const uint32_t totalTxBytes = 104857600;
int simDuration = 100000; // In Seconds
static uint32_t currentTxBytes = 0;
static const uint32_t writeSize = 1446;
uint8_t data[writeSize];
int number_of_sources = 60;
std::map<int, _Float64> fctMap;
int fctDone = 0;
int fctMax = simDuration;

void StartFlow (Ptr<Socket>, Ipv4Address, uint16_t);
void StartFlow2 (Ptr<Socket>, Ipv4Address, uint16_t);
void WriteUntilBufferFull (Ptr<Socket>, uint32_t);

void SendStuff (Ptr<Socket> sock, Ipv4Address dstaddr, uint16_t port);
void BindSock (Ptr<Socket> sock, Ptr<NetDevice> netdev);
void printFctStats ();

int queue_number = 120;

uint64_t queueSize;
uint64_t queueSize2;
uint64_t queueSize3;
uint64_t queueSize4;
uint64_t queueSize5;
uint64_t queueSize6;

Ptr<OutputStreamWrapper> qSize_stream;

uint64_t droppedPackets;
uint64_t droppedPackets2;
uint64_t droppedPackets3;
uint64_t droppedPackets4;
uint64_t droppedPackets5;
uint64_t droppedPackets6;
uint64_t previously_droppedPackets;
uint64_t previously_droppedPackets2;
uint64_t previously_droppedPackets3;
uint64_t previously_droppedPackets4;
uint64_t previously_droppedPackets5;
uint64_t previously_droppedPackets6;
//float transmittedPackets;
Ptr<OutputStreamWrapper> dropped_stream;

uint32_t pkt_count = 0;
uint32_t pkt_count2 = 0;
uint32_t pkt_count3 = 0;
uint32_t pkt_count4 = 0;
uint32_t pkt_count5 = 0;
uint32_t pkt_count6 = 0;
uint32_t pkt_count7 = 0;
uint32_t pkt_count8 = 0;
uint32_t prev_pkt_count = 0;
uint32_t prev_pkt_count2 = 0;
uint32_t prev_pkt_count3 = 0;
uint32_t prev_pkt_count4 = 0;
uint32_t prev_pkt_count5 = 0;
uint32_t prev_pkt_count6 = 0;
uint32_t prev_pkt_count7 = 0;
uint32_t prev_pkt_count8 = 0;
uint32_t Http_data_count = 0;
uint32_t Http_data_count2 = 0;
uint32_t Http_data_count3 = 0;
uint32_t Http_data_count4 = 0;
uint32_t Http_data_count5 = 0;
uint32_t Http_data_count6 = 0;
uint32_t Http_data_count7 = 0;
uint32_t Http_data_count8 = 0;
uint32_t prev_Http_data_count = 0;
uint32_t prev_Http_data_count2 = 0;
uint32_t prev_Http_data_count3 = 0;
uint32_t prev_Http_data_count4 = 0;
uint32_t prev_Http_data_count5 = 0;
uint32_t prev_Http_data_count6 = 0;
uint32_t prev_pHttp_data_count7 = 0;
uint32_t prev_Http_data_count8 = 0;
Time prevTime = Seconds (0);
Ptr<OutputStreamWrapper> bottleneckTransmittedStream;

uint64_t packetsTransmitted;
uint64_t previous_transmitted_packets = 0;
uint64_t packetsTransmitted2;
uint64_t previous_transmitted_packets2 = 0;
uint64_t packetsTransmitted3;
uint64_t previous_transmitted_packets3 = 0;
uint64_t packetsTransmitted4;
uint64_t previous_transmitted_packets4 = 0;
uint64_t packetsTransmitted5;
uint64_t previous_transmitted_packets5 = 0;
uint64_t packetsTransmitted6;
uint64_t previous_transmitted_packets6 = 0;
Time prevTime02 = Seconds (0);
Ptr<OutputStreamWrapper> utilization;

uint64_t cwnd[60];
Ptr<OutputStreamWrapper> congestion_window;

/**
 * --------------------------------------------------------------------- 
 *                  Functions for tracing Queue Size
 * --------------------------------------------------------------------- 
*/

static void
plotQsizeChange (uint32_t oldQSize, uint32_t newQSize)
{
  queueSize = newQSize;
}
static void
plotQsizeChange2 (uint32_t oldQSize, uint32_t newQSize)
{
  queueSize2 = newQSize;
}
static void
plotQsizeChange3 (uint32_t oldQSize, uint32_t newQSize)
{
  queueSize3 = newQSize;
}
static void
plotQsizeChange4 (uint32_t oldQSize, uint32_t newQSize)
{
  queueSize4 = newQSize;
}
static void
plotQsizeChange5 (uint32_t oldQSize, uint32_t newQSize)
{
  queueSize5 = newQSize;
}
static void
plotQsizeChange6 (uint32_t oldQSize, uint32_t newQSize)
{
  queueSize6 = newQSize;
}

static void
TraceQueueSize ()
{

  *qSize_stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << queueSize << "\t"
                              << queueSize2 << "\t" << queueSize3 << "\t" << queueSize4 << "\t"
                              << queueSize5 << "\t" << queueSize6 << std::endl;
}

static void
StartTracingQueueSize ()
{
  Config::ConnectWithoutContext (
      "/NodeList/" + std::to_string (queue_number) +
          "/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/PacketsInQueue",
      MakeCallback (&plotQsizeChange));
  Config::ConnectWithoutContext (
      "/NodeList/" + std::to_string (queue_number + 2) +
          "/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/PacketsInQueue",
      MakeCallback (&plotQsizeChange2));
  Config::ConnectWithoutContext (
      "/NodeList/" + std::to_string (queue_number + 4) +
          "/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/PacketsInQueue",
      MakeCallback (&plotQsizeChange3));
  Config::ConnectWithoutContext (
      "/NodeList/" + std::to_string (queue_number + 6) +
          "/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/PacketsInQueue",
      MakeCallback (&plotQsizeChange4));
  Config::ConnectWithoutContext (
      "/NodeList/" + std::to_string (queue_number + 8) +
          "/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/PacketsInQueue",
      MakeCallback (&plotQsizeChange5));
  Config::ConnectWithoutContext (
      "/NodeList/" + std::to_string (queue_number + 10) +
          "/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/PacketsInQueue",
      MakeCallback (&plotQsizeChange6));
}

/**
 * --------------------------------------------------------------------- 
 *                  Functions for tracing Packet Loss 
 * --------------------------------------------------------------------- 
*/

static void
RxDrop (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
  droppedPackets++;
}
static void
RxDrop2 (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
  droppedPackets2++;
}
static void
RxDrop3 (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
  droppedPackets3++;
}
static void
RxDrop4 (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
  droppedPackets4++;
}
static void
RxDrop5 (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
  droppedPackets5++;
}
static void
RxDrop6 (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
  droppedPackets6++;
}

static void
TraceDroppedPacket (std::string droppedTrFileName)
{
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number) +
                                     "/DeviceList/*/TxQueue/Drop",
                                 MakeBoundCallback (&RxDrop, dropped_stream));
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 2) +
                                     "/DeviceList/*/TxQueue/Drop",
                                 MakeBoundCallback (&RxDrop2, dropped_stream));
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 4) +
                                     "/DeviceList/*/TxQueue/Drop",
                                 MakeBoundCallback (&RxDrop3, dropped_stream));
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 6) +
                                     "/DeviceList/*/TxQueue/Drop",
                                 MakeBoundCallback (&RxDrop4, dropped_stream));
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 8) +
                                     "/DeviceList/*/TxQueue/Drop",
                                 MakeBoundCallback (&RxDrop5, dropped_stream));
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 10) +
                                     "/DeviceList/*/TxQueue/Drop",
                                 MakeBoundCallback (&RxDrop6, dropped_stream));
}

static void
TraceDroppedPkts ()
{
  float packetLoss = droppedPackets - previously_droppedPackets;
  float packetLoss2 = droppedPackets2 - previously_droppedPackets2;
  float packetLoss3 = droppedPackets3 - previously_droppedPackets3;
  float packetLoss4 = droppedPackets4 - previously_droppedPackets4;
  float packetLoss5 = droppedPackets5 - previously_droppedPackets5;
  float packetLoss6 = droppedPackets6 - previously_droppedPackets6;
  *dropped_stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << packetLoss << "\t"
                                << packetLoss2 << "\t" << packetLoss3 << "\t" << packetLoss4 << "\t"
                                << packetLoss5 << "\t" << packetLoss6 << std::endl;
  previously_droppedPackets = droppedPackets;
  previously_droppedPackets2 = droppedPackets2;
  previously_droppedPackets3 = droppedPackets3;
  previously_droppedPackets4 = droppedPackets4;
  previously_droppedPackets5 = droppedPackets5;
  previously_droppedPackets6 = droppedPackets6;
}

/**
 * --------------------------------------------------------------------- 
 *                  Functions for tracing Network Throughput
 * --------------------------------------------------------------------- 
*/

// For saving packets received at the sink at each time instant into the defined ascii stream
static void
TraceBottleneckTx (uint32_t pktSize)
{
  Time currrent_time = Now ();
  float btl_thr = ((pkt_count - prev_pkt_count) * 8 * pktSize) /
                  (1000 * 1000 * (currrent_time.GetSeconds () - prevTime.GetSeconds ()));
  float btl_thr2 = ((pkt_count2 - prev_pkt_count2) * 8 * pktSize) /
                   (1000 * 1000 * (currrent_time.GetSeconds () - prevTime.GetSeconds ()));
  float btl_thr3 = ((pkt_count3 - prev_pkt_count3) * 8 * pktSize) /
                   (1000 * 1000 * (currrent_time.GetSeconds () - prevTime.GetSeconds ()));
  float btl_thr4 = ((pkt_count4 - prev_pkt_count4) * 8 * pktSize) /
                   (1000 * 1000 * (currrent_time.GetSeconds () - prevTime.GetSeconds ()));
  float btl_thr5 = ((pkt_count5 - prev_pkt_count5) * 8 * pktSize) /
                   (1000 * 1000 * (currrent_time.GetSeconds () - prevTime.GetSeconds ()));
  float btl_thr6 = ((pkt_count6 - prev_pkt_count6) * 8 * pktSize) /
                   (1000 * 1000 * (currrent_time.GetSeconds () - prevTime.GetSeconds ()));
  float btl_thr7 = ((pkt_count7 - prev_pkt_count7) * 8 * pktSize) /
                   (1000 * 1000 * (currrent_time.GetSeconds () - prevTime.GetSeconds ()));
  float btl_thr8 = ((pkt_count8 - prev_pkt_count8) * 8 * pktSize) /
                   (1000 * 1000 * (currrent_time.GetSeconds () - prevTime.GetSeconds ()));
  *bottleneckTransmittedStream->GetStream ()
      << Simulator::Now ().GetSeconds () << "\t" << btl_thr << "\t" << btl_thr2 << "\t" << btl_thr3
      << "\t" << btl_thr4 << "\t" << btl_thr5 << "\t" << btl_thr6 << "\t" << btl_thr7 << "\t"
      << btl_thr8 << std::endl;
  prev_pkt_count = pkt_count;
  prev_pkt_count2 = pkt_count2;
  prev_pkt_count3 = pkt_count3;
  prev_pkt_count4 = pkt_count4;
  prev_pkt_count5 = pkt_count5;
  prev_pkt_count6 = pkt_count6;
  prev_pkt_count7 = pkt_count7;
  prev_pkt_count8 = pkt_count8;
  prevTime = currrent_time;
}

//For counting the number of packets received at the sink
static void
SinkRxCount (Ptr<const Packet> p, const Address &ad)
{
  pkt_count++;
}
static void
SinkRxCount2 (Ptr<const Packet> p, const Address &ad)
{
  pkt_count2++;
}
static void
SinkRxCount3 (Ptr<const Packet> p, const Address &ad)
{
  pkt_count3++;
}
static void
SinkRxCount4 (Ptr<const Packet> p, const Address &ad)
{
  pkt_count4++;
}
static void
SinkRxCount5 (Ptr<const Packet> p, const Address &ad)
{
  pkt_count5++;
}
static void
SinkRxCount6 (Ptr<const Packet> p, const Address &ad)
{
  pkt_count6++;
}
static void
SinkRxCount7 (Ptr<const Packet> p, const Address &ad)
{
  pkt_count7++;
}
static void
SinkRxCount8 (Ptr<const Packet> p, const Address &ad)
{
  pkt_count8++;
}
static void
HtttpTxCount (Ptr<const Packet> p)
{
  Http_data_count = Http_data_count + +p->GetSize ();
}
static void
HtttpTxCount2 (Ptr<const Packet> p)
{
  Http_data_count2 = Http_data_count2 + +p->GetSize ();
}
static void
HtttpTxCount3 (Ptr<const Packet> p)
{
  Http_data_count3 = Http_data_count3 + +p->GetSize ();
}
static void
HtttpTxCount4 (Ptr<const Packet> p)
{
  Http_data_count4 = Http_data_count4 + +p->GetSize ();
}
static void
HtttpTxCount5 (Ptr<const Packet> p)
{
  Http_data_count5 = Http_data_count5 + +p->GetSize ();
}
static void
HtttpTxCount6 (Ptr<const Packet> p)
{
  Http_data_count6 = Http_data_count6 + +p->GetSize ();
}
static void
HtttpTxCount7 (Ptr<const Packet> p)
{
  Http_data_count7 = Http_data_count7 + +p->GetSize ();
}
static void
HtttpTxCount8 (Ptr<const Packet> p)
{
  Http_data_count8 = Http_data_count8 + +p->GetSize ();
}

static void
StartTracingSink ()
{
  for (int i = 0; i < 5; i++)
    {
      Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                         "/ApplicationList/*/$ns3::PacketSink/Rx",
                                     MakeCallback (&SinkRxCount)); // Route 1
      if (i == 4)
        Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                           "/ApplicationList/*/$ns3::ThreeGppHttpServer/Tx",
                                       MakeCallback (&HtttpTxCount));
    }

  for (int i = 5; i < 10; i++)
    {
      Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                         "/ApplicationList/*/$ns3::PacketSink/Rx",
                                     MakeCallback (&SinkRxCount2)); // Route 2
      if (i == 9)
        Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                           "/ApplicationList/*/$ns3::ThreeGppHttpServer/Tx",
                                       MakeCallback (&HtttpTxCount2));
    }

  for (int i = 10; i < 24; i++)
    {
      Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                         "/ApplicationList/*/$ns3::PacketSink/Rx",
                                     MakeCallback (&SinkRxCount3)); // Route 3
      if (i == 23)
        Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                           "/ApplicationList/*/$ns3::ThreeGppHttpServer/Tx",
                                       MakeCallback (&HtttpTxCount3));
    }

  for (int i = 24; i < 29; i++)
    {
      Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                         "/ApplicationList/*/$ns3::PacketSink/Rx",
                                     MakeCallback (&SinkRxCount4)); // Route 4
      if (i == 28)
        Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                           "/ApplicationList/*/$ns3::ThreeGppHttpServer/Tx",
                                       MakeCallback (&HtttpTxCount4));
    }

  for (int i = 29; i < 32; i++)
    {
      Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                         "/ApplicationList/*/$ns3::PacketSink/Rx",
                                     MakeCallback (&SinkRxCount5)); // Route 5
      if (i == 31)
        Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                           "/ApplicationList/*/$ns3::ThreeGppHttpServer/Tx",
                                       MakeCallback (&HtttpTxCount5));
    }

  for (int i = 32; i < 53; i++)
    {
      Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                         "/ApplicationList/*/$ns3::PacketSink/Rx",
                                     MakeCallback (&SinkRxCount6)); // Route 6
      if (i == 52)
        Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                           "/ApplicationList/*/$ns3::ThreeGppHttpServer/Tx",
                                       MakeCallback (&HtttpTxCount6));
    }

  for (int i = 53; i < 57; i++)
    {
      Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                         "/ApplicationList/*/$ns3::PacketSink/Rx",
                                     MakeCallback (&SinkRxCount7)); // Route 7
      if (i == 56)
        Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                           "/ApplicationList/*/$ns3::ThreeGppHttpServer/Tx",
                                       MakeCallback (&HtttpTxCount7));
    }

  for (int i = 57; i < 60; i++)
    {
      Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                         "/ApplicationList/*/$ns3::PacketSink/Rx",
                                     MakeCallback (&SinkRxCount8)); // Route 8
      if (i == 59)
        Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i + (i + 1)) +
                                           "/ApplicationList/*/$ns3::ThreeGppHttpServer/Tx",
                                       MakeCallback (&HtttpTxCount8));
    }
}

/**
 * --------------------------------------------------------------------- 
 *                  Functions for tracing Link Utilization
 * --------------------------------------------------------------------- 
*/

static void
TxxPacket (Ptr<const Packet> p)
{
  packetsTransmitted++;
}
static void
Txx2Packet (Ptr<const Packet> p)
{
  packetsTransmitted2++;
}
static void
Txx3Packet (Ptr<const Packet> p)
{
  packetsTransmitted3++;
}
static void
Txx4Packet (Ptr<const Packet> p)
{
  packetsTransmitted4++;
}
static void
Txx5Packet (Ptr<const Packet> p)
{
  packetsTransmitted5++;
}
static void
Txx6Packet (Ptr<const Packet> p)
{
  packetsTransmitted6++;
}

static void
TraceUtilization (uint32_t pktSize)
{
  Time currrent_time = Now ();
  float btl_thr = ((packetsTransmitted - previous_transmitted_packets) * 8 * pktSize) /
                  (1000 * 1000 * (currrent_time.GetSeconds () - prevTime02.GetSeconds ()));
  float btl_thr2 = ((packetsTransmitted2 - previous_transmitted_packets2) * 8 * pktSize) /
                   (1000 * 1000 * (currrent_time.GetSeconds () - prevTime02.GetSeconds ()));
  float btl_thr3 = ((packetsTransmitted3 - previous_transmitted_packets3) * 8 * pktSize) /
                   (1000 * 1000 * (currrent_time.GetSeconds () - prevTime02.GetSeconds ()));
  float btl_thr4 = ((packetsTransmitted4 - previous_transmitted_packets4) * 8 * pktSize) /
                   (1000 * 1000 * (currrent_time.GetSeconds () - prevTime02.GetSeconds ()));
  float btl_thr5 = ((packetsTransmitted5 - previous_transmitted_packets5) * 8 * pktSize) /
                   (1000 * 1000 * (currrent_time.GetSeconds () - prevTime02.GetSeconds ()));
  float btl_thr6 = ((packetsTransmitted6 - previous_transmitted_packets6) * 8 * pktSize) /
                   (1000 * 1000 * (currrent_time.GetSeconds () - prevTime02.GetSeconds ()));
  *utilization->GetStream () << Simulator::Now ().GetSeconds () << "\t" << btl_thr / 100 << "\t"
                             << btl_thr2 / 40 << "\t" << btl_thr3 / 40 << "\t" << btl_thr4 / 60
                             << "\t" << btl_thr5 / 60 << "\t" << btl_thr6 / 60 << std::endl;
  previous_transmitted_packets = packetsTransmitted;
  previous_transmitted_packets2 = packetsTransmitted2;
  previous_transmitted_packets3 = packetsTransmitted3;
  previous_transmitted_packets4 = packetsTransmitted4;
  previous_transmitted_packets5 = packetsTransmitted5;
  previous_transmitted_packets6 = packetsTransmitted6;
  prevTime02 = currrent_time;
}

static void
StartTracingUtilization ()
{
  packetsTransmitted = 0;
  previous_transmitted_packets = 0;
  //Router 01
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 1) +
                                     "/DeviceList/2/PhyTxEnd",
                                 MakeCallback (&TxxPacket));
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 1) +
                                     "/DeviceList/3/PhyTxEnd",
                                 MakeCallback (&TxxPacket));
  //Router 02
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 3) +
                                     "/DeviceList/2/PhyTxEnd",
                                 MakeCallback (&Txx2Packet));
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 3) +
                                     "/DeviceList/3/PhyTxEnd",
                                 MakeCallback (&Txx2Packet));
  //Router 03
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 5) +
                                     "/DeviceList/2/PhyTxEnd",
                                 MakeCallback (&Txx3Packet));
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 5) +
                                     "/DeviceList/3/PhyTxEnd",
                                 MakeCallback (&Txx3Packet));
  //Router 04
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 7) +
                                     "/DeviceList/2/PhyTxEnd",
                                 MakeCallback (&Txx4Packet));
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 7) +
                                     "/DeviceList/3/PhyTxEnd",
                                 MakeCallback (&Txx4Packet));
  //Router 05
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 9) +
                                     "/DeviceList/2/PhyTxEnd",
                                 MakeCallback (&Txx5Packet));
  //Router 06
  for (int i = 2; i < 51; i++)
    Config::ConnectWithoutContext ("/NodeList/" + std::to_string (queue_number + 11) +
                                       "/DeviceList/" + std::to_string (i) + "/PhyTxEnd",
                                   MakeCallback (&Txx6Packet));
}
/**
 * --------------------------------------------------------------------- 
 *                  Functions for tracing Congestion Window
 * --------------------------------------------------------------------- 
*/
static void
plotCwnd (Ptr<OutputStreamWrapper> stream, uint32_t i, uint32_t oldval, uint32_t newval)
{
  cwnd[i] = newval;
}

static void
TraceCwnd ()
{
  *congestion_window->GetStream () << Simulator::Now ().GetSeconds ();
  for (int i = 0; i < number_of_sources - 8; i++)
    *congestion_window->GetStream () << "\t" << cwnd[i];
  *congestion_window->GetStream () << std::endl;
}

void
StartTraceCwnd (uint32_t socketId)
{
  int UDP_Source_number[8] = {4, 9, 23, 28, 31, 52, 56, 59};
  int itr = 0;
  for (int i = 0; i < number_of_sources - 8; i++)
    {
      if (i == UDP_Source_number[itr])
        {
          i++;
          itr++;
        }
      Config::ConnectWithoutContext ("/NodeList/" + std::to_string (i * 2) +
                                         "/$ns3::TcpL4Protocol/SocketList/" +
                                         std::to_string (socketId) + "/CongestionWindow",
                                     MakeBoundCallback (&plotCwnd, congestion_window, i));
    }
}

void
TrackTotalRx (Ptr<PacketSink> pktSink, int index)
{
  // std::cout << pktSink->GetTotalRx () << " ";
  Time now = Simulator::Now ();
  if (now.GetSeconds () > 10 * fctMax)
    exit (0);

  if (pktSink->GetTotalRx () < totalTxBytes)
    {
      Simulator::Schedule (Seconds (0.01), &TrackTotalRx, pktSink, index);
    }
  else
    {
      fctMap[index] = now.GetSeconds ();
      fctDone++;
      fctMax = now.GetSeconds ();
      std::cout << now.GetSeconds () << " " << index << " done\n";
    }

  if (fctDone == 52)
    {
      printFctStats ();
    }
}

int
main (int argc, char *argv[])
{
  // --------------------------------------------------
  //        Variable Declaration & Configurations
  // --------------------------------------------------
  std::string flavour = "TcpCubic"; //TCP variant considered
  // int simDuration = 250; // In Seconds
  std::string RTT = "94ms"; //round-trip time of each TCP flow
  int number_of_nodes = 12 + (number_of_sources * 2);
  int number_of_links = 15 + (number_of_sources * 2);
  int router_starting_index = number_of_sources * 2;
  int pktSize = 1446;
  float queue_size = 2084;
  float R6_queue_size = 2084 * (0.6);

  /**
     * -----------------------------------------------------------------
     * Getting Input from Command Line 
     * -----------------------------------------------------------------
     */

  CommandLine cmd (__FILE__);
  cmd.AddValue ("RTT", "Round Trip Time", RTT);
  cmd.AddValue ("R6_queue_size", "Queue Size", R6_queue_size);
  cmd.AddValue ("flavour", "TCP Flavour", flavour);
  cmd.AddValue ("thEnabled", "ThEnabled", thEnabled);

  cmd.Parse (argc, argv);

  NS_LOG_UNCOND ("QueueSize : " << R6_queue_size << "\tRound Trip Time: " << RTT);
  std::cout << "QueueSize : " << R6_queue_size << "\tRound Trip Time: " << RTT;

  /**
     * -----------------------------------------------------------------
     * Configurations 
     * -----------------------------------------------------------------
     */

  ConfigStore config;
  config.ConfigureDefaults ();
  config.ConfigureAttributes ();
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (pktSize));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (1));
  Config::SetDefault ("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (20 * 1000));

  // --------------------------------------------------
  //            Node Initilisation
  // --------------------------------------------------
  NodeContainer nodes;
  nodes.Create (number_of_nodes);

  InternetStackHelper internet;
  internet.Install (nodes);

  // --------------------------------------------------
  //            Point-to-point links
  // --------------------------------------------------
  NodeContainer containers[number_of_links];
  int a = 0;
  for (int i = 0; i < number_of_sources; i++)
    containers[a++] = NodeContainer (nodes.Get (i * 2), nodes.Get (router_starting_index));

  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index), nodes.Get (router_starting_index + 1));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 1), nodes.Get (router_starting_index + 2));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 1), nodes.Get (router_starting_index + 4));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 2), nodes.Get (router_starting_index + 3));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 4), nodes.Get (router_starting_index + 5));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 3), nodes.Get (router_starting_index + 10));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 3), nodes.Get (router_starting_index + 6));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 5), nodes.Get (router_starting_index + 6));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 5), nodes.Get (router_starting_index + 8));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 6), nodes.Get (router_starting_index + 7));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 7), nodes.Get (router_starting_index + 10));
  containers[a++] = NodeContainer (nodes.Get (router_starting_index + 10),
                                   nodes.Get (router_starting_index + 11));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 7), nodes.Get (router_starting_index + 8));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 8), nodes.Get (router_starting_index + 9));
  containers[a++] =
      NodeContainer (nodes.Get (router_starting_index + 9), nodes.Get (router_starting_index + 10));

  for (int i = 0; i < 5; i++)
    containers[a++] =
        NodeContainer (nodes.Get (router_starting_index + 3), nodes.Get (i + (i + 1))); // Route 1

  for (int i = 5; i < 10; i++)
    containers[a++] =
        NodeContainer (nodes.Get (router_starting_index + 11), nodes.Get (i + (i + 1))); // Route 2

  for (int i = 10; i < 24; i++)
    containers[a++] =
        NodeContainer (nodes.Get (router_starting_index + 11), nodes.Get (i + (i + 1))); // Route 3

  for (int i = 24; i < 29; i++)
    containers[a++] =
        NodeContainer (nodes.Get (router_starting_index + 11), nodes.Get (i + (i + 1))); // Route 4

  for (int i = 29; i < 32; i++)
    containers[a++] =
        NodeContainer (nodes.Get (router_starting_index + 9), nodes.Get (i + (i + 1))); // Route 5

  for (int i = 32; i < 53; i++)
    containers[a++] =
        NodeContainer (nodes.Get (router_starting_index + 11), nodes.Get (i + (i + 1))); // Route 6

  for (int i = 53; i < 57; i++)
    containers[a++] =
        NodeContainer (nodes.Get (router_starting_index + 11), nodes.Get (i + (i + 1))); // Route 7

  for (int i = 57; i < 60; i++)
    containers[a++] =
        NodeContainer (nodes.Get (router_starting_index + 5), nodes.Get (i + (i + 1))); // Route 8

  // We create the channels first without any IP addressing information

  // Defining the links to be used between nodes
  double range = (double) (stoi (RTT.substr (0, RTT.length () - 2)));
  double min = range - (0.1 * range);
  double max = range + (0.1 * range);

  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (min));
  x->SetAttribute ("Max", DoubleValue (max));

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
  p2p.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize",
                QueueSizeValue (QueueSize (std::to_string (0 / number_of_sources) +
                                           "p"))); // p in 1000p stands for packets
  p2p.DisableFlowControl ();

  PointToPointHelper bottleneck_p2p;
  bottleneck_p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  bottleneck_p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
  bottleneck_p2p.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize",
                           QueueSizeValue (QueueSize ("2084p"))); // p in 1000p stands for packets
  bottleneck_p2p.DisableFlowControl ();

  // --------------------------------------------------
  //            Setting up TCP Flavor
  // --------------------------------------------------

  Ptr<TcpL4Protocol> proto;
  //Setting TCP flavor for TCP sources
  //For each router, last node needs to be UDP stacked with HTTP. Secondly, from the remaining nodes, first half are to be TCPNewReno and second half are to be TCPCubic.
  // Route 01
  for (int i = 0; i < (5 - 1); i++)
    {
      proto = nodes.Get (i * 2)->GetObject<TcpL4Protocol> ();
      if (i < 2)
        proto->SetAttribute ("SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
      else
        proto->SetAttribute ("SocketType", TypeIdValue (TcpCubic::GetTypeId ()));
    }
  //Route 02
  for (int i = 5; i < (10 - 1); i++)
    {
      proto = nodes.Get (i * 2)->GetObject<TcpL4Protocol> ();
      if (i < 7)
        proto->SetAttribute ("SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
      else
        proto->SetAttribute ("SocketType", TypeIdValue (TcpCubic::GetTypeId ()));
    }
  //Route 03
  for (int i = 10; i < (24 - 1); i++)
    {
      proto = nodes.Get (i * 2)->GetObject<TcpL4Protocol> ();
      if (i < 17)
        proto->SetAttribute ("SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
      else
        proto->SetAttribute ("SocketType", TypeIdValue (TcpCubic::GetTypeId ()));
    }
  //Route 04
  for (int i = 24; i < (29 - 1); i++)
    {
      proto = nodes.Get (i * 2)->GetObject<TcpL4Protocol> ();
      if (i < 26)
        proto->SetAttribute ("SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
      else
        proto->SetAttribute ("SocketType", TypeIdValue (TcpCubic::GetTypeId ()));
    }
  //Route 05
  for (int i = 29; i < (32 - 1); i++)
    {
      proto = nodes.Get (i * 2)->GetObject<TcpL4Protocol> ();
      if (i < 30)
        proto->SetAttribute ("SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
      else
        proto->SetAttribute ("SocketType", TypeIdValue (TcpCubic::GetTypeId ()));
    }
  //Route 06
  for (int i = 32; i < (53 - 1); i++)
    {
      proto = nodes.Get (i * 2)->GetObject<TcpL4Protocol> ();
      if (i < 42)
        proto->SetAttribute ("SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
      else
        proto->SetAttribute ("SocketType", TypeIdValue (TcpCubic::GetTypeId ()));
    }
  //Route 07
  for (int i = 53; i < (57 - 1); i++)
    {
      proto = nodes.Get (i * 2)->GetObject<TcpL4Protocol> ();
      if (i < 55)
        proto->SetAttribute ("SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
      else
        proto->SetAttribute ("SocketType", TypeIdValue (TcpCubic::GetTypeId ()));
    }
  //Route 08
  for (int i = 57; i < (60 - 1); i++)
    {
      proto = nodes.Get (i * 2)->GetObject<TcpL4Protocol> ();
      if (i < 58)
        proto->SetAttribute ("SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
      else
        proto->SetAttribute ("SocketType", TypeIdValue (TcpCubic::GetTypeId ()));
    }

  // --------------------------------------------------
  //            NetDevice Containers
  // --------------------------------------------------

  NetDeviceContainer devices[number_of_links];
  for (uint32_t i = 0; i < number_of_links; i++)
    {
      double delay = (x->GetValue ()) / 4;
      std::string delay_str = std::to_string (delay) + "ms";
      p2p.SetChannelAttribute ("Delay", StringValue (delay_str));
      switch (i)
        {
        case 63:
          bottleneck_p2p.SetDeviceAttribute ("DataRate", StringValue ("40Mbps"));
          queue_size = 2084 * (0.4);
          bottleneck_p2p.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize",
                                   QueueSizeValue (QueueSize (std::to_string (queue_size) + "p")));
          break;
        case 64:
          bottleneck_p2p.SetDeviceAttribute ("DataRate", StringValue ("40Mbps"));
          queue_size = 2084 * (0.4);
          bottleneck_p2p.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize",
                                   QueueSizeValue (QueueSize (std::to_string (queue_size) + "p")));
          break;
        case 69:
          bottleneck_p2p.SetDeviceAttribute ("DataRate", StringValue ("60Mbps"));
          queue_size = 2084 * (0.6);
          bottleneck_p2p.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize",
                                   QueueSizeValue (QueueSize (std::to_string (queue_size) + "p")));
          break;
        case 71:
          bottleneck_p2p.SetDeviceAttribute ("DataRate", StringValue ("60Mbps"));
          queue_size = R6_queue_size;
          bottleneck_p2p.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize",
                                   QueueSizeValue (QueueSize (std::to_string (queue_size) + "p")));
          break;
        case 73:
          bottleneck_p2p.SetDeviceAttribute ("DataRate", StringValue ("60Mbps"));
          queue_size = 2084 * (0.6);
          bottleneck_p2p.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize",
                                   QueueSizeValue (QueueSize (std::to_string (queue_size) + "p")));
          break;
        default:
          bottleneck_p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
          queue_size = 2084;
          bottleneck_p2p.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize",
                                   QueueSizeValue (QueueSize (std::to_string (queue_size) + "p")));
          break;
        }
      if (i < number_of_sources)
        devices[i] = p2p.Install (containers[i]);
      else if (i >= (number_of_sources + 15))
        devices[i] = p2p.Install (containers[i]);
      else
        devices[i] = bottleneck_p2p.Install (containers[i]);
    }

  // --------------------------------------------------
  //              IP addresses.
  // --------------------------------------------------
  Ipv4AddressHelper ipv4;
  Ipv4InterfaceContainer ip_addresses[number_of_links];
  std::string k = "1.";
  int j = 1;

  for (uint32_t i = 0; i < number_of_links; i++)
    {
      std::string ip_address = "10." + k + std::to_string (j) + ".0";
      ipv4.SetBase (ip_address.c_str (), "255.255.255.0");
      ip_addresses[i] = ipv4.Assign (devices[i]);
      if (i < number_of_sources - 1)
        j++;
      switch (i)
        {
        case 59:
          j = 1;
          k = "2.";
          break;
        case 60:
          j = 1;
          k = "3.";
          break;
        case 61:
          j = 2;
          k = "3.";
          break;
        case 62:
          j = 1;
          k = "5.";
          break;
        case 63:
          j = 2;
          k = "5.";
          break;
        case 64:
          j = 1;
          k = "10.";
          break;
        case 65:
          j = 2;
          k = "10.";
          break;
        case 66:
          j = 3;
          k = "10.";
          break;
        case 67:
          j = 4;
          k = "10.";
          break;
        case 68:
          j = 1;
          k = "15.";
          break;
        case 69:
          j = 1;
          k = "20.";
          break;
        case 70:
          j = 1;
          k = "25.";
          break;
        case 71:
          j = 2;
          k = "20.";
          break;
        case 72:
          j = 2;
          k = "25.";
          break;
        case 73:
          j = 3;
          k = "25.";
          break;
        case 74:
          j = 1;
          k = "30.";
          break;
        default:
          break;
        }
      if (i > (number_of_sources + 15 - 1))
        j++;
    }

  // --------------------------------------------------
  //           Source based Static Routing
  // --------------------------------------------------
  Ptr<Ipv4> ipv4_pointers[number_of_nodes];
  for (uint32_t i = 0; i < number_of_nodes; i++)
    {
      ipv4_pointers[i] = nodes.Get (i)->GetObject<Ipv4> ();
    }

  int interface = number_of_sources + 1;
  std::string source_ip, destination_ip;
  // 0 - Source, 1 - Destination
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> staticRouting[number_of_nodes];
  for (uint32_t i = 0; i < number_of_nodes; i++)
    {
      staticRouting[i] = ipv4RoutingHelper.GetStaticRouting (ipv4_pointers[i]);
    }

  // Route No 01
  // ----------------------------------------------------------------------------------------------------
  for (int i = 0; i < 5; i++)
    {
      destination_ip = "10.30." + std::to_string (i + 1) + ".2";
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            Ipv4Address ("10.2.1.2"), interface);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.1.2"), 2);
      staticRouting[router_starting_index + 2]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.1.2"), 2);

      source_ip = "10.30." + std::to_string (i + 1) + ".1";
      destination_ip = "10.1." + std::to_string (i + 1) + ".1";
      staticRouting[i + (i + 1)]->AddHostRouteTo (destination_ip.c_str (), source_ip.c_str (), 1);
      staticRouting[router_starting_index + 3]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.1.1"), 1);
      staticRouting[router_starting_index + 2]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.1.1"), 1);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.2.1.1"), 1);
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            destination_ip.c_str (), i + 1);
    }

  // Route No 02
  // ----------------------------------------------------------------------------------------------------
  for (int i = 5; i < 10; i++)
    {
      destination_ip = "10.30." + std::to_string (i + 1) + ".2";
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            Ipv4Address ("10.2.1.2"), interface);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.1.2"), 2);
      staticRouting[router_starting_index + 2]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.1.2"), 2);
      staticRouting[router_starting_index + 3]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.10.2.2"), 3);
      staticRouting[router_starting_index + 6]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.15.1.2"), 3);
      staticRouting[router_starting_index + 7]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.20.1.2"), 2);
      staticRouting[router_starting_index + 10]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.25.1.2"), 3);

      source_ip = "10.30." + std::to_string (i + 1) + ".1";
      destination_ip = "10.1." + std::to_string (i + 1) + ".1";
      staticRouting[i + (i + 1)]->AddHostRouteTo (destination_ip.c_str (), source_ip.c_str (), 1);
      staticRouting[router_starting_index + 11]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.25.1.1"), 1);
      staticRouting[router_starting_index + 10]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.20.1.1"), 2);
      staticRouting[router_starting_index + 7]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.15.1.1"), 1);
      staticRouting[router_starting_index + 6]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.10.2.1"), 1);
      staticRouting[router_starting_index + 3]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.1.1"), 1);
      staticRouting[router_starting_index + 2]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.1.1"), 1);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.2.1.1"), 1);
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            destination_ip.c_str (), i + 1);
    }

  // Route No 03
  // ----------------------------------------------------------------------------------------------------
  // src - > R1 -> R2 -> R6 -> des
  for (int i = 10; i < 24; i++)
    {
      destination_ip = "10.30." + std::to_string (i + 1) + ".2";
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            Ipv4Address ("10.2.1.2"), interface);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.1.2"), 2);
      staticRouting[router_starting_index + 2]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.1.2"), 2);
      staticRouting[router_starting_index + 3]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.10.1.2"), 2);
      staticRouting[router_starting_index + 10]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.25.1.2"), 3);

      source_ip = "10.30." + std::to_string (i + 1) + ".1";
      destination_ip = "10.1." + std::to_string (i + 1) + ".1";
      staticRouting[i + (i + 1)]->AddHostRouteTo (destination_ip.c_str (), source_ip.c_str (), 1);
      staticRouting[router_starting_index + 11]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.25.1.1"), 1);
      staticRouting[router_starting_index + 10]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.10.1.1"), 1);
      staticRouting[router_starting_index + 3]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.1.1"), 1);
      staticRouting[router_starting_index + 2]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.1.1"), 1);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.2.1.1"), 1);
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            destination_ip.c_str (), i + 1);
    }
  // Route No 04
  // ----------------------------------------------------------------------------------------------------
  // src - > R1 -> R3 -> R4 -> R6 -> des
  for (int i = 24; i < 29; i++)
    {
      destination_ip = "10.30." + std::to_string (i + 1) + ".2";
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            Ipv4Address ("10.2.1.2"), interface);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.2.2"), 3);
      staticRouting[router_starting_index + 4]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.2.2"), 2);
      staticRouting[router_starting_index + 5]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.10.3.2"), 2);
      staticRouting[router_starting_index + 6]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.15.1.2"), 3);
      staticRouting[router_starting_index + 7]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.20.1.2"), 2);
      staticRouting[router_starting_index + 10]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.25.1.2"), 3);

      source_ip = "10.30." + std::to_string (i + 1) + ".1";
      destination_ip = "10.1." + std::to_string (i + 1) + ".1";
      staticRouting[i + (i + 1)]->AddHostRouteTo (destination_ip.c_str (), source_ip.c_str (), 1);
      staticRouting[router_starting_index + 11]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.25.1.1"), 1);
      staticRouting[router_starting_index + 10]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.20.1.1"), 2);
      staticRouting[router_starting_index + 7]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.15.1.1"), 1);
      staticRouting[router_starting_index + 6]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.10.3.1"), 2);
      staticRouting[router_starting_index + 5]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.2.1"), 1);
      staticRouting[router_starting_index + 4]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.2.1"), 1);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.2.1.1"), 1);
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            destination_ip.c_str (), i + 1);
    }
  // Route No 05
  // ----------------------------------------------------------------------------------------------------
  for (int i = 29; i < 32; i++)
    {
      destination_ip = "10.30." + std::to_string (i + 1) + ".2";
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            Ipv4Address ("10.2.1.2"), interface);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.2.2"), 3);
      staticRouting[router_starting_index + 4]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.2.2"), 2);
      staticRouting[router_starting_index + 5]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.10.3.2"), 2);
      staticRouting[router_starting_index + 6]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.15.1.2"), 3);
      staticRouting[router_starting_index + 7]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.20.2.2"), 3);
      staticRouting[router_starting_index + 8]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.25.2.2"), 3);

      source_ip = "10.30." + std::to_string (i + 1) + ".1";
      destination_ip = "10.1." + std::to_string (i + 1) + ".1";
      staticRouting[i + (i + 1)]->AddHostRouteTo (destination_ip.c_str (), source_ip.c_str (), 1);
      staticRouting[router_starting_index + 9]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.25.2.1"), 1);
      staticRouting[router_starting_index + 8]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.20.2.1"), 2);
      staticRouting[router_starting_index + 7]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.15.1.1"), 1);
      staticRouting[router_starting_index + 6]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.10.3.1"), 2);
      staticRouting[router_starting_index + 5]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.2.1"), 1);
      staticRouting[router_starting_index + 4]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.2.1"), 1);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.2.1.1"), 1);
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            destination_ip.c_str (), i + 1);
    }
  // Route No 06
  // ----------------------------------------------------------------------------------------------------
  // src -> R1 -> R3 -> R4 -> R5 -> R6 -> des
  for (int i = 32; i < 53; i++)
    {
      destination_ip = "10.30." + std::to_string (i + 1) + ".2";
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            Ipv4Address ("10.2.1.2"), interface);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.2.2"), 3);
      staticRouting[router_starting_index + 4]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.2.2"), 2);
      staticRouting[router_starting_index + 5]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.10.3.2"), 2);
      staticRouting[router_starting_index + 6]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.15.1.2"), 3);
      staticRouting[router_starting_index + 7]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.20.2.2"), 3);
      staticRouting[router_starting_index + 8]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.25.2.2"), 3);
      staticRouting[router_starting_index + 9]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.25.3.2"), 2);
      staticRouting[router_starting_index + 10]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.25.1.2"), 3);

      source_ip = "10.30." + std::to_string (i + 1) + ".1";
      destination_ip = "10.1." + std::to_string (i + 1) + ".1";
      staticRouting[i + (i + 1)]->AddHostRouteTo (destination_ip.c_str (), source_ip.c_str (), 1);
      staticRouting[router_starting_index + 11]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.25.1.1"), 1);
      staticRouting[router_starting_index + 10]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.25.3.1"), 4);
      staticRouting[router_starting_index + 9]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.25.2.1"), 1);
      staticRouting[router_starting_index + 8]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.20.2.1"), 2);
      staticRouting[router_starting_index + 7]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.15.1.1"), 1);
      staticRouting[router_starting_index + 6]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.10.3.1"), 2);
      staticRouting[router_starting_index + 5]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.2.1"), 1);
      staticRouting[router_starting_index + 4]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.2.1"), 1);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.2.1.1"), 1);
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            destination_ip.c_str (), i + 1);
    }
  // Route No 07
  // ----------------------------------------------------------------------------------------------------
  // src -> R1 -> R3 -> R5-> R6 -> des
  for (int i = 53; i < 57; i++)
    {
      destination_ip = "10.30." + std::to_string (i + 1) + ".2";
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            Ipv4Address ("10.2.1.2"), interface);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.2.2"), 3);
      staticRouting[router_starting_index + 4]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.2.2"), 2);
      staticRouting[router_starting_index + 5]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.10.4.2"), 3);
      staticRouting[router_starting_index + 8]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.25.2.2"), 3);
      staticRouting[router_starting_index + 9]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.25.3.2"), 2);
      staticRouting[router_starting_index + 10]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.25.1.2"), 3);

      source_ip = "10.30." + std::to_string (i + 1) + ".1";
      destination_ip = "10.1." + std::to_string (i + 1) + ".1";
      staticRouting[i + (i + 1)]->AddHostRouteTo (destination_ip.c_str (), source_ip.c_str (), 1);
      staticRouting[router_starting_index + 11]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.25.1.1"), 1);
      staticRouting[router_starting_index + 10]->AddHostRouteTo (destination_ip.c_str (),
                                                                 Ipv4Address ("10.25.3.1"), 4);
      staticRouting[router_starting_index + 9]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.25.2.1"), 1);
      staticRouting[router_starting_index + 8]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.10.4.1"), 1);
      staticRouting[router_starting_index + 5]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.2.1"), 1);
      staticRouting[router_starting_index + 4]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.2.1"), 1);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.2.1.1"), 1);
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            destination_ip.c_str (), i + 1);
    }
  // Route No 08
  // ----------------------------------------------------------------------------------------------------
  // src -> R1 -> R3 -> des
  for (int i = 57; i < 60; i++)
    {
      destination_ip = "10.30." + std::to_string (i + 1) + ".2";
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            Ipv4Address ("10.2.1.2"), interface);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.2.2"), 3);
      staticRouting[router_starting_index + 4]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.2.2"), 2);

      source_ip = "10.30." + std::to_string (i + 1) + ".1";
      destination_ip = "10.1." + std::to_string (i + 1) + ".1";
      staticRouting[i + (i + 1)]->AddHostRouteTo (destination_ip.c_str (), source_ip.c_str (), 1);
      staticRouting[router_starting_index + 5]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.5.2.1"), 1);
      staticRouting[router_starting_index + 4]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.3.2.1"), 1);
      staticRouting[router_starting_index + 1]->AddHostRouteTo (destination_ip.c_str (),
                                                                Ipv4Address ("10.2.1.1"), 1);
      staticRouting[router_starting_index]->AddHostRouteTo (destination_ip.c_str (),
                                                            destination_ip.c_str (), i + 1);
    }

  // Two routes to same destination - setting separate metrics.
  // You can switch these to see how traffic gets diverted via different routes
  // Adding Destinations to each source link
  // ----------------------------------------------------------------------------------------------------
  for (int i = 0; i < number_of_sources; i++)
    {
      destination_ip = "10.30." + std::to_string (i + 1) + ".2";
      source_ip = "10.1." + std::to_string (i + 1) + ".2";
      staticRouting[i * 2]->AddHostRouteTo (destination_ip.c_str (), source_ip.c_str (), 1, 5);
    }

  // --------------------------------------------------
  //       Packet Sink and TCP Sockets Scheduling
  // --------------------------------------------------

  // There are no apps that can utilize the Socket Option so doing the work directly..
  // Taken from tcp-large-transfer example
  NS_LOG_UNCOND ("Reached TCP Socket Scheduling");
  std::cout << ("Reached TCP Socket Scheduling");
  Ptr<Socket> src_socket[number_of_sources - 8];
  int UDP_sources_number[8] = {4, 9, 23, 28, 31, 52, 56, 59};
  int number_of_sources_in_each_route[8] = {4, 4, 13, 4, 2, 20, 3, 2};
  uint16_t dstport = 12345, itr = 0;
  std::string dstaddr[number_of_sources - 8];

  //Route wise creating 52 TCP sockets
  for (uint32_t i = 0; i < 8; i++)
    {
      int starting_index = UDP_sources_number[i] - number_of_sources_in_each_route[i];
      int last_index = UDP_sources_number[i];
      for (uint32_t j = starting_index; j < last_index; j++)
        {
          src_socket[itr] = Socket::CreateSocket (nodes.Get (j * 2),
                                                  TypeId::LookupByName ("ns3::TcpSocketFactory"));
          dstaddr[itr] = "10.30." + std::to_string (j + 1) + ".2";
          itr++;
        }
    }

  double mean = 0.1; // more like a ~ 0.06
  double bound = 1;
  Ptr<ExponentialRandomVariable> expRandomVariable = CreateObject<ExponentialRandomVariable> ();
  expRandomVariable->SetAttribute ("Mean", DoubleValue (mean));
  expRandomVariable->SetAttribute ("Bound", DoubleValue (bound));

  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), dstport));
  ApplicationContainer apps[number_of_sources - 8];
  itr = 0;
  for (uint32_t i = 0; i < 8; i++)
    {
      int starting_index = UDP_sources_number[i] - number_of_sources_in_each_route[i];
      int last_index = UDP_sources_number[i];
      for (uint32_t j = starting_index; j < last_index; j++)
        {
          apps[itr] = sink.Install (nodes.Get (j + (j + 1)));
          Ptr<PacketSink> pktSink = StaticCast<PacketSink> (apps[i].Get (0));
          apps[itr].Start (Seconds (0.0));
          Simulator::Schedule (Seconds (0.01), &TrackTotalRx, pktSink, j);
          apps[itr].Stop (Seconds (simDuration));
          itr++;
        }
    }

  double stime = 0;
  for (uint32_t i = 0; i < number_of_sources - 8; i++)
    {
      double gap = expRandomVariable->GetValue ();
      Simulator::Schedule (Seconds (gap), &StartFlow, src_socket[i],
                           Ipv4Address (dstaddr[i].c_str ()), dstport);
    }

  for (int i = 1; i < simDuration; i++)
    {
      for (int j = 0; j < number_of_sources - 8; j++)
        {
          double gap = expRandomVariable->GetValue ();
          Simulator::Schedule (Seconds (i + gap), &StartFlow2, src_socket[j],
                               Ipv4Address (dstaddr[j].c_str ()), dstport);
        }
    }

  // --------------------------------------------------
  //       UDP Sockets Scheduling
  // --------------------------------------------------

  NS_LOG_UNCOND ("UDP Applications on Node 4, 9, 23, 28, 31, 52, 56, 59");
  std::cout << ("UDP Applications on Node 4, 9, 23, 28, 31, 52, 56, 59");
  int number_of_UDP_sources = 8;
  uint16_t port = 4000;
  ApplicationContainer UDPApps[number_of_UDP_sources];
  OnOffHelper clientHelper ("ns3::UdpSocketFactory", Address ());
  clientHelper.SetConstantRate (DataRate ("1Mb/s"));
  ApplicationContainer UDPclientApps[number_of_UDP_sources];

  PacketSinkHelper UDPSink ("ns3::UdpSocketFactory",
                            InetSocketAddress (Ipv4Address::GetAny (), port));
  for (int i = 0; i < number_of_UDP_sources; i++)
    {
      UDPApps[i] =
          UDPSink.Install (nodes.Get (UDP_sources_number[i] + (UDP_sources_number[i] + 1)));
      UDPApps[i].Start (Seconds (0.0));
      UDPApps[i].Stop (Seconds (simDuration));
      AddressValue remoteAddress (InetSocketAddress (
          ip_addresses[UDP_sources_number[i] + number_of_sources + 15].GetAddress (1), port));
      clientHelper.SetAttribute ("Remote", remoteAddress);
      UDPclientApps[i] = (clientHelper.Install (nodes.Get (UDP_sources_number[i] * 2)));
      UDPclientApps[i].Start (Seconds (0.0));
      UDPclientApps[i].Stop (Seconds (simDuration));
    }

  // --------------------------------------------------
  //       HTTP Sockets Scheduling
  // --------------------------------------------------
  NS_LOG_UNCOND ("HTTP Applications on Nodes 4, 9, 23, 28, 31, 52, 56, 59");
  std::cout << ("HTTP Applications on Nodes 4, 9, 23, 28, 31, 52, 56, 59");
  int number_of_HTTP_Servers = 8;
  ApplicationContainer serverApplications[number_of_HTTP_Servers];
  ApplicationContainer clientApplications[number_of_HTTP_Servers];
  for (int i = 0; i < number_of_HTTP_Servers; i++)
    {
      Address serverAddress =
          Address (ip_addresses[UDP_sources_number[i] + number_of_sources + 15].GetAddress (1));

      ThreeGppHttpServerHelper serverHelper (serverAddress);
      serverApplications[i] =
          serverHelper.Install (nodes.Get (UDP_sources_number[i] + (UDP_sources_number[i] + 1)));

      Ptr<ThreeGppHttpServer> httpServer =
          serverApplications[i].Get (0)->GetObject<ThreeGppHttpServer> ();
      PointerValue varPtr_server;
      httpServer->GetAttribute ("Variables", varPtr_server);
      Ptr<ThreeGppHttpVariables> httpVariables = varPtr_server.Get<ThreeGppHttpVariables> ();
      httpVariables->SetMainObjectSizeMean (1024000); //1Mb
      httpVariables->SetEmbeddedObjectSizeMean (1024000); //1Mb
      serverApplications[i].Start (Seconds (0.0));
      serverApplications[i].Stop (Seconds (simDuration));

      ThreeGppHttpClientHelper HTTPclientHelper (serverAddress);
      clientApplications[i] = HTTPclientHelper.Install (nodes.Get (UDP_sources_number[i] * 2));
      clientApplications[i].Start (Seconds (0.0));
      clientApplications[i].Stop (Seconds (simDuration));
    }

  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream =
      Create<OutputStreamWrapper> ("dynamic-global-routing.routes", std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (2), routingStream);

  /**
       * -----------------------------------------------------------------
       *                  Trace Files and Simulation Run
       * -----------------------------------------------------------------
      */
  std::string iterator;
  if (R6_queue_size > 100)
    iterator = "DropTail-" + RTT.substr (0, RTT.length () - 2) + "RTT";
  else if (R6_queue_size == 100)
    iterator = "Threshold100-" + RTT + "RTT";
  else if (R6_queue_size == 15)
    iterator = "Threshold15-" + RTT + "RTT";

  dir += iterator + "/";
  struct stat buffer;
  [[maybe_unused]] int retVal;
  if ((stat (dir.c_str (), &buffer)) == 0)
    {
      std::string dirToRemove = "rm -rf " + dir;
      retVal = system (dirToRemove.c_str ());
      NS_ASSERT_MSG (retVal == 0, "Error in return value");
    }
  std::string dirToSave = "mkdir -p " + dir;
  retVal = system (dirToSave.c_str ());

  // Configuring file stream to write the Qsize
  // AsciiTraceHelper ascii_qsize;
  // qSize_stream = ascii_qsize.CreateFileStream(dir + "QS.txt");

  // AsciiTraceHelper ascii_dropped;
  // dropped_stream = ascii_dropped.CreateFileStream (dir + "Loss.txt");

  // AsciiTraceHelper ascii_qsize_tx;
  // bottleneckTransmittedStream = ascii_qsize_tx.CreateFileStream(dir + "th.txt");

  // AsciiTraceHelper ascii_tx;
  // utilization = ascii_tx.CreateFileStream(dir + "U.txt");

  AsciiTraceHelper ascii_cwnd;
  congestion_window = ascii_cwnd.CreateFileStream (dir + "cwnd.txt");

  // Simulator::Schedule( Seconds(stime), &StartTracingQueueSize);
  // Simulator::Schedule( Seconds(stime), &StartTracingSink);
  // Simulator::Schedule( Seconds(stime), &StartTracingUtilization);
  // Simulator::Schedule( Seconds(stime), &TraceDroppedPacket, dir + "Loss.txt");
  Simulator::Schedule (Seconds (stime), &StartTraceCwnd, 0);
  for (int time = stime; time < simDuration;)
    {
      // Simulator::Schedule( Seconds(time), &TraceQueueSize);
      // Simulator::Schedule( Seconds(time), &TraceDroppedPkts);
      // Simulator::Schedule( Seconds(time), &TraceUtilization, 1446);
      // Simulator::Schedule( Seconds(time), &TraceBottleneckTx, 1446);
      Simulator::Schedule (Seconds (time), &TraceCwnd);
      time = time + 1;
    }
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
/**
 * -----------------------------------------------------------------
 *                  Socket Binding Functions
 * -----------------------------------------------------------------
*/
void
StartFlow (Ptr<Socket> localSocket, Ipv4Address servAddress, uint16_t servPort)
{
  NS_LOG_INFO ("Starting flow at time " << Simulator::Now ().GetSeconds ());
  currentTxBytes = 0;
  localSocket->Bind ();
  localSocket->Connect (InetSocketAddress (servAddress, servPort)); //connect

  // tell the tcp implementation to call WriteUntilBufferFull again
  // if we blocked and new tx buffer space becomes available
  localSocket->SetSendCallback (MakeCallback (&WriteUntilBufferFull));
  WriteUntilBufferFull (localSocket, localSocket->GetTxAvailable ());
}

void
StartFlow2 (Ptr<Socket> localSocket, Ipv4Address servAddress, uint16_t servPort)
{
  NS_LOG_INFO ("Starting flow again at time " << Simulator::Now ().GetSeconds ());
  currentTxBytes = 0;
  localSocket->SetSendCallback (MakeCallback (&WriteUntilBufferFull));
  WriteUntilBufferFull (localSocket, localSocket->GetTxAvailable ());
}

void
WriteUntilBufferFull (Ptr<Socket> localSocket, uint32_t txSpace)
{
  while (currentTxBytes < totalTxBytes && localSocket->GetTxAvailable () > 0)
    {
      uint32_t left = totalTxBytes - currentTxBytes;
      uint32_t dataOffset = currentTxBytes % writeSize;
      uint32_t toWrite = writeSize - dataOffset;
      toWrite = std::min (toWrite, left);
      toWrite = std::min (toWrite, localSocket->GetTxAvailable ());
      int amountSent = localSocket->Send (&data[dataOffset], toWrite, 0);
      if (amountSent < 0)
        {
          // we will be called again when new tx space becomes available.
          return;
        }
      currentTxBytes += amountSent;
    }
  // localSocket->Close ();
}

void
printFctStats ()
{
  std::ofstream myfile;
  myfile.open (dir + "fct.dat", std::fstream::in | std::fstream::out | std::fstream::app);
  myfile << "Flow\t FCT \n";
  for (auto x : fctMap)
    {
      int index = x.first;
      _Float64 fct = x.second;
      myfile << index << "\t" << fct << "\n";
    }
  myfile.close ();
  std::cout << "\nAFCT saved!\n";
  exit (0);
}