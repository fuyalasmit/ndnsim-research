#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
  // Simulation duration
  double simTime = 120.0;

  CommandLine cmd;
  cmd.AddValue("simTime", "Simulation time", simTime);
  cmd.Parse(argc, argv);

  // Create nodes
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

  // Optional names (debugging)
  Names::Add("p1", p1);
  Names::Add("p2", p2);
  Names::Add("r1", r1);
  Names::Add("r2", r2);
  Names::Add("r3", r3);
  Names::Add("r4", r4);

  // Create results directory
  system("mkdir -p results");

  // Point-to-point links
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

  // Install NDN stack
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.Install(allNodes);

  // Routing helper
  ns3::ndn::GlobalRoutingHelper grHelper;
  grHelper.Install(allNodes);

  // Producers
  ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("Prefix", StringValue("/ndn"));
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

  producerHelper.Install(p1);
  producerHelper.Install(p2);

  // Add origins and calculate routes
  // this build fib routes.
  grHelper.AddOrigins("/ndn", p1);
  grHelper.AddOrigins("/ndn", p2);
  ns3::ndn::GlobalRoutingHelper::CalculateRoutes();

  // Consumers
  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute("Prefix", StringValue("/ndn"));
  consumerHelper.SetAttribute("Frequency", StringValue("10"));

  consumerHelper.Install(c1);
  consumerHelper.Install(c2);
  consumerHelper.Install(c3);
  consumerHelper.Install(c4);
  consumerHelper.Install(c5);
  consumerHelper.Install(c6);

  // Tracers
  // this is kinds measurement tool, to record metrics.
  ns3::ndn::L3RateTracer::InstallAll(
      "results/tree-normal-rate-trace.txt",
      Seconds(1.0));

  ns3::ndn::CsTracer::InstallAll(
      "results/tree-normal-cs-trace.txt",
      Seconds(1.0));

  ns3::ndn::AppDelayTracer::InstallAll(
      "results/tree-normal-app-delays.txt");

  // Run simulation
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}