-- Secondary subtitle manipulation script

local function show_current_ssid()
    mp.osd_message("“secondary-sid” is set to “" .. mp.get_property("secondary-sid", "") .. "”")
end

local function get_ssid_info()
    local cur_sid = mp.get_property("sid")
    local cur_ssid = mp.get_property("secondary-sid")
    local available_ssids = {}
    local cur_ssid_idx = nil
    local tracks_n = mp.get_property("track-list/count")
    for i = 0, tracks_n-1 do
        if mp.get_property("track-list/" .. i .. "/type") == "sub" then
            local sid = mp.get_property("track-list/" .. i .. "/id")
            if sid ~= cur_sid then
                table.insert(available_ssids, sid)
                if sid == cur_ssid then
                    cur_ssid_idx = #available_ssids
                end
            end
        end
    end
    return available_ssids, cur_ssid_idx
end

local function change_ssid(direction)
    local available_ssids, cur_ssid_idx = get_ssid_info()
    if #available_ssids == 0 then
        mp.osd_message('No candidates for secondary subtitle!')
        return
    end
    local new_ssid = nil
    if cur_ssid_idx == nil then
        if direction == 1 then
            new_ssid = available_ssids[1]
        else
            new_ssid = available_ssids[#available_ssids]
        end
    else
        local new_ssid_idx = cur_ssid_idx + direction
        if new_ssid_idx < 1 or new_ssid_idx > #available_ssids then
            new_ssid = "no"
        else
            new_ssid = available_ssids[new_ssid_idx]
        end
    end
    mp.set_property("secondary-sid", new_ssid)
    show_current_ssid()
end

mp.add_key_binding("k", "ssid_next", function () change_ssid(1) end)
mp.add_key_binding("K", "ssid_prev", function () change_ssid(-1) end)
