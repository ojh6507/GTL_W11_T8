-- 캐릭터 속성
turnSpeed = 80
moveSpeed = 10 -- 변수명 일관성 (MoveSpeed -> moveSpeed)

-- 중력 및 점프 관련 속성
gravity = -9.8 * 10 -- 중력 가속도 (단위는 게임 스케일에 맞춰 조정, 예시로 *2)
jumpForce = 30     -- 점프 시 초기 수직 속도
verticalVelocity = 0
isGrounded = true  -- 캐릭터가 지면에 있는지 여부 (간단한 플래그)
                   -- 실제로는 Raycast 등으로 감지해야 함

function BeginPlay()
    print("Begin")
    -- 초기화: 캐릭터가 지면에 있다고 가정
    isGrounded = true
    verticalVelocity = 0
end

function EndPlay()
    print("[EndPlay]")
end

function OnOverlap(OtherActor)
end

function InitializeLua()
    controller("W", OnPressW)
    controller("S", OnPressS)
    controller("A", OnPressA)
    controller("D", OnPressD)
    controller("V", OnPressV)
end

function OnPressW(dt)
    local currentPos = actor.Location
    currentPos = currentPos + actor:Forward() * dt * moveSpeed
    actor.Location = currentPos
end

function OnPressS(dt)
    local currentPos = actor.Location
    currentPos = currentPos - actor:Forward() * dt * moveSpeed
    actor.Location = currentPos
end

function OnPressA(dt)
    local rot = actor.Rotator
    rot.Yaw = rot.Yaw - turnSpeed * dt
    actor.Rotator = rot
end

function OnPressD(dt)
    local rot = actor.Rotator
    rot.Yaw = rot.Yaw + turnSpeed * dt
    actor.Rotator = rot
end

function OnPressV(dt)
    if isGrounded then
        verticalVelocity = jumpForce -- 점프 시 수직 속도 부여
        isGrounded = false           -- 점프하면 공중 상태
    end
end

function CheckGroundStatus()
    if actor.Location.Z <= 0 then
        if verticalVelocity < 0 then
            isGrounded = true
            verticalVelocity = 0            
            local pos = actor.Location
            pos.Z = 0
            actor.Location = pos
        end
    else
        isGrounded = false
    end
end

function Tick(dt)
    CheckGroundStatus()

    if not isGrounded then
        verticalVelocity = verticalVelocity + gravity * dt
    end

    local currentPos = actor.Location

    currentPos.Z = currentPos.Z + verticalVelocity * dt

    actor.Location = currentPos
end
