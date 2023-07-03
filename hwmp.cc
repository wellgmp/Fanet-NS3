#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/olsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mesh-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("HWMP");

int main (int argc, char *argv[])
{
    AnimationInterface *pAnim;

  double txp = 30;  //dBm
  int nUav;
  int vel;
  std::cout<<"Ingrese el numero de nodos UAV: (N<255) \n";
  std::cin>>nUav;
  std::cout<<"\n La red FANET contiene "<< nUav <<" nodos UAV \n";
  std::cout<<"\n Ingrese la velocidad promedio maxima [m/s] (Esto modifica el parametro MeanVelocity): \n";
  std::cin>>vel;
  int nSinks = 2;    //numero de nodos sink (receptores)
  int iNodes = (nUav/2);    //nodos intermedios
  int sender = iNodes + nSinks;  //nodos emisores
  double TotalTime = 100; //segundos
  std::string rate ("100Kbps");
  std::string tr_name ("fanet");
  uint32_t packetSize = 512; // bytes
  double m_randomStart; ///< random start
  uint32_t m_nIfaces; ///< number interfaces
  bool m_chan; ///< channel
  bool      m_pcap = true; ///< PCAP
  bool      m_ascii = true; ///< ASCII
  std::string m_stack; ///< stack
  std::string m_root; ///< root
  std::string phyMode ("DsssRate11Mbps");
  NetDeviceContainer meshDevices;
  /// Addresses of interfaces:
  Ipv4InterfaceContainer interfaces;
  /// MeshHelper. Report is not static methods
  MeshHelper mesh;
  AsciiTraceHelper ascii;
  m_randomStart= 0.1;

  m_nIfaces = 1;
  m_chan= false;
  m_stack = "ns3::Dot11sStack";
  m_root = "ff:ff:ff:ff:ff:ff";
 

   CommandLine cmd (__FILE__);
   cmd.AddValue ("nUav", "Numero de nodos", nUav);//  25,50
   cmd.Parse (argc, argv);
   
   if (m_ascii)
    {
      PacketMetadata::Enable ();
    }

  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

  NodeContainer uav;
  uav.Create (nUav);

  // wifi phy y canal con helpers
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211g);

  YansWifiPhyHelper wifiPhy;// =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // mac, disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));
  wifiPhy.Set ("RxGain", DoubleValue (-95));

  wifiMac.SetType ("ns3::AdhocWifiMac");

  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, uav);


mesh = MeshHelper::Default ();
  if (!Mac48Address (m_root.c_str ()).IsBroadcast ())
    {
      mesh.SetStackInstaller (m_stack, "Root", Mac48AddressValue (Mac48Address (m_root.c_str ())));
    }
  else
    {
      //If root is not set, we do not use "Root" attribute, because it
      //is specified only for 11s
      mesh.SetStackInstaller (m_stack);
    }
  if (m_chan)
    {
      mesh.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
    }
  else
    {
      mesh.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
    }
  mesh.SetMacType ("RandomStart", TimeValue (Seconds (m_randomStart)));
  // Set number of interfaces - default is single-interface mesh point
  mesh.SetNumberOfInterfaces (m_nIfaces);
  // Install protocols and return container if MeshPointDevices
  meshDevices = mesh.Install (wifiPhy, uav);

  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << vel << "]";

MobilityHelper mobility;
mobility.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
  "Bounds", BoxValue (Box (0, 510, 0, 510, 0, 25)),
  "TimeStep", TimeValue (Seconds (0.5)),
  "Alpha", DoubleValue (0.85),
  "MeanVelocity", StringValue (ssSpeed.str ()),
  "MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"),
  "MeanPitch", StringValue ("ns3::UniformRandomVariable[Min=0.05|Max=0.05]"),
  "NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"),
  "NormalDirection", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"),
  "NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.02|Bound=0.04]"));
mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
  "X", StringValue ("ns3::UniformRandomVariable[Min=0|Max=1000]"),
  "Y", StringValue ("ns3::UniformRandomVariable[Min=0|Max=1000]"),
  "Z", StringValue ("ns3::UniformRandomVariable[Min=0|Max=25]"));
mobility.Install (uav);

  if (m_pcap)
    wifiPhy.EnablePcapAll (std::string ("mp-"));
  wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("test.tr"));

  InternetStackHelper internet;
  //internet.SetRoutingHelper (list); 
  internet.Install (uav);


 Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.0.0.0", "255.255.252.0");
  Ipv4InterfaceContainer ifcont = ipv4.Assign (devices);

