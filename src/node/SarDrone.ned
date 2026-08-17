package echosar.node;

import echosar.sensing.AbstractObstacleSensor;
import inet.node.aodv.AodvRouter;

module SarDrone extends AodvRouter
{
    submodules:
        obstacleSensor: AbstractObstacleSensor {
            @display("p=50,50");
        }
}
