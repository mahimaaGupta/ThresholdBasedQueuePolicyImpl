/**
 *          Single Bottleneck Dumbbell Topology
*/

#include <iostream>
#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h" 
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/config-store-module.h"
#include "ns3/node.h"
#include "ns3/netanim-module.h"


#define MAX_SOURCES 100;

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TCPSCRIPT");

/**
 * -----------------------------------------------------------------------
 * Declaration of all variables required for tracing the given parameters
 * -----------------------------------------------------------------------
*/

uint64_t queueSize;
Ptr<OutputStreamWrapper> qSize_stream;

uint64_t droppedPackets;
uint64_t previously_droppedPackets;
//float transmittedPackets;
Ptr<OutputStreamWrapper> dropped_stream;

uint32_t pkt_count = 0;
uint32_t prev_pkt_count = 0;
Time prevTime = Seconds (0);
Ptr<OutputStreamWrapper> bottleneckTransmittedStream;

uint64_t packetsTransmitted;
uint64_t previous_transmitted_packets = 0;
Time prevTime02 = Seconds (0);
Ptr<OutputStreamWrapper> utilization;

/**
 * --------------------------------------------------------------------- 
 *                  Functions for tracing Queue Size
 * --------------------------------------------------------------------- 
*/

static void plotQsizeChange (uint32_t oldQSize, uint32_t newQSize){
    queueSize = newQSize;
}

static void TraceQueueSize(){
    *qSize_stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << queueSize << std::endl;
}

static void StartTracingQueueSize(){
    Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::PointToPointNetDevice/TxQueue/PacketsInQueue", MakeCallback(&plotQsizeChange));
}

/**
 * --------------------------------------------------------------------- 
 *                  Functions for tracing Packet Loss 
 * --------------------------------------------------------------------- 
*/

static void RxDrop(Ptr<OutputStreamWrapper> stream,  Ptr<const Packet> p){
   droppedPackets++;
} 

static void TraceDroppedPacket(std::string droppedTrFileName){
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/TxQueue/Drop", MakeBoundCallback(&RxDrop, dropped_stream));
     //Config::ConnectWithoutContext("/NodeList/0/DeviceList/*/$ns3::PointToPointNetDevice/MacTxDrop", MakeBoundCallback(&RxDrop, dropped_stream));
    // Config::ConnectWithoutContext("/NodeList/0/DeviceList/*/$ns3::PointToPointNetDevice/PhyRxDrop", MakeBoundCallback(&RxDrop, dropped_stream));
    // Config::ConnectWithoutContext("/NodeList/0/DeviceList/*/$ns3::PointToPointNetDevice/PhyTxDrop", MakeBoundCallback(&RxDrop, dropped_stream));

}

static void TraceDroppedPkts(){
    float packetLoss = droppedPackets - previously_droppedPackets;
    *dropped_stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << packetLoss << std::endl;
    previously_droppedPackets = droppedPackets;
}

/**
 * --------------------------------------------------------------------- 
 *                  Functions for tracing Network Throughput
 * --------------------------------------------------------------------- 
*/

// For saving packets received at the sink at each time instant into the defined ascii stream
static void
TraceBottleneckTx(uint32_t pktSize){
    Time currrent_time = Now();
    float btl_thr = ((pkt_count-prev_pkt_count)*8*pktSize)/(1000 * 1000*(currrent_time.GetSeconds() - prevTime.GetSeconds()));
    *bottleneckTransmittedStream->GetStream() << Simulator::Now().GetMilliSeconds() << "\t" << btl_thr << std::endl;
    prev_pkt_count = pkt_count;
    prevTime = currrent_time;
}


//For counting the number of packets received at the sink
static void SinkRxCount(Ptr<const Packet> p, const Address &ad )
{  
  //std::cout << "One packet received" << std::endl;
  pkt_count++;
  //std::cout << pkt_count << std::endl; const Address &ad
}

// Call SinkRxCount function everytime a packet is received at the application layer of the sink node 
static void 
StartTracingSink(){
    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&SinkRxCount));
    //Config::ConnectWithoutContext("/NodeList/1/DeviceList/0/$ns3::PointToPointNetDevice/PhyTxEnd", MakeCallback(&SinkRxCount));
}

