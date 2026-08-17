namespace echosar;

message VictimAssignment
{
    string alertId;
    string victimId;
    double victimPositionX;
    double victimPositionY;
    double victimPositionZ;
    simtime_t detectionTimestamp;
}
