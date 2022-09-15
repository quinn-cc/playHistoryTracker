/*
 Grue's Play History Tracker
 - This plugin keeps track of rampages, killing sprees, etc.
 This is one of the base plugins that come with bzflag, although this version is
 edited.
 - Awards bounty points for stopping rampages by opponents, penalizes teammates
   that end rampages.
   
 Special notes:
 - This works with the avengerFlag, dimensionDoorFlag, and annihilationFlag plugin,
   however those plugins are not necessary to run this one.

 Copyright 2022 Quinn Carmack
 May be redistributed under either the LGPL or MIT licenses. 

 ./configure --enable-custom-plugins=playHistoryTracker
*/


#include "bzfsAPI.h"
#include <map>

class PlayHistoryTracker : public bz_Plugin
{
public:
    virtual const char* Name ()
    {
        return "Grue's Play History Tracker";
    }
    virtual void Init (const char* /* config */)
    {
        // Register our events
        Register(bz_ePlayerDieEvent);
        Register(bz_ePlayerPartEvent);
        Register(bz_ePlayerJoinEvent);
    }

    virtual void Event (bz_EventData *eventData);

protected:
    std::map<int, int> spreeCount;
};

BZ_PLUGIN(PlayHistoryTracker)

void PlayHistoryTracker::Event(bz_EventData *eventData)
{
    switch (eventData->eventType)
    {
		case bz_ePlayerDieEvent:
		{
		    bz_PlayerDieEventData_V2 *data = (bz_PlayerDieEventData_V2*)eventData;
		    
		    // Create variables to store the callsigns
		    std::string victimCallsign = bz_getPlayerCallsign(data->playerID);
		    std::string killerCallsign = "";
		    
		    int killerID = data->killerID;
		    
		    // Handles the possibility it was a player's world weapon.
	    	uint32_t shotGUID = bz_getShotGUID(killerID, data->shotID);
	    	if (bz_shotHasMetaData(shotGUID, "owner"))
	    		killerID = bz_getShotMetaDataI(shotGUID, "owner");
	    		
		    bz_BasePlayerRecord* victimRecord = bz_getPlayerByIndex(data->playerID);
		    bz_BasePlayerRecord* killerRecord = bz_getPlayerByIndex(killerID);
		    if (!victimRecord || !killerRecord) return;
		    bz_freePlayerRecord(victimRecord);
		    bz_freePlayerRecord(killerRecord);
		    
		    killerCallsign = bz_getPlayerCallsign(killerID);
		    
		    // Handle the victim
		    if (spreeCount.find(data->playerID) != spreeCount.end())
		    {
		    	bz_ApiString flagHeldWhenKilled = bz_getFlagName(data->flagHeldWhenKilled);
		    	bz_ApiString flagKilledWith = data->flagKilledWith;
		    	bool isAvengedGeno = (flagHeldWhenKilled == "AV" && flagKilledWith == "R*") || (flagHeldWhenKilled == "AV" && flagKilledWith == "G*");
		    	bool isAnnihilation = flagHeldWhenKilled == "AN" && data->playerID == killerID;
		    	
		    	if (!isAvengedGeno && !isAnnihilation)
		    	{
				    // Store a quick reference to their former spree count
				    int spreeTotal = spreeCount[data->playerID];

					// Check if they were on any sort of rampage
					if (spreeTotal >= 5)
					{
						std::string message;
						std::string bountyMessage;
						int points = 2*(spreeTotal/5);

						// Generate an appropriate message, if any
						if (spreeTotal >= 5 && spreeTotal < 10)
						{
							if (data->playerID == killerID)
							{
								message = victimCallsign + std::string(" self-pwned, ending that rampage.");
							}
							else if (bz_getPlayerTeam(data->playerID) == bz_getPlayerTeam(killerID))
							{
								message = victimCallsign + std::string("'s rampage was stopped by teammate ") + killerCallsign;
								bountyMessage = "Stopping teammate " + victimCallsign + "'s rampage lost you 2 extra points";
							}
							else
							{
								message = victimCallsign + std::string("'s rampage was stopped by ") + killerCallsign;
								bountyMessage = "Stopping " + victimCallsign + "'s rampage earned you 2 extra bounty points";
							}
						}
						else if (spreeTotal >= 10 && spreeTotal < 15)
						{
							if (data->playerID == killerID)
							{
								message = victimCallsign + std::string(" killed themselves in their own killing spree!");
							}
							else if (bz_getPlayerTeam(data->playerID) == bz_getPlayerTeam(killerID))
							{
								message = victimCallsign + std::string("'s killing spree was halted by their own teammate ") + killerCallsign;
								bountyMessage = "Halting teammate " + victimCallsign + "'s killing spree lost you 4 extra points";
							}
							else
							{
								message = victimCallsign + std::string("'s killing spree was halted by ") + killerCallsign;
								bountyMessage = "Halting " + victimCallsign + "'s killing spree earned you 4 extra bounty points";
							}
						}
						else if (spreeTotal >= 15 && spreeTotal < 20)
						{
							if (data->playerID == killerID)
							{
								message = victimCallsign + std::string(" for some reason blitz'ed themselves...");
							}
							else if (bz_getPlayerTeam(data->playerID) == bz_getPlayerTeam(killerID))
							{
								message = victimCallsign + std::string("'s blitzkrieg was abrubtly ended by teammate ") + killerCallsign;
								bountyMessage = "Ending teammate " + victimCallsign + "'s blitzkrieg lost you 6 extra points. You're supposed to be family.";
							}
							else
							{
								message = victimCallsign + std::string("'s blitzkrieg was finally ended by ") + killerCallsign;
								bountyMessage = "Ending " + victimCallsign + "'s blitzkrieg earned you 6 extra bounty points";
							}
						}
						else if (spreeTotal >= 20)
					   	{
					   		if (data->playerID == killerID)
					   		{
								message = std::string("The only force that could stop ") + victimCallsign + std::string(" is ") + victimCallsign + std::string("... and they did");
							}
							else if (bz_getPlayerTeam(data->playerID) == bz_getPlayerTeam(killerID))
							{
								message = std::string("The unstoppable reign of ") + victimCallsign + std::string(" was ended by TEAMMATE ") + killerCallsign + std::string(". What the hell?!");
								bountyMessage = "Ending teammate " + victimCallsign + "'s reign of destruction LOST you " + std::to_string(points) + " extra points! C'mon!";
							}
							else
							{
								message = std::string("Finally! The unstoppable reign of ") + victimCallsign + std::string(" was ended by ") + killerCallsign;
								bountyMessage = "Ending " + victimCallsign + "'s reign of destruction earned you " + std::to_string(points) + " extra bounty points";
							}
						}
						
						bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, message.c_str());

						// Only award bounty if it's not a self-kill and not a teammate & not the server
						if (data->playerID != killerID)
						{
							if (bz_getPlayerTeam(data->playerID) == bz_getPlayerTeam(killerID))
								bz_incrementPlayerLosses(killerID, points);
							else
								bz_incrementPlayerWins(killerID, points);
						}
				    }

				    // Since they died, release their spree counter
				    spreeCount[data->playerID] = 0;
		        }
		    }

		    // Handle the killer (if it wasn't also the victim)
		    if (data->playerID != killerID && spreeCount.find(killerID) != spreeCount.end())
		    {
		        // Store a quick reference to their newly incremented spree count
		        int spreeTotal = ++spreeCount[killerID];

		        std::string message;

		        // Generate an appropriate message, if any
		        if (spreeTotal == 5)
		            message = killerCallsign + std::string(" is on a rampage!");
		        else if (spreeTotal == 10)
		            message = killerCallsign + std::string(" is on a killing spree!");
		        else if (spreeTotal == 15)
		        {
		            message = killerCallsign + std::string(" is going blitz! 6 bounty points on their head!");
		        }
		        else if (spreeTotal == 20)
		        {
		            message = killerCallsign + std::string(" is unstoppable!! Come on people; 8 bounty points!!");
		        }
		        else if (spreeTotal > 20 && spreeTotal%5 == 0)
		        {
		        	int points = 2*(spreeTotal/5);
		            message = killerCallsign + std::string(" continues to obliterate... the bounty is now up to ") + std::to_string(points) + std::string(" points!");
		        }

		        // If we have a message to send, then send it
		        if (message.size())
		            bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, message.c_str());
		    }
		} break;
		case bz_ePlayerJoinEvent:
		{
		    // Initialize the spree counter for the player that just joined
		    spreeCount[((bz_PlayerJoinPartEventData_V1*)eventData)->playerID] = 0;
		} break;
		case bz_ePlayerPartEvent:
		{
		    // Erase the spree counter for the player that just left
		    std::map<int, int >::iterator itr = spreeCount.find(((bz_PlayerJoinPartEventData_V1*)eventData)->playerID);
		    if (itr != spreeCount.end())
		        spreeCount.erase(itr);
		} break;
		default:
		    break;
    }
}
