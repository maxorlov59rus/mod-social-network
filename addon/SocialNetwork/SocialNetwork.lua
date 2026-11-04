--[[
    Social Network addon-companion for mod-social-network (AzerothCore 3.3.5).

    Communicates with the server through the core AddonChannelCommandHandler
    protocol: prefix "AzerothCore", message type 'h' (human-readable command),
    server replies with 'm' (message body) / 'o' (OK) / 'f' (failed) frames.

    Protocol frame layout (one frame per chat message, type ADDON):
        "AzerothCore	" .. opcode .. counter(4) .. [payload]
]]

local ADDON_PREFIX = "AzerothCore"

-- Frame type constants (server side, AddonChannelCommandHandler)
local FRAME_ACK = "a"
local FRAME_OK = "o"
local FRAME_FAILED = "f"
local FRAME_MESSAGE = "m"

-- Pending request registry: counter -> {callback, frame{}}
local pending = {}
local counter = 0

local COLORS = {
    title = "|cffffd100",
    text = "|cffffffff",
    dim = "|cff999999",
    red = "|cffcc4444",
}

---------------------------------------------------------------------
-- Networking
---------------------------------------------------------------------

local function NextCounter()
    counter = (counter + 1) % 10000
    return string.format("%04d", counter)
end

local function SendCommand(commandText, callback)
    local id = NextCounter()
    pending[id] = { callback = callback, frame = {} }

    -- Addon channel messages are limited by the client to 255 bytes per frame
    SendAddonMessage(ADDON_PREFIX, "h" .. id .. commandText, "WHISPER", UnitName("player"))
end

local function OnAddonMessage(_, prefix, message, _, sender)
    if prefix ~= ADDON_PREFIX or sender ~= UnitName("player") then
        return
    end

    -- Frame: type(1) + counter(4) [+ body]
    if #message < 5 then
        return
    end

    local frameType = message:sub(1, 1)
    local id = message:sub(2, 5)
    local body = message:sub(6)
    local request = pending[id]
    if not request then
        return
    end

    if frameType == FRAME_ACK then
        request.acked = true
        return
    end

    if frameType == FRAME_MESSAGE then
        table.insert(request.frame, body)
        return
    end

    if frameType == FRAME_OK or frameType == FRAME_FAILED then
        pending[id] = nil
        local text = table.concat(request.frame, "\n")
        if request.callback then
            request.callback(text, frameType == FRAME_OK)
        end
    end
end

---------------------------------------------------------------------
-- UI helpers
---------------------------------------------------------------------

local mainFrame, scrollFrame, scrollChild, output, input

local function SetOutput(text)
    if output then
        output:SetText(COLORS.text .. text)
        scrollFrame:UpdateScrollChildRect()
    end
end

local function SendAndView(commandText)
    SendCommand(commandText, function(response, ok)
        SetOutput(ok and response or (COLORS.red .. response))
    end)
end

---------------------------------------------------------------------
-- Frame construction
---------------------------------------------------------------------

