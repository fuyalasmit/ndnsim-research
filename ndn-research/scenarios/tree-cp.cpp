#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

using namespace ns3;

int main(int argc, char* argv[])
{
  double simTime = 120.0;
  double attackStart = 60.0;

  CommandLine cmd;
  cmd.AddValue("simTime", "Simulation time", simTime);
  cmd.Parse(argc, argv);

  NodeContainer producers, routers, consumers;
  producers.Create(2);
  routers.Create(4);
  consumers.Create(6);

  NodeContainer allNodes;
  allNodes.Add(producers);
  allNodes.Add(routers);
  allNodes.Add(consumers);

  Ptr<Node> p1 = producers.Get(0);
  Ptr<Node> p2 = producers.Get(1);

  Ptr<Node> c1 = consumers.Get(0);
  Ptr<Node> c2 = consumers.Get(1);
  Ptr<Node> c3 = consumers.Get(2);
  Ptr<Node> c4 = consumers.Get(3);
  Ptr<Node> c5 = consumers.Get(4);
  Ptr<Node> c6 = consumers.Get(5);

  system("mkdir -p results");

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("10ms"));

  // topology
  p2p.Install(p1, routers.Get(0));
  p2p.Install(p2, routers.Get(0));

  p2p.Install(routers.Get(0), routers.Get(1));
  p2p.Install(routers.Get(0), routers.Get(2));

  p2p.Install(routers.Get(1), routers.Get(3));
  p2p.Install(routers.Get(2), routers.Get(3));

  p2p.Install(routers.Get(1), c1);
  p2p.Install(routers.Get(1), c2);
  p2p.Install(routers.Get(2), c3);
  p2p.Install(routers.Get(2), c4);
  p2p.Install(routers.Get(3), c5);
  p2p.Install(routers.Get(3), c6);

  // NDN stack
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.Install(allNodes);

  ns3::ndn::GlobalRoutingHelper gr;

  gr.Install(allNodes);

  // Legit producer
  ns3::ndn::AppHelper legitProducer("ns3::ndn::Producer");
  legitProducer.SetPrefix("/ndn");
  legitProducer.SetAttribute("PayloadSize", StringValue("1024"));
  legitProducer.Install(p1);

  gr.AddOrigins("/ndn", p1);

  // Malicious producer (pollution)
  ns3::ndn::AppHelper malProducer("ns3::ndn::Producer");
  malProducer.SetPrefix("/ndn/pollution");
  malProducer.SetAttribute("PayloadSize", StringValue("1024"));
  malProducer.Install(p2);

  gr.AddOrigins("/ndn/pollution", p2);

  // IMPORTANT: also register sub-prefixes for proper forwarding
  for (int i = 0; i < 100; i++) {
    gr.AddOrigins("/ndn/pollution/random-" + std::to_string(i), p2);
  }

  ns3::ndn::GlobalRoutingHelper::CalculateRoutes();

  // Legit traffic
  ns3::ndn::AppHelper legitConsumer("ns3::ndn::ConsumerCbr");
  legitConsumer.SetPrefix("/ndn");
  legitConsumer.SetAttribute("Frequency", StringValue("10"));

  legitConsumer.Install(c2);
  legitConsumer.Install(c3);
  legitConsumer.Install(c4);
  legitConsumer.Install(c5);

  // Attack traffic (CACHE POLLUTION)
  ns3::ndn::AppHelper attack("ns3::ndn::ConsumerCbr");
  attack.SetPrefix("/ndn/pollution");
  attack.SetAttribute("Frequency", StringValue("20"));
  attack.SetAttribute("LifeTime", StringValue("2s"));

  auto a1 = attack.Install(c1);
  a1.Start(Seconds(attackStart));

  auto a2 = attack.Install(c6);
  a2.Start(Seconds(attackStart));

  // Tracers
  ns3::ndn::L3RateTracer::InstallAll(
      "results/tree-cp-rate-trace.txt", Seconds(1));

  ns3::ndn::CsTracer::InstallAll(
      "results/tree-cp-cs-trace.txt", Seconds(1));

  ns3::ndn::AppDelayTracer::InstallAll(
      "results/tree-cp-app-delays.txt");

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}