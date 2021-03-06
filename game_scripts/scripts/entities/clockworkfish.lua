-- Copyright (C) 2007, 2010 - Bit-Blot
--
-- This file is part of Aquaria.
--
-- Aquaria is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License
-- as published by the Free Software Foundation; either version 2
-- of the License, or (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
--
-- See the GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

if not v then v = {} end
if not AQUARIA_VERSION then dofile("scripts/entities/entityinclude.lua") end

-- ================================================================================================
-- M A U L
-- ================================================================================================

-- entity specific
local STATE_FIRE			= 1000
local STATE_PULLBACK		= 1001
v.fireDelay = 0

v.dir = 0
 
-- ================================================================================================
-- FUNCTIONS
-- ================================================================================================

function init(me)
	setupBasicEntity(
	me,
	"",					-- texture
	6,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	32,								-- collideRadius 
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	64,							-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	-1							-- updateCull -1: disabled, default: 4000
	)
		
	entity_setDropChance(me, 50)
	
	entity_setMaxSpeedLerp(me, 0.5)
	entity_setMaxSpeedLerp(me, 1, 1, -1, 1)
	
	entity_initSkeletal(me, "ClockworkFish")
	entity_animate(me, "idle", -1)

	entity_setDeathParticleEffect(me, "TinyRedExplode")
	entity_scale(me, 0.75, 0.75)
	entity_setDamageTarget(me, DT_AVATAR_BITE, false)
end

function update(me, dt)	
	entity_handleShotCollisions(me)	
	entity_touchAvatarDamage(me, 32, 1, 1200)
	
	if v.dir==0 then
		entity_addVel(me, -1000, 0)
	else
		entity_addVel(me, 1000, 0)
	end
	
	entity_updateMovement(me, dt)
end

function enterState(me)
	if entity_isState(me, STATE_DEAD) then
		entity_sound(me, "MetalExplode", math.random(200)+1000)
		spawnParticleEffect("ClockworkFishDie", entity_getPosition(me))
	end
end

function exitState(me)
end

function hitSurface(me)
	entity_flipHorizontal(me)
	if v.dir == 0 then
		v.dir = 1
	elseif v.dir == 1 then 
		v.dir = 0
	end
end

function activate(me)
end
