#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "myapp.h"


using namespace ns3;
using namespace dsr;

NS_LOG_COMPONENT_DEFINE ("uav");

int main (int argc, char *argv[])
{
  AnimationInterface *pAnim;
  double txp = 30;  //dBm

  int nUav;
  int protocol;
  std::string m_protocolName;
  m_protocolName = "protocol";
  int vel;
  std::cout<<"Ingrese el numero de nodos UAV: (N<255) \n";
  std::cin>>nUav;
  std::cout<<"\n La red FANET contiene "<< nUav <<" nodos UAV \n";
  std::cout<<"\n Ingrese el numero del protocolo (1=OLSR;2=AODV;3=DSDV;4=AOMDV; 5=DSR): \n";
  std::cin>>protocol;
  std::cout<<"\n Ingrese la velocidad promedio maxima [m/s] (Esto modifica el parametro MeanVelocity): \n";
  std::cin>>vel;
  int nSinks = 2;    //numero de nodos sink (receptores)
  int iUav = (nUav/2);    //nodos intermedios
  int sender = iUav + nSinks;  //nodos emisores

  double TotalTime = 100; //segundos
  std::string rate ("100kbps");
  std::string phyMode ("DsssRate11Mbps");
  std::string tr_name ("fanet");
  uint32_t packetSize = 512; // bytes
 uint32_t numPackets = 100000;


  //./waf --run "uav --nUav=50 --protocol=2"         
   CommandLine cmd (__FILE__);
   cmd.AddValue ("nUav", "Numero de UAVs", nUav);//  100, 200, 300, 400, 500
   cmd.AddValue ("protocol", "1 AODV, 2 OLSR, 3 DSDV, ,4 AOMDV 5 DSR", protocol);
   cmd.Parse (argc, argv);

  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

  NodeContainer uav;
  uav.Create (nUav);

  //wifi phy y canal con helpers
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

  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << vel << "]";


MobilityHelper mobility;
mobility.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
  "Bounds", BoxValue (Box (0, 500, 0, 500, 10, 100)),
  "TimeStep", TimeValue (Seconds (0.5)),
  "Alpha", DoubleValue (0.85),
  "MeanVelocity", StringValue (ssSpeed.str ()),
  "MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"),
  "MeanPitch", StringValue ("ns3::UniformRandomVariable[Min=0.05|Max=0.05]"),
  "NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"),
  "NormalDirection", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"),
  "NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.02|Bound=0.04]"));
mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
  "X", StringValue ("ns3::UniformRandomVariable[Min=0|Max=500]"),
  "Y", StringValue ("ns3::UniformRandomVariable[Min=0|Max=500]"),
  "Z", StringValue ("ns3::UniformRandomVariable[Min=0|Max=25]"));
mobility.Install (uav);


  AodvHelper aodv;
  OlsrHelper olsr;
  DsdvHelper dsdv;
  DsrHelper dsr;
  DsrMainHelper dsrMain;
  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;

  if(protocol==1)
  {
      list.Add (olsr, 100);
      m_protocolName = "OLSR";
  }
  else if(protocol==2)
{
      list.Add (aodv, 100);
      m_protocolName = "AODV";
}
  else if(protocol==3)
{
      list.Add (dsdv, 100);
      m_protocolName = "DSDV";
}
 else if (protocol==4)
 {
      list.Add (aodv, 100);
      m_protocolName = "AOMDV";
  }
 else if (protocol==5)
 {
      m_protocolName = "DSR";
  }
else
{
 NS_FATAL_ERROR ("Seleccione el protocolo adecuado:" << protocol);
}

if (protocol < 5)
    {
      internet.SetRoutingHelper (list);
      internet.Install (uav);
    }
  else if (protocol == 5)
    {
      internet.Install (uav);
      dsrMain.Install (dsr, uav);
    }



NS_LOG_INFO ("assigning ip address");

 Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.0.0.0", "255.255.252.0");
  Ipv4InterfaceContainer ifcont = ipv4.Assign (devices);

 uint16_t port = 9; 

    /* Inicializar aplicaciones */
  // Udp


if (protocol==4)
{
  for(int i=0;i<nSinks;i++)
    {
   Address sinkAddress1 (InetSocketAddress (ifcont.GetAddress (i), port)); 
   PacketSinkHelper packetSinkHelper1 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
   ApplicationContainer sinkApps1 = packetSinkHelper1.Install (uav.Get (i)); 
   sinkApps1.Start (Seconds (0.));
   sinkApps1.Stop (Seconds (TotalTime));

    Ptr<Socket> ns3UdpSocket1 = Socket::CreateSocket (uav.Get (sender+i),UdpSocketFactory::GetTypeId ()); 
   // Iniciar aplicacion para todos los nodos
   Ptr<MyApp> app1 = CreateObject<MyApp> ();
   app1->Setup (ns3UdpSocket1, sinkAddress1, packetSize, numPackets, DataRate (rate));
   uav.Get (sender+i)->AddApplication (app1);
   ns3UdpSocket1->SetAllowBroadcast (true);
   app1->SetStartTime (Seconds (0.));
   app1->SetStopTime (Seconds (TotalTime));
  }
}
else
{
  for(int i = 0; i<nSinks; i++)
{
         
         OnOffHelper onoff("ns3::UdpSocketFactory",InetSocketAddress(ifcont.GetAddress(i), port)); 
         onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
         onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
         onoff.SetAttribute("PacketSize", UintegerValue(packetSize));
         onoff.SetAttribute ("DataRate", DataRateValue (rate));
         ApplicationContainer apps = onoff.Install(uav.Get(sender+i)); 
         apps.Start(Seconds(1.0));
         apps.Stop(Seconds(TotalTime));
}
}


   FlowMonitorHelper flowmon;
   Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  wifiPhy.EnablePcap (m_protocolName, devices);
  NS_LOG_INFO ("Run Simulation");

   Simulator::Stop (Seconds (TotalTime));
          pAnim= new AnimationInterface ("uav.xml");
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
        std::cout<<" El protocolo seleccionado es:"<< m_protocolName <<" \n";
        std::cout<<" La velocidad de movimiento es: "<< vel <<" \n";
	NS_LOG_UNCOND("Paquetes enviados = " << txPacketsum);
	NS_LOG_UNCOND("Paquetes recibidos = " << rxPacketsum);
        NS_LOG_UNCOND("Total Paquetes Perdidos = " << (txPacketsum-rxPacketsum));
        NS_LOG_UNCOND("Total Bytes Enviados = "<<txBytessum);
        NS_LOG_UNCOND("Total Bytes Recibidos = "<<rxBytessum);
	NS_LOG_UNCOND("Delay: " << Delaysum / txPacketsum << " ms");
	std::cout << "Throughput Promedio= "<<throughput/flowId << " Mbit/s" << std::endl;
        std::cout << "Packet Delivery Ratio: " << ((rxPacketsum *100) / txPacketsum) << "%" << "\n";

  Simulator::Destroy ();
}


