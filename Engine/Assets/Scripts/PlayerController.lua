-- ============================================================================
-- ApexEngine PlayerController Lua Script
-- Demonstrates exposed gameplay API bindings, physics interaction & math
-- ============================================================================

PlayerController = {}
PlayerController.__index = PlayerController

function PlayerController:New(entityId)
    local self = setmetatable({}, PlayerController)
    self.entityId = entityId
    self.moveSpeed = 8.5
    self.jumpForce = 12.0
    self.isGrounded = true
    self.accumulatedTime = 0.0
    return self
end

function PlayerController:OnStart()
    Apex.Log("PlayerController initialized for entity: " .. tostring(self.entityId))
end

function PlayerController:OnUpdate(deltaTime)
    self.accumulatedTime = self.accumulatedTime + deltaTime

    -- Read input from Engine Input System
    local moveX = 0.0
    local moveZ = 0.0

    if Apex.Input.IsKeyDown(Apex.Key.W) then moveZ = moveZ + 1.0 end
    if Apex.Input.IsKeyDown(Apex.Key.S) then moveZ = moveZ - 1.0 end
    if Apex.Input.IsKeyDown(Apex.Key.A) then moveX = moveX - 1.0 end
    if Apex.Input.IsKeyDown(Apex.Key.D) then moveX = moveX + 1.0 end

    -- Normalize movement vector
    local length = math.sqrt(moveX * moveX + moveZ * moveZ)
    if length > 0.001 then
        moveX = (moveX / length) * self.moveSpeed * deltaTime
        moveZ = (moveZ / length) * self.moveSpeed * deltaTime
        Apex.Physics.Translate(self.entityId, moveX, 0.0, moveZ)
    end

    -- Jump impulse
    if Apex.Input.IsKeyPressed(Apex.Key.Space) and self.isGrounded then
        Apex.Physics.ApplyImpulse(self.entityId, 0.0, self.jumpForce, 0.0)
        Apex.Audio.PlayOneShot("Assets/Audio/SFX_Jump.wav", 1.0)
        self.isGrounded = false
    end
end

function PlayerController:OnCollisionEnter(otherEntity)
    Apex.Log("Player collided with entity: " .. tostring(otherEntity))
    self.isGrounded = true
end

return PlayerController