uint16_t port = 9; 

 for(int i = 0; i<nSinks; i++)
{
         
  UdpEchoServerHelper echoServer (port);
  ApplicationContainer serverApps = echoServer.Install (uav.Get (i));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (TotalTime));
  UdpEchoClientHelper echoClient (ifcont.GetAddress (i), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue ((uint32_t)(TotalTime*(1/1))));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (packetSize));
  ApplicationContainer clientApps = echoClient.Install (uav.Get (sender+i));
  clientApps.Start (Seconds (0.0));
  clientApps.Stop (Seconds (TotalTime));
          
}

   wifiPhy.EnablePcap ("HWMP", devices);
   FlowMonitorHelper flowmon;
   Ptr<FlowMonitor> monitor = flowmon.InstallAll();


  NS_LOG_INFO ("Run Simulation.");

   Simulator::Stop (Seconds (TotalTime));
          pAnim= new AnimationInterface ("hwmp.xml");
            pAnim->SetMaxPktsPerTraceFile(99999999999999);
            pAnim->SetMobilityPollInterval(Seconds(1));
            pAnim->EnablePacketMetadata(true);

     uint32_t uavImg = pAnim->AddResource("/home/wellington/Downloads/uav.png");
     for(uint32_t i=0;i<uav.GetN ();i++){
pAnim->UpdateNodeDescription (uav.Get (i), ""); 
Ptr<Node> wid= uav.Get (i);
uint32_t nodeId = wid->GetId ();
pAnim->UpdateNodeImage (nodeId, uavImg);
pAnim->UpdateNodeColor(uav.Get(i), 255, 255, 0); 
pAnim->UpdateNodeSize (nodeId, 40.0,5.0);}
  Simulator::Run ();

Ptr < Ipv4FlowClassifier > classifier = DynamicCast < Ipv4FlowClassifier >(flowmon.GetClassifier());
	std::map < FlowId, FlowMonitor::FlowStats > stats = monitor->GetFlowStats();

	double Delaysum = 0;
	uint64_t txPacketsum = 0;
	uint64_t rxPacketsum = 0;
	uint32_t txPacket = 0;
	uint32_t rxPacket = 0;
        uint32_t PacketLoss = 0;
        uint64_t txBytessum = 0; 
	uint64_t rxBytessum = 0;
       double delay;
       	double throughput = 0;
        int flowId = 0;
       
	for (std::map < FlowId, FlowMonitor::FlowStats > ::const_iterator iter = stats.begin(); iter != stats.end(); ++iter) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
                NS_LOG_UNCOND("*****************************************");
		NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
          
                  txPacket = iter->second.txPackets;
                  rxPacket = iter->second.rxPackets;
                  PacketLoss = txPacket - rxPacket;
                  delay = iter->second.delaySum.GetMilliSeconds();
                  
              
           //<< iter->second.txBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
          std::cout << "  Tx Packets: " << iter->second.txPackets << "\n";
          std::cout << "  Rx Packets: " << iter->second.rxPackets << "\n";
          std::cout << "  Packet Loss: " << PacketLoss << "\n";
  	  std::cout << "  Tx Bytes:   " << iter->second.txBytes << "\n";
          std::cout << "  Rx Bytes:   " << iter->second.rxBytes << "\n";
          std::cout << "  Throughput: " << iter->second.rxBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
          NS_LOG_UNCOND("  Per Node Delay: " << delay / txPacket << " ms");
          std::cout << "   PDR for current flow ID : " << ((rxPacket *100) / txPacket) << "%" << "\n";
          
      

            flowId ++;
                       
		txPacketsum += iter->second.txPackets;
		rxPacketsum += iter->second.rxPackets;
		txBytessum += iter->second.txBytes;
		rxBytessum += iter->second.rxBytes;
		Delaysum += iter->second.delaySum.GetMilliSeconds();
                throughput += iter->second.rxBytes * 8.0 / 9.0 / 1000 / 1000;
	}
          
	    NS_LOG_UNCOND("***********Resultados promedio*************");
        std::cout<<" La red FANET contiene "<< nUav <<" nodos UAV \n";
        std::cout<<" La velocidad de movimiento es: "<< vel <<" \n";
	NS_LOG_UNCOND("Paquetes enviados = " << txPacketsum);
	NS_LOG_UNCOND("Paquetes recibidos = " << rxPacketsum);
        NS_LOG_UNCOND("Total Paquetes Perdidos = " << (txPacketsum-rxPacketsum));
        NS_LOG_UNCOND("Total Bytes Enviados = "<<txBytessum);
        NS_LOG_UNCOND("Total Bytes Recibidos = "<<rxBytessum);
	NS_LOG_UNCOND("Delay: " << Delaysum / txPacketsum << " ms");
	std::cout << "Throughput Promedio = "<<throughput/flowId << " Mbit/s" << std::endl;
        std::cout << "Packet Delivery Ratio: " << ((rxPacketsum *100) / txPacketsum) << "%" << "\n";

  Simulator::Destroy ();
}