/**
 * --------------------------------------------------------------------- 
 *                  Functions for tracing Link Utilization
 * --------------------------------------------------------------------- 
*/

static void TxxPacket(Ptr<const Packet> p ){
    packetsTransmitted++;
}

static void TraceUtilization(uint32_t pktSize){
    Time currrent_time = Now();
    float btl_thr = ((packetsTransmitted-previous_transmitted_packets)*8*pktSize)/(1000 * 1000*(currrent_time.GetSeconds() - prevTime02.GetSeconds()));
    *utilization->GetStream() << Simulator::Now().GetSeconds() << "\t" << btl_thr/100 << std::endl;
    previous_transmitted_packets = packetsTransmitted ;
    prevTime02 = currrent_time;
}

static void StartTracingUtilization(){
    packetsTransmitted = 0;
    previous_transmitted_packets = 0;
    Config::ConnectWithoutContext("/NodeList/0/DeviceList/*/$ns3::PointToPointNetDevice/PhyTxEnd", MakeCallback(&TxxPacket));
}

/**
 * --------------------------------------------------------------------- 
 *                  Main Function for ns3 Simulation
 * --------------------------------------------------------------------- 
*/


int 
main(int argc, char *argv[])
{   
    ConfigStore config;
    config.ConfigureDefaults ();
    config.ConfigureAttributes ();

    /**
     * -----------------------------------------------------------------
     * Declaration of all variables and their respective default values
     * -----------------------------------------------------------------
    */

    std::string bottleneckBandwidth= "100Mbps";  //bandwidth of the bottleneck link
    std::string bottleneckDelay = "1ms";          //bottleneck link has negligible propagation delay
    std::string accessBandwidth = "2Mbps";
    std::string RTT = "198ms";   		//round-trip time of each TCP flow
    std::string flavour = "TcpNewReno";		//TCP variant considered
    std::string queueSize = "2084p";      //in packets. Size of the buffer at the bottleck link
    uint32_t pktSize = 1446;        //in Bytes. 1458 to prevent fragments
    uint32_t enbBotTrace = 0;
    float startTime = 0; 	//start time of the simulation in seconds
    float simDuration = 250;        //in seconds. endtime = startTime + simDuration
    uint32_t cleanup_time = 2;
    float stopTime = startTime + simDuration;
    float samplingRate = 0.1f;
    uint32_t nSources = 60; //no of TCP sources
    std::string tcpModel ("ns3::"+flavour);

    double mean = 0.1;   // more like a ~ 0.06
    double bound = 1;
    Ptr<ExponentialRandomVariable> expRandomVariable = CreateObject<ExponentialRandomVariable> ();

    
    droppedPackets = 0;
    previously_droppedPackets = 0;
    std::string iterator = "42";
    

    std::string root_dir;
    std::string qsizeTrFileName = "Queue_size";
    std::string droppedTrFileName = "Packet_Loss";
    std::string bottleneckTxFileName = "Transmission";

   
     /**
     * -----------------------------------------------------------------
     * Getting Input from Command Line 
     * -----------------------------------------------------------------
     */


    CommandLine cmd;
    cmd.AddValue("bottleneckBandwidth", "Bottleneck Bandwidth", bottleneckBandwidth);
    cmd.AddValue("bottleneckDelay", "Bottleneck Delay", bottleneckDelay);
    cmd.AddValue("nSources", "Number of Sources", nSources);
    cmd.AddValue("flavour", "TCP Flavour", flavour);
    cmd.AddValue("pktSize", "Packet Size", pktSize);
    cmd.AddValue("RTT", "Round Trip Time", RTT);
    cmd.AddValue("samplingRate","Sampling Rate", samplingRate);
    cmd.AddValue("iterator", "Iterator", iterator);
    cmd.Parse(argc, argv);

    NS_LOG_UNCOND("Bandwidth : "<< bottleneckBandwidth << "\tDelay : " << bottleneckDelay << "\tNumber of Sources:" << nSources);
    NS_LOG_UNCOND("TCP Flavor : " << flavour << "\tPacket Size : " << pktSize << "\tRound Trip Time: " << RTT);
  

    /**
     * -----------------------------------------------------------------
     * Setting Up the Configurations for ns3 Simulation
     * -----------------------------------------------------------------
    */


    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue(tcpModel));
    //Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (100000000));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (pktSize));
    //Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (2));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue (1));
    Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (20*1000));
    
    
    /**
     * -----------------------------------------------------------------
     *          Creating nodes & respective Node Containers
     * -----------------------------------------------------------------
    */

    // Defining the nodes 
    NodeContainer nodes;
    nodes.Create (2+nSources*2);
    // Source nodes
    NodeContainer sr1 [nSources];
    // Destination nodes
    NodeContainer dr2 [nSources];
    NodeContainer r1r2 = NodeContainer(nodes.Get(0), nodes.Get(1));
    for( uint32_t i = 0; i< nSources ; i++){
        sr1[i] = NodeContainer(nodes.Get(i+2), nodes.Get(0));
        dr2[i] = NodeContainer(nodes.Get(2+nSources+i), nodes.Get(1));
    }
   

    // Defining the links to be used between nodes
    double range = (double) (stoi(RTT.substr(0, RTT.length()-2)));
    double min = range - (0.1 * range);
    double max = range + (0.1 * range);
    
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
    x->SetAttribute ("Min", DoubleValue (min));
    x->SetAttribute ("Max", DoubleValue (max));


    /**
     * -----------------------------------------------------------------
     *              Creating PointToPoint Channels 
     * -----------------------------------------------------------------
    */


    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue(bottleneckBandwidth));
    bottleneck.SetChannelAttribute("Delay", StringValue(bottleneckDelay));
    bottleneck.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue (QueueSize (queueSize))); // p in 1000p stands for packets
    bottleneck.DisableFlowControl();

    PointToPointHelper p2p_s[nSources];
    PointToPointHelper p2p_d[nSources];
    for (uint32_t i = 0; i < nSources; i++)
    {
        double delay = (x->GetValue())/4;
        //std::cout << delay*2 << std::endl;
        std::string delay_str = std::to_string(delay) + "ms";
        p2p_s[i].SetDeviceAttribute ("DataRate", StringValue(accessBandwidth));
        p2p_s[i].SetChannelAttribute ("Delay", StringValue(delay_str));
        p2p_s[i].DisableFlowControl();
        
        p2p_d[i].SetDeviceAttribute ("DataRate", StringValue(accessBandwidth));
        p2p_d[i].SetChannelAttribute ("Delay", StringValue(delay_str));
        p2p_d[i].DisableFlowControl();
    }
    
    
    /**
     * -----------------------------------------------------------------
     * Create NetDevice Containers for inter and intra router links
     * -----------------------------------------------------------------
    */


    NetDeviceContainer r1_r2 = bottleneck.Install(r1r2);
    NetDeviceContainer s_r1[nSources];
    NetDeviceContainer d_r2[nSources];

    
    for( uint32_t i = 0; i<nSources; i++){
        s_r1[i] = p2p_s[i].Install(sr1[i]);
        d_r2[i] = p2p_d[i].Install(dr2[i]);
    }

    /**
     * -----------------------------------------------------------------
     *          Assigning IPv4 Addresses to all the nodes
     * -----------------------------------------------------------------
    */

    //  Assigning ip address to each node.
    InternetStackHelper stack;
    stack.Install(nodes);
    Ipv4AddressHelper address;

    Ipv4InterfaceContainer ip_s_r1 [nSources] ;
    Ipv4InterfaceContainer ip_d_r2 [nSources] ;

    for (uint32_t i = 0; i < nSources; i++)
    {     
        std::string ip = "10.1."+std::to_string(i+2)+".0";
        address.SetBase(ip.c_str(), "255.255.255.0");
        ip_s_r1[i] = address.Assign(s_r1[i]);

        std::string ip2 = "10.1."+std::to_string(i+2+nSources)+".0";
        address.SetBase(ip2.c_str(), "255.255.255.0");
        ip_d_r2[i] = address.Assign(d_r2[i]);
    }
    
    address.SetBase("10.1.1.0","255.255.255.0");
    address.Assign(r1_r2);

    /**
     * -----------------------------------------------------------------
     *                  Packet Sink Helper
     * -----------------------------------------------------------------
    */


    uint16_t sinkPort = 8080;
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress( Ipv4Address::GetAny(), sinkPort));

    Address sinkAddress[nSources];
    ApplicationContainer sinkApp[nSources];

    for (uint32_t i = 0; i < nSources; i++)
    {
        sinkAddress[i] = *(new Address(InetSocketAddress(ip_d_r2[i].GetAddress(0), sinkPort)));
        sinkApp[i] = packetSinkHelper.Install(nodes.Get(2+nSources+i));
        sinkApp[i].Start(Seconds(startTime));
        sinkApp[i].Stop(Seconds(stopTime));
    }
    


    /**
     * -----------------------------------------------------------------
     *                  TCP Bulk Send Helper
     * -----------------------------------------------------------------
    */
    
    
    ApplicationContainer sourceApps[nSources];

   
    expRandomVariable->SetAttribute ("Mean", DoubleValue (mean));
    expRandomVariable->SetAttribute ("Bound", DoubleValue (bound));

    double stime = startTime;
    // Configuring the application at each source node.
    for (uint32_t i = 0; i < nSources; i++)
    {
        
        BulkSendHelper tmp_source("ns3::TcpSocketFactory",InetSocketAddress (ip_d_r2[i].GetAddress (0), sinkPort));
           
        // Set the amount of data to send in bytes.  Zero is unlimited.
        tmp_source.SetAttribute ("MaxBytes", UintegerValue (0));
        sourceApps[i] = tmp_source.Install (nodes.Get (2+i));
        
        sourceApps[i].Start (Seconds (stime));
        sourceApps[i].Stop (Seconds (stopTime));
        double gap = expRandomVariable->GetValue();

        stime += gap;
    }


    /**
     * -----------------------------------------------------------------
     *                  Trace Files and Simulation Run
     * -----------------------------------------------------------------
    */


    // Configuring file stream to write the Qsize
    AsciiTraceHelper ascii_qsize;
    qSize_stream = ascii_qsize.CreateFileStream(qsizeTrFileName + iterator + ".txt");

    AsciiTraceHelper ascii_dropped;
    dropped_stream = ascii_dropped.CreateFileStream (droppedTrFileName + iterator + ".txt");

   // Configuring file stream to write the no of packets transmitted by the bottleneck
    AsciiTraceHelper ascii_qsize_tx;
    bottleneckTransmittedStream = ascii_qsize_tx.CreateFileStream(bottleneckTxFileName + iterator + ".txt");

    AsciiTraceHelper ascii_tx;
    utilization = ascii_tx.CreateFileStream("Utilization" + iterator + ".txt");

    Simulator::Schedule( Seconds(stime), &StartTracingQueueSize);
    Simulator::Schedule( Seconds(stime), &StartTracingSink);
    Simulator::Schedule( Seconds(stime), &StartTracingUtilization);
    Simulator::Schedule( Seconds(stime), &TraceDroppedPacket, droppedTrFileName);
    // Simulator::Schedule( Seconds(stime), &TraceTransmittedForPacketLoss);

    
    // writing the congestion windows size, queueSize, packetTx to files periodically ( 1 sec. )
    for (int time = stime; time < stopTime; )
    {   
        Simulator::Schedule( Seconds(time), &TraceQueueSize);
        Simulator::Schedule( Seconds(time), &TraceDroppedPkts);
        Simulator::Schedule( Seconds(time), &TraceUtilization, pktSize);
        time = time + 1;
    }

    for (float ttime = stime; ttime < stopTime; )
    {   
        Simulator::Schedule( Seconds(ttime), &TraceBottleneckTx, pktSize);
        ttime = ttime + samplingRate;
    }
    

    if ( enbBotTrace == 1 ){
        AsciiTraceHelper bottleneck_ascii;
        bottleneck.EnableAscii(bottleneck_ascii.CreateFileStream ("bottleneck.tr"), s_r1[0]);
        bottleneck.EnablePcapAll("bottleneck-", false);
    }


    Simulator::Stop (Seconds (stopTime+cleanup_time));
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    Simulator::Run ();
    Simulator::Destroy ();

    return 0;

}
