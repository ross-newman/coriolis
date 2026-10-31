#ifndef CORIOLIS_MISSIONS_H_
#define CORIOLIS_MISSIONS_H_

#include <string>
#include <vector>

#include "coriolis/commander.h"
#include "coriolis/galaxy.h"

namespace coriolis {

/// The story is a chain of jobs, offered one after another at any station: carry a sealed packet, clear pirates from
/// the lanes, carry a second packet, break the pirate fleet, and carry the plans of a galactic hyperdrive to the
/// engineer who will build one. It is written for this game. The next step on offer is `Commander::story_step`.
constexpr int kStorySteps = 5;

/// The job on offer at the commander's present system, if there is one: the next step of the story, with its place and
/// reward worked out. False if the story is finished. (Nothing is on offer while a mission is under way; that check is
/// the game's.) The same commander in the same system is always offered the same job.
bool MissionOffered(const Commander& c, const Galaxy& galaxy, Mission* out);

/// The title of a mission in capitals, for the screen
std::string MissionTitle(const Mission& m);
/// The briefing as lines that fit an 80 column screen. `galaxy` gives the name of the destination.
std::vector<std::string> MissionBriefing(const Mission& m, const Galaxy& galaxy);
/// A line for the status screen: what the pilot has to do and how far along it is
std::string MissionStatus(const Mission& m, const Galaxy& galaxy);
/// What the pilot is told on completing a mission
std::string MissionDone(const Mission& m);

/// Is the job done, given that the ship is now docked at `system`? A delivery is done in its destination system, a hunt
/// when enough pirates are down, at any station.
bool MissionComplete(const Mission& m, int system);

/// Pay for a finished mission and move the story on. The last step also fits a galactic hyperdrive, or pays for one if
/// the ship has it already.
void PayMission(Commander& c);

}  // namespace coriolis

#endif  // CORIOLIS_MISSIONS_H_
