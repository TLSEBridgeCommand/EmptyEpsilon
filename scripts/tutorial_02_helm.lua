-- Name: Helm
-- Description: 
--- -------------------
--- [Operations (Helms Station) Info]
--- -------------------
--- Overview: 
--- It is important that the Operations Officers familiarise themselves with the movement ability of the ship.
---
--- Data: 
--- - In the upper-left corner, the Helms officer's screen displays the ship's energy (max is 1,500), current heading in degrees, and current speed in Units/minute.
---
--- Engines: 
--- - The Helm screen includes two sliders
--- - The left slider controls the impulse engines, from -100% (full reverse) to 0% (full stop) to 100% (full ahead).
--- - The right slider controls the warp drive through 4 different speed settings (warp one through warp four).
---
--- Setting a Heading: 
--- - The Helms officer has a short-range radar. Pressing inside this radar sets the ship's heading in that direction. 
--- - Be sure to communicate bwtween each other when it combat, as beam weapons and certain missiles can only fire in the direction of the current heading.
---
--- Combat Maneuvers: 
--- - The Helms screen includes a Combat Maneuver panel. It allows your ship to make quick movement in any direction, and is best use sparingly to avoid collisions to missiles or haserdous terrain. Combat maneuvers can be exhausted but recharge over time.
---
--- Docking: 
--- -The Helms officer can dock with stations as well as other ships when they are within 1U. While docked, the ship can't engage its engines or fire weapons, but its energy recharges faster, repairs take less time, and the Relay officer can request missile weapon rearmament. The Helms officer is also responsible for undocking the ship.
---
--- Retrieving Objects: 
--- -The Helms officer may also responsible for piloting the ship into supply drops and other retrievable items.
-- Type: Basic
require("utils.lua")
function init()
    --Create the player ship
    player = PlayerSpaceship():setFaction("UCN"):setTemplate("UCS Hoplite Class Destroyer")
    tutorial:setPlayerShip(player)

    tutorial:showMessage(_([[[Welcome to the bridge tutorial.
    Note that this tutorial is designed to give you a quick overview of the basic operations, but does not cover every single aspect.
    
    Press "Next" to continue]]), true)
    tutorial_list = {
        helmsTutorial,
        endOfTutorial
    }
    tutorial:onNext(function()
        tutorial_list_index = 1
        startSequence(tutorial_list[tutorial_list_index])
    end)
end

-- TODO: Need to refactor this region into a utility (*Need help LUA hates me)
--[[ Assist function in creating tutorial sequences --]]
function startSequence(sequence)
    current_sequence = sequence
    current_index = 1
    runNextSequenceStep()
end

function runNextSequenceStep()
    local data = current_sequence[current_index]
    current_index = current_index + 1
    if data == nil then
        tutorial_list_index = tutorial_list_index + 1
        if tutorial_list[tutorial_list_index] ~= nil then
            startSequence(tutorial_list[tutorial_list_index])
        else
            tutorial:finish()
        end
    elseif data["message"] ~= nil then
        tutorial:showMessage(data["message"], data["finish_check_function"] == nil)
        if data["finish_check_function"] == nil then
            update = nil
            tutorial:onNext(runNextSequenceStep)
        else
            update = function(delta)
                if data["finish_check_function"]() then
                    runNextSequenceStep()
                end
            end
            tutorial:onNext(nil)
        end
    elseif data["run_function"] ~= nil then
        local has_next_step = current_index <= #current_sequence
        data["run_function"]()
        if has_next_step then
            runNextSequenceStep()
        end
    end
end

function createSequence()
    return {}
end

function addToSequence(sequence, data, data2)
    if type(data) == "string" then
        if data2 == nil then
            table.insert(sequence, {message = data})
        else
            table.insert(sequence, {message = data, finish_check_function = data2})
        end
    elseif type(data) == "function" then
        table.insert(sequence, {run_function = data})
    end
end

function resetPlayerShip()
    player:setJumpDrive(false)
    player:setWarpDrive(false)
    player:setImpulseMaxSpeed(1)
    player:setRotationMaxSpeed(1)
    for _, system in ipairs({"reactor", "beamweapons", "missilesystem", "maneuver", "impulse", "warp", "jumpdrive", "frontshield", "rearshield"}) do
        player:setSystemHealth(system, 1.0)
        player:setSystemHeat(system, 0.0)
        player:setSystemPower(system, 1.0)
        player:commandSetSystemPowerRequest(system, 1.0)
        player:setSystemCoolant(system, 0.0)
        player:commandSetSystemCoolantRequest(system, 0.0)
    end
    player:setPosition(0, 0)
    player:setRotation(0)
    player:commandImpulse(0)
    player:commandWarp(0)
    player:commandTargetRotation(0)
    player:commandSetShields(false)
    player:setWeaponStorageMax("homing", 0)
    player:setWeaponStorageMax("nuke", 0)
    player:setWeaponStorageMax("mine", 0)
    player:setWeaponStorageMax("emp", 0)
    player:setWeaponStorageMax("hvli", 0)
end
--End Region Tut Utils


helmsTutorial = createSequence()
addToSequence(helmsTutorial, function()
    tutorial:switchViewToScreen(0)
    tutorial:setMessageToTopPosition()
    resetPlayerShip()
    player:setJumpDrive(false)
    player:setWarpDrive(false)
    player:setImpulseMaxSpeed(0);
    player:setRotationMaxSpeed(0);
end)
addToSequence(helmsTutorial, _([[This is the helms screen.
As the helms officer, you command your ship's movement in space.]]))
addToSequence(helmsTutorial, function() player:setImpulseMaxSpeed(100) end)
addToSequence(helmsTutorial, _([[Your primary controls are your impulse engines and maneuvering thrusters.
Your impulse controls are on the left side of the screen. You can impulse forwards as well as backwards.

Raise your impulse level to 100% to fly forward right now.]]), function() return distance(player, 0, 0) > 1000 end)
addToSequence(helmsTutorial, function() player:setImpulseMaxSpeed(0):commandImpulse(0):setRotationMaxSpeed(10) end)
addToSequence(helmsTutorial, _([[Good. You now know how to move forward.

Your impulse engines have been disabled for now. Next, let's rotate your ship.
Simply press a heading on the radar screen to rotate your ship towards that heading.
Try rotating to heading 200 now.]]), function() return math.abs(player:getHeading() - 200) < 1.0 end)
addToSequence(helmsTutorial, function() player:setImpulseMaxSpeed(100) end)
addToSequence(helmsTutorial, function() prev_object = CpuShip():setTemplate("King George Battleship"):setFaction("UCN"):setCallSign("UCS Warspite"):setPosition(0, -1500):setScanned(true) end)
addToSequence(helmsTutorial, _([[Excellent.

Next up: docking. You can dock with stations as well as other ships. In most cases, docking replenishes energy, repairs your hull, and allows the relay officer to request resupply.
To dock, maneuver within 1u of a station and press the "Initiate Docking" button, from which point docking is fully automated.
Maneuver to UCS Warspite and request permission to dock.]]), function() return player:isDocked(prev_object) end)
addToSequence(helmsTutorial, _([[Now that you are docked, your movement is locked. As helms officer, there is nothing else you can do but undock, so do that now.]]), function() return not player:isDocked(prev_object) end)
addToSequence(helmsTutorial, function() prev_object:destroy() end)
addToSequence(helmsTutorial, function() prev_object = CpuShip():setFaction("corsair"):setTemplate("Gunship"):setPosition(-1500, 1500):orderIdle():setScanned(true):setHull(15):setShieldsMax(15) end)
addToSequence(helmsTutorial, function() player:commandSetTarget(prev_object, 0) end)
addToSequence(helmsTutorial, _([[As a member of Operations team, it is your task to keep your ship's beams on your target.
I've set up an stationary enemy ship as a target. Destroy it with your beam weapons. Note that at every shot, the corresponding firing arc will change color.]]), function() return not prev_object:isValid() end)
addToSequence(helmsTutorial, _([[As officers of the United Confideration Navy it is your responsibility to remember that aggression is not always the solution.

On to the next task: moving long distances.
Most ships are equipped with a warp drive which will allow you to traverse long distances quickly.]]))
addToSequence(helmsTutorial, function() player:setWarpDrive(true) end)
addToSequence(helmsTutorial, _([[Unlike impulse engines, warp drive can only move your ship forward. It also consumes energy at a faster rate, and can easily overheat.
Use the warp drive to move more than 30u away from this starting point.]]), function() return distance(player, 0, 0) > 30000 end)
addToSequence(helmsTutorial, _([[Always remember to communicate with the Engineering team when going to warp, and keep a good communication between all your team members to keep the ship running smoothly.]]))

endOfTutorial = createSequence()
addToSequence(endOfTutorial, function() tutorial:switchViewToMainScreen() end)
addToSequence(endOfTutorial, _([[This concludes the Helm screen tutorial, there is plenty more for you to learn on the job about your ship's Operations. 
Please don't press anything else on your screen, and let the officer taking you through the tutorial know that you have finished training.]]))

