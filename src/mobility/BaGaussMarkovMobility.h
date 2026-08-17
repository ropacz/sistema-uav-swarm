#pragma once

#include "inet/mobility/single/GaussMarkovMobility.h"

namespace echosar {

class BaGaussMarkovMobility : public inet::GaussMarkovMobility
{
  protected:
    bool baOverride = false;
    bool holding = false;
    int waypointId = 0;

    virtual void setTargetPosition() override;

  public:
    void moveTo(const inet::Coord& destination, double horizontalSpeed,
                double climbSpeed, double descentSpeed);
    void resumeNormal();
    bool isHolding() const { return holding; }
    int getWaypointId() const { return waypointId; }
};

} // namespace echosar
