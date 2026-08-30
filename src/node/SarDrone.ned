package echosar.node;

import echosar.camera.AbstractObstacleSensor;
import inet.node.aodv.AodvRouter;

module SarDrone extends AodvRouter
{
    submodules:
        obstacleSensor: AbstractObstacleSensor {
            @display("p=50,50");
        }
}
