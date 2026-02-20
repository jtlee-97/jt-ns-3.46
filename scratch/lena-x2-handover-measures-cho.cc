/*
 * Copyright (c) 2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-common.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <fstream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LenaX2HandoverMeasures");

std::ofstream cho_summary_log;
std::ofstream cho_detail_log;

std::string
ExtractBetween(const std::string& text, const std::string& beginToken, const std::string& endToken)
{
    size_t beginPos = text.find(beginToken);
    if (beginPos == std::string::npos)
    {
        return "-";
    }
    beginPos += beginToken.size();
    size_t endPos = text.find(endToken, beginPos);
    if (endPos == std::string::npos)
    {
        return "-";
    }
    return text.substr(beginPos, endPos - beginPos);
}

std::string
CompactContextId(const std::string& context)
{
    std::string nodeId = ExtractBetween(context, "/NodeList/", "/");
    std::string deviceId = ExtractBetween(context, "/DeviceList/", "/");
    return "N" + nodeId + "D" + deviceId;
}

std::string
CompactContextType(const std::string& context)
{
    size_t pos = context.find("/DeviceList/");
    if (pos == std::string::npos)
    {
        return "Unknown";
    }
    pos = context.find('/', pos + std::string("/DeviceList/").size());
    if (pos == std::string::npos || pos + 1 >= context.size())
    {
        return "Unknown";
    }

    std::string tail = context.substr(pos + 1);
    const std::string token = "$ns3::";
    size_t tokenPos = 0;
    while ((tokenPos = tail.find(token, tokenPos)) != std::string::npos)
    {
        tail.erase(tokenPos, token.size());
    }
    return tail;
}

std::string
SummarizeChoActor(const std::string& eventName)
{
    if (eventName == "A3ConditionSatisfied")
    {
        return "actor=Source-gNB action=PROCESS io=internal";
    }
    if (eventName == "ChoPrepareStart")
    {
        return "actor=Source-gNB action=PROCESS io=internal";
    }
    if (eventName == "HoRequestSentToCandidates")
    {
        return "actor=Source-gNB action=TX io=out";
    }
    if (eventName == "HoRequestAckReceived")
    {
        return "actor=Source-gNB action=RX io=in";
    }
    if (eventName == "RrcReconfigurationWithChoSent")
    {
        return "actor=Source-gNB action=TX io=out";
    }
    if (eventName == "ChoStoredInUe")
    {
        return "actor=UE action=PROCESS io=internal";
    }
    if (eventName == "ChoExecutionStart")
    {
        return "actor=UE action=PROCESS io=internal";
    }
    if (eventName == "TriggerHandoverCalled")
    {
        return "actor=Source-gNB action=PROCESS io=internal";
    }
    if (eventName == "ChoCancelledConditionLeave")
    {
        return "actor=Source-gNB action=PROCESS io=internal";
    }
    if (eventName == "ChoPendingEntryRemoved")
    {
        return "actor=Source-gNB action=PROCESS io=internal";
    }
    if (eventName == "AlgorithmInitialized")
    {
        return "actor=Source-gNB action=PROCESS io=internal";
    }
    return "actor=Unknown action=PROCESS io=internal";
}

std::string
SummarizeChoMessage(const std::string& eventName)
{
    if (eventName == "HoRequestSentToCandidates" || eventName == "HoRequestAckReceived")
    {
        return "message=HoPreparation";
    }
    if (eventName == "RrcReconfigurationWithChoSent" || eventName == "ChoStoredInUe")
    {
        return "message=RrcReconfiguration(CHO)";
    }
    if (eventName == "TriggerHandoverCalled")
    {
        return "message=TriggerHandover";
    }
    if (eventName == "A3ConditionSatisfied")
    {
        return "message=MeasurementReport/A3";
    }
    return "message=" + eventName;
}

void
NotifyConnectionEstablishedUe(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    cho_summary_log << Simulator::Now().GetMilliSeconds() << "ms [UE-CONN] [" << CompactContextId(context)
                    << "] IMSI=" << imsi << " Cell=" << cellid << " RNTI=" << rnti << std::endl;
}

void
NotifyHandoverStartUe(std::string context,
                      uint64_t imsi,
                      uint16_t cellid,
                      uint16_t rnti,
                      uint16_t targetCellId)
{
    cho_summary_log << Simulator::Now().GetMilliSeconds() << "ms [UE-HO-START] ["
                    << CompactContextId(context) << "] IMSI=" << imsi << " srcCell=" << cellid
                    << " tgtCell=" << targetCellId << " RNTI=" << rnti << std::endl;
}

void
NotifyHandoverEndOkUe(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    cho_summary_log << Simulator::Now().GetMilliSeconds() << "ms [UE-HO-END] [" << CompactContextId(context)
                    << "] IMSI=" << imsi << " newCell=" << cellid << " RNTI=" << rnti << std::endl;
}

void
NotifyConnectionEstablishedEnb(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    cho_summary_log << Simulator::Now().GetMilliSeconds() << "ms [ENB-CONN] [" << CompactContextId(context)
                    << "] Cell=" << cellid << " IMSI=" << imsi << " RNTI=" << rnti << std::endl;
}

void
NotifyHandoverStartEnb(std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti,
                       uint16_t targetCellId)
{
    cho_summary_log << Simulator::Now().GetMilliSeconds() << "ms [ENB-HO-START] ["
                    << CompactContextId(context) << "] srcCell=" << cellid << " tgtCell="
                    << targetCellId << " IMSI=" << imsi << " RNTI=" << rnti << std::endl;
}

void
NotifyHandoverEndOkEnb(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    cho_summary_log << Simulator::Now().GetMilliSeconds() << "ms [ENB-HO-END] [" << CompactContextId(context)
                    << "] newCell=" << cellid << " IMSI=" << imsi << " RNTI=" << rnti << std::endl;
}

void
NotifyChoEvent(std::string context,
               uint64_t timestampMs,
               uint16_t rnti,
               std::string eventName,
               uint16_t targetCellId,
               uint8_t servingRsrp,
               uint8_t targetRsrp,
               uint16_t candidateCount)
{
    const std::string actorInfo = SummarizeChoActor(eventName);
    const std::string messageInfo = SummarizeChoMessage(eventName);
    cho_summary_log << timestampMs << "ms [CHO-EVENT] [" << CompactContextId(context) << "] " << eventName
                    << " " << actorInfo << " " << messageInfo
                    << " UE-RNTI=" << rnti << " tgtCell=" << targetCellId
                    << " srvRsrp=" << static_cast<uint16_t>(servingRsrp)
                    << " tgtRsrp=" << static_cast<uint16_t>(targetRsrp)
                    << " cand=" << candidateCount << std::endl;
}

void
NotifyChoDetailedEvent(std::string context,
                       uint64_t timestampMs,
                       uint16_t rnti,
                       std::string stepName,
                       std::string detail)
{
    cho_detail_log << timestampMs << "ms [CHO-DETAIL] [" << CompactContextId(context) << "] ["
                   << CompactContextType(context) << "] " << stepName << " UE-RNTI=" << rnti
                   << " | " << detail << std::endl;

    const bool isRaStep =
        stepName.rfind("Step5_RA_", 0) == 0 || stepName == "Step5_RandomAccessStart" ||
        stepName == "Step5_RandomAccessComplete";

    if (isRaStep)
    {
        cho_summary_log << timestampMs << "ms [CHO-RA] [" << CompactContextId(context)
                       << "] " << stepName << " UE-RNTI=" << rnti << " | " << detail
                       << std::endl;
    }
}

int
main(int argc, char* argv[])
{
    cho_summary_log.open("cho-summary.log");
    cho_detail_log.open("cho-detail.log");

    uint16_t numberOfUes = 1;
    uint16_t numberOfEnbs = 7;
    uint16_t numBearersPerUe = 0;
    double distance = 500.0;
    double yForUe = 500.0;
    double speed = 20;
    double simTime = 95.0;
    double enbTxPowerDbm = 46.0;

    double choHysteresisDb = 0.0;
    uint16_t choTttMs = 256;
    uint16_t measurementReportDelayMs = 1;
    uint16_t choDecisionDelayMs = 1;
    uint16_t hoRequestPropagationDelayMs = 1;
    uint16_t hoPreparationDelayMs = 8;
    uint16_t hoPreparationPerTargetOffsetMs = 1;
    uint16_t choCommandDeliveryDelayMs = 2;
    uint16_t randomAccessDurationMs = 6;
    uint16_t randomAccessStepDelayMs = 1;
    uint16_t pathSwitchDelayMs = 2;

    Config::SetDefault("ns3::UdpClient::Interval", TimeValue(MilliSeconds(10)));
    Config::SetDefault("ns3::UdpClient::MaxPackets", UintegerValue(1000000));
    Config::SetDefault("ns3::LteHelper::UseIdealRrc", BooleanValue(true));

    CommandLine cmd(__FILE__);
    cmd.AddValue("simTime", "Total duration of the simulation (in seconds)", simTime);
    cmd.AddValue("speed", "Speed of the UE (default = 20 m/s)", speed);
    cmd.AddValue("enbTxPowerDbm", "TX power [dBm] used by eNBs (default = 46.0)", enbTxPowerDbm);
    cmd.AddValue("choHysteresisDb", "CHO hysteresis in dB (default = 0.0)", choHysteresisDb);
    cmd.AddValue("choTttMs", "CHO time-to-trigger in ms (default = 256)", choTttMs);
    cmd.AddValue("measurementReportDelayMs", "MR send->receive delay in ms", measurementReportDelayMs);
    cmd.AddValue("choDecisionDelayMs", "CHO decision processing delay in ms", choDecisionDelayMs);
    cmd.AddValue("hoRequestPropagationDelayMs", "HO request/ACK one-way propagation delay in ms", hoRequestPropagationDelayMs);
    cmd.AddValue("hoPreparationDelayMs", "Admission-control base delay in ms", hoPreparationDelayMs);
    cmd.AddValue("hoPreparationPerTargetOffsetMs",
                 "Additional admission-control delay per candidate index in ms",
                 hoPreparationPerTargetOffsetMs);
    cmd.AddValue("choCommandDeliveryDelayMs", "Source->UE CHO command delay in ms", choCommandDeliveryDelayMs);
    cmd.AddValue("randomAccessDurationMs", "UE random-access duration in ms", randomAccessDurationMs);
    cmd.AddValue("randomAccessStepDelayMs", "UE random-access per-step timeline delay in ms", randomAccessStepDelayMs);
    cmd.AddValue("pathSwitchDelayMs", "Modeled path-switch delay in ms", pathSwitchDelayMs);
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");

    lteHelper->SetHandoverAlgorithmType("ns3::ConditionalHandoverAlgorithm");
    lteHelper->SetHandoverAlgorithmAttribute("Hysteresis", DoubleValue(choHysteresisDb));
    lteHelper->SetHandoverAlgorithmAttribute("TimeToTrigger", TimeValue(MilliSeconds(choTttMs)));
    lteHelper->SetHandoverAlgorithmAttribute("MeasurementReportDelay",
                                             TimeValue(MilliSeconds(measurementReportDelayMs)));
    lteHelper->SetHandoverAlgorithmAttribute("ChoDecisionDelay",
                                             TimeValue(MilliSeconds(choDecisionDelayMs)));
    lteHelper->SetHandoverAlgorithmAttribute("HoRequestPropagationDelay",
                                             TimeValue(MilliSeconds(hoRequestPropagationDelayMs)));
    lteHelper->SetHandoverAlgorithmAttribute("HoPreparationDelay",
                                             TimeValue(MilliSeconds(hoPreparationDelayMs)));
    lteHelper->SetHandoverAlgorithmAttribute("HoPreparationPerTargetOffset",
                                             TimeValue(MilliSeconds(hoPreparationPerTargetOffsetMs)));
    lteHelper->SetHandoverAlgorithmAttribute("ChoCommandDeliveryDelay",
                                             TimeValue(MilliSeconds(choCommandDeliveryDelayMs)));
    lteHelper->SetHandoverAlgorithmAttribute("RandomAccessDuration",
                                             TimeValue(MilliSeconds(randomAccessDurationMs)));
    lteHelper->SetHandoverAlgorithmAttribute("RandomAccessStepDelay",
                                             TimeValue(MilliSeconds(randomAccessStepDelayMs)));
    lteHelper->SetHandoverAlgorithmAttribute("PathSwitchDelay",
                                             TimeValue(MilliSeconds(pathSwitchDelayMs)));

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numberOfEnbs);
    ueNodes.Create(numberOfUes);

    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    enbPositionAlloc->Add(Vector(500.0, distance, 0));
    enbPositionAlloc->Add(Vector(980.0, distance, 0));
    enbPositionAlloc->Add(Vector(1020.0, distance, 0));
    enbPositionAlloc->Add(Vector(1060.0, distance, 0));
    enbPositionAlloc->Add(Vector(1480.0, distance, 0));
    enbPositionAlloc->Add(Vector(1520.0, distance, 0));
    enbPositionAlloc->Add(Vector(1560.0, distance, 0));

    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator(enbPositionAlloc);
    enbMobility.Install(enbNodes);

    MobilityHelper ueMobility;
    ueMobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    ueMobility.Install(ueNodes);
    ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0, yForUe, 0));
    ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(speed, 0, 0));

    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(enbTxPowerDbm));
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    for (uint16_t i = 0; i < numberOfUes; i++)
    {
        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(0));
    }

    uint16_t dlPort = 10000;
    uint16_t ulPort = 20000;

    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetAttribute("Min", DoubleValue(0));
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.010));

    for (uint32_t u = 0; u < numberOfUes; ++u)
    {
        Ptr<Node> ue = ueNodes.Get(u);
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

        for (uint32_t b = 0; b < numBearersPerUe; ++b)
        {
            ++dlPort;
            ++ulPort;

            ApplicationContainer clientApps;
            ApplicationContainer serverApps;

            UdpClientHelper dlClientHelper(ueIpIfaces.GetAddress(u), dlPort);
            clientApps.Add(dlClientHelper.Install(remoteHost));
            PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                                                InetSocketAddress(Ipv4Address::GetAny(), dlPort));
            serverApps.Add(dlPacketSinkHelper.Install(ue));

            UdpClientHelper ulClientHelper(remoteHostAddr, ulPort);
            clientApps.Add(ulClientHelper.Install(ue));
            PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory",
                                                InetSocketAddress(Ipv4Address::GetAny(), ulPort));
            serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

            Ptr<EpcTft> tft = Create<EpcTft>();
            EpcTft::PacketFilter dlpf;
            dlpf.localPortStart = dlPort;
            dlpf.localPortEnd = dlPort;
            tft->Add(dlpf);
            EpcTft::PacketFilter ulpf;
            ulpf.remotePortStart = ulPort;
            ulpf.remotePortEnd = ulPort;
            tft->Add(ulpf);
            EpsBearer bearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
            lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(u), bearer, tft);

            Time startTime = Seconds(startTimeSeconds->GetValue());
            serverApps.Start(startTime);
            clientApps.Start(startTime);
        }
    }

    lteHelper->AddX2Interface(enbNodes);

    lteHelper->EnablePhyTraces();
    lteHelper->EnableMacTraces();
    lteHelper->EnableRlcTraces();
    lteHelper->EnablePdcpTraces();

    Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats();
    rlcStats->SetAttribute("EpochDuration", TimeValue(Seconds(1)));
    Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats();
    pdcpStats->SetAttribute("EpochDuration", TimeValue(Seconds(1)));

    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                    MakeCallback(&NotifyHandoverStartEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                    MakeCallback(&NotifyHandoverStartUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                    MakeCallback(&NotifyHandoverEndOkEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                    MakeCallback(&NotifyHandoverEndOkUe));
    Config::Connect(
        "/NodeList/*/DeviceList/*/$ns3::LteEnbNetDevice/LteHandoverAlgorithm/$ns3::ConditionalHandoverAlgorithm/ChoEvent",
        MakeCallback(&NotifyChoEvent));
    Config::Connect(
        "/NodeList/*/DeviceList/*/$ns3::LteEnbNetDevice/LteHandoverAlgorithm/$ns3::ConditionalHandoverAlgorithm/ChoDetailedEvent",
        MakeCallback(&NotifyChoDetailedEvent));

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    cho_summary_log.close();
    cho_detail_log.close();

    return 0;
}
