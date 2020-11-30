/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include <fstream>
#include <iostream>


// This file is modified from /examples/tutorials/third.cc
// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

// This structure stores global variables, which are needed to calculate throughput and delay every second
struct DataForThpt
{
  FlowMonitorHelper flowmon;	
  Ptr<FlowMonitor> monitor;
  uint64_t lastTotalRxBytes;	//Total bytes received in all flows before the starting of current window.				
  double lastDelaySum;			//Total Delay sum in all flows before the starting of current window.
  uint32_t lastRxPackets;		//Total number of received packets in all flows before starting the current window.
}data;							//data is a structure variable which will store all these global variables.

double averageDelay;			
std::ofstream delayStream;
std::ofstream throughputStream;

//This function is being called every 0.2 seconds, It measures delay and throughput in every 0.2s time window.
//It calculates overall throughput in that window of all flows in the network.
static void
Throughput ()
{
  
	data.monitor->CheckForLostPackets ();
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (data.flowmon.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = data.monitor->GetFlowStats ();

	uint64_t total_bytes=0;
	uint32_t total_RxPackets=0;
	double total_DelaySum=0;

	//Iterating through every flow
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
	{

		total_bytes+=iter->second.rxBytes;
		total_DelaySum+=iter->second.delaySum.GetDouble();
		total_RxPackets+=iter->second.rxPackets;
	}
	uint64_t dataInLastSecond=total_bytes-data.lastTotalRxBytes;
	uint32_t RxPacketsInLastSecond=total_RxPackets-data.lastRxPackets;
	double DelaySumInLastSecond=total_DelaySum-data.lastDelaySum;

	data.lastTotalRxBytes=total_bytes;
	data.lastDelaySum=total_DelaySum;
	data.lastRxPackets=total_RxPackets;
	
	if(total_RxPackets!=0)
	{
		averageDelay=(total_DelaySum/1000000)/total_RxPackets;
	}

	if(!(RxPacketsInLastSecond==0 || DelaySumInLastSecond==0))
	{
		NS_LOG_UNCOND("Delay "<<(DelaySumInLastSecond/RxPacketsInLastSecond)/(1000000)<<"ms");
		delayStream<<(DelaySumInLastSecond/RxPacketsInLastSecond)/(1000000)<<std::endl;
	}
	else
	{
		NS_LOG_UNCOND("Delay "<<0<<"ms");
		delayStream<<0<<std::endl;
	}
	NS_LOG_UNCOND("Throughput "<<(dataInLastSecond*8.0*5)/(1024*1024)<<"Mbps");
	throughputStream<<(dataInLastSecond*8.0*5)/(1024*1024)<<std::endl;

	Simulator::Schedule (Seconds (0.2), &Throughput);
}

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  //Number of CSMA(LAN) nodes
  uint32_t nCsma = 3;

  //Number of STA(Stations)
  uint32_t nWifi = 3;

  bool tracing = false;
  uint32_t maxBytes = 0;

  //Error Model -> Default = NistErrorRateModel
  std::string errorModelType = "ns3::NistErrorRateModel";

  //RAA algorithm (WifiManager Class) -> Default = MinstrelHt
  std::string raaAlgo = "MinstrelHt";

  //Variables to set rates of various channels in topology, Refer base topology structure.
  uint32_t csmaRate=150;
  uint32_t csmaDelay=9000;
  uint32_t p2pRate=50;
  uint32_t p2pDelay=10;
  
//Command-Line argument to make it interactive.
  CommandLine cmd (__FILE__);
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue ("raa","MinstrelHt / ConstantRate / Ideal", raaAlgo);
  cmd.AddValue ("maxBytes", "Max number of Bytes to be sent", maxBytes);
  cmd.AddValue ("p2pRate", "Mbps", p2pRate);
  cmd.AddValue ("p2pDelay", "MilliSeconds", p2pDelay);
  cmd.AddValue ("csmaDelay", "NanoSeconds", csmaDelay);
  cmd.AddValue ("csmaRate", "Mbps", csmaRate);


  cmd.Parse (argc,argv);

  std::string raa_name=raaAlgo;
  raaAlgo = "ns3::"+raaAlgo+"WifiManager";

//Store values of Throughput and delay in respective files for plotting graph
  delayStream.open("Delay_"+raa_name+"_"+std::to_string(nWifi)+".csv");
  throughputStream.open("Throughput_"+raa_name+"_"+std::to_string(nWifi)+".csv");

  // The underlying restriction of 18 is due to the grid position
  // allocator's configuration; the grid layout will exceed the
  // bounding box if more than 18 nodes are provided.
  if (nWifi > 18)
    {
      std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
      return 1;
    }

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (std::to_string(p2pRate)+"Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (std::to_string(p2pDelay)+"ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue (std::to_string(csmaRate)+"Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (csmaDelay)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = p2pNodes.Get (0);


  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  
  //Delay model
  channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

  //Loss model
  channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
                                  "Exponent", DoubleValue (0.3),
                                  "ReferenceLoss", DoubleValue (4.0));

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  //Error Model
  phy.SetErrorRateModel (errorModelType);

  WifiHelper wifi;

  //Setting Wifi Standard
  wifi.SetStandard (WIFI_STANDARD_80211ac);

  //Setting Raa Algorithm
  wifi.SetRemoteStationManager (raaAlgo);

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
  

  //Bounds for the Rectangle Grid
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Speed", 
                            StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", RectangleValue (Rectangle (-100, 100, -100, 100)));
  mobility.Install (wifiStaNodes);

//Setting Mobility model
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);

  NS_LOG_INFO ("Create Applications.");

// Creating a BulkSendApplication and install it on one of the wifi-nodes(except AP)

  uint16_t port = 8808  // random port for TCP server listening.
  

  //Setting packetsize (Bytes)
  uint32_t packetSize = 1024;
  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (csmaInterfaces.GetAddress (nCsma), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  source.SetAttribute("SendSize", UintegerValue(packetSize));
  ApplicationContainer sourceApps;
  for(int i = 0; i<int(nWifi); i++)
  {
  sourceApps.Add(source.Install (wifiStaNodes.Get (i)));
  }
  sourceApps.Start (Seconds (2.0));
  sourceApps.Stop (Seconds (6.0));

// Creating a PacketSinkApplication and install it on one of the CSMA nodes

  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (csmaInterfaces.GetAddress(nCsma), port));
  ApplicationContainer sinkApps = sink.Install (csmaNodes.Get (nCsma));
  sinkApps.Start (Seconds (1.0));
  sinkApps.Stop (Seconds (6.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //Initialisation of global variable which are used for Throughput and Delay Calculation.
  data.monitor=data.flowmon.InstallAll();
  data.lastTotalRxBytes=0;
  data.lastRxPackets=0;
  data.lastDelaySum=0;
  Simulator::Schedule (Seconds (1.0), &Throughput);

  Simulator::Stop (Seconds (7.0));

  if (tracing == true)
    {
      pointToPoint.EnablePcapAll ("third_p2p");
      phy.EnablePcap ("third_phy", apDevices.Get (0));
      csma.EnablePcap ("third_csma", csmaDevices.Get (0), true);
    }

  Simulator::Run ();
  Simulator::Destroy ();

  delayStream.close();
  throughputStream.close();

  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
  std::cout << "Average Throughput: "<< sink1->GetTotalRx ()*8.0/(4*1024*1024)<<" Mbps"<<std::endl;
  std::cout << "Average Delay: "<< averageDelay <<"ms"<<std::endl;

  return 0;
}
