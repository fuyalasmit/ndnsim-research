#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

using namespace ns3;

int
main(int argc, char* argv[])
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
  Ptr<Node> r1 = routers.Get(0);
  Ptr<Node> r2 = routers.Get(1);
  Ptr<Node> r3 = routers.Get(2);
  Ptr<Node> r4 = routers.Get(3);
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

  p2p.Install(p1, r1);
  p2p.Install(p2, r1);
  p2p.Install(r1, r2);
  p2p.Install(r1, r3);
  p2p.Install(r2, r4);
  p2p.Install(r3, r4);
  p2p.Install(r2, c1);
  p2p.Install(r2, c2);
  p2p.Install(r3, c3);
  p2p.Install(r3, c4);
  p2p.Install(r4, c5);
  p2p.Install(r4, c6);

  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.Install(allNodes);

  ns3::ndn::GlobalRoutingHelper grHelper;
  grHelper.Install(allNodes);

  ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/ndn");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(p1);
  producerHelper.Install(p2);

  grHelper.AddOrigins("/ndn", p1);
  grHelper.AddOrigins("/ndn", p2);

  ns3::ndn::GlobalRoutingHelper::CalculateRoutes();

  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/ndn");
  consumerHelper.SetAttribute("Frequency", StringValue("10"));

  consumerHelper.Install(c1);
  consumerHelper.Install(c2);
  consumerHelper.Install(c3);
  consumerHelper.Install(c4);
  consumerHelper.Install(c5);
  consumerHelper.Install(c6);

  ns3::ndn::AppHelper attack1("ns3::ndn::ConsumerCbr");
  attack1.SetPrefix("/ndn/fake/video");
  attack1.SetAttribute("Frequency", StringValue("40"));
  attack1.SetAttribute("LifeTime", StringValue("2s"));
  auto a1 = attack1.Install(c1);
  a1.Start(Seconds(attackStart));

  ns3::ndn::AppHelper attack2("ns3::ndn::ConsumerCbr");
  attack2.SetPrefix("/ndn/random-attack");
  attack2.SetAttribute("Frequency", StringValue("30"));
  attack2.SetAttribute("LifeTime", StringValue("1500ms"));
  auto a2 = attack2.Install(c1);
  a2.Start(Seconds(attackStart));

  ns3::ndn::AppHelper attack3("ns3::ndn::ConsumerCbr");
  attack3.SetPrefix("/ndn/fake/image");
  attack3.SetAttribute("Frequency", StringValue("20"));
  attack3.SetAttribute("LifeTime", StringValue("1s"));
  auto a3 = attack3.Install(c1);
  a3.Start(Seconds(attackStart));

  ns3::ndn::AppHelper attack4("ns3::ndn::ConsumerCbr");
  attack4.SetPrefix("/ndn/random-attack-2");
  attack4.SetAttribute("Frequency", StringValue("10"));
  attack4.SetAttribute("LifeTime", StringValue("4s"));
  auto a4 = attack4.Install(c1);
  a4.Start(Seconds(attackStart));

  ns3::ndn::L3RateTracer::InstallAll(
      "results/tree-ifa-rate-trace.txt", Seconds(1));

  ns3::ndn::CsTracer::InstallAll(
      "results/tree-ifa-cs-trace.txt", Seconds(1));

  ns3::ndn::AppDelayTracer::InstallAll(
      "results/tree-ifa-app-delays.txt");

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}