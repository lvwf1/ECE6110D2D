#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include <ns3/buildings-helper.h>

using namespace ns3;

int main(int argc, char *argv[]) {

    uint16_t port = 8080;
    double delay = 2.0;
    double interval = 0.05;
    double simTime = 10.0;
    uint64_t dataRate = 50000000;

    CommandLine cmd;
    cmd.AddValue("delay", "Link latency in miliseconds", delay);
    cmd.AddValue("dataRate", "Data rate in bps", dataRate);
    cmd.AddValue("interval", "UDP client packet interval", interval);
    cmd.Parse(argc, argv);

    // Create 5 nodes, 1 for base station, 4 for user mobiles.
    NodeContainer baseNode;
    NodeContainer userNodes;

    baseNode.Create(1);
    userNodes.Create(4);

    Ptr<LteHelper> LTEHelper = CreateObject<LteHelper>();

    // Set the nodes position
    Ptr<ListPositionAllocator> userPosition = CreateObject<ListPositionAllocator>();
    userPosition->Add(Vector(10, 10, 0));
    userPosition->Add(Vector(0, 10, 0));
    userPosition->Add(Vector(20, 10, 0));
    userPosition->Add(Vector(-10, 10, 0));
    Ptr<ListPositionAllocator> basePosition = CreateObject<ListPositionAllocator>();
    basePosition->Add(Vector(5, -20, 0));

    MobilityHelper mobilityHelper;
    mobilityHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    // Install userNodes to Mobility Model
    mobilityHelper.SetPositionAllocator(userPosition);
    mobilityHelper.Install(userNodes);
    BuildingsHelper::Install(userNodes);

    // Install baseNode to Mobility Model
    mobilityHelper.SetPositionAllocator(basePosition);
    mobilityHelper.Install(baseNode);
    BuildingsHelper::Install(baseNode);

    // Create Device and install them on nodes.
    NetDeviceContainer baseDev = LTEHelper->InstallEnbDevice(baseNode);
    NetDeviceContainer userDev1 = LTEHelper->InstallUeDevice(userNodes.Get(0));
    NetDeviceContainer userDev2 = LTEHelper->InstallUeDevice(userNodes.Get(1));
    NetDeviceContainer userDev3 = LTEHelper->InstallUeDevice(userNodes.Get(2));
    NetDeviceContainer userDev4 = LTEHelper->InstallUeDevice(userNodes.Get(3));

    // Attach the user mobile to base Station.
    LTEHelper->Attach(userDev1, baseDev.Get(0));
    LTEHelper->Attach(userDev2, baseDev.Get(0));
    LTEHelper->Attach(userDev3, baseDev.Get(0));
    LTEHelper->Attach(userDev4, baseDev.Get(0));

    // Create the topology.
    PointToPointHelper pointHelper;

    // Create channels between user mobiles and base station.
    pointHelper.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay)));
    pointHelper.SetDeviceAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    NetDeviceContainer dev1 = pointHelper.Install(userNodes.Get(0), baseNode.Get(0));
    NetDeviceContainer dev2 = pointHelper.Install(userNodes.Get(1), baseNode.Get(0));
    NetDeviceContainer dev3 = pointHelper.Install(userNodes.Get(2), baseNode.Get(0));
    NetDeviceContainer dev4 = pointHelper.Install(userNodes.Get(3), baseNode.Get(0));

    // Install nodes to internet stack
    InternetStackHelper stackHelper;
    stackHelper.Install(userNodes);
    stackHelper.Install(baseNode);

    // Add IP address to devices
    Ipv4AddressHelper ipv4AddressHelper;

    ipv4AddressHelper.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ip1 = ipv4AddressHelper.Assign(dev1);

    ipv4AddressHelper.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ip2 = ipv4AddressHelper.Assign(dev2);

    ipv4AddressHelper.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer ip3 = ipv4AddressHelper.Assign(dev3);

    ipv4AddressHelper.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer ip4 = ipv4AddressHelper.Assign(dev4);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Create UDP Server application on base station node
    UdpServerHelper server(port);
    ApplicationContainer serverApp = server.Install(baseNode.Get(0));

    // Set up the applications start and stop time
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(simTime));

    // Create UDP client application to set up direct connection between
    // user node 1 and user node 0
    uint32_t maxPacketSize = 512;
    uint32_t maxPacketCount = 10000;

    UdpClientHelper client1(ip2.GetAddress(0), port);
    client1.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    client1.SetAttribute("Interval", TimeValue(Seconds(interval)));
    client1.SetAttribute("PacketSize", UintegerValue(maxPacketSize));
    ApplicationContainer app1 = client1.Install(userNodes.Get(0));
    app1.Start(Seconds(2.0));
    app1.Stop(Seconds(simTime));

    // Create UDP client application to transfer packets between user node 0
    // and other nodes
    UdpClientHelper client2(ip2.GetAddress(0), port);
    client2.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    client2.SetAttribute("Interval", TimeValue(Seconds(interval)));
    client2.SetAttribute("PacketSize", UintegerValue(maxPacketSize));
    ApplicationContainer app2 = client2.Install(userNodes.Get(2));
    app2.Start(Seconds(2.0));
    app2.Stop(Seconds(simTime));

    UdpClientHelper client3(ip2.GetAddress(0), port);
    client3.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    client3.SetAttribute("Interval", TimeValue(Seconds(interval)));
    client3.SetAttribute("PacketSize", UintegerValue(maxPacketSize));
    ApplicationContainer app3 = client2.Install(userNodes.Get(3));
    app3.Start(Seconds(2.0));
    app3.Stop(Seconds(simTime));

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> moniter = flowmon.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = moniter->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "Tx Bytes: " << i->second.txBytes << "\n";
        std::cout << "Rx Bytes: " << i->second.rxBytes << "\n";
        std::cout << "Throughput: "
                  << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() -
                                                i->second.timeFirstTxPacket.GetSeconds()) / 1024 / 1024
                  << " Mbps\n";
    }

    Simulator::Destroy();

    return 0;
}
