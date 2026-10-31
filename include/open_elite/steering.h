#ifndef OPEN_ELITE_STEERING_H_
#define OPEN_ELITE_STEERING_H_

#include "open_elite/math.h"
#include "open_elite/world.h"

namespace open_elite {

/// How fast a laser bolt flies (on top of the shooter's own speed) and how long it lasts
constexpr double kBoltSpeed = 1800.0;
constexpr double kBoltLife = 1.8;

/// Set the pitch, roll and yaw controls so the nose comes round to a world direction. Close to the line it uses yaw and
/// pitch; far from it, it rolls the target into the plane of the wings and pulls, which turns faster.
void SteerToward(Entity& ship, const Vec3d& direction);

/// Set the controls to turn the ship to an orientation, nose and roof
void SteerToOrientation(Entity& ship, const Quat& target);

/// The direction to fire in to hit a moving target with bolts that keep the shooter's own velocity
Vec3d LeadDirection(const Vec3d& shooter_position, const Vec3d& shooter_velocity, const Vec3d& target_position,
                    const Vec3d& target_velocity, double bolt_speed);

/// The angle between two directions in radians
double AngleBetween(const Vec3d& a, const Vec3d& b);

}  // namespace open_elite

#endif  // OPEN_ELITE_STEERING_H_
