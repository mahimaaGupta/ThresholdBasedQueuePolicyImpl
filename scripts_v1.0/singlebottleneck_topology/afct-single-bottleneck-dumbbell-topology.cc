/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// Network topology
//
//       n0 ---------- n1 ---------- n2 ---------- n3
//            10 Mbps       1 Mbps        10 Mbps
//             1 ms         10 ms          1 ms
//
// - TCP flow from n0 to n3 using BulkSendApplication.
// - The following simulation output is stored in results/ in ns-3 top-level directory:
//   - cwnd traces are stored in cwndTraces folder
//   - queue length statistics are stored in queue-size.dat file
//   - pcaps are stored in pcap folder
//   - queueTraces folder contain the drop statistics at queue
//   - queueStats.txt file contains the queue stats and config.txt file contains
//     the simulation configuration.
// - The cwnd and queue length traces obtained from this example were tested against
//   the respective traces obtained from Linux Reno by using ns-3 Direct Code Execution.
//   See internet/doc/tcp.rst for more details.

#include <iostream>
#include <stdio.h>
#include <regex>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <ctime>
#include <sstream>
#include <sys/stat.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "router.h"

using namespace ns3;
std::string dir = "examples/results/ppt-afct-single/";
Time stopTime = Seconds (10000); // inf time
uint32_t segmentSize = 1500;
uint32_t numNodes = 60;
DataRate bottleneckBandwidth;
uint32_t rtt = 100;
std::string tcpType = "TcpNewReno";
bool isThresholdAQMEnabled = true;
uint64_t maxBytes = 300 * 1e6;
_Float64 avg_afct = 0;
_Float64 max_afct = 0;
_Float64 min_afct = 10000;

static uint32_t
GetNodeIdFromContext (std::string context)
{
  std::size_t const n1 = context.find_first_of ("/", 1);
  std::size_t const n2 = context.find_first_of ("/", n1 + 1);
  return std::stoul (context.substr (n1 + 1, n2 - n1 - 1));
}