local function EnsureFrame()
    if mainFrame then
        return
    end

    mainFrame = CreateFrame("Frame", "SocialNetworkFrame", UIParent, "BasicFrameTemplateWithInset")
    mainFrame:SetSize(580, 470)
    mainFrame:SetPoint("CENTER")
    mainFrame:SetMovable(true)
    mainFrame:EnableMouse(true)
    mainFrame:RegisterForDrag("LeftButton")
    mainFrame:SetScript("OnDragStart", mainFrame.StartMoving)
    mainFrame:SetScript("OnDragStop", mainFrame.StopMovingOrSizing)
    mainFrame:SetFrameStrata("HIGH")
    tinsert(UISpecialFrames, "SocialNetworkFrame") -- ESC closes

    mainFrame.title = mainFrame:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
    mainFrame.title:SetPoint("LEFT", mainFrame.TitleBg, "LEFT", 6, 0)
    mainFrame.title:SetText(COLORS.title .. "Социальная Сеть")

    -- Output area
    scrollFrame = CreateFrame("ScrollFrame", "SocialNetworkScroll", mainFrame, "UIPanelScrollFrameTemplate")
    scrollFrame:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 12, -40)
    scrollFrame:SetPoint("BOTTOMRIGHT", mainFrame, "BOTTOMRIGHT", -34, 220)

    scrollChild = CreateFrame("Frame", "SocialNetworkScrollChild", scrollFrame)
    scrollChild:SetSize(520, 250)
    scrollFrame:SetScrollChild(scrollChild)

    output = scrollChild:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    output:SetPoint("TOPLEFT")
    output:SetJustifyH("LEFT")
    output:SetWidth(520)

    -- Input box
    input = CreateFrame("EditBox", "SocialNetworkInput", mainFrame, "InputBoxTemplate")
    input:SetSize(400, 24)
    input:SetPoint("BOTTOMLEFT", mainFrame, "BOTTOMLEFT", 16, 186)
    input:SetAutoFocus(false)
    input:SetMaxLetters(200)

    mainFrame.sendButton = CreateFrame("Button", "SocialNetworkSend", mainFrame, "GameMenuButtonTemplate")
    mainFrame.sendButton:SetSize(130, 24)
    mainFrame.sendButton:SetText("Запись на стену")
    mainFrame.sendButton:SetPoint("BOTTOMLEFT", mainFrame, "BOTTOMLEFT", 424, 184)
    mainFrame.sendButton:SetScript("OnClick", function()
        local text = input:GetText()
        if text and #text > 0 then
            SendAndView("social post " .. text)
            input:SetText("")
            input:ClearFocus()
        end
    end)

    input:SetScript("OnEnterPressed", function()
        mainFrame.sendButton:GetScript("OnClick")()
    end)

    -- Navigation buttons; 5 columns x 5 rows above the bottom edge
    local function MakeButton(text, col, row, commandTemplate)
        local btn = CreateFrame("Button", "SocialNetworkBtn" .. col .. "_" .. row, mainFrame, "GameMenuButtonTemplate")
        btn:SetSize(104, 24)
        btn:SetText(text)
        btn:SetPoint("BOTTOMLEFT", mainFrame, "BOTTOMLEFT", 16 + (col - 1) * 110, 8 + (5 - row) * 30)
        if commandTemplate then
            btn:SetScript("OnClick", function()
                -- If input has text, append it as an argument (e.g. like <name>)
                local arg = input:GetText()
                local cmd = commandTemplate
                if arg and #arg > 0 then
                    cmd = cmd .. " " .. arg
                end
                SendAndView(cmd)
            end)
        end
        return btn
    end

    -- Row 1: viewing actions (no argument)
    MakeButton("Профиль", 1, 1, "social profile")
    MakeButton("Стена", 2, 1, "social wall")
    MakeButton("Входящие", 3, 1, "social inbox")
    MakeButton("Заявки", 4, 1, "social requests")
    MakeButton("Топ", 5, 1, "social top")

    -- Row 2: actions using the input field as argument
    MakeButton("Статус", 1, 2, "social status")
    MakeButton("Лайк", 2, 2, "social like")
    MakeButton("+Подписка", 3, 2, "social follow")
    MakeButton("-Подписка", 4, 2, "social unfollow")

    -- Row 3: message actions
    MakeButton("Сообщить", 1, 3, "social msg")
    MakeButton("Комментарий", 2, 3, "social comment")
    MakeButton("Комменты", 3, 3, "social comments")
    MakeButton("Удалить", 4, 3, "social delete")
    MakeButton("Репост", 5, 3, "social repost")

    -- Row 4: friends
    MakeButton("Друзья", 1, 4, "social friends")
    MakeButton("+Друг", 2, 4, "social friend")
    MakeButton("✓Принять", 3, 4, "social friendaccept")
    MakeButton("✗Отклонить", 4, 4, "social frienddecline")
    MakeButton("-Друг", 5, 4, "social frienddel")

    -- Row 5: enemies and guild
    MakeButton("Враги", 1, 5, "social enemies")
    MakeButton("+Враг", 2, 5, "social enemy")
    MakeButton("-Враг", 3, 5, "social enemydel")
    MakeButton("Гильдия", 4, 5, "social guild")

    -- Hint line
    mainFrame.hint = mainFrame:CreateFontString(nil, "OVERLAY", "GameFontDisable")
    mainFrame.hint:SetPoint("BOTTOMLEFT", mainFrame, "BOTTOMLEFT", 16, 168)
    mainFrame.hint:SetText(COLORS.dim ..
        "Поле ввода: текст (статус/запись), имя игрока (лайк/подписка/друг/враг/сообщить), id записи (комменты/удалить/репост).")
end

local function Show()
    EnsureFrame()
    mainFrame:Show()

    -- Load profile on open
    SendAndView("social profile")
end

---------------------------------------------------------------------
-- Slash commands: /socn, /соцсеть
---------------------------------------------------------------------

SLASH_SOCIALNETWORK1 = "/socn"
SLASH_SOCIALNETWORK2 = "/соцсеть"
SlashCmdList["SOCIALNETWORK"] = function(msg)
    msg = (msg or ""):gsub("^%s+", ""):gsub("%s+$", "")

    if msg == "" then
        Show()
        return
    end

    -- Forward any argument as a server command (e.g. /socn wall Vasya)
    SendCommand("social " .. msg, function(response, ok)
        DEFAULT_CHAT_FRAME:AddMessage(COLORS.title .. "[СоцСеть] " .. (ok and COLORS.text or COLORS.red) .. response)
    end)
end

---------------------------------------------------------------------
-- Event wiring
---------------------------------------------------------------------

local eventFrame = CreateFrame("Frame")
eventFrame:RegisterEvent("ADDON_LOADED")
eventFrame:RegisterEvent("CHAT_MSG_ADDON")
eventFrame:SetScript("OnEvent", function(_, event, ...)
    if event == "ADDON_LOADED" and ... == "SocialNetwork" then
        if C_ChatInfo then
            C_ChatInfo.RegisterAddonMessagePrefix(ADDON_PREFIX)
        else
            RegisterAddonMessagePrefix(ADDON_PREFIX)
        end
    elseif event == "CHAT_MSG_ADDON" then
        OnAddonMessage(...)
    end
end)
