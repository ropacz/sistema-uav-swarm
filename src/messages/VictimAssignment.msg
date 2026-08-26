namespace echosar;

message VictimAssignment
{
    string victimId;
    double victimPositionX;
    double victimPositionY;
    double victimPositionZ;
    simtime_t detectionTimestamp;
}