// Function to check queue length of Router 1
void
CheckQueueSize (Ptr<QueueDisc> queue)
{
  uint32_t qSize = queue->GetCurrentSize ().GetValue ();

  // Check queue size every 1/5 of a second
  // if (Simulator::Now () < tracingDuration + tracingStartTime)
  // Simulator::Schedule (Seconds (0.001), &CheckQueueSize, queue);
  std::ofstream fPlotQueue (std::stringstream (dir + "queueSize.dat").str ().c_str (),
                            std::ios::out | std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueue.close ();
}

// Function to trace change in cwnd at n0
static void
CwndChange (std::string context, uint32_t oldCwnd, uint32_t newCwnd)
{
  uint32_t nodeId = GetNodeIdFromContext (context);
  std::string s = std::to_string (nodeId);
  std::ofstream fPlotQueue (dir + "cwndTraces/n" + s + ".dat", std::ios::out | std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << newCwnd / segmentSize << std::endl;
  fPlotQueue.close ();
}

uint32_t
get_flow_id (const std::string &str)
{
  std::regex flow_id_regex (R"(FlowId=(\d+))");
  std::smatch match;

  if (std::regex_search (str, match, flow_id_regex))
    return std::stoul (match[1]);
  return -1; // Return -1 if flow ID not found
}

static void
TraceLossEvents (Ptr<Packet> packet, Time now)
{
  std::stringstream ss;
  packet->PrintByteTags (ss);
  std::string byteTag = ss.str ();
  uint32_t flow_id = get_flow_id (byteTag);
  std::ofstream loss_events_file (dir + "/lossEvents.dat", std::ios::out | std::ios::app);
  loss_events_file << flow_id << " " << now.GetSeconds () << std::endl;
}

// Function to calculate drops in a particular Queue
static void
DropAtQueue (Ptr<OutputStreamWrapper> stream, Ptr<const QueueDiscItem> item)
{
  Time now = Simulator::Now ();
  *stream->GetStream () << now.GetSeconds () << " 1" << std::endl;
  Ptr<Packet> packet = item->GetPacket ();
  // if (now >= tracingStartTime)
  //   TraceLossEvents (packet, now);
}

// Trace Function for cwnd
void
TraceCwnd (uint32_t node, uint32_t cwndWindow,
           Callback<void, std::string, uint32_t, uint32_t> CwndTrace)
{
  Config::Connect ("/NodeList/" + std::to_string (node) + "/$ns3::TcpL4Protocol/SocketList/" +
                       std::to_string (cwndWindow) + "/CongestionWindow",
                   CwndTrace);
}

// Function to install BulkSend application
void
InstallBulkSend (Ptr<Node> node, Ipv4Address address, uint16_t port, std::string socketFactory,
                 uint32_t nodeId, uint32_t cwndWindow,
                 Callback<void, std::string, uint32_t, uint32_t> CwndTrace)
{
  BulkSendHelper source (socketFactory, InetSocketAddress (address, port));
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (node);
  sourceApps.Start (Seconds (0.0));
  // if (Simulator::Now () < tracingDuration + tracingStartTime)
  //   Simulator::Schedule (tracingStartTime, &TraceCwnd, nodeId, cwndWindow, CwndTrace);
  sourceApps.Stop (stopTime);
}

void
TrackTotalRx (Ptr<PacketSink> pktSink)
{
  // std::cout << pktSink->GetTotalRx () << " ";
  if (pktSink->GetTotalRx () < maxBytes)
    {
      Simulator::Schedule (Seconds (0.001), &TrackTotalRx, pktSink);
    }
  else
    {
      Time now = Simulator::Now ();
      avg_afct += now.GetSeconds ();
      min_afct = std::min (min_afct, now.GetSeconds ());
      max_afct = std::max (max_afct, now.GetSeconds ());
      std::cout << now.GetSeconds () << " done\n";
    }
}

// Function to install sink application
void
InstallPacketSink (Ptr<Node> node, uint16_t port, std::string socketFactory)
{
  PacketSinkHelper sink (socketFactory, InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (node);
  Ptr<PacketSink> pktSink = StaticCast<PacketSink> (sinkApps.Get (0));
  sinkApps.Start (Seconds (0.0));
  Simulator::Schedule (Seconds (0.001), &TrackTotalRx, pktSink);
  sinkApps.Stop (stopTime);
}

// Function to calculate variable access link delay
Time *
variedAccessLinkDelays (int numNodes, int mean)
{
  Time *delays = (Time *) malloc (numNodes * sizeof (Time));

  int sum = 0, x;
  for (int i = 0; i < numNodes - 1; i++)
    {
      x = rand () % mean + 1;
      sum += x;
      delays[i] = MilliSeconds (x);
    }

  x = mean * numNodes - sum;
  delays[numNodes - 1] = MilliSeconds (x);

  return delays;
}

// Calculate throughput and link utilisation
static void
TraceThroughputAndLU (Ptr<FlowMonitor> monitor, Ptr<Ipv4FlowClassifier> classifier,
                      P2PRouter *p2prouter)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  Time currTime = Now ();
  uint32_t currBytes = 0;

  auto count = stats.size () / 2;
  // aggregate rxBytes for first half flows (going towards sink):
  for (auto itr = stats.begin (); count > 0; ++itr, --count)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (itr->first);
      // std::cout << "Flow " << itr->first << " (" << t.sourceAddress << " -> "
      //           << t.destinationAddress << ")\n";
      currBytes += itr->second.rxBytes;
    }

  // Throughput is in MegaBits/Second
  double throughput = 8 * (currBytes - p2prouter->prevBytes) /
                      (1000 * 1000 * (currTime.GetSeconds () - p2prouter->prevTime.GetSeconds ()));
  double link_util = (throughput * 1000 * 1000 * 100 / p2prouter->linkBandwidth.GetBitRate ());

  std::ofstream thr (dir + "/throughput.dat", std::ios::out | std::ios::app);
  std::ofstream lu (dir + "/linkUtilization.dat", std::ios::out | std::ios::app);
  thr << currTime.GetSeconds () << " " << throughput << std::endl;
  lu << currTime.GetSeconds () << " " << link_util << std::endl;

  p2prouter->prevTime = currTime;
  p2prouter->prevBytes = currBytes;

  // Simulator::Schedule (Seconds (0.001 * rtt), &TraceThroughputAndLU, monitor, classifier,
  //  p2prouter);
}

int
main (int argc, char *argv[])
{
  uint32_t stream = 1;
  std::string socketFactory = "ns3::TcpSocketFactory";
  std::string qdiscTypeId = "ns3::FifoQueueDisc";
  uint32_t delAckCount = 1;
  std::string recovery = "ns3::TcpClassicRecovery";
  QueueSize queueSize = QueueSize ("2084p");

  CommandLine cmd;
  cmd.AddValue ("qdiscTypeId", "Queue disc for gateway (e.g., ns3::CoDelQueueDisc)", qdiscTypeId);
  cmd.AddValue ("segmentSize", "TCP segment size (bytes)", segmentSize);
  cmd.AddValue ("delAckCount", "Delayed ack count", delAckCount);
  cmd.AddValue ("numNodes", "Number of nodes in the sender", numNodes);
  cmd.AddValue ("roundTripTime", "Round trip time of a network packet", rtt);
  cmd.AddValue ("tcpVariant", "Type of tcp varient you want to use", tcpType);
  cmd.AddValue ("stopTime", "Stop time for applications / simulation time will be stopTime",
                stopTime);
  cmd.AddValue ("recovery", "Recovery algorithm type to use (e.g., ns3::TcpPrrRecovery", recovery);
  cmd.AddValue ("thEnabled", "AQM is threshold", isThresholdAQMEnabled);
  cmd.Parse (argc, argv);

  if (isThresholdAQMEnabled)
    {
      if (tcpType == "TcpNewReno")
        {
          queueSize = QueueSize ("15p");
        }
      else
        {
          queueSize = QueueSize ("50p");
        }
    }

  dir += std::to_string (numNodes) + "-" + tcpType + "-" + std::to_string (rtt) + "-" +
         (isThresholdAQMEnabled ? "withThresh" : "withoutThresh") + "/";

  bottleneckBandwidth = DataRate ("100Mbps"); // 100Mbps for actual sims
  DataRate accessLinkBandwidth = DataRate ((1.2 * bottleneckBandwidth.GetBitRate ()) / numNodes);
  // 100 - 1 - 1 divided by 4
  Time *accessLinkDelays = variedAccessLinkDelays (numNodes, (rtt * 0.24));

  // Set recovery algorithm and TCP variant
  Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
                      TypeIdValue (TypeId::LookupByName (recovery)));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + tcpType));

  // Set default sender and receiver buffer size as 1MB
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 27));
  // Receive buffer size is 1GB to allow for sufficiently large receiving window
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 30));

  // Set default initial congestion window as 10 segments
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));

  // Set default delayed ack count to a specified value
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (delAckCount));

  // Set default segment size of TCP packet to a specified value
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (segmentSize));

  // Create nodes
  NodeContainer leftNodes, rightNodes;
  P2PRouter *p2prouter =
      new P2PRouter (dir, queueSize, bottleneckBandwidth, MicroSeconds (1), qdiscTypeId);
  leftNodes.Create (numNodes);
  rightNodes.Create (numNodes);

  std::vector<NetDeviceContainer> leftToRouter;
  std::vector<NetDeviceContainer> routerToRight;

  // Create the point-to-point link helpers and connect leaf nodes to router
  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.DisableFlowControl ();
  pointToPointLeaf.SetDeviceAttribute ("DataRate", DataRateValue (accessLinkBandwidth));
  pointToPointLeaf.DisableFlowControl ();
  for (uint32_t i = 0; i < numNodes; ++i)
    {
      pointToPointLeaf.SetChannelAttribute ("Delay", TimeValue (accessLinkDelays[i]));
      leftToRouter.push_back (
          pointToPointLeaf.Install (leftNodes.Get (i), p2prouter->routers.Get (0)));
      routerToRight.push_back (
          pointToPointLeaf.Install (p2prouter->routers.Get (1), rightNodes.Get (i)));
    }

  InternetStackHelper internetStack;

  internetStack.Install (leftNodes);
  internetStack.Install (rightNodes);
  internetStack.Install (p2prouter->routers);

  // Assign IP addresses to all the network devices
  // TODO: For N > 3.2k, change bitmask to accomodate larger network
  Ipv4AddressHelper ipAddresses ("10.0.0.0", "255.255.0.0");

  Ipv4InterfaceContainer r1r2IPAddress = ipAddresses.Assign (p2prouter->netDevice);
  ipAddresses.NewNetwork ();

  std::vector<Ipv4InterfaceContainer> leftToRouterIPAddress;
  for (uint32_t i = 0; i < numNodes; i++)
    {
      leftToRouterIPAddress.push_back (ipAddresses.Assign (leftToRouter[i]));
      ipAddresses.NewNetwork ();
    }

  std::vector<Ipv4InterfaceContainer> routerToRightIPAddress;
  for (uint32_t i = 0; i < numNodes; i++)
    {
      routerToRightIPAddress.push_back (ipAddresses.Assign (routerToRight[i]));
      ipAddresses.NewNetwork ();
    }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Enable/Disable SACK in TCP
  // Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (isSack));

  // Create directories to store dat files
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
  NS_ASSERT_MSG (retVal == 0, "Error in return value");
  // retVal = system ((dirToSave + "/pcap/").c_str ());
  // NS_ASSERT_MSG (retVal == 0, "Error in return value");
  retVal = system ((dirToSave + "/queueTraces/").c_str ());
  NS_ASSERT_MSG (retVal == 0, "Error in return value");
  retVal = system ((dirToSave + "/cwndTraces/").c_str ());
  NS_ASSERT_MSG (retVal == 0, "Error in return value");

  p2prouter->installQueueDiscipline ();

  // Calls function to check queue size
  // Simulator::Schedule (tracingStartTime, &CheckQueueSize, p2prouter->qd.Get (0));
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> streamWrapper;

  // Create dat to store packets dropped and marked at the router
  streamWrapper = asciiTraceHelper.CreateFileStream (dir + "/queueTraces/drop-0.dat");
  p2prouter->qd.Get (0)->TraceConnectWithoutContext (
      "Drop", MakeBoundCallback (&DropAtQueue, streamWrapper));

  // Install packet sink at receiver side for N nodes
  uint16_t port = 50000;
  for (uint32_t i = 0; i < numNodes; i++)
    {
      InstallPacketSink (rightNodes.Get (i), port, socketFactory);
    }
  // Install BulkSend application for N nodes
  for (uint32_t i = 0; i < numNodes; i++)
    {
      // NodeId 0 and 1 are assigned to routers, hence (2 + i) will be the nodeId
      InstallBulkSend (leftNodes.Get (i), routerToRightIPAddress[i].GetAddress (1), port,
                       socketFactory, 2 + i, 0, MakeCallback (&CwndChange));
    }
  // Enable PCAP on all the point to point interfaces
  // pointToPointLeaf.EnablePcapAll (dir + "pcap/ns-3", true);
  // AsciiTraceHelper ascii;
  // pointToPointLeaf.EnableAsciiAll (ascii.CreateFileStream (dir + "tcp.tr"));

  // Check for dropped packets using Flow Monitor
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  // Simulator::Schedule (tracingStartTime, &TraceThroughputAndLU, monitor, classifier, p2prouter);

  Simulator::Stop (stopTime);
  Simulator::Run ();

  // Store queue stats in a file
  // std::ofstream afctPath ();
  // afctPath << "AFCT = " << afct / numNodes << "\n";
  std::ofstream myfile;
  myfile.open (dir + "queueStats.txt", std::fstream::in | std::fstream::out | std::fstream::app);
  myfile << std::endl;
  myfile << "Stat for Queue 1";
  myfile << p2prouter->qd.Get (0)->GetStats ();
  myfile.close ();

  myfile.open (dir + "/afct2.dat", std::fstream::in | std::fstream::out | std::fstream::app);
  myfile << "AFCT = " << avg_afct / numNodes << "\n";
  myfile << "Min AFCT = " << min_afct << "\n";
  myfile << "Max AFCT = " << max_afct << "\n";
  myfile.close ();

  // Store configuration of the simulation in a file
  myfile.open (dir + "config.txt", std::fstream::in | std::fstream::out | std::fstream::app);
  myfile << "qdiscTypeId " << qdiscTypeId << "\n";
  myfile << "stream  " << stream << "\n";
  myfile << "segmentSize " << segmentSize << "\n";
  myfile << "delAckCount " << delAckCount << "\n";
  myfile << "stopTime " << stopTime.As (Time::S) << "\n";
  myfile.close ();

  Simulator::Destroy ();

  return 0;
}