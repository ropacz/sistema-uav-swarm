package echosar.app;

import inet.applications.contract.IApp;

simple DroneApp like IApp
{
    parameters:
        @class(echosar::DroneApp);
        string droneId = default("");
        int appPort = default(5000);
        int victimAlertPayloadBytes = default(320);
        double retryInterval @unit(s) = default(30s);
        double ackTimeout @unit(s) = default(5s);
        int maxAttempts = default(5);
        double alertTtl @unit(s) = default(180s);
        double linkWindow @unit(s) = default(10s);
        double teamSilenceTimeout @unit(s) = default(3s);
        double maintenanceInterval @unit(s) = default(0.1s);
        double pdrThreshold = default(0.8);
        double rssiThreshold @unit(dBm) = default(-80dBm);
        bool baEnabled = default(true);
        int maxBaCycles = default(2);
        double maximumRepositionDistance @unit(m) = default(25m);
        double minimumAltitude @unit(m) = default(6m);
        double maximumAltitude @unit(m) = default(20m);
        double areaMinX @unit(m) = default(0m);
        double areaMaxX @unit(m) = default(1000m);
        double areaMinY @unit(m) = default(0m);
        double areaMaxY @unit(m) = default(1000m);
        double horizontalSpeed @unit(mps) = default(13mps);
        double climbSpeed @unit(mps) = default(5mps);
        double descentSpeed @unit(mps) = default(3mps);
        double flightTimeLimit @unit(s) = default(1800s);
        int applicationIpTtl = default(32);
        double wLink = default(0.60);
        double wObstacle = default(0.25);
        double wMove = default(0.15);
        double obstacleSigma @unit(m) = default(10m);
        double obstacleSafetyMargin @unit(m) = default(1m);
        double linkNormalizationDistance @unit(m) = default(1000m);
        int batPopulation = default(20);
        int batIterations = default(50);
        int batInitializationAttempts = default(100);
        double batFrequencyMin = default(0);
        double batFrequencyMax = default(2);
        double batInitialAmplitude = default(0.9);
        double batInitialPulseRate = default(0.5);
        double batAmplitudeDecay = default(0.9);
        double batPulseGrowth = default(0.9);
        double batLocalSearchScale = default(0.1);
        @signal[positionUpdateRssi](type=double; unit=dBm);
        @statistic[positionUpdateRssi](source=positionUpdateRssi; record=vector,mean,min,max);
        @signal[linkWindowPdr](type=double);
        @statistic[linkWindowPdr](source=linkWindowPdr; record=vector,mean);
        @signal[repositionDistance](type=double; unit=m);
        @statistic[repositionDistance](source=repositionDistance; record=vector,sum,mean);
        @signal[recoveryTime](type="simtime_t");
        @statistic[recoveryTime](source=recoveryTime; record=vector,mean; unit=s);
        @display("i=misc/drone");
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
        input assignmentIn;
}
